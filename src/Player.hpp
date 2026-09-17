// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/rigid_body3d.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/quaternion.hpp>
#include <godot_cpp/variant/basis.hpp>
#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters3d.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/rich_text_label.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/font_variation.hpp>
#include <godot_cpp/classes/canvas_layer.hpp>
#include <godot_cpp/classes/viewport.hpp>


#include <algorithm>
// #include "util/Util.hpp"
// #include "util/TrigApprox.h"
// #include "util/Position.h"
// #include "util/CoordinateConversion.hpp"
// #include "../../Client.h"
// #include "../space/Planet.h"

#include "body/Body.hpp"
#include "util/serialization/KVSerializer.hpp"
// #include "util/BMaterial.hpp"
// #include "../Entity.h"
// #include "../../magic/Instrument.hpp"
#include <cmath>
// #include "util/Random.h"
// #include "../../magic/cast/cast_analysis/CastAnalysis.hpp"
#include "magic/cast/MagicCast.hpp"
#include <memory>
#include "blocks/Schemata.hpp"
#include <limits>
// #include "../../blocks/Assets.hpp"
#include "util/TrigApprox.h"
#include "util/PositionDouble.h"
#include "util/GodotUtil.hpp"
#include "util/DebugShapes.hpp"
#include "blocks/MeshMaker.hpp"
#include "world/World.hpp"
#include "util/StellarCoordinate.hpp"

using namespace std;
using namespace godot;


class Crosshair : public Control {
    GDCLASS(Crosshair, Control);

public:
    static void _bind_methods() {}

    void _draw() override {
        Vector2 center = get_viewport()->get_visible_rect().size / 2.0;
        float size = 10.0;
        Color color(1, 1, 1);

        draw_line(center - Vector2(size, 0), center + Vector2(size, 0), color, 2.0);
        draw_line(center - Vector2(0, size), center + Vector2(0, size), color, 2.0);

		// Vector2 t1 = Vector2(0, 5);
		// Vector2 t2 = Vector2(get_viewport()->get_visible_rect().size.x, 5);
        // draw_line(t1, t2, color, 2.0);
    }

    void _process(double delta) override {
        queue_redraw(); // redraw every frame
    }

    void _ready() override {
        set_anchors_preset(PRESET_FULL_RECT); // stretch to full viewport
    }
};

struct Player : public Node3D
{
	GDCLASS(Player, Node3D)
	
public: 

	World* world;

	RigidBody3D *hit_box;
	double body_size = 2 * CoordinateConversion::BLOCK_SCALE * CoordinateConversion::WORLD_SCALE;

	Camera3D* FollowCamera;

	// class Client* client;

	
	
	shared_ptr<Planet> planet;
	shared_ptr<StellarCoordinate> steller_location;
	LocationType locationType = LocationType::SPACE;

	// main parameters
	double base_moveSpeed = 20;
	double base_jumpForce = 20;
	double base_avgSpeed = 50;
	double moveSpeed = 20;
	double rotationRate = 0.005;
	double jumpForce = 20;

	// speed control
	double avgSpeed = 100; // once the speed is over this value you can't accelerate as much
	double speedMult; // determined at runtime to limit the acceleration the player can do
	double speedDamp = .05;
	Vector3 appliedForces;

	// rotation control
	Vector3 forward = GodotUtil::forward_v;
	Vector3 right = -GodotUtil::right_v;
	Quaternion rotation = Quaternion(0,0,0,0);
	double rotationDecay = 1000000; // bascially fights against whatever rotation we have to bring it to zero
	double rotationDecayThreshold = 10;

	// camera
	Vector3 playerHeadPosition = Vector3(0, 1, 0);
	double cameraZoom = 1;
	double zoomSensitivity = 10;
	double camera_rotation_y = TrigApprox::PIE;
	double camera_rotation_x = 0;
	double maxCameraRotation = TrigApprox::PI_3_HALVES - 0.001;
	double minCameraRotation = TrigApprox::PI_HALVES + 0.001;



	// body
	shared_ptr<Body> body;
    

	// terrain popping
	bool set_to_spawn = true;
	float offset_from_chunk = 0;
	// USphereComponent* CollisionSphere;
	Vector3 last_position = Vector3(0, 0, 0);


	// magic casting
	bool instrument_equipped = false;
	// shared_ptr<Instrument> instrument;
	shared_ptr<MagicCast> cast = std::make_shared<MagicCast>();
	// ConstructorHelpers::FObjectFinder<UStaticMesh> block1 = MeshMaker::create_cube_mesh(1);


	// placing blocks
	Ref<Mesh> selected_block = MeshMaker::make_cube(0.25 * CoordinateConversion::BLOCK_SCALE * CoordinateConversion::WORLD_SCALE);
	Ref<Material> selected_block_material = MeshMaker::make_material(BMaterial::path(BMaterial::DIRT));
	double selected_block_size = 0.25;
	double place_distance = 15;


	// character editing
	double edit_move_speed = 0.002;
	bool charactor_editor_equipped = false;
	bool editing_character = false;
	// shared_ptr<BodyPart> currently_animating_limb = nullptr;
	Vector3 edit_position; // position of camera when editing character


	// schemata making
	bool schemata_tool_equipped = false;
	bool start_selected = false;
	Vector3 schemata_start;
	Vector3 schemata_end;
	shared_ptr<Schemata> selected_schemata = nullptr;

	// schemata placement mode
	bool schemata_placement_mode = false;
	int schemata_rotation = 0;
	vector<MultiMeshInstance3D*> ghost_block_instances;
	vector<shared_ptr<Block>> current_placement_blocks;
	Vector3 last_ghost_anchor;


	// character animation editing



	// mouse clicks avoid debug locking
	bool debug_switch = false;



	// debug
	bool debug = true;
	int tick = 0;
	RichTextLabel* debug_label = nullptr;

		
	// Sets default values
	// TSubclassOf<class UUserWidget> cross_hair_class;
	Player() {}

	void init(World* world, shared_ptr<Planet> planet, shared_ptr<StellarCoordinate> steller_location, LocationType locationType) {
		this->world = world;
		this->planet = planet;
		this->steller_location = steller_location;
		this->locationType = locationType;
	}

	// Switch between zero-gravity space flight and planet-surface gravity
	void set_location_mode(LocationType type) {
		this->locationType = type;
		if (type == LocationType::SPACE) {
			this->hit_box->set_gravity_scale(0.0);
			this->moveSpeed = this->base_moveSpeed / 10;
			this->jumpForce = this->base_jumpForce / 10;
			this->avgSpeed = this->base_avgSpeed / 10;
			// space markers live 5000–10000 godot units out; raise the far plane so they render
			if (this->FollowCamera) {
				this->FollowCamera->set_near(1.0);
				this->FollowCamera->set_far(40000.0);
			}
		} else {
			this->hit_box->set_gravity_scale(this->planet ? this->planet->gravity / 9.8 : 0.0);
			this->moveSpeed = this->base_moveSpeed;
			this->jumpForce = this->base_jumpForce;
			this->avgSpeed = this->base_avgSpeed;
			if (this->FollowCamera) {
				this->FollowCamera->set_near(0.05);
				this->FollowCamera->set_far(10000.0);
			}
		}
	}

	// Recalculate only the existing orbit angle for the new terrain down direction.
	void orient_camera_for_surface(const Vector3 &space_down) {
		if (!FollowCamera) return;
		Vector3 from = space_down.normalized();
		if (from.length_squared() < 1e-12) return;

		// The original camera equation gives look.dot(down) == sin(camera_rotation_y - PI).
		Vector3 camera_look = GodotUtil::forward(FollowCamera).normalized();
		double down_component = Math::clamp((double)camera_look.dot(from), -1.0, 1.0);
		camera_rotation_y = TrigApprox::PIE + asin(down_component);
		camera_rotation_y = Math::clamp(camera_rotation_y, minCameraRotation, maxCameraRotation);
	}

	static void _bind_methods() {
	}

