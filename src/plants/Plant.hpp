#pragma once

#include <cstdlib>
#include <cmath>
#include <string>
#include "Trunk.hpp"
#include "Leaf.hpp"
#include "../blocks/Block.h"
#include "util/Position.h"
#include "util/Random.h"
#include "space/Planet.h"
#include "../blocks/WorldBlockPool.hpp"
#include "../blocks/DeferredWorldBlockPool.hpp"
#include "util/serialization/KVSerializer.hpp"
#include "../blocks/Schemata.hpp"

using namespace std;

class Planet;

class Plant {
public:
	shared_ptr<Trunk> trunk;
	shared_ptr<vector<shared_ptr<Block>>> blocks;
	shared_ptr<Leaf> leaf;
	string material;
	double how_advanced;

	Plant(int species, PlantType type, const shared_ptr<Planet>& planet)
	{
		this->how_advanced = std::max<int64_t>(1, planet->evolutionStage);
		this->trunk = std::make_shared<Trunk>(planet->seed + species, planet->gravity, this->how_advanced);
		trunk->height = adjustHeightForType(trunk->height, type);
		this->material = BMaterial::get_random_wood(planet->wood_materials, planet->seed+species);

		this->leaf = std::make_shared<Leaf>();
		this->leaf->material = BMaterial::get_random_leaf(planet->seed + species);
		shared_ptr<Schemata> plant_schemata = std::make_shared<Schemata>();
		KVSerializer::deserialize(Util::read_file("assets/Plants/branches/pine_small.kv"), plant_schemata);
		this->leaf->length = plant_schemata->size->x;
		this->leaf->blocks = plant_schemata->blocks;

		vector<shared_ptr<Block> > trunk_blocks = Plant::genTrunk(
			trunk->seed,
			trunk->height,
			trunk->stoutness,
			trunk->pointiness,
			trunk->straightness,
			trunk->grainTwist,
			1,
			type,
			planet
		);

		Plant::genBranches(
			this->leaf,
			trunk_blocks,
			trunk->seed,
			trunk->height,
			trunk->stoutness,
			trunk->pointiness,
			trunk->straightness,
			trunk->grainTwist,
			trunk->canopyHeightRatio,
			trunk->splitTrunk,
			trunk->branchDirection,
			type,
			trunk->branchiness,
			planet
		);

		vector<shared_ptr<Block> > root_blocks = Plant::genRoots(
			trunk->seed,
			trunk->height,
			trunk->stoutness,
			trunk->pointiness,
			trunk->straightness,
			trunk->grainTwist,
			type,
			planet
		);
		trunk_blocks.insert(trunk_blocks.end(), root_blocks.begin(), root_blocks.end());

		this->blocks = std::make_shared<vector<shared_ptr<Block>>>();
		for (shared_ptr<Block>  block : trunk_blocks) {

			shared_ptr<Block> translatedBlock = std::make_shared<Block>();
			translatedBlock->position = make_shared<Position>(
				block->position->x, 
				block->position->y,
				block->position->z
			);
			translatedBlock->material = block->material;
			translatedBlock->size = block->size;

			this->blocks->push_back(translatedBlock);
		}
	}

	Plant() {
		this->leaf = std::make_shared<Leaf>();
		this->blocks = std::make_shared<vector<shared_ptr<Block>>>();
	}

	~Plant()
	{
	}


	shared_ptr<Plant> translate(shared_ptr<Position> position) {
		shared_ptr<Plant> plant = std::make_shared<Plant>();
		// copy trunk and branches
		for (shared_ptr<Block> block : *this->blocks.get()) {
		shared_ptr<Block> translatedBlock = std::make_shared<Block>();
			translatedBlock->position = make_shared<Position>(
				block->position->x + position->x,
				block->position->y + position->y,
				block->position->z + position->z
			);
			translatedBlock->material = block->material;
			translatedBlock->size = block->size;
			plant->blocks->push_back(translatedBlock);
		}

		// copy leaves
		plant->leaf = std::make_shared<Leaf>();
		plant->leaf->blocks = this->leaf->blocks;
		plant->leaf->material = this->leaf->material;
		plant->leaf->no_up_down_rotation = this->leaf->no_up_down_rotation;
		plant->leaf->length = this->leaf->length;
		for (shared_ptr<LeafTransform> transform : this->leaf->transforms) {
			shared_ptr<LeafTransform> translatedTransform = std::make_shared<LeafTransform>();
			translatedTransform->position_double = std::make_shared<PositionDouble>(
				transform->position_double->x + position->x,
				transform->position_double->y + position->y,
				transform->position_double->z + position->z
			);
			translatedTransform->pointing_direction = std::make_shared<Rot>(
				transform->pointing_direction->x,
				transform->pointing_direction->y
			);
			translatedTransform->scale = transform->scale;
			plant->leaf->transforms.push_back(translatedTransform);
		}

		// Materialize translated leaves alongside the trunk and branches.
		for (shared_ptr<LeafTransform>& transform : plant->leaf->transforms) {
			vector<shared_ptr<Block>> leaf_blocks = plant->leaf->apply_transform(transform);
			for (shared_ptr<Block>& block : leaf_blocks) {
				block->material = plant->leaf->material;
				plant->blocks->push_back(block);
			}
		}

		return plant;
	}

