#pragma once

#include "Space.hpp"
#include "util/Random.h"
#include "util/CoordinateConversion.hpp"
#include "util/BMaterial.hpp"
#include "util/stb_image.h"
#include "../server/Interface.hpp"
#include "../chunks/Chunk.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>
#include <deque>
#include <unordered_map>
#include <array>

using namespace std;

/**
 * This is used to make a skybox of far away objects.
 * Anything with an angular radius > .14 should be rendered with 
 * World Gen 'Space'
 */
struct SkyGen {

    // Equirectangular panorama dimensions (2:1 aspect ratio).
    static constexpr int IMAGE_WIDTH  = 2048;
    static constexpr int IMAGE_HEIGHT = 1024;

    // Float components per pixel — RGBA float.
    static constexpr int FLOATS_PER_PIXEL = 4;

    // Total float count in the returned array.
    static constexpr int TOTAL_FLOATS =
        IMAGE_WIDTH * IMAGE_HEIGHT * FLOATS_PER_PIXEL;

    // ── Sharpness / brightness by angular size ──────────────────────
    // Blocks with a small angular radius (far away) keep the soft quadratic
    // blur; as a block's angular radius grows (you approach), the splat
    // concentrates into a brighter, sharper core.
    static constexpr double SOFT_RAD = 0.006;  // below → fuzzy falloff²
    static constexpr double SHARP_RAD = 0.06;  // above → fully sharp/boosted

    // Cloud rendering: materials classified by texture luminance as dim
    // clouds (flat, firm-edged, faint) vs bright lights (gaussian sharp ramp).
    static constexpr double CLOUD_LUM = 0.5;   // below → cloud, above → light
    static constexpr double CLOUD_EDGE = 0.6;  // rim occupies the outer 40%
    static constexpr double CLOUD_DIM = 0.4;   // per-block cloud opacity
    static constexpr double CLOUD_DARK = 0.7;  // cloud colour multiplier (dimmer gas)
    // Per-pixel mottle: most cloud pixels are faint, a few fleck brighter, so
    // gas reads as uneven dust instead of a flat screen of cloth. mottle_frac
    // of pixels take the dark branch; the rest bloom (multiplier > 1).
    static constexpr double CLOUD_MOTTLE_FRAC = 0.8;
    static constexpr double CLOUD_MOTTLE_DARK = 0.18;  // dark pixel base multiplier
    static constexpr double CLOUD_MOTTLE_DARK_VAR = 0.5;
    static constexpr double CLOUD_MOTTLE_BRIGHT = 0.7; // bright fleck multiplier
    static constexpr double CLOUD_MOTTLE_BRIGHT_VAR = 1.6;

    // True when the material's texture sample is dim/tinted enough to read as
    // a gas/dust cloud rather than a bright light source.
    static bool is_cloud(const string& material) {
        const auto& c = cached_texture_colour(material);
        return (c[0] + c[1] + c[2]) * (1.0 / 3.0) < CLOUD_LUM;
    }

    // Deterministic 0..1 hash from a block seed + pixel coords — stable across
    // frames, so the mottle pattern doesn't shimmer as the camera moves.
    static double mottle_hash(int64_t seed, int px, int py) {
        uint64_t h = (uint64_t)seed ^ (uint64_t)(px * 0x9E3779B1) ^ (uint64_t)(py * 0x85EBCA77);
        h ^= h >> 33; h *= 0xFF51AFD7ED558CCDull;
        h ^= h >> 29; h *= 0xC4CEB9FE1A85EC53ull;
        h ^= h >> 32;
        return (double)((h & 0x7FFFFFFFFFFFFFFFull) % 1000000) / 1000000.0;
    }

    // Mottle multiplier for a cloud pixel: ~CLOUD_MOTTLE_FRAC of pixels are
    // dark (base multiplier), the rest bloom toward brighter flecks.
    static double cloud_mottle(int64_t seed, int px, int py) {
        double h = mottle_hash(seed, px, py);
        double v = mottle_hash(seed, px + 17, py - 31);
        if (h < CLOUD_MOTTLE_FRAC)
            return CLOUD_MOTTLE_DARK + CLOUD_MOTTLE_DARK_VAR * v;
        return CLOUD_MOTTLE_BRIGHT + CLOUD_MOTTLE_BRIGHT_VAR * v;
    }

    // Standard smooth Hermite ramp, 0 at e0 → 1 at e1 (kept local; SkyGen
    // is a standalone header and must not depend on godot's math.hpp).
    static double smoothstep(double e0, double e1, double x) {
        double t = clamp((x - e0) / (e1 - e0), 0.0, 1.0);
        return t * t * (3.0 - 2.0 * t);
    }

    // ── Sky-box renderer ─────────────────────────────────────────

