# AGENTS.md

## Communication Style
- Keep explanations, summaries, and responses **brief and to the point** — short lines, minimal prose. The user skims and misses stuff in long walls of text.

## Space ↔ Planet Surface Transition (`Game.hpp`, `World.hpp`)
- `Game::start_game` spawns the player in SPACE mode first, then `request_regen()`. A `std::thread space_worker` runs `SpaceGen::generate_space(current_stellar)` (skybox + space blocks + landing detection) **off the main thread**; the main thread drains the result in `apply_space_result()` (called at the top of `_process`) and does only the node work (skybox set, player re-anchor, planet entry/exit).
- **`SpaceGenResult::game_scale`** (LY→godot-units, `5000/nearest_dist`) is exposed for converting player godot movement back to LY. Do NOT conflate with `CoordinateConversion::game_scale(SpaceGenLevel, meters)` (meters/10^k) used by `World::init_for_space`.
- **Stellar tracking (`update_stellar_from_movement`)**: each tick `current_stellar += offset_coordinate(current_stellar, godot_delta/last_game_scale)`. On every non-landing regen the player is re-anchored to godot origin (moving-world-origin), so they never drift from the origin.
- **Regen loop (`space_transition_tick` in `_process`)**: every `space_regen_interval` (0.5s), skip regen unless moved `>= SPACE_REGEN_MOVED_LY` (0.05). While `on_planet`, regen is skipped entirely (only altitude check). **Skybox refresh is throttled separately**: `skybox_accumulator` (primed to `space_skybox_interval` so the first regen applies immediately) is incremented every frame in `_process`; `apply_space_result` only calls `GodotHdr::set_hdr_skybox` when it reaches `space_skybox_interval` (5s), otherwise it `delete[]`s the freshly-rendered pano. Blocks/landing still update per regen.
- **Landing (`enter_planet`)**: init `rootChunk` (root_square from `worldSize/CHUNK_SIZES[Chunk::topTypeFor(worldSize)]`) + `world->init(planet)` once (`if (!cubePool)`), set `genType=TERRAIN`, `TERRAIN_GEN_ON=true`, `player->init` + `set_location_mode(TERRAIN)`, drop player at front pole (`sphere_location_to_world_position(Vector3(0,0,1), planet)` = map center) + surface height + 3, `world->start()` if not running. Stores `landed_dir` (unit planet→player).
- **Leaving (`leave_planet`)**: triggered when `world->playerPosition->y > Planet::maxHeight` (15000). Sets `TERRAIN_GEN_ON=false`, `genType=SPACE`, queues terrain despawn, `set_location_mode(SPACE)`, and **nudges `current_stellar` outward by `maxHeight - height_m` along `landed_dir`** so the next regen's landing check (`dist <= radius + surface_height`) doesn't immediately re-land the player.
- **Space-block spawn/despawn via `DeferredWorldBlockPool`**: `Game` owns its own pool (`space_deferred` + backing `space_cube_pool`, Game as parent) created in `_ready` — `world->cubePool` only exists after planet entry, and its `spawn_blocks` uses terrain frame + worldSize wrap, neither of which fits space blocks. The worker thread swaps a single group (`space_group_id`): `space_deferred->despawn_blocks(group_id)` then `spawn_space_blocks(group_id, blocks, player_location, game_scale)` (if not landing). `WorldBlockPool::spawn_space_blocks` groups by material+size, converts via `position_double + offset` (fallback `position->to_position_double()`), spawns with **no collider container (no hit boxes)** via `MeshMaker::spawn_multimesh_instances`, and tracks under `group_id_to_mesh_instances` so `despawn_blocks` frees them. `DeferredWorldBlockPool::spawn_space_blocks` stashes the blocks and `call_deferred`s the spawn to the main thread (thread-safe, same as the terrain path). `space_worker` is stopped/joined in `~Game` and `_exit_tree`.
- **Paced terrain despawn (`World`)**: `queue_all_terrain_for_despawn()` sets `terrain_despawn_requested` (atomic). Worker loop, when `!TERRAIN_GEN_ON`, snapshots group ids from `chunk_to_current_type_in_world` keys (== the `stringTagNoY()` group ids `despawnChunk` uses) and frees `terrain_despawn_per_tick` (3) groups per iteration via `deferred_cube_pool->despawn_blocks` (call_deferred, safe from worker thread).
- **`Player::set_location_mode(LocationType)`**: toggles gravity (0 in space, `planet->gravity/9.8` on surface), moveSpeed/jumpForce/avgSpeed between base and base/10. `_ready` calls it with the init locationType. Base values live in `base_moveSpeed/base_jumpForce/base_avgSpeed`.
- **`GodotHdr::set_hdr_skybox(parent, data, w, h)`** now takes a parent node and attaches a `WorldEnvironment` child (created if missing) via `set_skybox` — the skybox environment needs a WorldEnvironment node to render.

## Project Shape
- This is a Godot GDExtension project, not a UE5 project.
- Startup flow: `project.godot` -> `game_scene.tscn` -> `Game` in `src/game.hpp`.
- Extension registration lives in `src/register_types.cpp`.

## Core Areas
- `src/world/World.hpp`: world generation, worker thread, and block spawning.
- `src/Player.hpp`: player controller, camera, body, and input.
- `src/blocks/WorldBlockPool.hpp`: world blocks, spawning, despawning, and edits.
- `src/body/`: body-part containers and animation support.
- `src/npcs/AlienGen.hpp`: procedural alien generation.
- `src/buildings_and_cities/styles/StyleGen.hpp`: column generation (`make_column`), wall generation (`make_wall`), frame and roof style application, light/sconce generation (`make_light`), window frame placement (`place_perimeter_frame`).

## Build And Test
- `SConstruct` is the source of truth for the build.
- The extension build includes `src/**` except `src/TEST`.
- Focused standalone test build: `scons target=template_debug debug_symbols=yes test_build`.
  (Other scons flags can trigger a full gdextension rebuild, so stick to this command.)
- Cleanup when the build gets wedged: `sh clean.sh`.

## Repo Rules
- Trust `project.godot` and `SConstruct` over README/setup prose if they conflict.
- Some old comments and docs still mention UE5; do not treat them as current architecture.
- All generation systems are supposed to be deterministic and seed-driven; changing seeds or offsets can change generated output.
- Keep changes minimal and do not revert unrelated user edits.
- **Comment style**: brief comments preferred — a short line per logical step explaining *why*, not *what*. Avoid long prose or doc-comment walls in implementation code.

## Server TCP Keep-Alive & Framing
- TCP connections are **persistent** (not per-request). `Server::tcp_thread` accepts and spawns a **detached** thread per connection (`handle_connection`), which loops `receive -> process -> repeat` until the client disconnects. `Server::active_tcp_connections` (atomic) counts live connection threads; `~Server` sets `data.running=false` and waits for the counter to drain (bounded by the socket timeout) so detached threads can't touch freed `Data`.
- `handle_connection` (Server.hpp) uses a per-connection `vector<char>` buffer and `Interface::message_length()` to frame messages. `recv == 0` = client closed; `recv == -1` with `EAGAIN`/`EWOULDBLOCK`/`WSAETIMEDOUT` = idle timeout, keep looping. `MAX_TCP_BUFFER` (512KB) caps a single message / buffer growth.
- `Interface::message_length(message, available)` returns the byte length of the message at `message`, `0` if not enough is buffered yet, `-1` if malformed. Fixed-size messages (PING, LOGOUT, AUTH, STOP, CLIENT_LOCAL_POSITION_UPDATE) return their compile-time `BYTES` constant — zero framing overhead. Variable messages parse their embedded size fields, so the whole message must be buffered before the length is known.