	shared_ptr<Plant> convert_to_low_res_and_translate(shared_ptr<Position> position)
	{
		shared_ptr<Plant> plant = std::make_shared<Plant>();

		// // figure out which 4x4 blocks have at least 2 1x1 blocks in them ( add them to the plant if they do )
		// unordered_map<string, int> block_counts;
		// for (shared_ptr<Block> block : *this->blocks.get()) {
		// 	shared_ptr<Position> position_4x4 = make_shared<Position>(
		// 		(block->position->x / 4) * 4,
		// 		(block->position->y / 4) * 4,
		// 		(block->position->z / 4) * 4
		// 	);
		// 	string key = position_4x4->toString();
		// 	if (block_counts.find(key) != block_counts.end()) {
		// 		block_counts[key] += 1;
		// 		if (block_counts[key] >= 6) {
		// 			shared_ptr<Block> translatedBlock = std::make_shared<Block>();
		// 			translatedBlock->position = make_shared<Position>(
		// 				position_4x4->x + position->x,
		// 				position_4x4->y + position->y,
		// 				position_4x4->z + position->z
		// 			);
		// 			translatedBlock->material = block->material;
		// 			translatedBlock->size = block->size;
		// 			plant->blocks->push_back(translatedBlock);
		// 		}
		// 	}
		// 	else {
		// 		block_counts[key] = 1;
		// 	}
		// }

		// // copy leaves
		// plant->leaf = std::make_shared<Leaf>();
		// plant->leaf->blocks = this->leaf->blocks;
		// plant->leaf->material = this->leaf->material;
		// plant->leaf->no_up_down_rotation = this->leaf->no_up_down_rotation;
		// plant->leaf->length = this->leaf->length;
		// for (shared_ptr<LeafTransform> transform : this->leaf->transforms) {
		// 	shared_ptr<LeafTransform> translatedTransform;
		// 	translatedTransform->position_double = std::make_shared<PositionDouble>(
		// 		transform->position_double->x + position->x,
		// 		transform->position_double->y + position->y,
		// 		transform->position_double->z + position->z
		// 	);
		// 	translatedTransform->pointing_direction = std::make_shared<Rot>(
		// 		transform->pointing_direction->x,
		// 		transform->pointing_direction->y
		// 	);
		// 	plant->leaf->transforms.push_back(translatedTransform);
		// }

		return plant;
	}

private:
	vector<shared_ptr<Block> > genTrunk(
		int64_t seed, 
		int height,
		int stoutness,
		int pointiness,
		double straightness,
		int grainTwist,
		int varient, 
		PlantType type, 
		const shared_ptr<Planet>& planet
	) {
		vector<shared_ptr<Block> > blocks;
		set<string> block_positions;
		
		int64_t varientSpike = seed + varient;
		
		// XXX based off the type make the tree have different attributes


		/*
			determine main trunk pattern
			- straightness
			- twistPattern
			- layered
			- stoutness
			- height
		*/ 

		// MAKE TRUNK

		/*
			Trunk width at different y levels is calculated as follows:

			q*m*y^t+b

			b = g*0.000029*h^1.25*s
			f = g*0.000029*h^1.25*(100-(s+(100-s)*p/100))

			m = (50-s)/50

			t = -1.5/(1+3.8^-(s-50)) + 2

			q = (f - b)/(m*h^t)


			g: gravity
			h: tree height
			s: stoutness
			p: pointiness
			y: y level up the tree
			b: bottom width
			f: top width
			m q t: fudge factors determined in desmos

			I'm mostly writing this down so I can come back to it easy-> Probably not necessary to know, but if you'd like to figure it out I'd
			reccomend putting this all into desmos->com-> Then you can play with the knobs

			https://www->desmos->com/calculator/xevkisivtu

		*/



		// width function variable calculation
		double bottomWidth = planet->gravity * 0.000015 * pow(height, 1.25) * stoutness;
		double topWidth = planet->gravity * 0.000015 * pow(height, 1.25) * ( 100 - (stoutness + (100-stoutness) * pointiness / 100) );
		
		double m = (50 - stoutness) / 50.0;
		m = (m == 0) ? 0.0001 : m;
		double t = -1.5 / ( 1 + pow(3.8, -(stoutness - 50)) ) + 2;
		double q = (topWidth - bottomWidth) / (m * pow(height, t) );


		/*
			A lot of trees have a twist or corkscrew trunk-> If straightness is high, then we still do the twist but the radius is really small
			
			x = r*sin(s*z/h)
			y = r*cos(s*z/h)
			z = z

			r: raidus of spiral
			h: tree height
			s: spiral tightness, 0-12 usually


		*/
		
		// make starting slice block pattern (radius's for each 12th of the circle)
		vector<double> radiusPoints;
		for (int i = 0; i < 12; i++) {
			radiusPoints.push_back(
				Random::randDouble(varientSpike+i, .8, 1, 1.2) // averaged around 1
			);
		}

		// build trunk
		double spiralTightness = 2 * TrigApprox::PI_2; // could be a property-> Defines how many revolutions in the spiral in the height of the tree->
		shared_ptr<Block> last_point_for_branch_work = std::make_shared<Block>(); // we save this block to add to the list last so we can determine the trunk tip when making branches
		for (int y = 0; y < height; y++) {

			// slice spiral offset
			int xStraightnessOffset = straightness * sin(spiralTightness * y / height);
			int zStraightnessOffset = straightness * cos(spiralTightness * y / height);

			//shared_ptr<WoodSlice> trunkSlice = make_shared<WoodSlice>();
			shared_ptr<Position> straightnessOffset = make_shared<Position>(xStraightnessOffset, y, zStraightnessOffset);


			// slice grain twist (rotating the radius points by the graintwist)
			int numTurns = y * grainTwist * grainTwist / 100;
			vector<double> radiusPointsTwisted;
			for (int i = 0; i < 12; i++) {
				radiusPointsTwisted.push_back(
					radiusPoints[(i + numTurns) % 12]
				);
			}


			// width
			double width = q * m * pow(y, t) + bottomWidth;

			// make slice
			int bound = width * 2 * 2; // width is meters, x2 for the largest radius possible, x2 because our blocks are 0.5 meters
			bound = (bound == 0) ? 1 : bound;
			for (double z = -bound; z < bound; z++) {
				for (double x = -bound; x < bound; x++) {

					// figure out which 12th of the circle we're in
					double angle = atan2(z + 0.000001, x); // 1e-6 divide by zero prevention
					if (angle < 0) angle += TrigApprox::PI_2;
					double radiusIndex = 12 * angle / TrigApprox::PI_2;

					// figure out the indices that fits between
					int firstIndex = (int) floor(radiusIndex) % 12;
					int secondIndex = (int) ceil(radiusIndex) % 12;

					// interpolate between those too points
					double firstWeight = abs(firstIndex - radiusIndex);
					firstWeight *= firstWeight;
					double secondWeight = abs(secondIndex - radiusIndex);
					secondWeight *= secondWeight;

					double interpolatedRadius = radiusPointsTwisted[firstIndex] * firstWeight + radiusPointsTwisted[secondIndex] * secondWeight;
					interpolatedRadius /= (firstWeight + secondWeight);


					// XXX make sure we don't make duplicate blocks
					// if the point is close enough add a block (x0.5 since our blocks are 0.5 meters)
					if (interpolatedRadius * width > euclideanDistance(x, z, 0, 0) * 0.5) {
						
						shared_ptr<Position> pos = Position::buildS(
							x + straightnessOffset->x, 
							y, 
							z + straightnessOffset->z
						);
						shared_ptr<Block> block = std::make_shared<Block>();
						block->position = pos;
						block->material = this->material;
						block->size = 1;

						string pos_str = pos->toString();
						if (!block_positions.contains(pos_str)) {
							bool is_center_point = x == 0 && z == 0;
							if (!is_center_point) {
								blocks.push_back(block);
								block_positions.insert(pos_str);
							}
							else {
								// collect the last point for branch work here
								bool is_null = last_point_for_branch_work->material.empty();
								if (!is_null)
									blocks.push_back(last_point_for_branch_work);
								last_point_for_branch_work = block;
							}
						
						}

						// shared_ptr<Position> inSlicePosition = Position::buildS(x, y, z);
						//trunkSlice->blocks[pos->toString()] = block;
					}
				}
			}
			



		}

		bool is_null = last_point_for_branch_work->material.empty();
		if (!is_null)
			blocks.push_back(last_point_for_branch_work);

		// make sure there is at least one block
		if (blocks.empty()) {
			shared_ptr<Block>  block = std::make_shared<Block>();
			block->position = make_shared<Position>(0, 0, 0);
			block->material = material;
			block->size = 1;
			blocks.push_back(block);
		}

		return blocks;
	}


