#pragma once


#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/core/memory.hpp>  // for memnew
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/box_shape3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/concave_polygon_shape3d.hpp>
#include <godot_cpp/classes/multi_mesh.hpp>
#include <godot_cpp/classes/multi_mesh_instance3d.hpp>
#include <godot_cpp/classes/collision_object3d.hpp>
#include <godot_cpp/classes/area3d.hpp>
#include <godot_cpp/classes/static_body3d.hpp>


#include "Block.h"
#include "util/CoordinateConversion.hpp"
#include "util/Position.h"


using namespace godot;
using namespace std;




struct MeshMaker {

	inline static double TEXTURE_FILE_SQUARE = 32;


	// static meshes for spawning blocks
	inline static double BLOCK_NUM_TO_GODOT_SIZE = CoordinateConversion::BLOCK_SCALE * CoordinateConversion::WORLD_SCALE;
	inline static Ref<Mesh> block64;
	inline static Ref<Mesh> block16;
	inline static Ref<Mesh> block4;
	inline static Ref<Mesh> block1;
	inline static Ref<Mesh> block_25;
	inline static Ref<Mesh> block_125;
	inline static unordered_map<double, Ref<Mesh>> meshes;
	inline static double block_2097152_MILLION_TO_BILLION_size = CoordinateConversion::game_scale(SpaceGenLevel::MILLION_TO_BILLION, 2097152);
	inline static double block_131072_MILLION_TO_BILLION_size = CoordinateConversion::game_scale(SpaceGenLevel::MILLION_TO_BILLION, 131072);
	inline static double block_8192_MILLION_TO_BILLION_size = CoordinateConversion::game_scale(SpaceGenLevel::MILLION_TO_BILLION, 8192);
	inline static Ref<Mesh> block_2097152_MILLION_TO_BILLION;
	inline static Ref<Mesh> block_131072_MILLION_TO_BILLION;
	inline static Ref<Mesh> block_8192_MILLION_TO_BILLION;
	inline static unordered_map<double, Ref<Mesh>> meshes_MILLION_TO_BILLION;
	inline static double block_8192_THOUSAND_TO_MILLION_size = CoordinateConversion::game_scale(SpaceGenLevel::THOUSAND_TO_MILLION, 8192);
	inline static double block_512_THOUSAND_TO_MILLION_size = CoordinateConversion::game_scale(SpaceGenLevel::THOUSAND_TO_MILLION, 512);
	inline static double block_16_THOUSAND_TO_MILLION_size = CoordinateConversion::game_scale(SpaceGenLevel::THOUSAND_TO_MILLION, 16);
	inline static Ref<Mesh> block_8192_THOUSAND_TO_MILLION;
	inline static Ref<Mesh> block_512_THOUSAND_TO_MILLION;
	inline static Ref<Mesh> block_16_THOUSAND_TO_MILLION;
	inline static unordered_map<double, Ref<Mesh>> meshes_THOUSAND_TO_MILLION;
	
	inline static unordered_map<string, Ref<ShaderMaterial>> materials;