    /**
     * Generates a sky box given blocks and a view position.
     * 
     */
    static void generate_sky_box(
        float* image,
        const vector<shared_ptr<Block>>& blocks,
        double vx, double vy, double vz
    ) {
        // Sort blocks by distance descending so larger/further objects
        // are drawn first and closer objects overdraw them.
        vector<shared_ptr<Block>> sorted = blocks;
        sort(sorted.begin(), sorted.end(),
            [vx, vy, vz](const shared_ptr<Block>& a, const shared_ptr<Block>& b) {
                if (!a->position_double) return false;
                if (!b->position_double) return true;
                double da = (a->position_double->x - vx)*(a->position_double->x - vx)
                          + (a->position_double->y - vy)*(a->position_double->y - vy)
                          + (a->position_double->z - vz)*(a->position_double->z - vz);
                double db = (b->position_double->x - vx)*(b->position_double->x - vx)
                          + (b->position_double->y - vy)*(b->position_double->y - vy)
                          + (b->position_double->z - vz)*(b->position_double->z - vz);
                return da > db; // descending
            });

        for (const auto& block : sorted) {
            // Skip blocks without a double-precision position.
            if (!block->position_double) continue;

            // Viewer → block offset in whatever coordinate system the caller
            // used (LY, game-space, etc. — all the same to this function).
            double dx = block->position_double->x - vx;
            double dy = block->position_double->y - vy;
            double dz = block->position_double->z - vz;
            double dist = sqrt(dx*dx + dy*dy + dz*dz);
            if (dist < 0.001) continue;  // coincident → skip

            // Unit direction toward the block on the sky.
            double ndx = dx / dist;
            double ndy = dy / dist;
            double ndz = dz / dist;

            // Stable per-block seed (position-derived) so the cloud mottle
            // pattern is fixed for a given block, not shimmering per frame.
            auto ppos = block->position_double;
            int64_t block_seed = (int64_t)llround(ppos->x) * 73856093
                ^ (int64_t)llround(ppos->y) * 19349663
                ^ (int64_t)llround(ppos->z) * 83492791;

            // Angular radius = atan(size / dist).  block->size carries the
            // object's physical LY extent. Sky blocks retain their native size,
            // while 3D-only object blocks carry a separate Godot render size.
            // Floor at 0.001 rad so even very distant objects leave a tiny
            // visible dot.
            double ang_radius = atan(block->size / dist);
            if (ang_radius < 0.001) ang_radius = 0.001;

            // Project direction onto equirectangular pixel coords.
            int cx, cy;
            dir_to_pixel(ndx, ndy, ndz, cx, cy);

            // Sample the material's texture PNG for a representative colour.
            const auto& col = cached_texture_colour(block->material);
            float fr = col[0], fg = col[1], fb = col[2], fa = col[3];
            bool cloud = is_cloud(block->material);

            // ── Compute pixel footprint ──────────────────────────────
            // Vertical extent: angular → pixel using the equirectangular
            // mapping (IMAGE_HEIGHT pixels covers π rad of latitude).
            int pr_y = max(1, (int)ceil(ang_radius * IMAGE_HEIGHT / M_PI));
            int y0 = max(0, cy - pr_y);
            int y1 = min(IMAGE_HEIGHT - 1, cy + pr_y);

            // Fast path: sub-pixel object → single pixel write, no trig loop.
            if (pr_y == 1 && y0 == cy && y1 == cy) {
                float a = fa * (cloud ? (float)CLOUD_DIM : 1.0f);
                if (cloud) a *= (float)cloud_mottle(block_seed, cx, cy);
                int idx = pixel_index(cx, cy);
                if (cloud) {
                    // Gas/dust composite toward the material colour (opaque-ish
                    // cloud) instead of additively accumulating to blowout.
                    image[idx + 0] += (fr * (float)CLOUD_DARK - image[idx + 0]) * a;
                    image[idx + 1] += (fg * (float)CLOUD_DARK - image[idx + 1]) * a;
                    image[idx + 2] += (fb * (float)CLOUD_DARK - image[idx + 2]) * a;
                } else {
                    image[idx + 0] += fr * a;
                    image[idx + 1] += fg * a;
                    image[idx + 2] += fb * a;
                }
                image[idx + 3] = min(image[idx + 3] + a, 1.0f);
                continue;
            }

            // Pre-compute centre direction angles for the inner loop.
            double lat = asin(ndy);
            double lon = atan2(ndx, ndz);
            double sin_lat = sin(lat);
            double cos_lat = cos(lat);

            // ── Multi-pixel glow splat ───────────────────────────────
            // Walk the bounding box in pixel space.  Horizontal radius
            // varies per row because equirectangular projection stretches
            // longitude lines near the poles.
            // Sharpness ramps in with angular size: nearer objects get a
            // steeper falloff (sharper edge) and a brighter core.
            double sharp_t = clamp((ang_radius - SOFT_RAD) / (SHARP_RAD - SOFT_RAD), 0.0, 1.0);
            double falloff_exp = 2.0 + 8.0 * sharp_t;
            double brightness = 1.0 + 2.0 * sharp_t;
            for (int py = y0; py <= y1; py++) {
                // Latitude of this pixel row.
                double row_lat = M_PI / 2.0 - ((double)py + 0.5) / IMAGE_HEIGHT * M_PI;
                double cos_row = max(cos(row_lat), 0.001);
                int pr_x = max(1, (int)ceil(ang_radius * IMAGE_WIDTH / (2.0 * M_PI) / cos_row));

                double sin_row = sin(row_lat);
                double cos_row_v = cos(row_lat);

                int xs = cx - pr_x;
                int xe = cx + pr_x;
                for (int px = xs; px <= xe; px++) {
                    // Wrap horizontally — equirectangular images are seamless.
                    int wx = px % IMAGE_WIDTH;
                    if (wx < 0) wx += IMAGE_WIDTH;

                    // Spherical angular distance between centre direction
                    // and this pixel's direction.
                    double plon = ((double)wx + 0.5) / IMAGE_WIDTH * 2.0 * M_PI - M_PI;
                    double dlon = plon - lon;
                    while (dlon > M_PI) dlon -= 2.0 * M_PI;
                    while (dlon < -M_PI) dlon += 2.0 * M_PI;

                    double ang_dist = acos(
                        sin_lat * sin_row + cos_lat * cos_row_v * cos(dlon)
                    );

                    // Sharpness-shaped falloff within the angular radius.
                    if (ang_dist < ang_radius) {
                        double falloff = 1.0 - ang_dist / ang_radius;
                        int idx = pixel_index(wx, py);
                        if (cloud) {
                            // Flat interior, firm rim: plateau then drop over
                            // the outer band. Composite toward the material
                            // colour so overlap plateaus instead of blowing up.
                            falloff = smoothstep(0.0, 1.0 - CLOUD_EDGE, falloff) * CLOUD_DIM;
                            float a = fa * (float)falloff * (float)cloud_mottle(block_seed, wx, py);
                            image[idx + 0] += (fr * (float)CLOUD_DARK - image[idx + 0]) * a;
                            image[idx + 1] += (fg * (float)CLOUD_DARK - image[idx + 1]) * a;
                            image[idx + 2] += (fb * (float)CLOUD_DARK - image[idx + 2]) * a;
                            image[idx + 3] = min<float>(image[idx + 3] + a, 1.0f);
                        } else {
                            falloff = pow(falloff, falloff_exp) * brightness;
                            image[idx + 0] += fr * (float)falloff * fa;
                            image[idx + 1] += fg * (float)falloff * fa;
                            image[idx + 2] += fb * (float)falloff * fa;
                            image[idx + 3] = min<float>(image[idx + 3] + fa * (float)falloff, 1.0f);
                        }
                    }
                }
            }
        }

    }