	// Called when the game starts or when spawned
	void _ready()
	{

		// Enable physics simulation
		this->hit_box = memnew(RigidBody3D);
        this->hit_box->set_name("hit_box");
        this->hit_box->set_mass(1.0); // optional
        add_child(this->hit_box);
        MeshInstance3D* mesh_instance = memnew(MeshInstance3D);
        this->hit_box->add_child(mesh_instance);
		// Ref<Mesh> mesh = MeshMaker::make_cube(1, BMaterial::path(BMaterial::DIRT));
		Ref<Mesh> mesh = MeshMaker::make_cube(
			body_size, 
			"",
			body_size
		);
		Ref<ShaderMaterial> material = MeshMaker::make_material("assets/textures/BDOG.png", "BDOG", 0.5f);
		mesh->surface_set_material(0, material);
        mesh_instance->set_mesh(mesh);
		CollisionShape3D* shape = MeshMaker::make_hit_box(body_size);
		this->hit_box->add_child(shape);


		// Set up camera following
        this->FollowCamera = memnew(Camera3D);
        this->FollowCamera->set_transform(Transform3D(Basis(), Vector3(0,8,-11)));
        this->FollowCamera->set_current(true);
        this->add_child(this->FollowCamera);
        this->FollowCamera->look_at(Vector3(0, 0, 0), Vector3(0, 1, 0)); // Look at origin, Y up
        
		// set_location_mode ran before the camera existed; apply the clip now
        if (this->locationType == LocationType::SPACE) {
            this->FollowCamera->set_near(1.0);
            this->FollowCamera->set_far(20000.0);
        }

		set_location_mode(this->locationType);

		// this->capture_cursor();
		// this->planet = client->world->planet;

		// set up body
		this->body = std::make_shared<Body>(this->hit_box, MeshMaker::meshes);


		// make crosshair
		CanvasLayer* canvas_layer = memnew(CanvasLayer);
		get_viewport()->call_deferred("add_child", canvas_layer);
		Crosshair* crosshair = memnew(Crosshair);
		canvas_layer->call_deferred("add_child", crosshair);


		// set up inputs
		InputMap *input = InputMap::get_singleton();

		add_input(input, "move_forward", KEY_W);
        add_input(input, "move_back", KEY_S);
        add_input(input, "move_right", KEY_D);
        add_input(input, "move_left", KEY_A);

        add_input(input, "jump", KEY_SPACE);
        add_input(input, "crouch", KEY_SHIFT);

        add_mouse_input(input, "left_click", MouseButton::MOUSE_BUTTON_LEFT);
        add_mouse_input(input, "right_click", MouseButton::MOUSE_BUTTON_RIGHT);
        add_mouse_input(input, "middle_click", MouseButton::MOUSE_BUTTON_MIDDLE);

        add_mouse_input(input, "scroll_up", MouseButton::MOUSE_BUTTON_WHEEL_UP);
        add_mouse_input(input, "scroll_down", MouseButton::MOUSE_BUTTON_WHEEL_DOWN);

        add_input(input, "escape", KEY_ESCAPE);

        add_input(input, "rotate_schemata_cw", KEY_R);
        add_input(input, "rotate_schemata_ccw", KEY_Q);


		// Player::capture_cursor();

		// set player position
		// this->hit_box->set_global_position(Vector3(0,300,0));


	}


	void add_input(InputMap* input, const String &action_name, Key keycode) {
		input->add_action(action_name);
		Ref<InputEventKey> ev;
		ev.instantiate();
		ev->set_keycode(keycode);
		input->action_add_event(action_name, ev);
    }

	void add_mouse_input(InputMap* input, const String &action_name, MouseButton keycode) {
		input->add_action(action_name);
		Ref<InputEventMouseButton> ev;
		ev.instantiate();
		ev->set_button_index(keycode);
		input->action_add_event(action_name, ev);
	}

	// Called every frame
	void _physics_process(double delta)
	{

		input(delta);

		if (!this->editing_character) {
			this->body->animate(delta, AnimationType::RUN);
		}
		
		calculateSpeedMult();

		rotationDamping();

		UpdateCameraPosition();

		handleStuckPlayer();

		reportWorldPosition();

		magicSwirling(delta);

		drawSchemataBox();

		ghostSchemata();

		debugInfo();


		// if (this->debug_switch) {
		//     // change to whatever you want to debug
		//     this->middle_click();
		//     this->debug_switch = false;
		// }

	}

	// connection User Input to Our methods down below
	void input(double delta)
	{
		Input *input = Input::get_singleton();


		// left right
		float move_forward_val = (input->is_action_pressed("move_forward") ? 1.0f : 0.0f)
			+ (input->is_action_pressed("move_back") ? -1.0f : 0.0f);

        float move_right_val = (input->is_action_pressed("move_right") ? -1.0f : 0.0f)
			+ (input->is_action_pressed("move_left") ? 1.0f : 0.0f);

        moveForward(move_forward_val);
        moveRight(move_right_val);

        // Jump & crouch
        if (input->is_action_pressed("jump")) {
			jump(1.0f);
		}
        if (input->is_action_pressed("crouch")) {
			slamDown(1.0f);
		}

        // Scroll
        if (input->is_action_just_pressed("scroll_up")) {
			scroll(0.1f);
		}
        if (input->is_action_just_pressed("scroll_down")) {
			scroll(-0.1f);
		}
		
		// mouse look
        Vector2 mouse_delta = input->get_last_mouse_velocity();
        turnX(-mouse_delta.x, delta);
        turnY(mouse_delta.y, delta);

		// mouse clicks
        if (input->is_action_just_released("left_click")) {
			left_click();
		}
        // if (input->is_action_just_released("left_click")) {
		// 	left_click_up();
		// }
        if (input->is_action_just_pressed("right_click")) {
			right_click();
		}
        if (input->is_action_just_pressed("middle_click")) {
			middle_click();
		}

		// schemata placement rotation
		if (schemata_placement_mode && selected_schemata != nullptr) {
			if (input->is_action_just_pressed("rotate_schemata_cw")) {
				selected_schemata->rotate_90_cw();
				schemata_rotation = (schemata_rotation + 1) % 4;
				last_ghost_anchor = Vector3(INFINITY, INFINITY, INFINITY);
			}
			if (input->is_action_just_pressed("rotate_schemata_ccw")) {
				selected_schemata->rotate_90_ccw();
				schemata_rotation = (schemata_rotation + 3) % 4;
				last_ghost_anchor = Vector3(INFINITY, INFINITY, INFINITY);
			}
		}

		// other
        if (input->is_action_just_pressed("escape")) {
			escape();
		}
	}

	void moveForward(float Value)
	{
		// Check if there's a valid controller and if the input value is non-zero
		if (Math::is_zero_approx(Value)) return;


		/*string forceString(TCHAR_TO_UTF8(*Direction.ToString()));
		LOG("forward: " + forceString);*/

		if (this->editing_character) {

			// Calculate the movement direction based on the camera look direction
			Vector3 direction = GodotUtil::forward(this->FollowCamera) * Value * moveSpeed * this->speedMult * this->edit_move_speed;
			
			this->edit_position += direction;
		}
		else {
			// Calculate the movement direction based on the controller's rotation
			Vector3 direction = this->forward * Value * moveSpeed * this->speedMult;

			// Apply a force in the forward direction based on the input value
			hit_box->apply_central_force(direction);
			this->appliedForces += direction;
		}
	}

	void moveRight(float Value)
	{
		// Check if there's a valid controller and if the input value is non-zero
		if (Math::is_zero_approx(Value)) return;

		this->right = GodotUtil::up_v.cross(this->forward);
		this->right = GodotUtil::safe_normalization(this->right);


		/*string forceString(TCHAR_TO_UTF8(*Direction.ToString()));
		LOG("right: " + forceString);*/

		if (this->editing_character) {

			// Calculate the movement direction based on the camera look direction
			Vector3 direction = GodotUtil::right(this->FollowCamera) * Value * moveSpeed * this->speedMult * this->edit_move_speed;
			
			this->edit_position -= direction;
		}
		else {
			// Calculate the movement direction based on the controller's rotation
			Vector3 direction = this->right * Value * moveSpeed * this->speedMult;

			// Apply a force in the right direction based on the input value
			hit_box->apply_central_force(direction);
			this->appliedForces += direction;
		}
	}