	// generates the branches based on the type 
	void genBranches(
		shared_ptr<Leaf> leaf,
		vector<shared_ptr<Block> >& blocks,
		int64_t seed, 
		int height, 
		int stoutness,
		int pointiness,
		double straightness,
		int grainTwist,
		int canopyHeightRatio, 
		bool splitTrunk, 
		double branch_direction, 
		PlantType type, 
		int branchiness,
		const shared_ptr<Planet>& planet
	) {

		// set branch_direction to pointing up if the height is really low
		if (height < 3)
			branch_direction = Random::randDouble(seed+(int)type, 2.3, 2.6);
		

		/*
			bounds for branches (3d shape)
			- canopyShape
			- canopyRadius
			- canopyHeightRatio
		*/ 

		
		// determine canopy shape

		// choose canopy width function (1 or 2 functions)
		int num_shapes = Random::randInt(seed, 1, 3);
		vector<int> functions;
		for (int i = 0; i < num_shapes; ++i) {
			functions.push_back(
				Random::randInt(seed+i, 0, 3)
			);
		}

		vector<double> radii = makeCanopyRadii(seed, height, canopyHeightRatio, functions, num_shapes, type);


		/*
			determine splitting pattern
			- branchiness
			- splitTrunk
			- branchDirection
			- branch sparseness
			- branch splitting type
		*/ 
		Pool< pair<double, Position> > branches_ungenerated; // distFromEdge, start position
		
		// determine where branches start on trunk (for non split trunk trees, the canopy should end at the top of the trunk-> Like a pine tree)
		int canopy_height = radii.size();
		int canopy_start_height = height - canopy_height;
		if (canopy_start_height == height) canopy_start_height = height - 1;

		// add starting branches
		int step = height / 20;
		vector<shared_ptr<Block> > new_blocks;
		if (step != 0 && canopy_height != 0) {

			for (int y = canopy_start_height; y < height; y += step) {
				int branches_at_level;
				if (!splitTrunk) 
					branches_at_level  = 6 * Random::randInt(seed+y, 25, min(25, branchiness), 100) / 100.0;
				else
					branches_at_level = 4 * Random::randInt(seed+y, 50, min(50, branchiness), 100) / 100.0;

				// add branches to stack
				for (int i = 0; i < branches_at_level; ++i) {
					Position pos = Position(0, y, 0);
					double split_dist = radii[y - canopy_start_height];
					branches_ungenerated.add(
						pair<double, Position>(split_dist, pos)
					);
				}
			}
			
			PositionMap branch_tips = PositionMap(10, 1024, -1024);

			// walk each branch to the canopy border or wherever is splits (or is limited by branch sparseness)
			double xrot = TrigApprox::PI_HALVES - atan(branch_direction); // what xrot most of the branches should have based on branch direction
			double spiralTightness = 2 * TrigApprox::PI_2; 
			int rand_spike = 0;
			while(!branches_ungenerated.empty()) {
				++rand_spike;

				// get next branch
				pair<double, Position> tuple = branches_ungenerated.fish();

				double dist_from_edge = tuple.first;
				Position pos = tuple.second;
				int varient = pos.x + pos.y + pos.z;

				// determine branch length
				double branch_length = dist_from_edge;
				branch_length *= Random::randDouble(seed+rand_spike, 0.3, 0.6);

				// rotate branch to based on branch direction and randomness
				if (branch_length > 1) {
					
					// make a branch
					vector<shared_ptr<Block> > branch_blocks = genTrunk(
						seed,
						branch_length,
						stoutness,
						pointiness,
						straightness,
						grainTwist,
						varient,
						type,
						planet
					);

					if (branch_blocks.empty()) continue;

					// rotate branch
					// XXX use split trunk or other branch attributes to split different ways
					shared_ptr<Rot>rotation = findGoodRotation(branch_tips, branch_length, xrot, pos, seed + rand_spike, branchiness, radii, canopy_start_height);
					// LOG(to_string(rotation->x) + " " + to_string(rotation->y));
					if (rotation->x >= 0) {
						vector<shared_ptr<Block> > rotatedBranch = rotateBocks(branch_blocks, rotation);
						translateBlocks(rotatedBranch, pos);
						new_blocks.insert(new_blocks.end(), rotatedBranch.begin(), rotatedBranch.end());

						// queue up branches that split off this branch
						shared_ptr<Position> branch_tip_pos = rotatedBranch[rotatedBranch.size() - 1]->position;
						double split_dist = Random::randDouble(seed+rand_spike, 0, branch_length / 2, branch_length);
						addBranches(branches_ungenerated, 2, radii, branch_tip_pos, canopy_start_height, branch_direction, split_dist);

						// add branch tip to branch tips
						branch_tips.add(std::make_shared<PositionDouble>(branch_tip_pos->x, branch_tip_pos->y, branch_tip_pos->z));
					}
					
				}
				
			}

		}
		
		// add twigs and leaves
		Plant::add_leaves(
			leaf, 
			radii,
			canopy_start_height,
			height,
			branch_direction,
			branchiness,
			seed,
			blocks,
			new_blocks
		);

		// add new blocks to the list
		blocks.insert(blocks.end(), new_blocks.begin(), new_blocks.end());
	}