## Interface Wire Format (`Interface.hpp`)
- Every message starts with a 1-bit type flag (SHORT = 2-bit type, LONG = 8-bit type). First 4 `RequestType` enum values (CLIENT_LOCAL_POSITION_UPDATE, BATCH_POSITION_UPDATE, HIT, ATTACK) are short; the rest are long.
- **Tiered ids**: all entity/player/schemata ids are `pack_id`/`unpack_id` — a 2-bit tier header followed by the id in that tier's width (8/13/16/64 bits). The smallest tier that fits is always chosen, so small ids are 10 bits total. `id_tier_bits` reads the width from `Id64::IdLengthTier`.
- **PNEG coords**: 54-bit signed offsets (`pack_pneg`/`unpack_pneg`, `PNEG_BIAS = 1ULL << 53`) used for chunk positions and Placement coords. `PositionUpdate` terrain coords are raw 63-bit two's-complement and must be unpacked with `unpack_signed` (sign-extends) — a plain cast reads negatives as huge positives.
- **Block payloads** (`pack_block_payload`/`unpack_block_payload`/`skip_block_payload`): a 1-bit storage-type flag + the `BlockStorage` chunk wire serialization. BIT_PACKED under 20 blocks, RANGE_COMPRESSED above. Material names are shipped inline (13-bit idx + 8-bit name length + bytes) so no shared material registry is needed.
- `BlockStorage.hpp` storage chunks are self-framing (fixed header + per-block records); `skip_block_payload` mirrors the math so `message_length` can frame the containing message. It reads name-length via `bit_offset - MATERIAL_NAME_LEN_BITS` after a raw `read()`.
- **Gotcha**: `message_length`'s `take` lambda must NOT add `bits` after `BitUtil::unpack` — `BitUtil::unpack` already advances the offset. Adding it again double-counts and reads shifted garbage (returned `-1`/`0` for valid messages).
- Fixed-size constants: `LOCAL_POSITION_BITS = 12`, `ROTATION_BITS = 4`, `HIT_VELOCITY_BITS = 13`, `MATERIAL_BITS = 13`, `KM_BITS_PER_LY = 44`, `MAX_SCHEMATA_BLOCKS_BITS = 22`, block count fields are 30 bits (BLOCKS/VEHICLE) and 11 bits (placements/batch/resync counts).
- `protocol_roundtrip_test()` in `test.cpp` packs/unpacks every message type and checks `message_length` framing (including partial-buffer returns 0) — a good place to extend when adding message types.
- `NetworkClient` holds a persistent `unique_ptr<TCPSocket> tcp` + `tcp_rx_buffer`; `ensure_tcp()` connects once and reconnects after a drop. `recieve_tcp` accumulates into `tcp_rx_buffer` and extracts complete messages via `message_length`, keeping the "wrong type -> `unexpected_responses`" behavior. Client marks `tcp_connected=false` on send/recv failure so the next call reconnects.
- **Gotcha**: `Security::rate_limit` keys on IP only (Security.cpp:24), so rapid sequential requests over a persistent connection are rate-limited exactly as before. The keep-alive test (`server_keepalive_test` in test.cpp) spaces pings >500ms apart (PING rate limit). `Ping.status` is packed in 1 bit, so the server's status 7 arrives as 1 on the wire.
- `Data::send_tcp` uses a heap `vector<uint8_t>` sized `MAX_TCP_SEND` (200KB), not a 4096 stack buffer (puzzle responses are ~17KB).
- **Puzzle image size** is packed in 18 bits (was 11 = max 2047, which truncated real 16875-byte captcha images). `puzzle_square = CAPTCHA_DIFFICULTY * MAX_DRAWING_SIZE / 2` (75), image = `square²×3`.
- `TCPSocket::server_init` sets `SO_REUSEADDR` so restarts don't hang in the bind retry loop during TIME_WAIT; the retry log only prints on actual bind failure.
- Verify with `scons target=template_debug debug_symbols=yes test_build` then run `bin/no_godot_test`.

## Sky Generation (`SkyGen`)
- `SkyGen::generate_sky_box(blocks, vx, vy, vz)` in `src/space/SkyGen.hpp` renders an equirectangular 2048×1024 RGBA float panorama from any set of blocks.
- No LY/game-scale/level awareness — just a dumb renderer: blocks have `position_double` in any coordinate system, `(vx,vy,vz)` is the viewer in the same system.
- `size = 1` assumed per block. Angular radius = `atan(1/dist)`. Minimum floor of 0.001 rad so tiny objects are still visible.
- Colors come from material PNG textures via `stbi_load`, center pixel sampled and cached per material. The sky materials (`NEBULA_GAS`/`NEBULA_DUST`/`STAR_GLOW`/`GALAXY_GLOW`) are solid-fill 32×32 sprites; their center-pixel luminances are ~0.39 / 0.34 / 0.90 / 0.71.
- **Cloud vs light classification by texture luminance**: `is_cloud(material)` = avg RGB luminance of the cached texture sample `< CLOUD_LUM (0.5)`. NEBULA_GAS (0.39) and NEBULA_DUST (0.34) → clouds; STAR_GLOW (0.90) and GALAXY_GLOW (0.71) → lights.
- **Clouds composite, lights glow**: a cloud splat uses `smoothstep(0.0, 1.0 - CLOUD_EDGE, falloff) * CLOUD_DIM` (flat interior, firm outer rim) and **blends toward the material colour** (`image += (mat*CLOUD_DARK - image) * a`, `CLOUD_DIM=0.4` per-block opacity, `CLOUD_DARK=0.7` colour dimmer) so overlapping blocks plateau at a dim cloud colour instead of accumulating additively past 1.0. Lights keep the additive gaussian ramp below. **Per-pixel cloud mottle**: `cloud_mottle(block_seed, px, py)` returns a deterministic 0..1 hash-derived multiplier (spatial noise, stable across frames); `CLOUD_MOTTLE_FRAC (0.8)` of pixels take the dark base (`dark 0.18 + var 0.5`), the rest bloom (`bright 0.7 + var 1.6`) — so an 80% of a cloud reads faint with a few brighter flecks (uneven dust) instead of a flat cloth. Multiplied into the alpha in both the single-pixel fast path and the multi-pixel cloud splat.
- Splat uses `acos(dot(pixel_dir, center_dir))` for correct angular distance, handling equirectangular polar distortion and horizontal seam wrapping.
- Fast single-pixel path for sub-pixel objects (clouds dimmed by `CLOUD_DIM`).
- **Sharpness/brightness ramp with angular size** (lights only): splat falloff is `pow(falloff, 2 + 8t) * (1 + 2t)` where `t = clamp((ang_radius - SOFT_RAD)/(SHARP_RAD - SOFT_RAD))`, `SOFT_RAD=0.006`, `SHARP_RAD=0.06`. Far blocks (tiny ang) keep the soft quadratic blur; near blocks (large ang) get bright sharp cores. Sub-pixel fast path unaffected.
- Stubs for old `render_galaxy`/`make_skybox`/`render_planet_disk` left so `test.cpp` compiles.