	void turnY(float Value, double delta)
	{
		// Check if the input value is non-zero
		if (Math::is_zero_approx(Value)) return;

		
		// looking around
		if (!this->cast->currently_casting) {
			this->camera_rotation_y += Value * rotationRate * delta;
			if (this->camera_rotation_y > this->maxCameraRotation) this->camera_rotation_y = this->maxCameraRotation;
			if (this->camera_rotation_y < this->minCameraRotation) this->camera_rotation_y = this->minCameraRotation;
		}

	}

	void turnX(float Value, double delta)
	{
		// Check if the input value is non-zero
		if (Math::is_zero_approx(Value)) return;

		// looking around
		if (!this->cast->currently_casting) {
			Quaternion rot = Quaternion(GodotUtil::up_v, Value * rotationRate * delta);
		
			this->forward = rot.xform(this->forward);
			this->right = rot.xform(this->right);

			// for shift look around
			bool shift = false;
			if (shift) {
				this->camera_rotation_x += Value * rotationRate * delta;
			}
		}
	
	}

	void jump(float Value)
	{
		// Check if the input value is non-zero
		if (Math::is_zero_approx(Value)) return;


		if (this->editing_character) {

			// Calculate the movement direction based on the camera look direction
			Vector3 direction = GodotUtil::up_v * this->jumpForce * this->edit_move_speed;
			
			this->edit_position += direction;
		}
		else {
			this->hit_box->apply_central_force(GodotUtil::up_v * this->jumpForce);
		}

	}

	void scroll(float value) {
		this->cameraZoom += value * this->zoomSensitivity;
		if (this->cameraZoom < 0.01) this->cameraZoom = 0.01;
	}

	void slamDown(float Value)
	{
		// Check if the input value is non-zero
		if (Math::is_zero_approx(Value)) return;

		if (this->editing_character) {

			// Calculate the movement direction based on the camera look direction
			Vector3 direction = GodotUtil::up_v * -this->jumpForce * this->edit_move_speed;
			
			this->edit_position += direction;
		}
		else {
			this->hit_box->apply_central_force(GodotUtil::up_v * - this->jumpForce);
		}
	}

	void left_click()
	{
		if (!this->world || !this->planet) return;
		// magic
		if (this->instrument_equipped) {
			// this->cast->start();
		}
		// intializing character editing mode
		else if (this->charactor_editor_equipped && !this->editing_character) {
			cout << "CHARACTER EDIT MODE" << endl;
			this->editing_character = true;
			this->edit_position = this->FollowCamera->get_global_position();

			CollisionShape3D* collider = Object::cast_to<CollisionShape3D>(this->hit_box->get_child(1));
			collider->set_disabled(true);
			this->hit_box->set_freeze_enabled(true);

			this->body->reset_animations();

			// set in position in grid
			shared_ptr<PositionDouble> position = CoordinateConversion::convert_fVector_to_position_double(
				this->hit_box->get_global_position(),
				this->world->cubePool->terrainZeroZero,
				this->world->planet->worldSize,
				0.25
			);
			Vector3 grid_position = CoordinateConversion::convert_position_double_to_fVector(
				position,
				this->world->cubePool->terrainZeroZero,
				this->world->planet->worldSize,
				0.25
			);
			Transform3D t = Transform3D(Basis(Quaternion(0.0f, 0.0f, 0.0f, 1.0f)), grid_position);
			this->hit_box->set_global_transform(t);

		}
		// block breaking
		else {
			cout << endl;
			cout << "You freakin attacked me bro! Or break my block" << endl;
			Ref<Mesh> mesh_comp = nullptr;
			CollisionShape3D* hit_collider = nullptr;
			Dictionary hit_result;
			Vector3 instance_location;
			double box_size;
			shared_ptr<PositionDouble> block_position = this->raycast_for_block_position(hit_result, mesh_comp, instance_location, box_size, hit_collider);
		
			if (block_position != nullptr) {
				cout << fixed << setprecision(2);
				cout << "block pos " << block_position->x << " " << block_position->y << " " << block_position->z << endl;

				// delete character block
				if (this->editing_character) {
					int instance_index = 0;
					Array positions = hit_collider->get_meta("positions");
					for (int i = 0; i < positions.size(); ++i) {
						if (positions[i] == instance_location) {
							instance_index = i;
						}
					}


					MultiMeshInstance3D* mm_instance = Object::cast_to<MultiMeshInstance3D>(hit_collider->get_parent());
					this->body->delete_block(mm_instance, instance_index);
				}
				// delete world block
				else {
					
					shared_ptr<Block> block = std::make_shared<Block>();
					block->position_double = block_position;
					block->size = box_size;
					this->world->cubePool->player_destroy_block(block);
					this->world->river_gen->report_chunk_change(block_position->x, block_position->z);
				}
			}

		}
	}

	void magicSwirling(float DeltaTime) {
		// if (this->cast->currently_casting) {
		// 	APlayerController* PC = Cast<APlayerController>(GetController());
		// 	double mouse_x, mouse_y;
		// 	PC->GetMousePosition(mouse_x, mouse_y);

		// 	this->cast->capture_mouse_sample(mouse_x, mouse_y);
		// 	UStaticMeshComponent* mesh = NewObject<UStaticMeshComponent>(this);
		// 	this->cast->spawn_cast_items(this, mesh, this->block1);
		// 	this->cast->swing_cast_items_around_player(mouse_x, mouse_y, this->FollowCamera, this);
		// }
	}

	void left_click_up() {
		
		// // magic
		// if (this->instrument_equipped) {

		// 	this->cast->finish(this->FollowCamera);
		// }
	}

	void right_click()
	{
		if (!this->world || !this->planet) return;
		// schemata placement confirm
		if (schemata_placement_mode && selected_schemata != nullptr && !current_placement_blocks.empty()) {
			for (auto* inst : ghost_block_instances) {
				inst->queue_free();
			}
			ghost_block_instances.clear();

			for (auto& block : current_placement_blocks) {
				this->world->cubePool->place_block(
					block->position_double,
					block->size,
					MeshMaker::meshes[block->size],
					MeshMaker::get_material(block->material),
					true
				);
			}
			this->world->river_gen->report_chunk_change(current_placement_blocks);

			DebugShapes::remove_debug_box();
			last_ghost_anchor = Vector3(INFINITY, INFINITY, INFINITY);
			return;
		}

		if (!this->instrument_equipped) {
			cout << std::fixed << std::setprecision(2) << endl;
			cout << "You tried to put a block on me bro!" << endl;

			Ref<Mesh> mesh_comp = nullptr;
			CollisionShape3D* hit_collider = nullptr;
			Dictionary hit_result;
			Vector3 instance_location;
			double box_size;
			shared_ptr<PositionDouble> hit_block_position = this->raycast_for_block_position(hit_result, mesh_comp, instance_location, box_size, hit_collider);
		

			// box_size = test;
			if (hit_block_position != nullptr) {

				cout << fixed << setprecision(2);
				cout << "block pos " << hit_block_position->x << " " << hit_block_position->y << " " << hit_block_position->z << endl;

				Vector3 hit_location = hit_result["position"];
				shared_ptr<PositionDouble> dimensions = std::make_shared<PositionDouble>(
					this->selected_block_size, this->selected_block_size, this->selected_block_size
				);
				shared_ptr<PositionDouble> block_position = compute_placement_base(
					hit_block_position,
					hit_location,
					instance_location,
					box_size,
					this->selected_block_size,
					dimensions,
					this->FollowCamera->get_global_position() - hit_location
				);

				if (this->editing_character) {


					Node* collider_container = hit_collider->get_parent();
					MultiMeshInstance3D* mesh = Object::cast_to<MultiMeshInstance3D>(collider_container->get_parent());
					this->body->spawn_block(mesh, this->selected_block, this->selected_block_material, block_position, this->selected_block_size, this->planet, this->world->cubePool->terrainZeroZero);

				}
				else {
					this->world->cubePool->place_block(
						block_position, 
						this->selected_block_size, 
						this->selected_block, 
						this->selected_block_material, 
						true
					);
					this->world->river_gen->report_chunk_change(block_position->x, block_position->z);
				}
			}

			// make a thing here for attching arms and legs to character
			// if a leg is equiped then you should be able to place it with moving the block
			// it should be attached by the hip block of the leg, or the should block of an arm
			// should be able to rotate is with Q E 90 degrees increments around the y axis
			// then you can edit the pieces like normal with blocks

		}


	}