	void add_leaves(
		shared_ptr<Leaf> leaf,
		const vector<double>& radii,
		const int& canopy_start_height,
		const int& height,
		const double& branch_direction,
		const int& branchiness,
		const int64_t& seed,
		const vector<shared_ptr<Block> >& trunk_blocks,
		const vector<shared_ptr<Block> >& branch_blocks
	) {

		// get rotations to be used for twigs and leaves
		double up_down_pos = branch_direction -1.57;

		// determine number of twigs/leaves on the tree (uses strange equation but allows for kind of a min number of leaves)
		int avaible_spawn_points = branch_blocks.size() + trunk_blocks.size();
		// int num_twigs = avaible_spawn_points * branchiness * 0.0007 + (branchiness / 100.0) * (pow(avaible_spawn_points - 600, 2) / 50000) * (-1 / (1 + pow(2.71828, -0.1 * (avaible_spawn_points - 600))) + 1);
		int num_twigs = branch_blocks.size() * 0.1 + trunk_blocks.size() * 0.01;
		if (num_twigs < 3) num_twigs = 3;

		// get the best spawn points for leaves
		vector<shared_ptr<Block> > all_positions;
		all_positions.insert(all_positions.end(), branch_blocks.begin(), branch_blocks.end());
		all_positions.insert(all_positions.end(), trunk_blocks.begin(), trunk_blocks.end());

		// add twigs and leaves
		PositionMap leaf_positions = PositionMap(4, 1024, -1024);
		for (int i = 0; i < num_twigs; ++i) {

			auto get_closest_neighbor_dist = [&](shared_ptr<PositionDouble>& twig_pos) {
				double closest = 1000000;
				for (shared_ptr<PositionDouble> nearby : leaf_positions.getNearby(twig_pos, 100, 4)) {
					double dist = euclideanDistance(nearby->x, nearby->z, twig_pos->x, twig_pos->z);
					if (dist < closest) closest = dist;
				}
				return closest;
			};
			
			// get random block to build twig off of, favoring blocks on branches
			shared_ptr<Block> block = branch_blocks.size() > 0? branch_blocks[Random::randInt(seed+i+12, 0, branch_blocks.size())] : all_positions[Random::randInt(seed+i+12, 0, all_positions.size())];
			shared_ptr<PositionDouble> best_pos = std::make_shared<PositionDouble>(block->position->x, block->position->y, block->position->z);

			// try to find positions that are farther away from other twigs (and favor ones higher up)
			int count = 0;
			double best_closest = get_closest_neighbor_dist(best_pos); // start with high spot first
			while (count < 8) {
				int index = Random::randInt(seed+i+count+leaf_positions.size, 0, all_positions.size());
				shared_ptr<Block>  block = all_positions[index];
				if (block->position->y >= canopy_start_height) {

					shared_ptr<PositionDouble> twig_pos = std::make_shared<PositionDouble>(block->position->x, block->position->y, block->position->z);

					double closest = get_closest_neighbor_dist(twig_pos);

					if (closest > best_closest && twig_pos->y >= best_pos->y) {
						best_closest = closest;
						best_pos = twig_pos;
					}
				}
				++count;
			}

			leaf_positions.add(best_pos);

			double x_p_dir = leaf->no_up_down_rotation? up_down_pos + best_pos->y + Random::randDouble(seed+i + 2, -0.3, 0.3) : 0;
			shared_ptr<Rot> pointing_direction = std::make_shared<Rot>(
				x_p_dir,
				Random::randDouble(seed+i, 0, TrigApprox::PI_2)
			);

			// add to transforms list
			int r_i = round(best_pos->y) - canopy_start_height;
			double radius = r_i < radii.size()? radii[r_i] : height * 0.7;
			double leaf_pos_dist = best_pos->euclideanDistance(std::make_shared<PositionDouble>(0,0,0));
			double desired_length = radius > leaf_pos_dist? radius - leaf_pos_dist : 1;
			double scale = desired_length < leaf->length? desired_length / leaf->length : 1;
			leaf->transforms.push_back(
				std::make_shared<LeafTransform>(best_pos, pointing_direction, scale)
			);
		}

	}

