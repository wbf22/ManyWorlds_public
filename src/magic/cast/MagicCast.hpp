#pragma once


#include "cast_analysis/CastAnalysis.hpp"
#include <vector>
#include "util/Util.hpp"
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>



using namespace std;



struct MagicCast {

	bool currently_casting = false;
	double casting_start_time = 0.0;
	vector<pair<double, double>> cast_mouse_samples;
	vector<CastShape> cast_shapes;
	// vector<UStaticMeshComponent*> cast_items;
	Vector3 instrument_tip_position;


	void start() {
        this->currently_casting = true;
        this->casting_start_time = Util::time();
	}


	void capture_mouse_sample(double mouse_x, double mouse_y) {

                // CAPTURE MOUSE SAMPLES
                if (this->cast_mouse_samples.size() < 256) {
                this->cast_mouse_samples.push_back(
                        std::pair<double, double>(mouse_x, mouse_y)
                );
                }
                // once at n samples restart
                else {
                CastShape shape = CastAnalysis::determine_shape(this->cast_mouse_samples);
                cout << "shape: " << cast_shape_to_name(shape) << endl;
                this->cast_shapes.push_back(shape);
                this->cast_mouse_samples.clear();
                }

	}

	void spawn_cast_items(Node3D* player, MeshInstance3D* mesh) {

        // // POP OUT NEW CAST ITEMS AS TIME GOES ON
        // double TIME_BETWEEN_NEW_CAST_ITEMS = 1;
        // double elapsed_time = (Util::time() - this->casting_start_time);
        // int num_items = elapsed_time / TIME_BETWEEN_NEW_CAST_ITEMS;
        // ++num_items;
        // if (num_items > this->cast_items.size()) {
        //     mesh->RegisterComponent();
        //     mesh->AttachToComponent(player->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        //     mesh->SetStaticMesh(mesh_asset.Object);
        //     mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        //     mesh->SetCollisionObjectType(ECC_WorldDynamic);
        //     mesh->SetSimulatePhysics(true);
        //     mesh->SetWorldLocation(player->GetActorLocation());
        //     mesh->SetWorldScale3D(FVector(1.0f, 1.0f, 1.0f)); 
        //     this->cast_items.push_back(mesh);
        // }
	}

	void swing_cast_items_around_player(double mouse_x, double mouse_y, Camera3D* FollowCamera, Node3D* player) {

        
        // // SWING CAST ITEMS AROUND PLAYER

        // // convert mouse position to world position around player
        // int CAST_SPHERE_RADIUS = 400;
        // FVector2D view_port;
        // GEngine->GameViewport->GetViewportSize(view_port);
        // double percent_x = mouse_x / view_port.X;
        // double percent_y = mouse_y / view_port.Y;
        // // Util::print("percent_x", percent_x);
        // // Util::print("percent_y", percent_y);
        // double half_raidus = CAST_SPHERE_RADIUS / 2;
        // double cast_x = CAST_SPHERE_RADIUS * percent_x;
        // double cast_y = CAST_SPHERE_RADIUS * percent_y;
        // cast_x -= half_raidus;
        // cast_y -= half_raidus;
        // /*
        //     sphere equation
        //         CAST_SPHERE_RADIUS^2 = x^2 + y^2 + z^2
        //         z = sqrt( CAST_SPHERE_RADIUS^2 - x^2 - y^2 )
        // */
        // double cast_z = sqrt( pow(CAST_SPHERE_RADIUS, 2) - pow(cast_x, 2) - pow(cast_y, 2) );
        // FVector tip_offset = FVector(cast_x, cast_y, cast_z);

        // // rotate instrument_tip_position to align with the player forward vector
        // FRotator rotation = FollowCamera->GetRightVector().Rotation();
        // this->instrument_tip_position = rotation.RotateVector(tip_offset);
        // this->instrument_tip_position += player->GetActorLocation();
        // // cout << "instrument pos" << endl;
        // // cout << this->instrument_tip_position.X << " " << this->instrument_tip_position.Y << " " << this->instrument_tip_position.Z << endl;


        
        // // add forces to cast items to have them adhere to 'instrument_tip_position'
        // double FOLLOW_FORCE = 40;
        // double MAX_FORCE = 16000;
        // for (UStaticMeshComponent* mesh : this->cast_items) {
        //     FVector diff = this->instrument_tip_position - mesh->GetComponentLocation();
        //     diff *= FOLLOW_FORCE;
        //     // if too strong cap with MAX_FORCE
        //     if (diff.Size() > MAX_FORCE) {
        //         diff.Normalize();
        //         diff *= MAX_FORCE;
        //     }
        //     mesh->AddForce(diff);
        // }

	}

	void finish(Camera3D* FollowCamera) {

        // determine shape



        // vector<pair<float, float>> test = {
        //     {1.3, 2.4},
        //     {1.3, 2.4},
        // };



        // cout << "vector<pair<double, double>> test = {" << endl;
        // for (int i = 0; i < this->cast_mouse_samples.size(); i++) {
        //     cout << "\t" << "{" << this->cast_mouse_samples[i].first << ", " << this->cast_mouse_samples[i].second << "}," << endl;
        // }
        // cout << "};" << endl << endl;

        // for (int i = 0; i < this->cast_mouse_samples.size(); i++) {
        //     cout << this->cast_mouse_samples[i].first << ", " << this->cast_mouse_samples[i].second << endl;
        // }
        // cout << endl << endl;




        // // DETERMINE CAST SHAPE
        // CastShape shape = CastAnalysis::determine_shape(this->cast_mouse_samples);
        // cout << "shape: " << cast_shape_to_name(shape) << endl;
        // this->cast_shapes.push_back(shape);


        // double sum = 0;
        // for (CastShape shape : this->cast_shapes) {
        //     sum += (int) shape;
        // }
        // double avg = sum / this->cast_shapes.size();
        // double amount_line = 1 - avg;
        // double amount_circle = avg;

        
        // // DETERMINE POWER
        // /*
        //     you'll have to graph these to see, but curves increase in power more slowly over time, but can get
        //     higher than lines. Lines increase in power quickly with time but don't get as high.
        
        // */
        // double elapsed_time = (Util::time() - this->casting_start_time);
        // double curve_power = log(elapsed_time + 1) / log(1.2);
        // double line_power = log(elapsed_time + 0.00001) / log(8) + 5;
        // double power = line_power * amount_line + curve_power * amount_circle;
        
        
        // cout << "CAST! " << "line: " << amount_line << " curve: " << amount_circle << endl;
        // cout << "POWER " << power << endl;
        // cout << "DURATION " << elapsed_time << endl;



        // //LAUNCH CAST ITEMS IN THE PLAYER FORWARD DIRECTION
        // double FORCE_MULTIPLIER = 4000;
        // FVector look = FollowCamera->GetForwardVector();
        // for (UStaticMeshComponent* mesh : this->cast_items) {
        //     mesh->AddImpulse(look * FORCE_MULTIPLIER * power);
        // }

        // this->currently_casting = false;
        // this->casting_start_time = 0.0;
        // this->cast_mouse_samples.clear();
        // this->cast_shapes.clear();
	}

};