## Roof Style (`apply_roof_style`)
- Signature: `apply_roof_style(shared_ptr<RoofStyleElement>, Grid<shared_ptr<Section>>, Grid<shared_ptr<Block>>&, int64_t)`.
- Takes a grid of `Section`s (each containing rooms) instead of a flat room list.
- Neighbor detection uses grid cell adjacency (N/E/S/W at `start->z + scale`, `start->x + scale`, `start->z - 1`, `start->x - 1`) — avoids O(n²) room overlap checks.
- "Section above" check uses grid cells at `start->y + vertical_scale`.
- Roof spans the combined world-space bounding box of all rooms in the section.
- `Section` and `RoomPattern` are defined in `Room.hpp` (moved from `BuildingGen.hpp` for visibility in `StyleGen.hpp`).
- Fake 1x1 section grid is created in `make_jut_out` for jut-out roofs.
- **SHED/TENT neighbor capping uses layered distribution**: `layer_target = y1_top + total_rise * (layer+1) / layers` for each layer. The `roof_height_at` closure takes `int layer`.
- **Neighbor height drives free/ridge classification**: A side is a "ridge side" (roof high point) only if the neighbor exists AND its wall top (`r->end->y`) >= `y1_top` (this section's wall top). If the neighbor is shorter or absent, it's a "free side" (eave). This means a 2-story section treats a 1-story neighbor as a free side — the roof goes above it.
- **`cumulative_base` tracking**: The old `cumulative_base += lmax_h * 0.5` overshoots when neighbor capping limits per-layer rise (`total_rise/layers << lmax_h`). Fixed by scanning actual `tops` max after each layer and setting `cumulative_base = max_y - y1_top` when any neighbor wall is taller than `y1_top`. This preserves the 50% overlap for non-capped roofs.
- **Directional inset**: The `layer * 3` inset (which shrinks each layer for the stepped pyramid look) only applies to free sides (no neighbor). Ridge sides (with neighbors) get no inset, so each layer's roof stays flush against the neighbor wall along x/z axes.

## Wall Generation (`make_wall`)
- Signature: `make_wall(WallAngle, WallStyleElement, start_pos, length, cutouts)` → `Grid<Block>`.
- Accepts `const vector<WallCutout>& cutouts` (default empty). Cutout check in cardinal loop: block at step `i` and height `hi` is skipped if run-axis coord in `[lo, hi]` AND y in `[lo_y, hi_y]`.
- `WallCutout` has `{lo, hi, lo_y, hi_y}` — world-space inclusive bounds.
- In `gen_building()` (`BuildingGen.hpp`), cutouts are collected for each outer face from:
  - Active door holes of rooms at that face (e.g., `room->holes[2]` for north face)
  - Hallway floor rooms (zero-height, `start->y == end->y`) at that face — full-height cutout `{room->span_x/z, y0, y1}` so hallway exits are open.
- `WallAngle`: N/NE/E/SE/S/SW/W/NW — dictates wall run direction and outward face.
- Cardinal angles (N/E/S/W): wall extends along `run_dir`, thickness centered on start.
- Diagonal angles (NE/SE/SW/NW): rendered as cardinal base + per-step shift of all blocks (wall + frame) by step index. Step index extracted from run-direction coordinate.
- `start` = center of wall cross-section at ground at one end.
- `outward_slope` = 0 means vertical (no inward shift). At height h, shift = slope × h. Both slopes taper the wall toward the top.
- Frame: non-sloping cardinal uses `apply_frame_style`. Sloping cardinal uses custom placement with side edges following the slope. Diagonal frames are shifted post-generation.

## `render_extended_glow`
- Signature: `render_extended_glow(data, view_pos, galaxy, obj_pos, r1,g1,b1, r2,g2,b2, brightness, seed, shape, stars)`.
- Computes `dx, dy, dz` internally via `ly_offset(view_pos, obj_pos)`.
- `ang_radius` removed — computed internally from the max angular separation of the 3 shape ref vectors.
- `shape`: `const ShapeRefs&` — struct with 3 `vector<double>` fields (`a`, `b`, `c`), each a viewer-relative {x,y,z} ly offset defining the object's extent in the galaxy's orientation frame.
- `stars`: `const vector<shared_ptr<Star>>&` — actual star objects with `location` field. When non-empty, renders each star at its individual position. When empty, falls back to 3 random bright spots.
- `get_orient_offsets` returns `ShapeRefs` by value.

## Planet Skybox (SKY mode)
- `GenType::SKY` in `resetDesiredChunks()` iterates the visible hemisphere via lat/long, creating chunk hierarchy on-the-fly (rather than pre-seeding with `makeEmptySubchunkForType`).
- `sphere_location_to_world_position` has a singularity at sphere poles (north/south), causing large non-linear jumps in world coordinates. The on-the-fly approach avoids needing to guess a render_distance that covers these jumps.
- Chunk hierarchy creation in SKY mode: walks from rootChunk down to target_type, calling `makeEmptySubChunks` at each intermediate level when subChunks are empty.
- After resetDesiredChunks returns for SKY, `playerMovementReset` must be set to `false` to prevent `world_gen()` from re-triggering `resetDesiredChunks` in an infinite loop.
- The SPACE mode branch in `world_gen()` only clears `playerMovementReset` for TERRAIN (line 425). SKY mode must clear it in its own resetDesiredChunks branch.
- `get_representatives_on_planet_surface` is not used by SKY mode — the iteration is inlined with hook for hierarchy creation.
- `makeEmptySubchunkForType`: when `getChunk(parent_type)` returns nullptr (missing intermediate hierarchy), the loop `continue`s safely (nullptr guard added).
## Wall Design (`WallDesignElement`)
- `WallDesignElement` struct in `Styles.hpp` has a `Grid<bool> pattern` — XY pattern indexed as `pattern[{x, 0, y}]`.
- `gen_wall_design_style(seed, base_size)` generates a `base_size × base_size` pattern:
  - Draws N random lines on the left half (horizontal, vertical, diagonal, sine wave).
  - Mirrors the result across the Y axis for symmetry.
- `StyleGroup::wall_design` is set with 50% probability per style group.
- In `gen_wall_style()`, if the group has a wall design, it's attached to the wall with 50% additional probability (25% overall).
- In `make_wall()`, applied after Step 1 (wall blocks placed), before Step 2 (frame):
  - Wall face is divided into `num_sx × num_sy` panels: `num_sx = round(length / base_size)`.
  - Pattern is scaled to fit each panel.
  - Slope offset shifts pattern x-position at each height layer by `outward_slope × hi`.
  - Decorative blocks placed one step outward from the exterior face at positions where `pattern[{px, 0, py}]` is true.

## Door Placement
- `Room` has `DoorHole holes[4]` (one per side: 0=front, 1=left, 2=back, 3=right) for pre-computed door opening bounds.
- In `layout_space()` (`BuildingGen.hpp`), holes are computed with hallway alignment for non-OPEN sections and centered for OPEN_ROOM sections.
- `get_blocks()` in `Room.hpp` reads pre-computed holes; falls back to centered computation if a hole is inactive (for code paths that don't pre-compute).
- Hallway-aligned rule: for each door side, compute the hole along the wall's axis at the coordinate closest to the hallway cross `[sz+srs, sz+srs+hs]`/`[sx+srs, sx+srs+hs]`. Three cases:
  - Wall starts after hallway → door flush with wall start
  - Wall ends before hallway → door flush with wall end
  - Wall overlaps hallway → door flush with hallway south/west boundary

## Stair Generation (`make_stair`)
- Signature: `make_stair(StairStyleElement, Section lower, Section upper, int lower_side, Grid<Block>&)` in `StyleGen.hpp`.
- `lower_side` (0-3) is the connection door side from `StairConnection::side`. Upper side is `(lower_side + 2) % 4`.
- **Hallway matching**: `find_hallway_for_side` matches zero-height hallway rooms to the connection side by comparing room bounds against section world bounds (computed from all rooms in the section). Side 0 matches `start->z == wmin_z`, side 1 matches `start->x == wmin_x`, side 2 matches `end->z == wmax_z`, side 3 matches `end->x == wmax_x`.
- **Spiral vs straight**: Chosen by doorway obstruction check (bounding box from lower to upper hallway centers inflated by half_width). If the stair path blocks any door hole → spiral; otherwise straight.
- **Spiral center**: Placed at the section boundary on the connection side (`cz = l_max_z - radius` for side 2, `cx = l_min_x + radius` for side 1, etc.) so the outer ring touches the section boundary.
- **Step dimensions**: `stair_width = min(hw, hz)` (the narrow hallway dimension), `half_width = max(1, stair_width / 2)`, `radius = max(1, half_width)`. 12 steps per rotation, each step is a 30° wedge from center to full radius. Wedges use `<=` thresholds (`ox*2 <= oz` / `oz*2 <= ox`) for even coverage.
- **Center pole**: Solid column from `sy` to `ey`, diameter = `max(1, (2*radius + 1) / 4)` blocks.
- **Positioning bug history**: Originally used `max(hw, hz)` which made stairs 2x too wide for side hallways. Hallway matching originally returned the first zero-height room (always center hallway) instead of the side-specific one. Spiral center was at hallway center + half_width offset instead of at the section boundary.

## City Layout & Generation (`CityGen.hpp`, `City.hpp`)
- `CityGen` in `CityGen.hpp` has two phases: Phase 1 (`plan_city_layout`) and Phase 2 (per-chunk generation).
- Data structures in `City.hpp` support 3D cities: `RoadPoint {x,z,y}`, `Plot {x0,z0,x1,z1,y0,y1,...}`, `District {x0,z0,x1,z1,y0,y1,...}`, `City {x0,z0,x1,z1,y0,y1,...}` with y fields defaulting to 0 for backward compatibility.
- `City::overlaps_chunk` and `City::district_at` are implemented.
- `City::generated_chunks` (`unordered_set<string>`) tracks "x,z" of generated chunks.
- `Plan` helpers implemented: `pick_building_type` (weighted zone→type), `is_steep_slope` (5-sample terrain check), `compute_fill` (dirt fill to level).
- `plan_city_layout` picks location via terrain sampling (above sea level, flat enough), sizes city from economy cash, picks shape style (radial/grid/organic), generates main roads, and assigns districts (civic center, commercial ring, industrial edge, residential fills, random parks).
- `generate_local_roads_and_plots` subdivides a chunk into local streets and rectangular plots. Grid (regularity > 50) uses evenly-spaced N-S/E-W streets; organic (regularity <= 50) uses jittered street positions aligned to the nearest ARTERIAL road bearing. Plots abutting main roads get increased setback. Min plot area from largest_citizens/BLOCK_SCALE.
- `BuildingType` has `NONE` and `PARK` in addition to the building types.
- `generate_local_roads_and_plots` signature: `(seed, chunk_x, chunk_z, chunk_size, district, main_roads = {})` — main_roads default empty for backward compat.
- `generate_chunk_buildings` orchestrates per-chunk generation: overlap/district check, local road generation, then for each plot picks building type, checks terrain slope, fills dirt, and builds via `BuildingGen::gen_building` with y-offset. Marks generated chunks in `city->generated_chunks`.

## SpaceGen Priority Queue (`generate_space`)
- `SpaceGen::generate_space()` in `SpaceGen.hpp` now has a **three-phase pipeline**:
  1. **Phase 1 — Hierarchy population + candidate collection**: For each galaxy, if `ang_radius >= RECURSE_THRESHOLD (0.001 rad)`, walk sub-structures (superclusters → clusters → cradles → solar systems → stars/planets). Each object above `SKIP_THRESHOLD (0.001 rad)` becomes a `RenderCandidate`. At each level, only descend if `ang_radius >= RECURSE_THRESHOLD` — this naturally terminates the hierarchy when objects get too small.
  2. **Phase 2 — Sort by priority**: `sort(candidates)` by `(Type, -angular_radius)`. Priority order: PLANET (0) > STAR (1) > SOLAR_SYSTEM (2) > STELLAR_CRADLE (3) > CLUSTER (4) > SUPER_CLUSTER (5) > NEBULA (6) > GALAXY (7).
  3. **Phase 3 — Block generation**: Iterate sorted candidates. If `ang_radius >= BLOCK_THRESHOLD (0.008 rad)` and `block_count < MAX_BLOCKS`, generate 3D blocks via `generate_candidate_blocks()`. Otherwise, create a single-block skybox representation.
  4. **Phase 4 — Skybox**: Combine `out_blocks` and `skybox_blocks`, render via `SkyGen::generate_sky_box()`.
- **Seeds**: No universe seed exists; every generator derives its RNG from `seed_from_coordinate(...)` on the object's own position, so output is deterministic and independent of call order. Caps (200, 600, etc.) sample via even stride, never RNG.
- **Uniform marker size — sky & 3D blocks agree by construction**: `cand.block_size_ly = 1.0/game_scale` (`SpaceGen.hpp:2181`) is preserved (do NOT overwrite `b->size`/force `1.0`). Every space block carries the same LY size `1.0/game_scale`, so at render its physical godot size is `size * game_scale` = a constant **1.0-godot-unit cube** (white-dwarf sub-halos `0.5×`). The 3D engine does the perspective; SkyGen already computes `atan(size/dist)`, so a sky splat at distance `d` and a 3D cube at the same godot distance show the same angle (angular size is invariant under the LY→godot `game_scale` mapping because numerator and denominator scale together). **Gotcha**: `Block::getSize()` returns an `int` and `MeshMaker::meshes` is keyed by block-units — neither fits fractional LY sizes, so spawn must use `block->size` (double) and `make_cube(size*game_scale)`, never `meshes[getSize()]`. Phase 5 single-block sky polys emit LY offsets + `block_size_ly` (uniform), not godot-scaled `size_game`.
- **Block-gen is sub-object markers, not fills**: `generate_*_blocks()` place one block per child at the child's real `location` (via `ly_offset(view_pos, child->location)` + grid-rounded `place()`):
  - `generate_galaxy_blocks`: 1 `GALAXY_GLOW` block per supercluster (cap 200, even stride) + 1 block per nebula point (cap 600, `NEBULA_DUST` if brightness sum > 0.3 else `NEBULA_GAS`). Old spiral-arm/bar/core/halo + non-spiral density fill removed. Consequence: RING galaxies lose their ring silhouette; ELLIPTICAL/CLOUD are sparse.
  - `generate_super_cluster_blocks`: 1 `GALAXY_GLOW` block per cluster (cap 200).
  - `generate_cluster_blocks`: 1 block per stellar cradle (cap 200; `STAR_GLOW` if `i%7==0` else `NEBULA_GAS`).
  - `generate_cradle_blocks`: 1 block per solar system (≤30, no cap).
- **Lazy self-generation guards**: `generate_galaxy_blocks`, `generate_super_cluster_blocks`, `generate_cluster_blocks`, `generate_cradle_blocks`, and the Phase 1 walk all guard with `size()==0`/`empty()` before calling the relevant `generate_*_super_clusters`/`generate_*_clusters`/`generate_*_cradle`/`generate_*_nebulae` populator. This lets block-gen regenerate sub-objects when Phase 1's RECURSE walk skipped them (parent too small to recurse), and prevents duplicate appends to `galaxy->nebulae` (a `vector`).
- **LOD: strict parent-XOR-children** — the Phase 1 walk at every level (galaxy, supercluster, cluster, cradle, solar-system) emits a parent as a candidate *only* when `ang < RECURSE_THRESHOLD` (then `continue`s); when recursable it descends instead. A parent and its children never both render. There is **no cap** on children: once a parent recurses, every child is emitted regardless of size (`SKIP_THRESHOLD` gates removed from the cluster/cradle/system/star/planet loops) — sub-pixel children become splats in Phase 3, recursable children recurse deeper. Rationale for no cap: clusters are ~100 LY and go sub-pixel far away, so capping would starve close superclusters of representation; the skybox splat path absorbs the volume.
- Previously dead-code hierarchy functions are now wired: `generate_super_cluster()`, `generate_cluster()`, `generate_stellar_cradle()` are called in Phase 1 when parent angular radius exceeds `RECURSE_THRESHOLD`.
- `RenderCandidate` struct at line 1687 holds type, angular_radius, viewer-relative LY offsets, and a union-style set of object pointers (`galaxy`, `super_cluster`, `cluster`, `cradle`, `solar_system`, `star`, `planet`, `nebula`).
- **Nebula candidates use per-nebula position/distance**: `Nebula` stores its `center` (`StellarCoordinate`, set in `generate_galaxy_nebulae`), and the Phase 1 nebula loop computes `viewer_dist(view_pos, nebula->center, ...)` for angular radius and sky placement. Previously it used galaxy-center `gdist`/`gx,gy,gz`, which over-sized and center-clumped nebulae when the viewer was inside the galaxy. Viewer-inside-a-nebula (`ndist < radius_ly`) candidates are skipped.
- **Nebula block LOD (`generate_nebula_blocks`, `NEBULA_ANG_RES=0.002`)**: `step_ly = max(block_size_ly, ndist * NEBULA_ANG_RES)` sizes the block footprint (physical size near, sky-pixel size far). Each filament anchor emits `n = clamp(lround((cloud_size/step_ly)^2), 1, CLOUD_MAX_PER_POINT)` blocks (**area-ratio LOD** — the squared count scales with how many block footprints the point's cloud covers; `CLOUD_MAX_PER_POINT=16` caps one anchor). Blocks land at a **center-biased radius** `cloud_size * pow(rand,2)` in a random 3D direction, so each anchor reads as a soft mottled puff rather than a dot. Material: `(base_r+g+b)*noise*neb_dark < 0.5 → NEBULA_DUST` else `NEBULA_GAS` (faint → dimmer DUST). **Global budget**: if total over-subscribes `NEBULA_MAX_BLOCKS=3000`, anchors are dropped farthest-first (never below one) so the near, dense cloud wins. No resolve ramp / even_stride / pepper anymore. Phase 5 routes NEBULA candidates straight to `generate_candidate_blocks` (no `BLOCK_THRESHOLD` single-block cliff for nebulae).
- **Nebula shape — filament skeleton** (not packed blobs, not wispy strands): `generate_nebula_strand_paths` walks `strands` (1–4) meandering centerlines along the dominant axis (`meander_walk`, `max_steps = meander_knots` fixed-step so large meander amplitudes can't stall distance-based termination), storing each path in `Nebula::strand_paths` and flattening every path point into `filament_points`. `generate_nebula_sub_strands` then branches **1–10 short meandering wisps** (3–5 knots, `radius_ly * randDouble(0.2, 0.6)` long, random 3D direction) off every path point, appending their points too. The full skeleton lives in `Nebula::filament_points` (a `vector<FilamentPoint>`: `{x,y,z}` LY offsets from the nebula centre + per-point `cloud_size` = `radius_ly * randDouble(0.04–0.15)` for wisps, `0.06–0.2` for path points). Since the skeleton is stored once (not re-scattered per block-gen), far/near LOD render the **identical shape** — block-gen just changes block count via area-ratio LOD. Old `scatter_nebula_point`/`halo_fraction`/`halo_width`/`tube`/`strand_spread`/`tube_var`/stored-point-cloud removed.
- `generate_galaxy_nebulae` builds the skeleton per nebula from the nebula seed `ns` (`strands ns+23`, `along_extent ns+24` 0.6–1.5, `meander_amp = along_extent * rand(ns+27, 0.3, 0.8)`, `meander_knots ns+28` 20–40, `wavy_seed ns+31`). `filament_points` is empty → block-gen lazily calls `generate_nebula_strand_paths` + `generate_nebula_sub_strands` with `nebula->generation_seed`.
- `Nebula` carries its render spec on the struct: `base_r/g/b`, `neb_dark`, filament basis `fax..fcz`, `strands`/`along_extent`/`meander_amp`/`meander_knots`/`wavy_seed`, `strand_paths`, and `filament_points` (set in `generate_galaxy_nebulae` where the values are computed). Galaxy-LOD markers iterate `filament_points` with the `NEBULA_MARKER_BUDGET=600` cap, offset by the nebula's `ly_offset` from the galaxy centre; marker material uses the same per-nebula brightness rule as block gen.
- **Per-point nebula RGB is discarded** (only picks `NEBULA_DUST` vs `NEBULA_GAS` by brightness) — SkyGen samples the material PNG, so the themed red/blue/teal/purple colors never render. Known limitation; would need per-theme materials.
- `generate_candidate_blocks()` (line 1724) dispatches to the appropriate existing block generator based on candidate type.
- `glow_material()` (line 1716) returns the skybox material string for a candidate type.
- **Dead spiral-arm helpers retained** (unused since galaxy fill removed, no `-Wall` in SConstruct): `galaxy_arm_radii`, `compute_bar_angle`, `iterate_spiral_arms`, `SpiralArmPoint`. `galaxy_disc_basis`/`galaxy_disc_point`/`galaxy_spiral_pos` are still live (`galaxy_spiral_pos` drives supercluster/nebula placement; disc basis used by black-hole blocks).
- **Supercluster shape**: `generate_super_cluster` scatters clusters as a filament cloud (via `scatter_filament_point`), not a box. `spread = max(50, radius_ly * 2)` (≈2× the old fixed ±500 box), plus a squared radial falloff for a dense core / sparse halo. Superclusters themselves are placed along the galaxy spiral with the *same* params as nebulae (`galaxy_spiral_pos(galaxy, cs, ..., 0.2, 2.5, 0.5, 0.2, 0.1, 2.0)`), matching the tighter arm distribution.
- **Cluster shape**: `generate_cluster` scatters stellar cradles the same way as superclusters — filament cloud via `scatter_filament_point` with `spread = max(50, radius_ly * 2)` plus the squared-falloff dense core (previously a ±50 axis-aligned box). `random_filament_axis(seed, ...)` builds the axis/basis (offsets `+20/+21/+22`); `weighted_ratios(seed, count, lo, hi)` builds child mass ratios (offset `+100`, matching old inline loops). Both used by nebulae/supercluster/cluster/cradle. Note `generate_galaxy_super_clusters` weight loop still uses `+1000` and is intentionally not refactored to `weighted_ratios`.

## Gas & Bright Spots
- `gas_fraction` (0..1, fraction of mass that is gas / star-forming fuel) lives on `Galaxy`, `SuperCluster`, `Cluster` in `Space.hpp`.
- **Galaxy**: set in `generate_galaxy_properties` from type (seed `gs+16`): SPIRAL/CLOUD `0.35–0.6`, RANDOM `0.25–0.5`, RING `0.15–0.35`, ELLIPTICAL `0.01–0.06`.
- **SuperCluster**: in `generate_galaxy_super_clusters` (seed `cs+30`), `gas_fraction = clamp(galaxy_gas * rand(0.2, 1.5))` — scatter so a few superclusters come out gas-rich.
- **Cluster**: in `generate_super_cluster` (seed `cs + c*13 + 70`), `gas_fraction = clamp(sc_gas * rand(0.3, 2.0))`.
- **Massive-star bright spots**: `StellarCradle::has_massive_stars` set in `generate_cluster` from cradle mass / system count (`seed_from_coordinate(cradle_pos)` for `randInt(1,30)` systems; flag true when `mass/systems >= 1.2`). `Cluster::massive_star_fraction` aggregates this. Cheap deterministic proxy — no per-frame star generation at cluster LOD.
- **Rendering is sparse-by-default and gas/brightness driven**: block budget (`eff_cap`) scales with intensity so dim regions render fewer blocks (`even_stride`):
  - `generate_super_cluster_blocks`: supercluster's bright spots = **gas-rich clusters** (`cluster->gas_fraction >= 0.5` → `GALAXY_GLOW`, else `NEBULA_GAS`); `eff_cap = 20 + 180*gas`.
  - `generate_cluster_blocks`: bright spots = **cradles hosting massive stars** (`has_massive_stars` → `STAR_GLOW`, else `NEBULA_GAS`); `eff_cap = 15 + 185*massive_star_fraction`.
  - `generate_galaxy_blocks`: supercluster markers dim to `NEBULA_GAS` when a supercluster is gas-poor, bright `GALAXY_GLOW` when gas-rich.
- **Nebulae scale with galaxy gas** (`generate_galaxy_nebulae`): count = `gas_fraction * randDouble(gs+40, 200, 600)` (spirals up to ~300+, ellipticals ~0), radius `scale_length * rand(0.05,0.25) * (0.5 + gas)`, point density `randInt(ns+6,20,120) * (0.5 + gas)`. Per-nebula shape is anisotropic (perp axes scaled by `ns+23`/`ns+24`, factors 0.3–1.5). Colors: 6 themes (added teal/green + purple), per-nebula darkness `ns+7` (0.1–1.0) and per-point noise `0.0–1.0` allow dark nebulae.
- **Shared coordinate/filament helpers** (in `SpaceGen`, used by multiple generators): `offset_coordinate(base, dx,dy,dz)` (base + LY offset → **km-aware** quadrant/light-year/km space: a fractional-LY offset is folded into the `km_*` fields via `floor(shifted)` carry + `llround(frac*KM_PER_LY)`, with `KM_PER_LY = 9460730472581.0`; supersedes the dead `galaxy_local_to_coord`) and its **integer-LY-only sibling `offset_coordinate_ly`** (km always 0, original truncating behavior) used for everything at solar-system scale and larger (galaxies, clusters, nebulae, cradles, systems). Only stars/planets and the player's moving coordinate carry km — systems and above sit at whole-light-year offsets. `orthonormal_basis(axis, &b, &c)`, `random_filament_axis(seed, &a, &b, &c)` (random axis from `seed+20/+21/+22` + basis), `scatter_filament_point(seed, extent, basis, &p)` (along ±80 / perp ±40 scaled by extent), `meander_walk(start, end, step_size, indirectness, smoothness, approach_div, damp_div, seed, max_steps, box)` (shared river/nebula meander: accumulated clamped perpendicular offset, eased to the axis near the end; `max_steps=-1` = distance-based for rivers, positive = fixed-step for nebula strands), `weighted_ratios(seed, count, lo, hi)` (mass ratios from `seed+i+100`), `even_stride(count, cap)` (cap-200 sampling without RNG). All seed-driven; the refactor is output-identical. Note: `offset_coordinate` uses floor-div (correct borrow) — the old per-site `normalise` lambdas cast to `uint16_t` before the borrow check, so negative LY offsets wrapped to `quad += 6` (~60k LY misplacement); that latent bug is fixed. Because the player's space coordinate now carries km (via `update_stellar_from_movement` → km-aware `offset_coordinate`), **landing is real-distance based** (`dist_m <= radius + surface_height`), not a coarse LY-cell snap; `leave_planet`'s `maxHeight` climb nudge now displaces real meters so it clears the landing radius and won't re-land. `generate_solar_system` places stars (primary at origin, companions 0.1–8 AU) and planets (0.3–30 AU) at near-orbital-plane km offsets (`orbit_km` lambda, seeds `i*10+8` / `p*100+40`), and the Phase 1 walk emits star/planet candidates at their per-object offset (`viewer_dist(view_pos, star/planet->location)`) instead of the system center. Verify with `scons target=template_debug debug_symbols=yes test_build` (has `km_coordinate_test`).

## SpaceGen per-object radii
- Every cosmic object that becomes a `RenderCandidate` carries its own `radius_ly`; `angular_radius_rad()` and block sizes in `generate_space()` must use the object's `radius_ly`, never hardcoded constants.
- `Galaxy::radius_ly = scale_length * 2.5` — set in `generate_galaxy_properties`; `galaxy_arm_radii()` (dead helper) falls back to `scale_length * 2.5` if 0.
- `SuperCluster`/`Cluster::radius_ly` set from mass in their generators (`pow(mass/N, 1/3) * scale`). Supercluster `radius_ly` also drives the cluster scatter spread in `generate_super_cluster`.
- `StellarCradle::radius_ly = max(5.0, pow(mass/1e4, 1/3) * 5.0)` — set in `generate_stellar_cradle`.
- `SolarSystem::radius_ly = 0.02 + mass * 0.04` — set in `generate_solar_system`.
- `Star::radius_ly` is a **glow radius** (real stellar radii are ~1e-7 LY, useless for rendering), varies by type: 2e-4 (dwarfs/remnants/black holes), 4e-4 (orange), 5e-4 (yellow), 1.5e-3 (white giant), 2e-3 (blue giant), 2.5e-3 (purple giant), 3e-3 (red giant).
- `Nebula::radius_ly = scale_length * randDouble(0.05, 0.25)` — set in `generate_galaxy_nebulae`; point scatter scales with radius. This is what makes nebulae visible from outside the galaxy (previously fixed 50 LY → invisible beyond ~50k LY). Nebulae are treated as galactic cloud complexes, not star-forming nebulae; the perp scatter is ±40 (vs along ±80) so they read as somewhat blobby filaments.
- Block sizes derive from radius: galaxy `1e9`, sc/cluster `radius*0.3`, cradle/system `radius*0.3`, star `radius`, nebula `radius*0.1`.
- `generate_cradle_blocks()` uses `cradle->radius_ly` as its spread (fallback 10.0).
- Known follow-up: `generate_galaxy_nebulae()` appends to `galaxy->nebulae` every `generate_space()` call (a `vector`, not an overwrite-by-key Grid) — unbounded growth across frames; the `empty()` guard in Phase 1 / `generate_galaxy_blocks` prevents duplicates within a call.

## Schemata Placement (revamped)
- **Flow**: Select blocks (middle-click × 2) → schemata created → placement mode entered automatically.
- **Placement mode**: Renders ghost blocks (no collision) at the block the player is looking at. Bounding box shown in green. Snap-to-grid on the target block.
- **Rotation**: `R` key → 90° CW, `Q` key → 90° CCW. Rotates stored block offsets in `Schemata::blocks`. Tracking `schemata_rotation` in Player.
- **Confirm**: Right-click places permanent blocks (with collision) at the ghost position. Stays in placement mode for rapid stacking.
- **Exit**: Escape cleans up ghost blocks and exits placement mode.
- **Ghost blocks**: Created via `WorldBlockPool::place_block_no_collision()` → `MeshMaker::spawn_multimesh_instances()` with `nullptr` collision_container (no physics body). Tracked in `Player::ghost_block_instances`.
- **Anchor**: Schemata blocks stored as offsets from min corner (most negative x/y/z). Placement anchor = target block position snapped to grid. World position = anchor + offset.

## Password Storage
- `Security::hash_password()` and `Security::is_password_valid()` in `Security.cpp` SHA256 the input before passing to bcrypt.
- DB stores `bcrypt(SHA256(password))` — cracking bcrypt yields SHA256(password), not the raw password.
- When the game client sends a password, it should SHA256 before writing into the request struct for transit protection. The server will SHA256 the received value again before bcrypt (double SHA256 is harmless).

## Chunk Hierarchy — Adaptive Top Tier
- **10 data tiers** (ChunkType): `CHUNK_1/4/16/64/512/8192/131072/2097152/16777216/134217728`. `level()` = ROOT 0, CHUNK_134217728 1, ..., CHUNK_1 10.
- **Adaptive top tier**: the root's direct children are *not* always CHUNK_2097152. `Chunk::topTypeFor(worldSize)` (Chunk.cpp) returns the largest tier whose `CHUNK_SIZES` ≤ planet `worldSize` (fallback CHUNK_2097152). `RootChunk` stores it in `Chunk::subChunkType`; `RootChunk::getSquareOfSub`/`positionToIndex` and `Chunk::newSubChunk`/`convertIndex` read `subChunkType` (fallback `smaller(type)`). Earth → CHUNK_16777216, Jupiter → CHUNK_134217728, small moons → CHUNK_2097152.
- **Callers must compute `root_square` from the top tier, not CHUNK_2097152**: `Game::enter_planet`, `basic_test`, `SpaceGen` PLANET case, and tests use `ceil(worldSize / CHUNK_SIZES[Chunk::topTypeFor(worldSize)])`.
- **Path walkers are adaptive**: `getPath/getChunk/getChunkOrClosestAncestor* /getParents` step `level(type) − level(topType) + 1` (the `+1` is the root→top step) starting at `topType()` (the root's `subChunkType`). Do NOT hardcode a CHUNK_2097152 start.
- **Smoothness is type-keyed, not level-indexed**: `CHUNK_SMOOTHNESS_MIN/MAX` are `unordered_map<ChunkType,double>`, filled from `Planet::min_/max_<tier>` fields (now 10 per map). The old `[8]` arrays were level-indexed (off-by-one + CHUNK_1 OOB read) — fixed.
- `getSquareOfSub()` (Chunk.cpp) returns the subchunk grid dim: 16777216/134217728 → 8 (64 children); 2097152/131072/8192 → 16; 512 → 8; 16/64/4 → 4; 1 → 1.
- Level-threshold comments assume this numbering: precipitation `< 6`, slope `> 5 && < 9`, lakes `<= 8`.

## Plant Generation (`Chunk.cpp`, `World.hpp`)
- **Ratios down to CHUNK_512, placements partitioned down to CHUNK_16**: `generate_plant_ratios` is only called for subchunks `>= CHUNK_512` (in `makeTerrain`); `generate_plant_placements` gates on `CHUNK_512`. During generation the 512 makes its placements once, then `Chunk::distribute_plant_placements()` partitions them into its 64 children; each 64 partitions its own set into its 16 children (child index from `(pos - position)/child_size`). So every 16 chunk owns exactly the plants inside it — no lookup/filter at spawn. Chunks 4/1 track no placements.
- **Precipitation is interpolated from parent + parent neighbors** (`getNeighborPrecipitationInterpolation`, mirrors the height `getNeighborChunkInterpolation`): inverse-square weights the parent, N, NE, E neighbors' `precipitation` to the subchunk position. In `makeTerrain` the parent computes it per subchunk and passes it into `generate_plant_ratios(planet, species_count, interp_precipitation)`, which stores it back on `this->precipitation` so `vegetation_amount()` and ground-cover stay consistent (no per-chunk boundary jumps).
- **Species count scales with chunk size** (not a worldSize constant): `base_species` (from worldSize, min 50) scaled by `chunk_size / CHUNK_8192`, clamped `[8, base_species]` — coarser chunks keep the full pool, CHUNK_512 gets the smallest subset. Shared helper `Chunk::plant_species_count(planet)` (used by `makeTerrain` and the spawn hook).
- **Spawning is driven by the terrain spawn flow in `World::world_gen`** (not a separate poll loop): when a `CHUNK_16` is spawned (`next_type == START_TYPE`) and the entry's `desired_type` is `CHUNK_1` or `CHUNK_4`, `World::spawn_plants_for_chunk16(chunk, blocks_to_spawn)` runs. It reads the chunk_16's own already-distributed `plant_placements`, sets the origin y to `chunk_16->position->y - 6`, translates each plant, and adds blocks via `addBlockToSpawn` (tracked in `chunk->plantBlocks`). Plants therefore ride the same chunk16 terrain group and despawn with it.
- **Despawn requires no placement reset**: the chunk_16 keeps its partitioned placements for its lifetime, so on respawn it just re-spawns the same batch. `despawnChunk` frees the chunk16 terrain group and erases `chunk->plantBlocks` from `blocks_in_world` so `addBlockToSpawn` re-adds them next time.
- **Gate is `PLANT_GEN_ON`** (from `planet->hasLife` at init). No `ALL_TREES_RENDER_DISTANCE` poll anymore; plants spawn only where detailed terrain (chunk_1/chunk_4) is generated near the player.

## Maintenance
- Update this file with important details discovered during work (key files, patterns, gotchas, architecture insights).
- Keep it concise but useful for the next agent.
## Economy Simulation (`Economies.hpp`)
- Core file: `src/economies/Economies.hpp`. Test in `src/TEST/test.cpp`.
- `Economy` struct has `production_amounts`, `desired_amounts` (flow rates, units/tick), `stored_amounts`, `local_prices`, `cash`, `max_storage_capacity`, `total_productive_capacity`, `max_production`, `economy_seed` (for determinism), `tick_counter`, `population`, `pending_events`.
- `technological_advancement` is `double` (drifts +0.01/tick baseline, +0.01 per advanced neighbor).
- `tick()` has 8 phases: capacity reallocation → production → consumption → want decay → storage cap → trade → price adjustment → government → random events → event processing → recursion.
- `balance(int iterations=20)` fast-converges an economy to near-equilibrium (no storage/trade/gov). Used to initialize economies instead of running hundreds of ticks.
- `EconomyEvent` struct with `TECH_LOSS` and `PRODUCTION_SHOCK` types. External systems call `register_event()`, consumed in Phase 7 of tick.
- Want system: `population_tied` products get want set to `population × 0.1` each tick. Other products decay toward `total_productive_capacity × 0.05` at category-dependent rates.
- Production shrink: unprofitable products (price < cost) lose 0.5%/tick of capacity, reducing `total_productive_capacity`.
- Constants: `PRICE_ADJUST_K=0.25`, `SHIFT_RATE=0.03`, `TRADE_FRICTION=0.02`, `SURPLUS_DAMPEN=0.05`, `CORRUPTION_LEAK=0.05`, `RANDOM_EVENT_CHANCE=0.03`, `WANT_DECAY_RAW=0.05`, `WANT_DECAY_MANUFACTURED=0.10`, `WANT_DECAY_SERVICE=0.20`, `TECH_BASELINE_DRIFT=0.01`.
- Future ideas: credit/debt, R&D spending to raise tech, tech spillover by trade volume, war-triggered events from external systems.

## Window Generation
- `WindowStyleElement` in `Styles.hpp`: fields `min_width, max_width, min_height, max_height, sill_height, frame_material`.
- `gen_window_style()` in `StyleGen.hpp`: creates window style with frame material from `group->frame->materials[0]`, defaults to `BMaterial::DEFAULT`.
- `place_perimeter_frame()` in `StyleGen.hpp`: places frame blocks (left, right, top, optional sill) around a rectangular hole on a given wall side at an exterior-offset position.
- Windows are placed **during wall generation** as `WallCutout`s (like door holes), making actual holes in the wall.
- Window on exterior walls only (same `!neighbor` check). 80% chance per available wall segment.
- Window segments: left-of-door and right-of-door on each exterior wall side. Random width/height within `min_width/max_width, min_height/max_height`.
- Window sill at `y0 + sill_height` (default 1 block above floor).
- `Room::window_sides` bitmask tracks which sides have windows (bit 0=south, 1=west, 2=north, 3=east).
- Chimney generation checks `room->window_sides` — if a window exists on the chimney's wall side, chimney is skipped (window takes precedence).
- Door frames are placed alongside window frames using `place_perimeter_frame(include_sill=false)` on the exterior face.
- Window cutouts are added to the same `cutouts` vector as door holes, ensuring they don't conflict with doors. Door holes pre-computed in `layout_space()` as `Room::holes[4]`.

## Prefab Furniture

### Placement functions (StyleGen.hpp)
- `place_prefab_against_wall(prefab, room, blocks, floor_offset, centered)`: places a `Grid<string>` prefab against a wall in a room. Parameters:
  - `floor_offset` (default 1): how many blocks above room floor ly=0 maps to (1 for beds/chairs, 0 for counters/sinks).
  - `centered` (default true): if true, prefab is centered along the wall; if false, flush to the wall's min corner.
- `place_prefab_centered(prefab, room, blocks, floor_offset)`: places a prefab centered in the room (for tables).
- Both skip if the prefab blocks an active door hole.
- Prefab coordinate system: `lx` = along wall, `ly` = vertical, `lz` = into room from wall.

### Prefab generators (StyleGen.hpp)
| Function | Grid Size | Description |
|----------|-----------|-------------|
| `make_prefab_bed()` | 4×2×2 | 4 corner legs, solid mattress at ly=1 |
| `make_prefab_table_with_chairs()` | 5×3×5 | 3×3 table top, chairs on south and north sides |
| `make_prefab_counter(length, occ_blocks)` | length × (occ/2) × (occ/2) | Solid rectangular counter |
| `make_prefab_sink(occ_blocks)` | 2 × (occ/2) × (occ/2) | Counter with basin depression at top center |
| `make_prefab_chair()` | 1×2×1 | Single seat + back |
| `make_prefab_cupboard(occ_blocks)` | (occ/2) × occ × (occ/2) | Solid box (wardrobe/cupboard), floor_offset=0 |

### Room-type → furniture mapping (BuildingGen.hpp section loop, after lights, before wall gen)
- **BEDROOM**: `make_prefab_bed()` placed against wall, centered, floor_offset=1; plus `make_prefab_cupboard()` as wardrobe
- **KITCHEN**: `make_prefab_counter()` flush to corner, floor_offset=0; plus sink centered; plus cupboard
- **DINING**: `make_prefab_table_with_chairs()` centered in room, floor_offset=1
- **BATHROOM**: `make_prefab_sink()` placed against wall, centered, floor_offset=0
- **All rooms (except DINING)**: wall chair via `make_prefab_chair()` with `place_prefab_against_wall`, floor_offset=1
- Replace any `make_prefab_*()` with custom prefab functions to change shapes.

### Furniture overlap avoidance
Both `place_prefab_against_wall` and `place_prefab_centered` do a **dry-run overlap check** before placing: they scan all destination cells via `blocks.has()` and skip the placement (try next wall side / return) if any cell is already occupied. This means furniture naturally avoids other furniture — e.g., a wall chair won't overlap a bed or counter because the overlap check will push it to a different wall side. Vertical bounds are also checked (`furn_top <= room->end->y`).