	// generates 2-5 roots at the base of the trunk, pointing mostly downward
	vector<shared_ptr<Block> > genRoots(
		int64_t seed,
		int height,
		int stoutness,
		int pointiness,
		double straightness,
		int grainTwist,
		PlantType type,
		const shared_ptr<Planet>& planet
	) {
		/*
			determine root attributes
			- rootVisibility
			- hangingRootHeight
		*/
		vector<shared_ptr<Block> > root_blocks;
		// bottom width of the trunk (same formula as genTrunk)
		double bottomWidth = planet->gravity * 0.000015 * pow(height, 1.25) * stoutness;
		if (bottomWidth < 0.5) bottomWidth = 0.5;

		// 2-5 roots
		int num_roots = Random::randInt(seed + 77, 2, 6);

		for (int i = 0; i < num_roots; ++i) {
			int64_t root_spike = seed + 77 + i;

			// root length is ~2x the bottom width of the tree (x2 again since blocks are 0.5m)
			double length_meters = bottomWidth * Random::randDouble(root_spike, 1.5, 2.0, 2.5);
			int root_length = max(2, (int)(length_meters * 2));

			// root diameter is based off the tree's bottom diameter divided by the
			// number of roots, with some randomness
			double desired_width = (bottomWidth * 2.0 / num_roots)
				* Random::randDouble(root_spike + 1, 0.7, 1.0, 1.3);
			// back-solve the stoutness genTrunk needs to produce that bottom width
			// ( bottomWidth = g * 0.000015 * h^1.25 * s  =>  s = w / (g * 0.000015 * h^1.25) )
			int root_stoutness = (int)(desired_width / (planet->gravity * 0.000015 * pow(root_length, 1.25)));
			root_stoutness = max(1, min(root_stoutness, 100));
			// taper roots to a point
			int root_pointiness = Random::randInt(root_spike + 2, 80, 96);

			// make the root
			vector<shared_ptr<Block> > root = genTrunk(
				seed,
				root_length,
				root_stoutness,
				root_pointiness,
				straightness,
				grainTwist,
				i + 31, // varient offset so roots don't mirror the branches
				type,
				planet
			);
			if (root.empty()) continue;

			// point mostly downward: straight down is PIE, perpendicular to the trunk
			// is PI_HALVES -- stay well inside that range so no root runs along the ground
			double xrot = Random::randDouble(
				root_spike + 3,
				TrigApprox::PI_HALVES + 0.45, // never closer than ~26deg to horizontal
				TrigApprox::PIE - 0.35,       // bias toward down
				TrigApprox::PIE - 0.1         // not perfectly vertical so they splay a bit
			);
			// spread the roots evenly around the trunk with a little jitter
			double yrot = (i * TrigApprox::PI_2 / num_roots)
				+ Random::randDouble(root_spike + 4, -0.3, 0.3);
			shared_ptr<Rot> rotation = std::make_shared<Rot>(xrot, yrot);

			vector<shared_ptr<Block> > rotated_root = rotateBocks(root, rotation);
			// attach at the trunk base
			translateBlocks(rotated_root, Position(0, 0, 0));
			root_blocks.insert(root_blocks.end(), rotated_root.begin(), rotated_root.end());
		}
		return root_blocks;
	}