	void middle_click() {
		if (!this->world || !this->planet) return;

		CollisionShape3D* hit_collider = nullptr;
		Ref<Mesh> mesh_comp = nullptr;
		Dictionary result;
		Vector3 instance_location;
		Vector3 cast_start;
		this->get_player_target_block(hit_collider, mesh_comp, result, instance_location, cast_start);


		// schemata stuff
		if (this->schemata_tool_equipped) {

			if (this->start_selected) {
				this->start_selected = false;

				// collect blocks in square
				vector<shared_ptr<Block>> blocks;
				shared_ptr<PositionDouble> start = CoordinateConversion::convert_fVector_to_position_double(
					this->schemata_start, 
					this->world->cubePool->terrainZeroZero,
					this->planet->worldSize, 
					this->selected_block_size
				);
				shared_ptr<PositionDouble> end = CoordinateConversion::convert_fVector_to_position_double(
					this->schemata_end, 
					this->world->cubePool->terrainZeroZero,
					this->planet->worldSize, 
					this->selected_block_size
				);

				double min_x = start->x;
				double max_x = end->x;
				double min_z = start->z;
				double max_z = end->z;
				double min_y = start->y;
				double max_y = end->y;
				if (abs(start->x + this->planet->worldSize - end->x) < abs(start->x - end->x)) {
					if (start->x + this->planet->worldSize > end->x) {
						min_x = end->x;
						max_x = start->x;
					}
				}
				else if (abs(start->x - (end->x + this->planet->worldSize)) < abs(start->x - end->x)) {
					if (start->x > (end->x + this->planet->worldSize)) {
						min_x = end->x;
						max_x = start->x;
					}
				}
				else {
					if (start->x > end->x) {
						min_x = end->x;
						max_x = start->x;
					}
				}

				if (abs(start->z + this->planet->worldSize - end->z) < abs(start->z - end->z)) {
					if (start->z + this->planet->worldSize > end->z) {
						min_z = end->z;
						max_z = start->z;
					}
				}
				else if (abs(start->z - (end->z + this->planet->worldSize)) < abs(start->z - end->z)) {
					if (start->z > (end->z + this->planet->worldSize)) {
						min_z = end->z;
						max_z = start->z;
					}
				}
				else {
					if (start->z > end->z) {
						min_z = end->z;
						max_z = start->z;
					}
				}

				if (start->y > end->y) {
					min_y = end->y;
					max_y = start->y;
				}
				

				// get blocks from edit pool
				this->world->cubePool->get_blocks_in_range(
					blocks, 
					this->world->cubePool->terrainZeroZero, 
					this->world->planet->worldSize, 
					min_x,
					min_y,
					min_z,
					max_x,
					max_y,
					max_z
				);

				// make schemata
				if (!blocks.empty()) {

					shared_ptr<PositionDouble> player_position = CoordinateConversion::convert_fVector_to_position_double(
						this->hit_box->get_global_position(), 
						this->world->cubePool->terrainZeroZero, 
						this->planet->worldSize, 
						0
					);
					shared_ptr<Schemata> new_schemata = std::make_shared<Schemata>(blocks, player_position, planet->worldSize);

					this->selected_schemata = new_schemata;
					this->schemata_placement_mode = true;
					this->schemata_rotation = 0;


					stringstream ss;
					KVSerializer::serialize(ss, new_schemata);
					Util::writeToFile("assets/Plants/branches/SCHEMATA.kv", ss.str());
					
				}

				DebugShapes::remove_debug_box();
			}
			else {
				if (mesh_comp != nullptr) {
					if (!this->start_selected) {
						this->start_selected = true;
						this->schemata_start = instance_location;
					}
				}
			}
		}

		// selecting blocks
		if (mesh_comp != nullptr) {
			// this->release_cursor();
			this->selected_block = mesh_comp;
			this->selected_block_material = mesh_comp->surface_get_material(0);
			this->selected_block_size = hit_collider->get_meta("size");
			string material_name = this->selected_block_material != nullptr? this->selected_block_material->get_name().utf8().get_data() : "none";
			cout << "selected block: " << material_name << " (size) " << this->selected_block_size << endl;
		}
		else {
			cout << "nothing selected" << endl;
		}

		// this->capture_cursor();
		
	}