	inline static const char* OLD_SHADER = R"(
				shader_type spatial;
				render_mode blend_mix, depth_draw_opaque, cull_back, depth_prepass_alpha;
				uniform sampler2D albedo_texture : source_color;
				uniform vec2 tex_size = vec2(32.0, 32.0);
				void fragment() {
						vec2 pixelUV = (floor(UV * tex_size) + 0.5) / tex_size;
						vec3 tex_color = texture(albedo_texture, pixelUV).rgb;
						ALBEDO = tex_color;
						ALPHA = 1.0;
				}
		)";

	// Assemble the shared pixel-look block shader from variant pieces. Every
	// block material uses the same core: nearest-neighbor texel sampling close up,
	// blended to plain UV mapping far away so distant plant speckle reads smooth
	// (scaled the same since the mesh UVs already repeat per meter). Only the
	// render_mode, extra uniforms, sample type, and body lines differ by variant.
	inline static String assemble_block_shader(bool is_opaque, float emission, bool double_sided) {
		String render_mode = is_opaque
			? "blend_mix, depth_draw_opaque, cull_back, depth_prepass_alpha"
			: "blend_mix, depth_draw_always, cull_back";
		// emissive materials are self-lit (unshaded) so they read as a light source
		// without casting real light; EMISSION x color crosses the HDR glow threshold.
		if (emission > 0.0f) render_mode = String("unshaded, ") + render_mode;

		String extra;
		if (!is_opaque) extra += "uniform float alpha_multiplier : hint_range(0.0, 1.0) = 1.0;\n";
		if (emission > 0.0f) extra += "uniform float emission = 1.0;\n";

		String sample = is_opaque
			? "vec3 tex_color = texture(albedo_texture, pixelUV).rgb;"
			: "vec4 tex_color = texture(albedo_texture, pixelUV);";

		String body;
		if (is_opaque) body = "ALBEDO = tex_color;\nALPHA = 1.0;";
		else body = "ALBEDO = tex_color.rgb;\nALPHA = tex_color.a * alpha_multiplier;";
		if (emission > 0.0f)
			body += is_opaque ? "\nEMISSION = tex_color * emission;" : "\nEMISSION = tex_color.rgb * emission;";

		String code = R"(
			shader_type spatial;
			render_mode __RENDER_MODE__;
			uniform sampler2D albedo_texture : source_color;
			uniform vec2 tex_size = vec2(32.0, 32.0);
			__EXTRA_UNIFORMS__
			void fragment() {
				vec2 pixelUV = (floor(UV * tex_size) + 0.5) / tex_size;
				__TEX_ALBEDO__
				__BODY__
			}
		)";
		code = code
			.replace("__RENDER_MODE__", render_mode)
			.replace("__EXTRA_UNIFORMS__", extra)
			.replace("__TEX_ALBEDO__", sample)
			.replace("__BODY__", body);


		if (double_sided) code = code.replace("cull_back", "cull_disabled");

		return code;
	}

	static Ref<ShaderMaterial> get_material(const string& material, bool double_sided = false) {
		string key = material + (double_sided ? "_double_sided" : "");
		auto cached = MeshMaker::materials.find(key);
		if (cached != MeshMaker::materials.end()) return cached->second;

		float alpha = 1.0f;
		float emission = 0.0f;
		if (material == BMaterial::WATER) alpha = 0.5f;
		else if (material == BMaterial::GALAXY_GLOW) alpha = 0.3f;
		else if (material == BMaterial::NEBULA_DUST) alpha = 0.25f;
		else if (material == BMaterial::NEBULA_GAS) alpha = 0.15f;
		else if (material == BMaterial::BLUE_STAR) {
			alpha = 0.5f;
			emission = 14.0f;
		}
		else if (material == BMaterial::LEAVES_GREEN) alpha = 0.5f;

		Ref<ShaderMaterial> result = MeshMaker::make_material(
			BMaterial::path(material), material, alpha, emission, double_sided
		);
		MeshMaker::materials[key] = result;
		return result;
	}

	// A view-dependent limb glow for distant planet atmospheres.
	static Ref<ShaderMaterial> make_atmosphere_material(Color color, float strength) {
		Ref<Shader> shader;
		shader.instantiate();
		shader->set_code(R"(
			shader_type spatial;
			render_mode unshaded, blend_add, depth_draw_never, cull_disabled;
			uniform vec4 atmosphere_color : source_color = vec4(0.25, 0.55, 1.0, 1.0);
			uniform float atmosphere_strength : hint_range(0.0, 1.0) = 0.0;

			void fragment() {
				float facing = abs(dot(normalize(NORMAL), normalize(VIEW)));
				float limb = pow(clamp(1.0 - facing, 0.0, 1.0), 2.5);
				ALBEDO = atmosphere_color.rgb;
				ALPHA = limb * atmosphere_strength * atmosphere_color.a;
			}
		)");

		Ref<ShaderMaterial> material;
		material.instantiate();
		material->set_shader(shader);
		material->set_shader_parameter("atmosphere_color", color);
		material->set_shader_parameter("atmosphere_strength", strength);
		return material;
	}
	

	// plants
	inline static unordered_map<string, Ref<Mesh>> LEAVES;
	inline static unordered_map<string, Ref<ShaderMaterial>> LEAF_TEXTURES;

	static void init_models() {
		
		MeshMaker::block64 = MeshMaker::make_cube(64 * BLOCK_NUM_TO_GODOT_SIZE, "", 1 * BLOCK_NUM_TO_GODOT_SIZE);
		MeshMaker::block16 = MeshMaker::make_cube(16 * BLOCK_NUM_TO_GODOT_SIZE, "", 1 * BLOCK_NUM_TO_GODOT_SIZE);
		MeshMaker::block4 = MeshMaker::make_cube(4 * BLOCK_NUM_TO_GODOT_SIZE, "", 1 * BLOCK_NUM_TO_GODOT_SIZE);
		MeshMaker::block1 = MeshMaker::make_cube(1 * BLOCK_NUM_TO_GODOT_SIZE, "", 1 * BLOCK_NUM_TO_GODOT_SIZE);
		MeshMaker::block_25 = MeshMaker::make_cube(0.25 * BLOCK_NUM_TO_GODOT_SIZE, "", 1 * BLOCK_NUM_TO_GODOT_SIZE);
		MeshMaker::block_125 = MeshMaker::make_cube(0.125 * BLOCK_NUM_TO_GODOT_SIZE, "", 1 * BLOCK_NUM_TO_GODOT_SIZE);
		MeshMaker::meshes = {
			{ 64, MeshMaker::block64 },
			{ 16, MeshMaker::block16 },
			{ 4, MeshMaker::block4 },
			{ 1, MeshMaker::block1 },
			{ 0.25, MeshMaker::block_25 },
			{ 0.125, MeshMaker::MeshMaker::block_125 },
		};
		MeshMaker::block_2097152_MILLION_TO_BILLION = MeshMaker::make_cube(block_2097152_MILLION_TO_BILLION_size, "", 1 * BLOCK_NUM_TO_GODOT_SIZE);
		MeshMaker::block_131072_MILLION_TO_BILLION = MeshMaker::make_cube(block_131072_MILLION_TO_BILLION_size, "", 1 * BLOCK_NUM_TO_GODOT_SIZE);
		MeshMaker::block_8192_MILLION_TO_BILLION = MeshMaker::make_cube(block_8192_MILLION_TO_BILLION_size, "", 1 * BLOCK_NUM_TO_GODOT_SIZE);
		MeshMaker::meshes_MILLION_TO_BILLION = {
			{ 2097152, MeshMaker::block_2097152_MILLION_TO_BILLION },
			{ 131072, MeshMaker::block_131072_MILLION_TO_BILLION },
			{ 8192, MeshMaker::block_8192_MILLION_TO_BILLION }
		};MeshMaker::
		MeshMaker::block_8192_THOUSAND_TO_MILLION = MeshMaker::make_cube(block_8192_THOUSAND_TO_MILLION_size, "", 1 * BLOCK_NUM_TO_GODOT_SIZE);
		MeshMaker::block_512_THOUSAND_TO_MILLION = MeshMaker::make_cube(block_512_THOUSAND_TO_MILLION_size, "", 1 * BLOCK_NUM_TO_GODOT_SIZE);
		MeshMaker::block_16_THOUSAND_TO_MILLION = MeshMaker::make_cube(block_16_THOUSAND_TO_MILLION_size, "", 1 * BLOCK_NUM_TO_GODOT_SIZE);
		MeshMaker::meshes_THOUSAND_TO_MILLION = {
			{ 8192, MeshMaker::block_8192_THOUSAND_TO_MILLION },
			{ 512, MeshMaker::block_512_THOUSAND_TO_MILLION },
			{ 16, MeshMaker::block_16_THOUSAND_TO_MILLION }
		};
		
		// plants
		MeshMaker::LEAVES = {
			{"pine_branch", MeshMaker::block1}
		};
		MeshMaker::LEAF_TEXTURES = {
			{"pine", MeshMaker::get_material(BMaterial::MOSS_SPARSE)}
		};
	}

    
	struct Face {
		Vector3 v0, v1, v2, v3;
		Vector3 normal;
	};

	/**
	 * Makes a cube of the specified size. 
	 * The texture_file can be an empty string. If that's the case no material will be assigned to the mesh.
	 * 
	 * texture_uv_scale controls how much the texture repeats on the face of the cube. By default
	 * the texture will repeat every meter (1). If you want it to repeat only once you'll want to
	 * set this to the same as the cube size.
	 */
    static Ref<Mesh> make_cube(float size, string texture_file="", float texture_uv_scale = 1.0f) {

        // Create the mesh data
		Ref<ArrayMesh> mesh;
		mesh.instantiate();

		PackedVector3Array vertices;
		PackedVector3Array normals;
		PackedInt32Array indices;


		// Define cube faces
		vector<Face> faces = make_cube_face_array(size);

		// for each face add the verticies and indices
		// (indices control how faces are drawn between vertices. If counter clockwise then front face. Otherwise back face)
		for (int f = 0; f < 6; f++) {

			// we create new vertices for each face so we can have flat shading and independent UVs (for displaying the texture on each face)
			int base = vertices.size();
			vertices.push_back(faces[f].v0);
			vertices.push_back(faces[f].v1);
			vertices.push_back(faces[f].v2);
			vertices.push_back(faces[f].v3);

			for (int i = 0; i < 4; i++)
				normals.push_back(faces[f].normal);

			// Two triangles per face (quad)

			indices.push_back(base + 0);
			indices.push_back(base + 2);
			indices.push_back(base + 1);
			indices.push_back(base + 0);
			indices.push_back(base + 3);
			indices.push_back(base + 2);

		}

		PackedVector2Array uvs;
		float uv_scale = size / texture_uv_scale;
		for (int f = 0; f < 6; f++) {
			uvs.push_back(Vector2(0, 0)); // v0
			uvs.push_back(Vector2(uv_scale, 0)); // v1
			uvs.push_back(Vector2(uv_scale, uv_scale)); // v2
			uvs.push_back(Vector2(0, uv_scale)); // v3
		}

		// Pack into Array for Godot
		Array arrays;
		arrays.resize(Mesh::ARRAY_MAX);
		arrays[Mesh::ARRAY_VERTEX] = vertices;
		arrays[Mesh::ARRAY_NORMAL] = normals;
		arrays[Mesh::ARRAY_INDEX] = indices;
		arrays[Mesh::ARRAY_TEX_UV] = uvs;

		// Add surface to mesh
		mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);

		// Material
		if (texture_file != "") {
			Ref<ShaderMaterial> material = make_material(texture_file);
			material->set_name(String::utf8(texture_file.c_str()));
			// material.instantiate();
			// material->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
			// material->set_albedo(Color(0.8, 0.8, 0)); 
			mesh->surface_set_material(0, material);
		}

        return mesh;
    }

	static vector<Face> make_cube_face_array(double size) {

		float s = size/2.0; // half-size

		// Define cube faces (counter-clockwise winding)
		return {
			// Front (+Z)
			{{-s, -s,  s}, { s, -s,  s}, { s,  s,  s}, {-s,  s,  s}, {0, 0, 1}},
			// Back (-Z)
			{{ s, -s, -s}, {-s, -s, -s}, {-s,  s, -s}, { s,  s, -s}, {0, 0, -1}},
			// Left (-X)
			{{-s, -s, -s}, {-s, -s,  s}, {-s,  s,  s}, {-s,  s, -s}, {-1, 0, 0}},
			// Right (+X)
			{{ s, -s,  s}, { s, -s, -s}, { s,  s, -s}, { s,  s,  s}, {1, 0, 0}},
			// Top (+Y)
			{{-s,  s,  s}, { s,  s,  s}, { s,  s, -s}, {-s,  s, -s}, {0, 1, 0}},
			// Bottom (-Y)
			{{-s, -s, -s}, { s, -s, -s}, { s, -s,  s}, {-s, -s,  s}, {0, -1, 0}}
		};

	}

	// build a ShaderMaterial from an already-populated Image: picks the shader
	// for the opacity/emission combo and sets the albedo/tex_size params. Shared
	// by make_material (loads the image from a file) and make_vegetated_material
	// (builds the image procedurally) so every material is made the same way.
	static Ref<ShaderMaterial> make_material_from_image(
		const Ref<Image>& image,
		const string& name,
		float alpha_override = 1.0f,
		float emission = 0.0f,
		bool double_sided = false
	) {
		// generate mipmaps so the far-range smooth sampling minifies without shimmer
		// (Image::load / set_data leave none by default)
		Ref<Image> mip_image = image->duplicate();
		mip_image->generate_mipmaps();

		Ref<ImageTexture> texture;
		texture.instantiate();
		texture->set_image(mip_image);

		// pick the shader for this opacity/emission combo (nearest-neighbor pixel look)
		bool is_opaque = alpha_override == 1.0f && image->detect_alpha() == Image::ALPHA_NONE;
		Ref<Shader> shader;
		shader.instantiate();
		String shader_code = MeshMaker::assemble_block_shader(is_opaque, emission, double_sided);
		shader->set_code(shader_code);

		Ref<ShaderMaterial> mat;
		mat.instantiate();
		mat->set_shader(shader);
		mat->set_name(String(name.c_str()));

		// set params for shader
		mat->set_shader_parameter("albedo_texture", texture);
		mat->set_shader_parameter("tex_size", Vector2(MeshMaker::TEXTURE_FILE_SQUARE, MeshMaker::TEXTURE_FILE_SQUARE));
		if (!is_opaque) {
			mat->set_shader_parameter("alpha_multiplier", alpha_override);
		}
		if (emission > 0.0f) {
			mat->set_shader_parameter("emission", emission);
		}

		return mat;
	}

	static Ref<ShaderMaterial> make_material(string texture_path_from_root, string name = "", float alpha_override = 1.0f, float emission = 0.0f, bool double_sided = false) {
		Ref<Image> image;
		image.instantiate();
		image->load(godot::String(texture_path_from_root.c_str()));
		return make_material_from_image(image, name, alpha_override, emission, double_sided);
	}

	static CollisionShape3D* make_hit_box(float block_godot_size) {
		Ref<BoxShape3D> box_shape;
		box_shape.instantiate();
		box_shape->set_size(Vector3(block_godot_size, block_godot_size, block_godot_size)); 

		CollisionShape3D* shape = memnew(CollisionShape3D);
		shape->set_shape(box_shape);
		return shape;
	}

	static CollisionShape3D* make_group_collider(
		Array block_positions, 
		Vector3 start_pos,
		double block_godot_size
	) {

		// Define faces
		vector<Face> faces = make_cube_face_array(block_godot_size);

		// ConcavePolygonShape3D expects a PoolVector3Array
		PackedVector3Array verts;
		for (Vector3 block_pos : block_positions) {

			Vector3 block_offset = block_pos - start_pos;

			// for each face add the verticies and indices
			// (indices control how faces are drawn between vertices. If counter clockwise then front face. Otherwise back face)
			for (int f = 0; f < 6; f++) {

				// we create new vertices for each face so we can have flat shading and independent UVs (for displaying the texture on each face)
				Vector3 v0 = faces[f].v0 + block_offset;
				Vector3 v1 = faces[f].v1 + block_offset;
				Vector3 v2 = faces[f].v2 + block_offset;
				Vector3 v3 = faces[f].v3 + block_offset;

				// Triangle 1
				verts.append(v0);
				verts.append(v2);
				verts.append(v1);

				// Triangle 2
				verts.append(v0);
				verts.append(v3);
				verts.append(v2);
			}
		}

		// Create the new ConcavePolygonShape3D
		Ref<ConcavePolygonShape3D> shape;
		shape.instantiate();
		shape->set_faces(verts);



		// Create the CollisionShape3D node
		CollisionShape3D* collider = memnew(CollisionShape3D);
		collider->set_shape(shape);


		// // DEBUG
		// Ref<ArrayMesh> mesh;
		// mesh.instantiate();
		// Array arrays;
		// arrays.resize(Mesh::ARRAY_MAX);
		// arrays[Mesh::ARRAY_VERTEX] = verts;
		// mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
		// MeshInstance3D* debug_mesh = memnew(MeshInstance3D);
		// debug_mesh->set_mesh(mesh);
		// collider->add_child(debug_mesh);


		return collider;
	}


	inline static int VERTS_PER_INSTANCE = 6 * 6; // 6 faces per cube, 2 traingles, 3 vertices each
	static CollisionShape3D* remake_group_collider(
		CollisionShape3D* existing_collider,
		int instance_index_removing
	) {
  
		int vertice_index = MeshMaker::VERTS_PER_INSTANCE * instance_index_removing;
		
		// copy verts excluding the ones for the new blocks
		Ref<ConcavePolygonShape3D> existing_shape = existing_collider->get_shape();
		PackedVector3Array existing_verts = existing_shape->get_faces();
		
		PackedVector3Array new_verts;
		int i = 0; 
		while (i < existing_verts.size()) {
			Vector3 vert = existing_verts[i];
			if (i == vertice_index) {
				i += MeshMaker::VERTS_PER_INSTANCE;
			}
			else {
				new_verts.append(vert);
				++i;
			}
		}

		// Create the new ConcavePolygonShape3D
		Ref<ConcavePolygonShape3D> shape;
		shape.instantiate();
		shape->set_faces(new_verts);

		// Create the CollisionShape3D node
		CollisionShape3D* collider = memnew(CollisionShape3D);
		collider->set_shape(shape);

		return collider;
	}

	static CollisionShape3D* add_to_collider(
		CollisionShape3D* existing_collider,
		Vector3 block_pos,
		Vector3 start_pos,
		double block_godot_size
	) {
		// copy verts excluding the ones for the new blocks
		Ref<ConcavePolygonShape3D> existing_shape = existing_collider->get_shape();
		PackedVector3Array existing_verts = existing_shape->get_faces();
		
		PackedVector3Array new_verts;
		int existing_size = existing_verts.size();
		for (int i = 0; i < existing_size; ++i) {
			Vector3 vert = Vector3(existing_verts[i].x, existing_verts[i].y, existing_verts[i].z);
			new_verts.append(vert);
		}

		// add new collider
		vector<Face> faces = make_cube_face_array(block_godot_size);
		Vector3 block_offset = block_pos - start_pos;
		for (int f = 0; f < 6; f++) {

			// we create new vertices for each face so we can have flat shading and independent UVs (for displaying the texture on each face)
			Vector3 v0 = faces[f].v0 + block_offset;
			Vector3 v1 = faces[f].v1 + block_offset;
			Vector3 v2 = faces[f].v2 + block_offset;
			Vector3 v3 = faces[f].v3 + block_offset;

			// Triangle 1
			new_verts.append(v0);
			new_verts.append(v2);
			new_verts.append(v1);

			// Triangle 2
			new_verts.append(v0);
			new_verts.append(v3);
			new_verts.append(v2);
		}

		// Create the new ConcavePolygonShape3D
		Ref<ConcavePolygonShape3D> shape;
		shape.instantiate();
		shape->set_faces(new_verts);
		int t = new_verts.size();

		// Create the CollisionShape3D node
		CollisionShape3D* collider = memnew(CollisionShape3D);
		collider->set_shape(shape);

		return collider;
	}



	static MultiMeshInstance3D* spawn_multimesh_instances(
		Node* parent,
		Ref<ShaderMaterial> godot_material, 
		Ref<Mesh> mesh, 
		vector<Vector3> spawn_positions,
		CollisionObject3D* collision_container, 
		double size
	) {

		vector<Transform3D> transforms;
		for (Vector3 pos : spawn_positions) {
			Transform3D t;
			t.origin = pos;
			transforms.push_back(t);
		}

		return spawn_multimesh_instances(
			parent,
			godot_material,
			mesh,
			transforms,
			collision_container,
			size
		);
	}



	static MultiMeshInstance3D* spawn_multimesh_instances(
		Node* parent,
		Ref<ShaderMaterial> godot_material, 
		Ref<Mesh> mesh, 
		vector<Transform3D>& spawn_positions,
		CollisionObject3D* collision_container, 
		double size
	) {
		if (mesh.is_null()) {
			cout << "NULL MESH" << endl;
		}
		
		// make a new UInstancedStaticMeshComponent
		Ref<MultiMesh> multi_mesh;
		multi_mesh.instantiate();
		multi_mesh->set_transform_format(MultiMesh::TransformFormat::TRANSFORM_3D);
		multi_mesh->set_mesh(mesh);
		multi_mesh->set_instance_count(spawn_positions.size());
		MultiMeshInstance3D* mm_instance = memnew(MultiMeshInstance3D);
		mm_instance->set_multimesh(multi_mesh);
		mm_instance->set_material_override(godot_material);
		parent->add_child(mm_instance);

		// spawn instances
		int i = 0;
		Array positions;
		for (Transform3D trans : spawn_positions) {

			positions.append(trans.origin);

			// add instance
			multi_mesh->set_instance_transform(i, trans);
			
			++i;
		}

		// add collider (optional — nullptr for ghost/no-collision blocks)
		if (collision_container != nullptr) {
			Vector3 start_pos = positions[0];
			Transform3D t;
			t.origin = start_pos;
			collision_container->set_transform(t);
			CollisionShape3D* hit_box = MeshMaker::make_group_collider(
				positions, 
				start_pos,
				size * CoordinateConversion::WORLD_SCALE * CoordinateConversion::BLOCK_SCALE
			);

			// set meta info for raycasting and deleting
			mm_instance->set_meta("size", size);
			hit_box->set_meta("size", size);
			hit_box->set_meta("start_pos", start_pos);
			hit_box->set_meta("positions", positions);
			
			// add to instance
			collision_container->add_child(hit_box);
			mm_instance->add_child(collision_container);
		}

		return mm_instance;
		
	}


	static Vector3 determine_block_pos(Vector3 hit_pos, Array& instance_positions) {
		Vector3 block_pos = instance_positions[0];
		float min_dist = hit_pos.distance_to(instance_positions[0]);
		for (Vector3 pos : instance_positions) {
			float dist = hit_pos.distance_to(pos);
			if (dist < min_dist) {
				min_dist = dist;
				block_pos = pos;
			}
		}

		return block_pos;
	}

	static void add_instance_to_multimesh(MultiMeshInstance3D* mesh, Vector3 position) {

		Ref<MultiMesh> mm = mesh->get_multimesh();

		// get old collider
		CollisionObject3D* collider = Object::cast_to<CollisionObject3D>(mesh->get_child(0));
		CollisionShape3D* hit_box = Object::cast_to<CollisionShape3D>(collider->get_child(0));

		double size = hit_box->get_meta("size");
		Vector3 start_pos = hit_box->get_meta("start_pos");
		Array positions = hit_box->get_meta("positions");

		// make new collider
		CollisionShape3D* new_hit_box = MeshMaker::add_to_collider(
			hit_box,
			position,
			start_pos,
			size * CoordinateConversion::BLOCK_SCALE * CoordinateConversion::WORLD_SCALE
		);
		CollisionObject3D* new_collider = memnew(StaticBody3D);
		new_collider->set_collision_layer_value(20, collider->get_collision_layer_value(20));
		new_collider->set_collision_layer(collider->get_collision_layer());
		new_collider->set_collision_mask(collider->get_collision_mask());

		Transform3D c_t;
		c_t.origin = start_pos;
		new_collider->set_transform(c_t);

		new_hit_box->set_meta("size", size);
		new_hit_box->set_meta("start_pos", start_pos);
		positions.append(position);
		new_hit_box->set_meta("positions", positions);
		
		new_collider->add_child(new_hit_box);
		mesh->add_child(new_collider);

		// add new instance
		if (positions.size() < mm->get_instance_count()) {
			// use the next unassigned instance
			Transform3D t;
			t.origin = position;
			mm->set_instance_transform(positions.size()-1, t);
		}
		else {
			// add more instances
			int count = mm->get_instance_count();
			mm->set_instance_count(count + 10);

			// set positions since they've been nullified when setting the instance count
			for (int i = 0; i < positions.size(); ++i) {
				Vector3 pos = positions[i];
				Transform3D tp;
				tp.origin = pos;
				mm->set_instance_transform(i, tp);
			}
		}


		// destroy the old collider
		collider->queue_free();
	}

	static void remove_instance_from_multimesh(MultiMeshInstance3D* mesh, int instance_index) {
		MeshMaker::remove_instance(mesh, instance_index);

        // get old collider
        CollisionObject3D* collider = Object::cast_to<CollisionObject3D>(mesh->get_child(0));
        CollisionShape3D* hit_box = Object::cast_to<CollisionShape3D>(collider->get_child(0));

        double size = hit_box->get_meta("size");
        Vector3 start_pos = hit_box->get_meta("start_pos");
        Array positions = hit_box->get_meta("positions");

        Ref<MultiMesh> mm = mesh->get_multimesh();
        if (mm->get_instance_count() != 0) {
            // make a new one with old block removed
            Vector3 block_pos = mm->get_instance_transform(instance_index).origin;
            CollisionShape3D* new_hit_box = MeshMaker::remake_group_collider(
                hit_box, 
                instance_index
            );

            // add the new collider to the instance
			CollisionObject3D* new_collider;
			if (Area3D* area = Object::cast_to<Area3D>(collider)) {
				new_collider = memnew(Area3D);
			} else if (StaticBody3D* body = Object::cast_to<StaticBody3D>(collider)) {
				new_collider = memnew(StaticBody3D);
			}
            Transform3D t;
            t.origin = start_pos;
            new_collider->set_transform(t);

            // set meta data for raycasts
            new_hit_box->set_meta("size", size);
            new_hit_box->set_meta("start_pos", start_pos);
			positions.remove_at(instance_index);
            new_hit_box->set_meta("positions", positions);

            // add to scene
            new_collider->add_child(new_hit_box);
            mesh->add_child(new_collider);


            // destroy the old collider
            collider->queue_free();
            
        }
	}

	
	static void remove_instance(MultiMeshInstance3D* mesh, int index) {
		if (!mesh) return;
		
		Ref<MultiMesh> mm = mesh->get_multimesh();
		if (!mm.is_valid()) return;

		int count = mm->get_instance_count();
		
		if (index >= 0 && index < count) {
			// Store ALL transforms BEFORE changing count
			Vector<Transform3D> all_transforms;
			for (int i = 0; i < count; i++) {
				all_transforms.push_back(mm->get_instance_transform(i));
			}
			
			// Change count (this CLEARS the buffer!)
			mm->set_instance_count(count - 1);
			
			// NOW set all the transforms back (skipping the deleted one)
			int new_idx = 0;
			for (int i = 0; i < count; i++) {
				if (i != index) {
					mm->set_instance_transform(new_idx, all_transforms[i]);
					new_idx++;
				}
			}
		}
	}





};