	int leaf_likelihood(shared_ptr<Position> pos) {
		return abs(pos->x) + abs(pos->z) + abs(pos->y) * 0.3;
	}

	// queues up a branch unless it's at the canopy border
	void addBranches(Pool<pair<double, Position>>& branches_ungenerated, int branches_at_level, vector<double>& radii, shared_ptr<Position>& branch_tip_pos, int canopy_start_height, double branch_direction, double split_dist)
	{
		if (radii.empty()) {
			return;
		}

		double radius_level = branch_tip_pos->y - canopy_start_height;
		if (radius_level < 0 || radius_level >= radii.size()) {
			return;
		}

		int branch_tip_dist = euclideanDistance(branch_tip_pos->x, branch_tip_pos->z, 0, 0);
		int corresponding_radii = interpolate_radii(radii, radius_level);
		if (branch_tip_dist > corresponding_radii - 1) {
			return;
		}


		for (int b = 0; b < branches_at_level; ++b) {
			int horizontal_coor = abs(
				max(branch_tip_pos->x, branch_tip_pos->z)
			);
			
			// determine how thick the branch should be using the branch direction ( use the branch direction as a slope and walk until we're out of the canopy defined boundary)
			double dist = 0;
			double dist_from_center = horizontal_coor;
		double last_radius_level = radius_level;
			double x_step = 1;
			bool done = radius_level > radii.size() || radius_level < 0; 
			while(!done) {
				radius_level += branch_direction * x_step;
				dist_from_center += x_step;

				// if we're below the canopy then this branch is done
				if (radius_level < 0 && dist_from_center < radii[0]) 
					break;

				// if we're out of the canopy boundary, stop
				bool out_of_bounds = radius_level >= radii.size() || radius_level < 0; // above canopy height
				if (!out_of_bounds) {
					// greater than canopy radius
					double interp = interpolate_radii(radii, radius_level);
					out_of_bounds = out_of_bounds || dist_from_center > interp;
				}

				// if we're out of bounds, interpolate the last radius
				if (out_of_bounds) {
					dist =  interpolate_radii(radii, last_radius_level);
					done = true;
				}

				// if we're at the split_dist, stop
				if (dist_from_center >= split_dist) {
					dist = split_dist;
					done = true;
				} 

				if (done) {
					branches_ungenerated.add(
						pair<double, Position >(dist, *branch_tip_pos.get())
					);
				}

				last_radius_level = radius_level;
			}
		
		}
	}