	static void capture_cursor() {
		Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_CAPTURED);
	}

	static void release_cursor() {
		Input::get_singleton()->set_mouse_mode(Input::MOUSE_MODE_VISIBLE);
	}

	void get_player_target_block(CollisionShape3D*& hit_collider, Ref<Mesh>& mesh, Dictionary& result,  Vector3& instance_location, Vector3& cast_start) {

		// Start and end points for the raycast
		cast_start = this->FollowCamera->get_global_transform().origin;
		Vector3 look = GodotUtil::forward(this->FollowCamera);
		Vector3 end = cast_start + (look * this->place_distance); // Raycast distance x units


		// perform raycast
		Array exclude;
		// exclude.append(this);
		// exclude.append(this->hit_box);
		uint32_t mask = this->editing_character? (1 << 19) : 0xFFFFFFFF;
		result = GodotUtil::raycast(cast_start, end, exclude, mask);
		if (!result.is_empty()) {
			// There was a hit
			// Vector3 hit_location = result["position"];
			Object* obj = result["collider"];
			StaticBody3D* body = Object::cast_to<StaticBody3D>(obj);
			if (body != nullptr && body->get_child_count() != 0) {
				hit_collider = Object::cast_to<CollisionShape3D>(body->get_child(0));
				MultiMeshInstance3D* mm_instance = Object::cast_to<MultiMeshInstance3D>(body->get_parent());
				if (mm_instance != nullptr) {
					mesh = mm_instance->get_multimesh()->get_mesh();
					Node3D* parent = Object::cast_to<Node3D>(mm_instance->get_parent());
					if (!parent) return;

					Vector3 block_pos = result["position"];
					block_pos -= parent->get_global_position();
					Array positions = hit_collider->get_meta("positions");
					instance_location = MeshMaker::determine_block_pos(block_pos, positions) + parent->get_global_position();
				}
				else {
					cout << "not a world block" << endl;
				}
			}
		}
	

	}

	shared_ptr<PositionDouble> raycast_for_block_position(Dictionary& result, Ref<Mesh>& mesh_comp, Vector3& instance_location, double& bounding_box_size, CollisionShape3D*& hit_collider) {

		if (!this->world || !this->world->planet) return nullptr;
		// If we hit something, try to break the block
		Vector3 cast_start;
		this->get_player_target_block(hit_collider, mesh_comp, result, instance_location, cast_start);
		if (hit_collider != nullptr) {

			bounding_box_size = hit_collider->get_meta("size");

			// cout << "instance_location " << instance_location.x << " " << instance_location.y << " " << instance_location.z << endl;
			shared_ptr<PositionDouble> position = CoordinateConversion::convert_fVector_to_position_double(
				instance_location,
				this->world->cubePool->terrainZeroZero,
				this->world->planet->worldSize,
				bounding_box_size
			);
			// cout << fixed << setprecision(2);
			// cout << "block pos " << position->x << " " << position->y << " " << position->z << endl;
			// cout << this->world->planet->worldSize << endl;

			return position;
		}
		else {
			// cout << "no hit" << endl;
		}

		return nullptr;
	}

	void escape() {
		if (this->editing_character) {
			this->editing_character = false;
			CollisionShape3D* collider = Object::cast_to<CollisionShape3D>(this->hit_box->get_child(1));
			collider->set_disabled(false);
			this->hit_box->set_freeze_enabled(false);
			// this->hit_box->SetSimulatePhysics(true);
			// this->hit_box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
		// clean up placement mode
		else if (this->schemata_placement_mode) {
			this->schemata_placement_mode = false;
			this->selected_schemata = nullptr;
			this->schemata_tool_equipped = false;
			for (auto* inst : ghost_block_instances) {
				inst->queue_free();
			}
			ghost_block_instances.clear();
			current_placement_blocks.clear();
			DebugShapes::remove_debug_box();
			cout << "exited schemata placement mode" << endl;
		}
		// debug stuff
		else {
			// if (!this->world->river_gen->river_simulations.empty()) {
			// 	this->world->river_gen->river_simulations[0]->started = true;
			// }
			this->schemata_tool_equipped = !this->schemata_tool_equipped;
			this->selected_schemata = nullptr;
			cout << "schemata_tool_equipped" << this->schemata_tool_equipped << endl;
		}
	}


	shared_ptr<PositionDouble> compute_placement_base(
		shared_ptr<PositionDouble>& hit_block_pos,
		Vector3 hit_location,
		Vector3 instance_location,
		double box_size,
		double placing_size,
		shared_ptr<PositionDouble>& dimensions,
		Vector3 look
	) {
		double SCALE = CoordinateConversion::BLOCK_SCALE * CoordinateConversion::WORLD_SCALE;

		shared_ptr<PositionDouble> block_position = std::make_shared<PositionDouble>(
			hit_block_pos->x, hit_block_pos->y, hit_block_pos->z
		);

		// face detection
		double x_diff = 2 * (hit_location.x - instance_location.x) / SCALE;
		double y_diff = 2 * (hit_location.y - instance_location.y) / SCALE;
		double z_diff = 2 * (hit_location.z - instance_location.z) / SCALE;
		x_diff = Util::round_if_almost(x_diff, box_size);
		y_diff = Util::round_if_almost(y_diff, box_size);
		z_diff = Util::round_if_almost(z_diff, box_size);

		bool is_bigger = placing_size > box_size;
		bool same_size = placing_size == box_size;
		bool is_smaller = placing_size < box_size;

		if (is_bigger) {
			x_diff = Util::truncate_to_precision(x_diff, box_size);
			y_diff = Util::truncate_to_precision(y_diff, box_size);
			z_diff = Util::truncate_to_precision(z_diff, box_size);

			block_position->x -= x_diff;

			if (y_diff > 0) {
				block_position->y += (y_diff / box_size) * placing_size;
			}
			else if (y_diff == 0) {
				block_position->y += placing_size - box_size;
			}
			else {
				block_position->y += (y_diff / box_size) * box_size;
			}
			block_position->z += z_diff;
		}
		else if (same_size) {
			x_diff = Util::truncate_to_precision(x_diff, placing_size);
			y_diff = Util::truncate_to_precision(y_diff, placing_size);
			z_diff = Util::truncate_to_precision(z_diff, placing_size);

			block_position->x -= x_diff;
			block_position->y += y_diff;
			block_position->z += z_diff;
		}
		else if (is_smaller) {
			double x_diff_d = 2 * (hit_location.x - instance_location.x) / CoordinateConversion::WORLD_SCALE;
			double y_diff_d = 2 * (hit_location.y - instance_location.y) / CoordinateConversion::WORLD_SCALE;
			double z_diff_d = 2 * (hit_location.z - instance_location.z) / CoordinateConversion::WORLD_SCALE;

			x_diff = Util::truncate_to_precision(x_diff_d, placing_size);
			y_diff = Util::truncate_to_precision(y_diff_d, placing_size);
			z_diff = Util::truncate_to_precision(z_diff_d, placing_size);

			if (x_diff_d > 0) x_diff += placing_size;
			if (y_diff_d < 0) y_diff -= placing_size;
			if (z_diff_d < 0) z_diff -= placing_size;

			x_diff -= box_size / 2;
			y_diff -= box_size / 2 - placing_size;
			z_diff += box_size / 2;

			block_position->x -= x_diff;
			block_position->y += y_diff;
			block_position->z += z_diff;
		}

		// look-direction adjustment for larger-than-hit objects
		if (is_bigger) {
			bool is_side = x_diff != 0;
			bool is_front_back = z_diff != 0;
			bool is_top_bottom = y_diff != 0;

			if (is_side) {
				bool west = x_diff > 0;
				bool right = west ? look.z > 0 : look.z < 0;

				if (west) {
					block_position->x -= dimensions->x - box_size;
				}

				if ((west && right) || (!west && !right)) {
					block_position->z -= dimensions->z - box_size;
				}
			}
			else if (is_front_back) {
				bool front = z_diff > 0;
				bool right = front ? look.x < 0 : look.x > 0;

				if (!front) {
					block_position->z -= dimensions->z - box_size;
				}

				if ((front && right) || (!front && !right)) {
					block_position->x -= dimensions->x - box_size;
				}
			}
			else if (is_top_bottom) {
				if (look.x < 0) {
					if (look.z > 0) {
						block_position->x -= dimensions->x - box_size;
						block_position->z -= dimensions->z - box_size;
					}
					else {
						block_position->x -= dimensions->x - box_size;
					}
				}
				else {
					if (look.z > 0) {
						block_position->z -= dimensions->z - box_size;
					}
				}
			}
		}

		return block_position;
	}

	void test_button(float Value) {
		
		// APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		// PlayerController->bShowMouseCursor = true;
	}   


	void drawSchemataBox() {
		if (this->schemata_tool_equipped && this->start_selected) {


			// raycast or go out 2m if raycast is far away
			Vector3 camera_location = this->FollowCamera->get_global_position();
			Vector3 look = GodotUtil::forward(this->FollowCamera);
			Vector3 cast_end = camera_location + (look * 15000.0f); // Raycast distance of 1000 units

			// perform raycast
			Array exclude;
			exclude.append(this);
			exclude.append(this->hit_box);
			Dictionary result = GodotUtil::raycast(camera_location, cast_end, exclude);

			bool bHit = !result.is_empty();
			this->schemata_end = this->hit_box->get_global_position() + this->playerHeadPosition + look * 4;
			this->schemata_end = Vector3(
				Util::round_to_precision(this->schemata_end.x, this->selected_block_size * CoordinateConversion::WORLD_SCALE), 
				Util::round_to_precision(this->schemata_end.y, this->selected_block_size * CoordinateConversion::WORLD_SCALE), 
				Util::round_to_precision(this->schemata_end.z, this->selected_block_size * CoordinateConversion::WORLD_SCALE)
			);
			if (bHit) {
				Vector3 hit_pos = result["position"];

				double dist_hit = (camera_location - hit_pos).length();
				if (dist_hit < 400) {
					Object* obj = result["collider"];
					StaticBody3D* body = Object::cast_to<StaticBody3D>(obj);
					CollisionShape3D* hit_collider = Object::cast_to<CollisionShape3D>(body->get_child(0));
					MultiMeshInstance3D* mm_instance = Object::cast_to<MultiMeshInstance3D>(body->get_parent());
					Ref<Mesh> mesh = mm_instance->get_multimesh()->get_mesh();

					Array positions = hit_collider->get_meta("positions");
					this->schemata_end = MeshMaker::determine_block_pos(hit_pos, positions);
				}
				else {
					// cout << "to far" << endl;
					bHit = false;
				}

			}
			else {
				// cout << "not hit" << endl;
			}


			// modify for block size
			Vector3 start = this->schemata_start;
			Vector3 end = this->schemata_end;

			// UE5Utils::print_Vector3("this->schemata_end", this->schemata_end);

			double x_diff = start.x - end.x;
			double y_diff = start.y - end.y;
			double z_diff = start.z - end.z;
			double real_size = this->selected_block_size * CoordinateConversion::WORLD_SCALE * CoordinateConversion::BLOCK_SCALE;
			double half_size = real_size / 2;
			if (z_diff > 0) {
				if (x_diff > 0) {
					// cout << "end is south and east" << endl;
					start.x += half_size;
					start.z += half_size;

					if (bHit) {
						end.x -= half_size;
						end.z -= half_size;
					}
				}
				else {
					// cout << "end is south and west" << endl;
					start.x -= half_size;
					start.z += half_size;

					if (bHit) {
						end.x += half_size;
						end.z -= half_size;
					}
				}
			}
			else {
				if (x_diff > 0) {
					// cout << "end is north and east" << endl;
					start.x += half_size;
					start.z -= half_size;

					if (bHit) {
						end.x -= half_size;
						end.z += half_size;
					}

				}
				else {
					// cout << "end is north and west" << endl;
					start.x -= half_size;
					start.z -= half_size;

					if (bHit) {
						end.x += half_size;
						end.z += half_size;
					}
					
				}
			}


			if (y_diff >= 0) {
				// cout << "end below" << endl;
				start.y += half_size;
				if (bHit) end.y -= half_size;
			}   
			else {
				// cout << "end above" << endl;
				start.y -= half_size;
				if (bHit) end.y += half_size;

			}


			DebugShapes::draw_debug_box(start, end);
		}
	
	}

	void ghostSchemata() {
		if (!this->schemata_placement_mode || this->selected_schemata == nullptr) return;

		// raycast to find target block
		Ref<Mesh> mesh_comp = nullptr;
		CollisionShape3D* hit_collider = nullptr;
		Dictionary hit_result;
		Vector3 instance_location;
		double box_size;
		shared_ptr<PositionDouble> hit_block_pos = this->raycast_for_block_position(
			hit_result, mesh_comp, instance_location, box_size, hit_collider
		);

		shared_ptr<PositionDouble> anchor_pos;
		Vector3 new_godot_anchor;
		if (hit_block_pos != nullptr) {
			Vector3 hit_location = hit_result["position"];
			anchor_pos = compute_placement_base(
				hit_block_pos,
				hit_location,
				instance_location,
				box_size,
				this->selected_block_size,
				selected_schemata->size,
				this->FollowCamera->get_global_position() - hit_location
			);
			new_godot_anchor = CoordinateConversion::convert_position_double_to_fVector(
				anchor_pos, this->world->cubePool->terrainZeroZero, this->planet->worldSize, 0
			);
		} else {
			Vector3 camera_loc = this->FollowCamera->get_global_position();
			Vector3 look_dir = GodotUtil::forward(this->FollowCamera);
			Vector3 end = camera_loc + look_dir * 4.0;
			double scale = this->selected_block_size * CoordinateConversion::WORLD_SCALE;
			end = Vector3(
				Util::round_to_precision(end.x, scale),
				Util::round_to_precision(end.y, scale),
				Util::round_to_precision(end.z, scale)
			);
			anchor_pos = CoordinateConversion::convert_fVector_to_position_double(
				end, this->world->cubePool->terrainZeroZero, this->planet->worldSize, 0
			);
			new_godot_anchor = end;
		}

		// compute world positions for all blocks
		vector<shared_ptr<Block>> new_placement;
		for (auto& block : selected_schemata->blocks) {
			double wx = anchor_pos->x + block->position_double->x;
			double wy = anchor_pos->y + block->position_double->y;
			double wz = anchor_pos->z + block->position_double->z;
			while (wx < 0) wx += planet->worldSize;
			while (wx >= planet->worldSize) wx -= planet->worldSize;
			while (wz < 0) wz += planet->worldSize;
			while (wz >= planet->worldSize) wz -= planet->worldSize;

			auto new_block = std::make_shared<Block>();
			new_block->position_double = std::make_shared<PositionDouble>(wx, wy, wz);
			new_block->size = block->size;
			new_block->material = block->material;
			new_placement.push_back(new_block);
		}

		// if anchor moved, refresh ghost blocks
		if (new_godot_anchor.distance_to(last_ghost_anchor) > 0.01f) {
			for (auto* inst : ghost_block_instances) {
				inst->queue_free();
			}
			ghost_block_instances.clear();

			for (auto& block : new_placement) {
				auto* inst = this->world->cubePool->place_block_no_collision(
					block->position_double,
					block->size,
					MeshMaker::meshes[block->size],
					MeshMaker::get_material(block->material)
				);
				ghost_block_instances.push_back(inst);
			}

			last_ghost_anchor = new_godot_anchor;
		}

		current_placement_blocks = new_placement;

		// draw bounding box
		double min_x = numeric_limits<double>::max();
		double min_y = numeric_limits<double>::max();
		double min_z = numeric_limits<double>::max();
		double max_x = -numeric_limits<double>::max();
		double max_y = -numeric_limits<double>::max();
		double max_z = -numeric_limits<double>::max();
		for (auto& block : new_placement) {
			double bx = block->position_double->x;
			double by = block->position_double->y;
			double bz = block->position_double->z;
			if (bx < min_x) min_x = bx;
			if (by < min_y) min_y = by;
			if (bz < min_z) min_z = bz;
			if (bx + block->size > max_x) max_x = bx + block->size;
			if (by + block->size > max_y) max_y = by + block->size;
			if (bz + block->size > max_z) max_z = bz + block->size;
		}
		Vector3 box_start = CoordinateConversion::convert_position_double_to_fVector(
			std::make_shared<PositionDouble>(min_x, min_y, min_z),
			this->world->cubePool->terrainZeroZero, this->planet->worldSize, 0
		);
		Vector3 box_end = CoordinateConversion::convert_position_double_to_fVector(
			std::make_shared<PositionDouble>(max_x, max_y, max_z),
			this->world->cubePool->terrainZeroZero, this->planet->worldSize, 0
		);
		DebugShapes::draw_debug_box(box_start, box_end, Color(0, 1, 0));
	}

	// Update the camera's position and rotation to match the player character's position and rotation
	void UpdateCameraPosition()
	{
		if (!FollowCamera) return;

		if (this->editing_character) {
			// determine rot for camera
			Quaternion rot_around_right = Quaternion(this->right, this->camera_rotation_y);
			Vector3 camera_point_dir = -rot_around_right.xform(this->forward);
			// Quaternion rot = Quaternion(Vector3(0, 0, -1), camera_point_dir.normalized());
			Quaternion rot = Basis().looking_at(camera_point_dir.normalized(), GodotUtil::up_v).get_quaternion();

			// set position and rotation
			this->FollowCamera->set_global_transform(Transform3D(Basis(rot), this->edit_position));
		}
		else {
		

			// determine camara position
			Vector3 player_position = this->hit_box->get_global_position() + playerHeadPosition;
			Quaternion rot = Quaternion(this->right, this->camera_rotation_y);
			Vector3 cameraRotationOffset = rot.xform(this->forward);
			Vector3 camera_position = cameraRotationOffset * this->cameraZoom + player_position;

			// move camera closer to player if something is in the way
			Vector3 trace_start = camera_position - player_position;
			trace_start = trace_start.normalized();
			trace_start += player_position;
			Array exclude;
			exclude.append(this);
			exclude.append(this->hit_box);
			Dictionary result = GodotUtil::raycast(trace_start, camera_position, exclude);

			if (!result.is_empty()) {
				// There was a hit
				Vector3 hit_location = result["position"];
				real_t distance = (hit_location - player_position).length();

				if (distance < 1.0) {
					camera_position = player_position;
				} else {
					camera_position = cameraRotationOffset * distance * 0.9 + player_position;
				}
			}

			// make camera look at player
			Vector3 to_player = player_position - camera_position;
			Quaternion rotToLookAtPlayer = Basis().looking_at(to_player.normalized(), GodotUtil::up_v).get_quaternion();
			Basis rot_basis = Basis(rotToLookAtPlayer);

			this->FollowCamera->set_global_transform(Transform3D(rot_basis, camera_position));
			

			// Draw a second line in a different color to compare
			// GodotUtil::draw_line(this->debug_line2, start, should_end);

		}


	}

	void calculateSpeedMult()
	{
		Vector3 velocity = this->hit_box->get_linear_velocity();

		// limit the amout of force that can be applied by the speed
		double magnitude = velocity.length();
		this->speedMult = magnitude < avgSpeed ? 1 : avgSpeed / magnitude;

		// dampen velocity
		/*Vector3 movementDamp = -this->appliedForces * magnitude * this->speedDamp;
		hit_box->AddForce( movementDamp );
		this->appliedForces += movementDamp;*/

	}

	void rotationDamping() {


		// Vector3 angularVelocity = hit_box->GetPhysicsAngularVelocityInDegrees();
		// float mult = (angularVelocity.Size() > this->rotationDecayThreshold ? this->rotationDecay : 0);

		// Vector3 decay = -angularVelocity * mult;
		// Vector3 current = hit_box->GetPhysicsAngularVelocityInDegrees();

		// hit_box->AddTorqueInDegrees(decay);
	}

	void handleStuckPlayer() {

		if (!this->world || !this->planet || !this->planet->rootChunk) return;
		// check if player position is underground
		shared_ptr<PositionDouble> player_position = CoordinateConversion::convert(this->hit_box->get_global_position(), this->world->cubePool->terrainZeroZero, 0);
		shared_ptr<Position> player_world_pos = CoordinateConversion::convert_fVector_to_position(this->hit_box->get_global_position(), this->world->cubePool->terrainZeroZero, this->planet->worldSize, 0);
		shared_ptr<Chunk> chunk = this->planet->rootChunk->getChunkOrClosestAncestorInWorld(ChunkType::CHUNK_1, player_world_pos->x, player_world_pos->z, this->world->cubePool->blocks_in_world);
		if (chunk->getType() <= ChunkType::CHUNK_16 && player_position->y < (chunk->position->y - chunk->getSize() * 4) ) {
			cout << "RESET player:" << endl;
			cout << "player_position " << player_position->toString() << endl; 
			cout << "player_world_pos " << player_world_pos->toString() << endl; 
			cout << "chunk->position " << chunk->position->toString() << endl; 

			// XXX handle caves here

			shared_ptr<PositionDouble> wrappedPosition = CoordinateConversion::find_position_closest_to_player(
				std::make_shared<PositionDouble>(chunk->position->x, chunk->position->y, chunk->position->z), 
				player_position, 
				this->planet->worldSize
			);
			Vector3 block_location = CoordinateConversion::convert(wrappedPosition, this->world->cubePool->terrainZeroZero, chunk->getSize());
			Vector3 new_location = this->hit_box->get_global_position();
			// cout << GodotUtil::vec_str(new_location) << endl;
			new_location.y = block_location.y + chunk->getSize() * 2 + 2;
			this->hit_box->set_global_position(new_location);
			// Vector3 pos = this->hit_box->get_global_position();
			// cout << GodotUtil::vec_str(pos) << endl;
			player_position = CoordinateConversion::convert(this->hit_box->get_global_position(), this->world->cubePool->terrainZeroZero, 0);
			cout << "reset stuck player " << GodotUtil::vec_str(new_location) << " " << player_position->toString() << endl;

		}
	}

	bool resetPlayerPosition() {
		if (!this->world || !this->planet) return false;
		// get chunk where player is
		shared_ptr<PositionDouble> player_position = CoordinateConversion::convert_fVector_to_position_double(this->hit_box->get_global_position(), this->world->cubePool->terrainZeroZero, this->planet->worldSize, 0);
		shared_ptr<Chunk> chunk = this->planet->rootChunk->getChunkOrClosestAncestorInWorld(ChunkType::CHUNK_1, player_position->x, player_position->z, this->world->cubePool->blocks_in_world);

		if (chunk->getType() <= ChunkType::CHUNK_16) {

			shared_ptr<PositionDouble> wrappedPosition = CoordinateConversion::find_position_closest_to_player(
				std::make_shared<PositionDouble>(chunk->position->x, chunk->position->y, chunk->position->z), 
				player_position, 
				this->planet->worldSize
			);
			Vector3 block_location = CoordinateConversion::convert(wrappedPosition, this->world->cubePool->terrainZeroZero, chunk->getSize());
			Vector3 new_location = this->hit_box->get_global_position();
			new_location.z = block_location.z + chunk->getSize() + 15;
			this->hit_box->set_global_position(new_location);
			cout << "set" << endl;

			return true;
		}
		
		return false;
	}

	void reportWorldPosition() {
		if (!this->world || !this->planet) return;
		Vector3 location = this->hit_box->get_global_position();
		shared_ptr<Position> player_world_pos = CoordinateConversion::convert_fVector_to_position(location, this->world->cubePool->terrainZeroZero, this->planet->worldSize, 0);
		this->world->set_player_position(player_world_pos);
		this->world->player_location = location;
	}



	// DEBUG

	void init_debug() {
		 // Create CanvasLayer
		CanvasLayer* canvas = memnew(CanvasLayer);

		// Optional: set layer index (higher = draws on top)
		canvas->set_layer(1);

		// Add to root node (or your scene root)
		get_tree()->get_current_scene()->add_child(canvas);

		// Create RichTextLabel
		RichTextLabel* debug_label = memnew(RichTextLabel);


		// --- Set font size ---
		debug_label->add_theme_font_size_override("font_size", 40);

		// Optional: set font size / theme, position
		debug_label->set_custom_minimum_size(Vector2(300, 600));
		debug_label->set_position(Vector2(10, 10));
		debug_label->set_scroll_active(false); // no scrollbars

		// Add RichTextLabel to the CanvasLayer
		canvas->add_child(debug_label);

		// Save pointer for later use
		this->debug_label = debug_label;
	}


	void addMessages(const std::vector<std::pair<String, string>>& messages_with_colors) {

		// setup if not made yet
		if (this->debug_label == nullptr) {
			this->init_debug();
		}

		// Build one big BBCode string
		String bbcode_text;
		for (const auto& pair : messages_with_colors) {
			const String& color_str = pair.first; // e.g., "red", "green"
			const String& message   = String::utf8(pair.second.c_str());
			bbcode_text += "[color=" + color_str + "]" + message + "[/color]\n";
		}

		this->debug_label->clear();          // Clear previous text
		this->debug_label->parse_bbcode(bbcode_text); // Parse and display BBCode

	}

	String RED    = "red";
    String GREEN  = "green";
    String BLUE   = "blue";
    String YELLOW = "yellow";
    String ORANGE = "orange";
	String PURPLE = "purple";
    String WHITE  = "white";
    String BLACK  = "black";
	void debugInfo() {


		if (!this->debug) return;

		this->tick++;

		vector<pair<String, string>> messages_with_colors;

		// Godot position
		string message = "Godot Position ";
		message += GodotUtil::vec_str(this->hit_box->get_global_position());
		messages_with_colors.push_back(
			pair<String, string>(BLUE, message )
		);

		if (this->world && this->planet) {


			// ManyWorlds position
			shared_ptr<Position> terrainZeroZero = this->world->cubePool->terrainZeroZero;
			Vector3 actor_location = this->hit_box->get_global_position();
			shared_ptr<PositionDouble> player_position = CoordinateConversion::convert_fVector_to_position_double(
				this->hit_box->get_global_position(), 
				terrainZeroZero, 
				this->planet->worldSize, 
				1
			);
			messages_with_colors.push_back(
				pair<String, string>(RED, "ManyWorlds " + player_position->toString())
			);

			// // convert back to UE5 position
			// Vector3 convertedBack = CoordinateConversion::convert_position_double_to_Vector3(
			//     player_position, 
			//     terrainZeroZero, 
			//     this->planet->worldSize, 
			//     1
			// );
			// messages_with_colors.push_back(
			//     pair<String, string>(string::Green, TCHAR_TO_UTF8(*convertedBack.ToString()))
			// );



			// target block

			Ref<Mesh> mesh_comp = nullptr;
			CollisionShape3D* hit_collider = nullptr;
			Dictionary hit_result;
			Vector3 instance_location;
			double box_size;
			shared_ptr<PositionDouble> target_block_position = this->raycast_for_block_position(hit_result, mesh_comp, instance_location, box_size, hit_collider);
			if (target_block_position != nullptr) {
				messages_with_colors.push_back(
					pair<String, string>(ORANGE, "target block " + target_block_position->toString())
				);
			}


			Vector3 camera_location = this->FollowCamera->get_global_position();
			Vector3 look = GodotUtil::forward(this->FollowCamera);
			Vector3 End = camera_location + (look * this->place_distance); // Raycast distance of 1000 units
			Array exclude;
			exclude.append(this);
			exclude.append(this->hit_box);
			Dictionary result = GodotUtil::raycast(camera_location, End, exclude);
			if (!result.is_empty()) {
				stringstream ss;
				Vector3 hit_location = result["position"];
				shared_ptr<PositionDouble> target_pos = CoordinateConversion::convert_fVector_to_position_double(
					hit_location, 
					terrainZeroZero, 
					this->planet->worldSize, 
					1
				);
				messages_with_colors.push_back(
					pair<String, string>(ORANGE, "target " + target_pos->toString())
				);
				messages_with_colors.push_back(
					pair<String, string>(ORANGE, "target godot " + GodotUtil::vec_str(hit_location))
				);
			}
			else {
				messages_with_colors.push_back(
					pair<String, string>(ORANGE, "target none")
				);
			}


			// Player View Direction
			Vector3 camera_forward = GodotUtil::forward(this->FollowCamera);
			vector<double> dirs = {abs(camera_forward.x), abs(camera_forward.y), abs(camera_forward.z)};
			string direction;
			if (dirs[0] > dirs[1] && dirs[0] > dirs[2]) {
				// More East or West
				if (camera_forward.x < 0) direction = "East";
				else direction = "West";
			} 
			else if (dirs[1] > dirs[0] && dirs[1] > dirs[2]) {
				// More Up or Down
				if (camera_forward.y > 0) direction = "Up";
				else direction = "Down";
			}
			else {
				// More North or South
				if (camera_forward.z > 0) direction = "North";
				else direction = "South";
			}
			messages_with_colors.push_back(
				pair<String, string>(RED, direction)
			);


			// Get Chunk Info
			vector<shared_ptr<Chunk>> chunks = this->planet->rootChunk->getChunksAtPosition(player_position->x, player_position->z);
			for (int i = 1; i < chunks.size(); i++) {
				shared_ptr<Chunk> chunk = chunks[i];
				if (chunk != nullptr) {
					string chunk_info = "Chunk: " + chunk->stringTag() + ", ID: " + std::to_string(chunk->getId());
					messages_with_colors.push_back(
						pair<String, string>(YELLOW, chunk_info)
					);
				}

			}
			
			// Collision Info
			// TArray<UPrimitiveComponent*> cols;
			// this->CollisionSphere->GetOverlappingComponents(cols);
			// string isColliding = cols.Num() > 1? "stuck" : "free";
			// GEngine->AddOnScreenDebugMessage(12, 5.f, string::Cyan, toFStr(isColliding));

			// spawn info
			string spawnDone = this->world->spawnDone? "spawn done" : "spawning...";
			messages_with_colors.push_back(
				pair<String, string>(PURPLE, spawnDone)
			);

			string priority_order = std::to_string((int) this->world->priority_order[0]) 
				+ " " + std::to_string((int) this->world->priority_order[1]) 
				+ " " + std::to_string((int) this->world->priority_order[2]);
			messages_with_colors.push_back(
				pair<String, string>(BLACK, priority_order)
			);

			string block_count = std::to_string(this->world->cubePool->getCount());
			messages_with_colors.push_back(
				pair<String, string>(RED, block_count)
			);


			// draw chunk lines
			if (chunks.size() > 4) {
				// DrawDebugLine(
				//     GetWorld(),
				//     Vector3(0, 0, 0),
				//     Vector3(0, 0, 100000000),
				//     RED,
				//     false, // persist
				//     5.0f, // duration
				//     0, // depth priority
				//     10.0f // thickness
				// );



				// shared_ptr<Chunk> chunk512 = chunks[4];
				// shared_ptr<PositionDouble> chunk_unwrapped = CoordinateConversion::find_position_closest_to_player(
				// 	std::make_shared<PositionDouble>(chunk512->position->x, chunk512->position->y, chunk512->position->z),
				// 	player_position, 
				// 	this->planet->worldSize
				// );
				// Vector3 swstart = CoordinateConversion::convert(chunk_unwrapped, terrainZeroZero, 0);
				// Vector3 swend = swstart + Vector3(0, 0, 102400);
				// DrawDebugLine(
				// 	GetWorld(),
				// 	swstart,
				// 	swend,
				// 	RED,
				// 	false, // persist
				// 	5.0f, // duration
				// 	0, // depth priority
				// 	10.0f // thickness
				// );

				// // NW corner
				// shared_ptr<PositionDouble> NW = make_shared<PositionDouble>(chunk_unwrapped->x, chunk_unwrapped->y, chunk_unwrapped->z + 512);
				// Vector3 nwstart = CoordinateConversion::convert(NW, terrainZeroZero, 0);
				// Vector3 nwend = nwstart + Vector3(0, 0, 102400);
				// DrawDebugLine(
				// 	GetWorld(),
				// 	nwstart,
				// 	nwend,
				// 	RED,
				// 	false, // persist
				// 	5.0f, // duration
				// 	0, // depth priority
				// 	10.0f // thickness
				// );

				// // NE corner
				// shared_ptr<PositionDouble> NE = make_shared<PositionDouble>(chunk_unwrapped->x + 512, chunk_unwrapped->y, chunk_unwrapped->z + 512);
				// Vector3 nestart = CoordinateConversion::convert(NE, terrainZeroZero, 0);
				// Vector3 neend = nestart + Vector3(0, 0, 102400);
				// DrawDebugLine(
				// 	GetWorld(),
				// 	nestart,
				// 	neend,
				// 	RED,
				// 	false, // persist
				// 	5.0f, // duration
				// 	0, // depth priority
				// 	10.0f // thickness
				// );

				// // SE corner
				// shared_ptr<PositionDouble> SE = make_shared<PositionDouble>(chunk_unwrapped->x + 512, chunk_unwrapped->y, chunk_unwrapped->z);
				// Vector3 sestart = CoordinateConversion::convert(SE, terrainZeroZero, 0);
				// Vector3 seend = sestart + Vector3(0, 0, 102400);
				// DrawDebugLine(
				// 	GetWorld(),
				// 	sestart,
				// 	seend,
				// 	RED,
				// 	false, // persist
				// 	5.0f, // duration
				// 	0, // depth priority
				// 	10.0f // thickness
				// );

				// // cross lines
				// DrawDebugLine(
				// 	GetWorld(),
				// 	sestart + Vector3(0, 0, 10000),
				// 	swstart + Vector3(0, 0, 10000),
				// 	RED,
				// 	false, // persist
				// 	5.0f, // duration
				// 	0, // depth priority
				// 	10.0f // thickness
				// );
				// DrawDebugLine(
				// 	GetWorld(),
				// 	swstart + Vector3(0, 0, 10000),
				// 	nwstart + Vector3(0, 0, 10000),
				// 	RED,
				// 	false, // persist
				// 	5.0f, // duration
				// 	0, // depth priority
				// 	10.0f // thickness
				// );
				// DrawDebugLine(
				// 	GetWorld(),
				// 	nwstart + Vector3(0, 0, 10000),
				// 	nestart + Vector3(0, 0, 10000),
				// 	RED,
				// 	false, // persist
				// 	5.0f, // duration
				// 	0, // depth priority
				// 	10.0f // thickness
				// );
				// DrawDebugLine(
				// 	GetWorld(),
				// 	nestart + Vector3(0, 0, 10000),
				// 	sestart + Vector3(0, 0, 10000),
				// 	RED,
				// 	false, // persist
				// 	5.0f, // duration
				// 	0, // depth priority
				// 	10.0f // thickness
				// );



				
			}

		}

		addMessages(messages_with_colors);


	}


};
