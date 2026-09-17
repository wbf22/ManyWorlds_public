#pragma once


#include "KeyFrame.h"
#include "../BodyPart.hpp"
#include <cmath>
#include <memory>



using namespace std;

enum class AnimationType
{
    RUN,
    IDLE,
    HIT,
    FALL,

    MAGIC,
    SWIM,
    FLY,
    HURT
};

struct BodyPart;

struct Animation
{

    /**
     * Key frames defined by the user or generated
     * 
     * Each keyframe represents 0.1 seconds. 
     */
    vector<shared_ptr<KeyFrame>> defined_key_frames;

    /**
     * Key frames lerped from the key frames above for a given fps
     */
    vector<shared_ptr<KeyFrame>> pre_rendered_key_frames;

    double length;

    /**
     * Applies this animation to a part. 
     * 
     * @param index the index of the keyframe to set (or if greater than the # of keyframes it will be looped)
     * @param block_mesh the UInstancedStaticMeshComponent to animate
     */
    void animate(int index, MultiMeshInstance3D* block_mesh) {
        index = index % this->pre_rendered_key_frames.size();

        shared_ptr<KeyFrame>& key_frame = this->pre_rendered_key_frames[index];

        block_mesh->set_transform(Transform3D(key_frame->rotation, key_frame->position));
    }

    /**
     * Pre-renders keyframes to our desired fps. 'animation_delta' is computed like so
     * 1s / fps
     * 
     * It represents how often the animation should change in seconds
     */
    void pre_render(double animation_delta) {
        double time = 0;
    
        // lerp between keyframes to make keyframes for each 'animation_delta' seconds
        int frame_i = time < this->defined_key_frames[0]->time? this->defined_key_frames.size() - 1 : 0;
        while (time < this->length) {

            double looped_time = fmod(time, this->length);

            // determine keyframes current time falls between
            bool right_spot = false;
            int next_frame_i = frame_i + 1;
            next_frame_i = next_frame_i % this->defined_key_frames.size();
            while(!right_spot) {
                double next_frame_time = this->defined_key_frames[next_frame_i]->time;
                double adjusted_time = looped_time;

                // handle if we're on last frame
                double frame_time = this->defined_key_frames[frame_i]->time;
                if (next_frame_i < frame_i && adjusted_time >= frame_time) {
                    adjusted_time = looped_time - this->length;
                }

                right_spot = adjusted_time < next_frame_time;
                
                if (!right_spot) {
                    ++frame_i;
                    ++next_frame_i;
                    frame_i = frame_i % this->defined_key_frames.size(); // loop animation if duration is longer
                    next_frame_i = next_frame_i % this->defined_key_frames.size();
                }
            }


            shared_ptr<KeyFrame> frame = this->defined_key_frames[frame_i];
            shared_ptr<KeyFrame> next_frame = this->defined_key_frames[next_frame_i];
    
            // determine time since frame_i
            double frame_time = this->defined_key_frames[frame_i]->time;
            double next_time = this->defined_key_frames[next_frame_i]->time;
            double time_over_frame_i = looped_time - frame_time;
            if (looped_time < frame_time && frame_i > next_frame_i){
                time_over_frame_i = (this->length - frame_time) + looped_time;
            }

            // determine time between frame_i and next_frame_i
            double frame_elapse = next_time - frame_time;
            if (next_time < frame_time) frame_elapse = (this->length - frame_time) + next_time;

            // lerp between keyframes
            double ratio = time_over_frame_i / frame_elapse;
            Vector3 position = frame->position.lerp(next_frame->position, ratio);
            Quaternion rotation = frame->rotation.slerp(next_frame->rotation, ratio);
            // FQuat rotation = frame->rotation;

            // cout << frame_i << endl;
            // cout << ratio << endl;

            // make a new pre-rendered keyframe
            this->pre_rendered_key_frames.push_back(
                std::make_shared<KeyFrame>(position, rotation)
            );

            // update time
            time += animation_delta;
        }
    }

};