	// find a good rotation for a branch or returns a negative x rotation if no good rotation is found (effected by branchiness)
	shared_ptr<Rot> findGoodRotation(PositionMap& branch_tips, int branch_length, double xrot, Position& pos, int64_t seed, int branchiness, vector<double>& radii, int canopy_start_height)
	{
		shared_ptr<Position> branch_end = std::make_shared<Position>(pos.x, pos.y + branch_length, pos.z);
		
		// try a few random rotations
		vector<pair<shared_ptr<Rot>, shared_ptr<PositionDouble>>> rotations;
		int i = 0;
		while (rotations.size() < 3) {
			shared_ptr<Rot>rotation = std::make_shared<Rot>(
				Random::randDouble(seed+i + branch_length, 0, xrot, TrigApprox::PIE),
				Random::randDouble(seed+i + branch_length + 1, 0, TrigApprox::PI_2)
			);
			shared_ptr<PositionDouble> rotated = Rot::rotate(std::make_shared<PositionDouble>(branch_end->x, branch_end->y, branch_end->z), rotation);
			rotations.push_back(pair<shared_ptr<Rot>, shared_ptr<PositionDouble>>(rotation, rotated));
			
			++i;
		}

		// choose the best one
		double closest = 0;
		double needed_distance = radii[pos.y - canopy_start_height] * (100 - branchiness) / 100.0; // determine if branchiness means this branch shouldn't be added
		shared_ptr<Rot>best_rotation = std::make_shared<Rot>(-1, 0); // if bad, return a negative x rotation
		for (pair<shared_ptr<Rot>, shared_ptr<PositionDouble>> rotation : rotations) {

			shared_ptr<PositionDouble> rotated = rotation.second;

			// determine which rotation is better
			vector<shared_ptr<PositionDouble>> nearby = branch_tips.getNearby(rotated, 100, 3);
			double closest_nearby = 1000000;
			for (shared_ptr<PositionDouble> nearby_tip : nearby) {
				double dist = euclideanDistance(nearby_tip->x, nearby_tip->z, rotated->x, rotated->z);
				if (dist < closest_nearby) closest_nearby = dist;
			}
			

			// if both are bad, return a negative x rotation
			// if (closest_nearby < closest && closest_nearby > needed_distance) {
			if (closest_nearby > closest || nearby.size() == 0) {
				best_rotation = rotation.first;
				closest = closest_nearby;
			}


		}

		return best_rotation;
	}


	void genRoots() {

		/*
			determine root attributes
			- rootVisibility
			- hangingRootHeight
		*/ 

		// make roots
	}

	//
	int adjustHeightForType(int height, PlantType type) {
		switch(type) {
			case PlantType::BABY:
				return height * 0.3;
				break;
			case PlantType::FOREST: 
				return height * 1.5;
				break;
			case PlantType::FIELD:
				return height * 0.8;
				break;
			default:
				return height;
		}


	}


	//
	int adjustCanopyRadiusForType(int radius, PlantType type) {
		switch(type) {
			case PlantType::BABY:
				return radius * 0.3;
				break;
			case PlantType::FOREST: 
				return radius * 0.8;
				break;
			case PlantType::FIELD:
				return radius * 1.5;
				break;
			default:
				return radius;
		}
	}


	vector<shared_ptr<Block> > rotateBocks(const vector<shared_ptr<Block> >& blocks, const shared_ptr<Rot>& rotation)
	{

		vector<shared_ptr<Block> > rotatedBlocks;
		// determine each block in each slice
		for (shared_ptr<Block>  block : blocks) {

			// rotate block by the rotation of trunk
			shared_ptr<Position> pos = block->position;
			shared_ptr<PositionDouble> rotated = Rot::rotate(std::make_shared<PositionDouble>(pos->x, pos->y, pos->z), rotation);

			// 
			shared_ptr<Block>  newBlock = std::make_shared<Block>();
			newBlock->position = make_shared<Position>(
				round(rotated->x),
				round(rotated->y),
				round(rotated->z)
			);
			newBlock->material = block->material;
			newBlock->size = block->size;
		
			rotatedBlocks.push_back(newBlock);
		}


		return rotatedBlocks;
	}