    // ── Texture colour cache ─────────────────────────────────────

    static const array<float, 4>& cached_texture_colour(const string& material) {
        static unordered_map<string, array<float, 4>> cache;
        auto it = cache.find(material);
        if (it != cache.end()) return it->second;

        array<float, 4> c = {0.5f, 0.5f, 0.5f, 0.3f};
        int w, h, n;
        uint8_t* tex = stbi_load(BMaterial::path(material).c_str(), &w, &h, &n, 4);
        if (tex) {
            int si = (h / 2 * w + w / 2) * 4;
            c[0] = tex[si + 0] / 255.0f;
            c[1] = tex[si + 1] / 255.0f;
            c[2] = tex[si + 2] / 255.0f;
            c[3] = tex[si + 3] / 255.0f;
            stbi_image_free(tex);
        }
        auto r = cache.emplace(material, c);
        return r.first->second;
    }



    // ── Direction ↔ pixel helpers ──────────────────────────────────

    /* Convert a normalised world direction to equirectangular pixel
       coordinates (y=0 = top = north pole / zenith). */
    static void dir_to_pixel(double dx, double dy, double dz,
                              int& px, int& py) {
        double lon = atan2(dx, dz);  // −π … π
        double lat = asin(dy);       // −π/2 … π/2

        px = (int)((lon + M_PI) / (2.0 * M_PI) * IMAGE_WIDTH);
        py = (int)(IMAGE_HEIGHT - (lat + M_PI / 2.0) / M_PI * IMAGE_HEIGHT);

        // wrap horizontal
        px %= IMAGE_WIDTH;
        if (px < 0) px += IMAGE_WIDTH;
        // clamp vertical
        if (py < 0) py = 0;
        if (py >= IMAGE_HEIGHT) py = IMAGE_HEIGHT - 1;
    }

    /* Convert equirectangular pixel coordinates back to a normalised
       world direction. */
    static void pixel_to_dir(int px, int py,
                              double& dx, double& dy, double& dz) {
        double lon = ((double)px + 0.5) / IMAGE_WIDTH  * 2.0 * M_PI - M_PI;
        double lat = M_PI / 2.0
                   - ((double)py + 0.5) / IMAGE_HEIGHT * M_PI;

        dx = cos(lat) * sin(lon);
        dy = sin(lat);
        dz = cos(lat) * cos(lon);
    }

    /* Return the floating-point array index for a given pixel. */
    static int pixel_index(int px, int py) {
        return (py * IMAGE_WIDTH + px) * FLOATS_PER_PIXEL;
    }

};