	void translateBlocks(vector<shared_ptr<Block> >& rotatedBranch, const Position& pos) {
		for (shared_ptr<Block> & block : rotatedBranch) {
			block->position->x += pos.x;
			block->position->y += pos.y;
			block->position->z += pos.z;
		}
	}



	vector<double> makeCanopyRadii(int64_t seed, int height, int canopyHeightRatio, vector<int> functions, int num_shapes, PlantType type)
	{
		vector<double> radii;

		// determine shape heights
		vector<int> heights;
		int type_adjusted_heigth = adjustHeightForType(height, type);
		int entire_canopy_height = type_adjusted_heigth * canopyHeightRatio / 100;
		heights.push_back(entire_canopy_height);
		if (num_shapes > 1) {
			int division = Random::randInt(height, 0, 100);
			heights[0] *= division / 100.0;
			heights.push_back(entire_canopy_height - heights[0]);
		}
		
		
		// make shapes
		int spike = height + (int) type;
		double mid_point_radius = Random::randDouble(seed+spike, 0, height * 0.7, height * 1.5);
		mid_point_radius = adjustCanopyRadiusForType(mid_point_radius, type);

		double current_radius = mid_point_radius;

		// second shape: start at bottom, iterate up to midpoint
		if (num_shapes > 1) {
			for (double y = -heights[1]; y < 0; ++y) {
				int function = functions[1];
				switch(function) {
					case 0: // PARABOLA y = mx^n - b or ((y + b) / m)^1/n
					{
						double n = Random::randDouble(height, 0.1, 1.0, 4.0);
						double m = heights[1] / pow(mid_point_radius, n);
						current_radius = pow(
							(y + heights[1]) / m,
							1/n
						);
						break;
					}
					case 1: // CIRCLE y = -msqrt(radius^2 - x^2) or x = sqrt(radius^2 - (y/-m)^2) 
					{
						// for circles the mid_point_radius is the radius, and then the circle is stretched to make the needed height
						double m = -heights[1] / mid_point_radius;
						current_radius = sqrt(
							pow(mid_point_radius, 2) - pow(y/m, 2)
						);
						break;
					}
					case 2: // random 
					{
						double m = Random::randDouble(height, 0.1, 10.0);
						double step = Random::randDouble(height+y, -2.0, 2.0);
						current_radius += step;
						if (current_radius < 0) current_radius += -step;
						break;
					}
				}
				radii.push_back(current_radius);
			}
		}
		

		// first shape: start at midpoint iterate up
		for (double y = 0; y < heights[0]; ++y) {
				int function = functions[0];
				switch(function) {
					case 0: // PARABOLA y = -mx^n + b or ((y - b) / -m)^1/n
					{
						// XXX  consider not calculating these constant values every time
						double n = Random::randDouble(height, 0.1, 1.0, 4.0);
						double m = heights[0] / pow(mid_point_radius, n);
						current_radius = pow(
							(y - heights[0]) / -m,
							1/n
						);
						break;
					}
					case 1: // CIRCLE y = msqrt(radius^2 - x^2) or x = sqrt(radius^2 - (y/m)^2) 
					{
						// for circles the mid_point_radius is the radius, and then the circle is stretched to make the needed height
						double m = heights[0] / mid_point_radius;
						current_radius = sqrt(
							pow(mid_point_radius, 2) - pow(y/m, 2)
						);
						break;
					}
					case 2: // random 
					{
						double m = Random::randDouble(height+1, 0.1, 10.0);
						double step = Random::randDouble(height+y, -2.0, 2.0);

						// if nearing the top start shrinking the radius
						if (Random::randInt(height, y, heights[0]) > heights[0] * 0.8 && step > 0)
							step *= -1;

						current_radius += step;
						if (current_radius < 0) current_radius += -step;
						break;
					}
				}

				radii.push_back(current_radius);
		}
		
		
		return radii;
	}

	double interpolate_radii(vector<double> &radii, double radius_level)
	{
		if (radii.empty()) {
			return 0;
		}

		radius_level = max(0.0, min(radius_level, (double)radii.size() - 1));
		int upper = ceil(radius_level);
		int lower = floor(radius_level);
		return (radii[upper] + radii[lower]) / 2.0;
	}


	double euclideanDistance(int x, int z, int otherX, int otherZ)
	{
		int deltaX = x - otherX;
		int deltaZ = z - otherZ;

		return sqrt(deltaX * deltaX + deltaZ * deltaZ);

	}

};
