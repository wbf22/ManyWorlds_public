// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <format>
#include "Random.h"



using namespace std;

enum class PrecipitationType : int {
	DESERT = 4,
	SEMI_ARID = 15,
	TEMPERATE = 35,
	RAIN_FOREST = 80,
	HURRICANE = 100
};


enum class WoodMaterialType {
	WOOD,
	FIBERS,
	SPONGE,
	VEIN_STONE,
	RESIN
};

/**
 * 
 */
class BMaterial {

public:

	BMaterial()
	{
	}

	~BMaterial()
	{
	}

	static int getHardness(string type)
	{
		// 0 is min hardness, 10 is the max hardness
		if (type == BMaterial::DEFAULT)
			return 5;

		return 5;
	}


	static inline const string DEFAULT = "DEFAULT";
	static inline const string TEST = "TEST";
	static inline const string BDOG = "BDOG";
	static inline const string VOID = "VOID";

	static inline const string GRANITE_WHITE = "GRANITE_WHITE";
	static inline const string GRANITE_SALMON = "GRANITE_SALMON";
	static inline const string GRANITE_GREY = "GRANITE_GREY";
	static inline const string BASALT = "BASALT"; // common in space
	static inline const string BASALT_DARK = "BASALT_DARK"; // common in space
	static inline const string ANDESTITE = "ANDESTITE"; // common in space
	static inline const string ANDESTITE_DARK = "ANDESTITE_DARK";
	static inline const string OBSIDIAN = "OBSIDIAN";
	static inline const string OBSIDIAN_SNOWFLAKE = "OBSIDIAN_SNOWFLAKE";
	static inline const string ANORTHOSITE = "ANORTHOSITE"; // on the moon and earth
	static inline const string CHONDRITE_GREY = "CHONDRITE_GREY"; // metorites and asteriods
	static inline const string CHONDRITE_RED = "CHONDRITE_RED"; // metorites and asteriods
	static inline const string PERIDOTITE = "PERIDOTITE";
	static inline const string RHYOLITE_GREEN = "RHYOLITE_GREEN";
	static inline const string RHYOLITE_SALMON = "RHYOLITE_SALMON";
	static inline const string RHYOLITE_GREY = "RHYOLITE_GREY";
	static inline const string NICKEL = "NICKEL";
	static inline const string IRON = "IRON";
	static inline const string GOLD = "GOLD";
	static inline const string COPPER = "COPPER";
	static inline const string TITANIUM = "TITANIUM";
	static inline const string SULFUR = "SULFUR";
	static inline const string PLATINUM = "PLATINUM";
	static inline const string ALUMINUM = "ALUMINUM";
	static inline const string TIN = "TIN";
	static inline const string SILVER = "SILVER";
	static inline const string LEAD = "LEAD";

	static inline const string LIMESTONE = "LIMESTONE"; // by marine organisms (or calcite)
	static inline const string LIMESTONE_WHITE = "LIMESTONE_WHITE";
	static inline const string LIMESTONE_TAN = "LIMESTONE_TAN";
	static inline const string SANDSTONE_PURPLE = "SANDSTONE_PURPLE";
	static inline const string SANDSTONE_RED = "SANDSTONE_RED";
	static inline const string SANDSTONE_ORANGE = "SANDSTONE_ORANGE";
	static inline const string SANDSTONE_YELLOW = "SANDSTONE_YELLOW";
	static inline const string SANDSTONE_WHITE = "SANDSTONE_WHITE";
	static inline const string SANDSTONE_GREY = "SANDSTONE_GREY";
	static inline const string TRAVERTINE = "TRAVERTINE";
	static inline const string SHALE = "SHALE"; // organic matter
	static inline const string SHALE_TAN = "SHALE_TAN";
	static inline const string SHALE_BROWN = "SHALE_BROWN";
	static inline const string CONGLOMERATE_TAN = "CONGLOMERATE_TAN"; // created under rivers
	static inline const string CONGLOMERATE_GREY = "CONGLOMERATE_GREY";
	static inline const string CONGLOMERATE_PINK = "CONGLOMERATE_PINK";
	static inline const string CONGLOMERATE_RED = "CONGLOMERATE_RED";
	static inline const string CHALK = "CHALK";
	static inline const string COAL = "COAL"; // organic
	static inline const string BAUXITE = "BAUXITE"; // aluminum collecting from intense weathering, common aluminum ore

	static inline const string MARBLE = "MARBLE"; // from limestone
	static inline const string MARBLE_GREY = "MARBLE_GREY";
	static inline const string MARBLE_TAN = "MARBLE_TAN";
	static inline const string MARBLE_PINK = "MARBLE_PINK";
	static inline const string SCHIST = "SCHIST";
	static inline const string SLATE = "SLATE"; // from shale
	static inline const string SLATE_BROWN = "SLATE_BROWN";
	static inline const string SLATE_PINK = "SLATE_PINK";
	static inline const string GNEISS = "GNEISS";
	static inline const string GNEISS_PINK = "GNEISS_PINK";
	static inline const string GNEISS_WHITE = "GNEISS_WHITE";
	static inline const string JADE = "JADE";
	static inline const string QUARTZITE = "QUARTZITE";
	static inline const string SAPPHIRE = "SAPPHIRE"; //alumimun rich

	static inline const string SAND = "SAND";
	static inline const string SAND_YELLOW = "SAND_YELLOW";
	static inline const string SAND_ORANGE = "SAND_ORANGE";
	static inline const string SAND_BROWN = "SAND_BROWN";

	static inline const string ICE = "ICE";
	static inline const string SNOW = "SNOW";
	static inline const string DIRTY_ICE_GREY = "DIRTY_ICE_GREY";
	static inline const string DIRTY_ICE_PINK = "DIRTY_ICE_PINK";
	static inline const string DIRT = "DIRT";
	static inline const string DUST = "DUST";
	static inline const string DUST_BLACK = "DUST_BLACK";
	static inline const string DUST_YELLOW = "DUST_YELLOW";
	static inline const string DUST_ORANGE = "DUST_ORANGE";
	static inline const string DUST_BROWN = "DUST_BROWN";
	static inline const string AMMONIA_ICE = "AMMONIA_ICE";
	static inline const string METHANE_ICE = "METHANE_ICE";

	static inline const string SOIL = "SOIL";
	static inline const string SOIL_DARK_ORGANIC = "SOIL_DARK_ORGANIC";
	static inline const string SOIL_TAN = "SOIL_TAN";
	static inline const string SOIL_RED = "SOIL_RED";

	static inline const string DEAD_GRASS = "DEAD_GRASS";
	static inline const string LEAVES = "LEAVES";
	static inline const string LEAVES_RED = "LEAVES_RED";
	static inline const string LEAVES_GREEN = "LEAVES_GREEN";
	static inline const string LEAVES_YELLOW = "LEAVES_YELLOW";
	static inline const string LEAVES_ORANGE = "LEAVES_ORANGE";
	static inline const string LEAVES_PURPLE = "LEAVES_PURPLE"; // rare
	static inline const string LEAVES_BLUE = "LEAVES_BLUE"; // rare
	static inline const string STICKS = "STICKS";
	static inline const string WOOD_CHIPS = "WOOD_CHIPS";
	static inline const string MOSS = "MOSS";
	static inline const string MOSS_SPARSE = "MOSS_SPARSE";
	static inline const string MOSS_ORANGE = "MOSS_ORANGE";
	static inline const string MOSS_RED = "MOSS_RED";

	static inline const string WOOD_BROWN = "WOOD_BROWN";
	static inline const string WOOD_TAN = "WOOD_TAN";
	static inline const string WOOD_RED = "WOOD_RED";
	static inline const string WOOD_ORANGE = "WOOD_ORANGE";
	static inline const string WOOD_YELLOW = "WOOD_YELLOW";
	static inline const string WOOD_GREEN = "WOOD_GREEN";
	static inline const string WOOD_BLUE = "WOOD_BLUE";
	static inline const string WOOD_PURPLE = "WOOD_PURPLE";
	static inline const string WOOD_PINK = "WOOD_PINK";
	static inline const string WOOD_GREY = "WOOD_GREY";
	static inline const string WOOD_WHITE = "WOOD_WHITE";
	static inline const string WOOD_BLACK = "WOOD_BLACK";

	static inline const string FIBERS_BROWN = "FIBERS_BROWN";
	static inline const string FIBERS_TAN = "FIBERS_TAN";
	static inline const string FIBERS_RED = "FIBERS_RED";
	static inline const string FIBERS_ORANGE = "FIBERS_ORANGE";
	static inline const string FIBERS_YELLOW = "FIBERS_YELLOW";
	static inline const string FIBERS_GREEN = "FIBERS_GREEN";
	static inline const string FIBERS_BLUE = "FIBERS_BLUE";
	static inline const string FIBERS_PURPLE = "FIBERS_PURPLE";
	static inline const string FIBERS_PINK = "FIBERS_PINK";
	static inline const string FIBERS_GREY = "FIBERS_GREY";
	static inline const string FIBERS_WHITE = "FIBERS_WHITE";
	static inline const string FIBERS_BLACK = "FIBERS_BLACK";

	static inline const string SPONGE_BROWN = "SPONGE_BROWN";
	static inline const string SPONGE_TAN = "SPONGE_TAN";
	static inline const string SPONGE_RED = "SPONGE_RED";
	static inline const string SPONGE_ORANGE = "SPONGE_ORANGE";
	static inline const string SPONGE_YELLOW = "SPONGE_YELLOW";
	static inline const string SPONGE_GREEN = "SPONGE_GREEN";
	static inline const string SPONGE_BLUE = "SPONGE_BLUE";
	static inline const string SPONGE_PURPLE = "SPONGE_PURPLE";
	static inline const string SPONGE_PINK = "SPONGE_PINK";
	static inline const string SPONGE_GREY = "SPONGE_GREY";
	static inline const string SPONGE_WHITE = "SPONGE_WHITE";
	static inline const string SPONGE_BLACK = "SPONGE_BLACK";

	static inline const string VEIN_STONE_BROWN = "VEIN_STONE_BROWN";
	static inline const string VEIN_STONE_TAN = "VEIN_STONE_TAN";
	static inline const string VEIN_STONE_RED = "VEIN_STONE_RED";
	static inline const string VEIN_STONE_ORANGE = "VEIN_STONE_ORANGE";
	static inline const string VEIN_STONE_YELLOW = "VEIN_STONE_YELLOW";
	static inline const string VEIN_STONE_GREEN = "VEIN_STONE_GREEN";
	static inline const string VEIN_STONE_BLUE = "VEIN_STONE_BLUE";
	static inline const string VEIN_STONE_PURPLE = "VEIN_STONE_PURPLE";
	static inline const string VEIN_STONE_PINK = "VEIN_STONE_PINK";
	static inline const string VEIN_STONE_GREY_BROWN = "VEIN_STONE_GREY";
	static inline const string VEIN_STONE_GREY_BLACK = "VEIN_STONE_WHITE";
	static inline const string VEIN_STONE_BLACK = "VEIN_STONE_BLACK";

	static inline const string RESIN_BROWN = "RESIN_BROWN";
	static inline const string RESIN_TAN = "RESIN_TAN";
	static inline const string RESIN_RED = "RESIN_RED";
	static inline const string RESIN_ORANGE = "RESIN_ORANGE";
	static inline const string RESIN_YELLOW = "RESIN_YELLOW";
	static inline const string RESIN_GREEN = "RESIN_GREEN";
	static inline const string RESIN_BLUE = "RESIN_BLUE";
	static inline const string RESIN_PURPLE = "RESIN_PURPLE";
	static inline const string RESIN_PINK = "RESIN_PINK";
	static inline const string RESIN_GREY = "RESIN_GREY";
	static inline const string RESIN_WHITE = "RESIN_WHITE";
	static inline const string RESIN_BLACK = "RESIN_BLACK";

	static inline const string GRAVEL_BLACK = "GRAVEL_BLACK";
	static inline const string GRAVEL_GREY = "GRAVEL_GREY";
	static inline const string GRAVEL_WHITE = "GRAVEL_WHITE";
	static inline const string GRAVEL_RED = "GRAVEL_RED";
	static inline const string GRAVEL_ORANGE = "GRAVEL_ORANGE";
	static inline const string GRAVEL_YELLOW = "GRAVEL_YELLOW";
	static inline const string GRAVEL_PINK = "GRAVEL_PINK";
	static inline const string STONES_BLACK = "STONES_BLACK";
	static inline const string STONES_GREY = "STONES_GREY";
	static inline const string STONES_WHITE = "STONES_WHITE";
	static inline const string STONES_RED = "STONES_RED";
	static inline const string STONES_ORANGE = "STONES_ORANGE";
	static inline const string STONES_YELLOW = "STONES_YELLOW";
	static inline const string STONES_PINK = "STONES_PINK";

	static inline const string SALT_WHITE = "SALT_WHITE";
	static inline const string SALT_PINK = "SALT_PINK";
	static inline const string SALT_ORANGE = "SALT_ORANGE";

	static inline const string WATER = "WATER";

	static inline const string GALAXY_GLOW = "GALAXY_GLOW";
	static inline const string NEBULA_DUST = "NEBULA_DUST";
	static inline const string NEBULA_GAS = "NEBULA_GAS";
	static inline const string BLUE_STAR = "BLUE_STAR";

	static inline const string SMOOTH_SKIN_RED = "SMOOTH_SKIN_RED";
	static inline const string SMOOTH_SKIN_ORANGE = "SMOOTH_SKIN_ORANGE";
	static inline const string SMOOTH_SKIN_YELLOW = "SMOOTH_SKIN_YELLOW";
	static inline const string SMOOTH_SKIN_GREEN = "SMOOTH_SKIN_GREEN";
	static inline const string SMOOTH_SKIN_BLUE = "SMOOTH_SKIN_BLUE";
	static inline const string SMOOTH_SKIN_PURPLE = "SMOOTH_SKIN_PURPLE";
	static inline const string SMOOTH_SKIN_PINK = "SMOOTH_SKIN_PINK";
	static inline const string SMOOTH_SKIN_GRAY = "SMOOTH_SKIN_GRAY";
	static inline const string SMOOTH_SKIN_WHITE = "SMOOTH_SKIN_WHITE";
	static inline const string SMOOTH_SKIN_BLACK = "SMOOTH_SKIN_BLACK";
	static inline const string SMOOTH_SKIN_BROWN = "SMOOTH_SKIN_BROWN";
	static inline const string SMOOTH_SKIN_TAN = "SMOOTH_SKIN_TAN";

	static inline const string SCALY_SKIN_RED = "SCALY_SKIN_RED";
	static inline const string SCALY_SKIN_ORANGE = "SCALY_SKIN_ORANGE";
	static inline const string SCALY_SKIN_YELLOW = "SCALY_SKIN_YELLOW";
	static inline const string SCALY_SKIN_GREEN = "SCALY_SKIN_GREEN";
	static inline const string SCALY_SKIN_BLUE = "SCALY_SKIN_BLUE";
	static inline const string SCALY_SKIN_PURPLE = "SCALY_SKIN_PURPLE";
	static inline const string SCALY_SKIN_PINK = "SCALY_SKIN_PINK";
	static inline const string SCALY_SKIN_GRAY = "SCALY_SKIN_GRAY";
	static inline const string SCALY_SKIN_WHITE = "SCALY_SKIN_WHITE";
	static inline const string SCALY_SKIN_BLACK = "SCALY_SKIN_BLACK";
	static inline const string SCALY_SKIN_BROWN = "SCALY_SKIN_BROWN";
	static inline const string SCALY_SKIN_TAN = "SCALY_SKIN_TAN";

	static inline const string FUR_SKIN_RED = "FUR_SKIN_RED";
	static inline const string FUR_SKIN_ORANGE = "FUR_SKIN_ORANGE";
	static inline const string FUR_SKIN_YELLOW = "FUR_SKIN_YELLOW";
	static inline const string FUR_SKIN_GREEN = "FUR_SKIN_GREEN";
	static inline const string FUR_SKIN_BLUE = "FUR_SKIN_BLUE";
	static inline const string FUR_SKIN_PURPLE = "FUR_SKIN_PURPLE";
	static inline const string FUR_SKIN_PINK = "FUR_SKIN_PINK";
	static inline const string FUR_SKIN_GRAY = "FUR_SKIN_GRAY";
	static inline const string FUR_SKIN_WHITE = "FUR_SKIN_WHITE";
	static inline const string FUR_SKIN_BLACK = "FUR_SKIN_BLACK";
	static inline const string FUR_SKIN_BROWN = "FUR_SKIN_BROWN";
	static inline const string FUR_SKIN_TAN = "FUR_SKIN_TAN";




	static inline vector<string> space_rocks = {
		GRANITE_WHITE,
		// GRANITE_SALMON,
		// GRANITE_GREY,
		BASALT,
		// BASALT_DARK,
		ANDESTITE,
		// ANDESTITE_DARK,
		// OBSIDIAN,
		// OBSIDIAN_SNOWFLAKE,
		// ANORTHOSITE,
		// CHONDRITE_GREY,
		CHONDRITE_RED,
		PERIDOTITE,
		// RHYOLITE_GREEN,
		RHYOLITE_SALMON,
		// RHYOLITE_GREY,
		// NICKEL,
		// IRON,
		// GOLD,
		// COPPER,
		// TITANIUM,
		// SULFUR,
		// PLATINUM,
		// ALUMINUM,
		// TIN,
		// SILVER,
		// LEAD,

		SCHIST,
		// GNEISS,
		// GNEISS_PINK,
		// GNEISS_WHITE,
		// JADE,
		// SAPPHIRE,

		// SAND,
		// SAND_YELLOW,
		// SAND_ORANGE,
		// SAND_BROWN,

		DIRT,
		DUST,
		// DUST_BLACK,
		// DUST_YELLOW,
		// DUST_ORANGE,
		// DUST_BROWN,

		// GRAVEL_BLACK,
		// GRAVEL_GREY,
		// GRAVEL_WHITE,
		// GRAVEL_RED,
		// GRAVEL_ORANGE,
		// GRAVEL_YELLOW,
		// GRAVEL_PINK,
		// STONES_BLACK,
		// STONES_GREY,
		// STONES_WHITE,
		// STONES_RED,
		// STONES_ORANGE,
		// STONES_YELLOW,
		// STONES_PINK,

		// SALT_WHITE,
		// SALT_PINK,
		// SALT_ORANGE,
	};

	static inline vector<string> water_formed_rocks = {
		// SANDSTONE_PURPLE,
		SANDSTONE_RED,
		// SANDSTONE_ORANGE,
		// SANDSTONE_YELLOW,
		// SANDSTONE_WHITE,
		// SANDSTONE_GREY,
		// TRAVERTINE,
		CONGLOMERATE_TAN,
		// CONGLOMERATE_GREY,
		// CONGLOMERATE_PINK,
		// CONGLOMERATE_RED,
		// BAUXITE,

		// QUARTZITE,

	};

	static inline vector<string> organic_rocks = {
		// LIMESTONE,
		// LIMESTONE_WHITE,
		// LIMESTONE_TAN,
		SHALE,
		// SHALE_TAN,
		// SHALE_BROWN,
		COAL,
		// CHALK,
		MARBLE,
		// MARBLE_GREY,
		// MARBLE_TAN,
		// MARBLE_PINK,
		// SLATE,
		// SLATE_BROWN,
		// SLATE_PINK,

	};

	static inline vector<string> organic_materials = {
		// SOIL_DARK_ORGANIC,

		// DEAD_GRASS,
		LEAVES,
		// LEAVES_RED,
		// LEAVES_GREEN,
		// LEAVES_YELLOW,
		// LEAVES_ORANGE,
		// LEAVES_PURPLE,
		// LEAVES_BLUE,
		STICKS,
		// WOOD_CHIPS,
		MOSS,
		MOSS_SPARSE,
		// MOSS_ORANGE,
		// MOSS_RED,
	};

	static inline vector<string> dry_substrates = {
		// SOIL,
		// SOIL_TAN,
		// SOIL_RED,
		SAND,
		// SAND_YELLOW,
		// SAND_ORANGE,
		// SAND_BROWN,
		DIRT,
		DUST,
		// DUST_BLACK,
		// DUST_YELLOW,
		// DUST_ORANGE,
		// DUST_BROWN,
		// GRAVEL_BLACK,
		// GRAVEL_GREY,
		// GRAVEL_WHITE,
		// GRAVEL_RED,
		// GRAVEL_ORANGE,
		// GRAVEL_YELLOW,
		// GRAVEL_PINK,
		// STONES_BLACK,
		// STONES_GREY,
		// STONES_WHITE,
		// STONES_RED,
		// STONES_ORANGE,
		// STONES_YELLOW,
		// STONES_PINK,
		// SALT_WHITE,
		// SALT_PINK,
		// SALT_ORANGE
	};

	static inline vector<string> high_dry_substrates = {
		DIRT,
		// GRAVEL_BLACK,
		// GRAVEL_GREY,
		// GRAVEL_WHITE,
		// GRAVEL_RED,
		// GRAVEL_ORANGE,
		// GRAVEL_YELLOW,
		// GRAVEL_PINK,
		// STONES_BLACK,
		// STONES_GREY,
		// STONES_WHITE,
		// STONES_RED,
		// STONES_ORANGE,
		// STONES_YELLOW,
		// STONES_PINK,
	};


	static inline vector<string> ice = {
		ICE,
		SNOW,
		// DIRTY_ICE_GREY,
		// DIRTY_ICE_PINK,
		// AMMONIA_ICE,
		// METHANE_ICE,
	};
	
	static inline vector<string> water_and_liquids = {
		WATER,
	};

	static inline vector<string> wood = {
		WOOD_BLACK,
		WOOD_BROWN,
		WOOD_ORANGE,
		WOOD_RED,
		WOOD_TAN,
		WOOD_YELLOW,
	};

	static inline vector<string> fibers = {
		FIBERS_BLACK,
		FIBERS_BROWN,
		FIBERS_BLUE,
		FIBERS_GREEN,
		FIBERS_GREY,
		FIBERS_ORANGE,
		FIBERS_PINK,
		FIBERS_PURPLE,
		FIBERS_RED,
		FIBERS_TAN,
		FIBERS_WHITE,
		FIBERS_YELLOW,
	};

	static inline vector<string> sponge = {
		SPONGE_BLACK,
		SPONGE_BROWN,
		SPONGE_BLUE,
		SPONGE_GREEN,
		SPONGE_GREY,
		SPONGE_ORANGE,
		SPONGE_PINK,
		SPONGE_PURPLE,
		SPONGE_RED,
		SPONGE_TAN,
		SPONGE_WHITE,
		SPONGE_YELLOW,
	};

	static inline vector<string> vein_stone = {
		VEIN_STONE_BLACK,
		VEIN_STONE_BROWN,
		VEIN_STONE_BLUE,
		VEIN_STONE_GREEN,
		VEIN_STONE_ORANGE,
		VEIN_STONE_PINK,
		VEIN_STONE_PURPLE,
		VEIN_STONE_RED,
		VEIN_STONE_TAN,
		VEIN_STONE_YELLOW,
		VEIN_STONE_GREY_BROWN,
		VEIN_STONE_GREY_BLACK,
	};

	static inline vector<string> resin = {
		RESIN_BLACK,
		RESIN_BROWN,
		RESIN_BLUE,
		RESIN_GREEN,
		RESIN_GREY,
		RESIN_ORANGE,
		RESIN_PINK,
		RESIN_PURPLE,
		RESIN_RED,
		RESIN_TAN,
		RESIN_WHITE,
		RESIN_YELLOW,
	};


	static inline vector<string> leaves = {
		LEAVES_BLUE,
		LEAVES_GREEN,
		LEAVES_PURPLE,
		LEAVES_YELLOW,
		LEAVES,
	};

	static string getRandomMaterial(
		int spike, 
		bool liquidPresent, 
		bool organicPresent, 
		bool includeOrganicMaterials
	) {
		
		// figure out which materials are included
		int total_options = space_rocks.size();
		if (liquidPresent) total_options += water_formed_rocks.size();
		if (organicPresent) total_options += organic_rocks.size();
		if (includeOrganicMaterials) total_options += organic_materials.size();

		// Get a random index based on the spike
		int index = abs(spike) % total_options;
		int offset = 0;
		if (index < space_rocks.size()) {
			return space_rocks[index];
		}
		else offset += space_rocks.size();

		if (liquidPresent && index < offset + water_formed_rocks.size()) {
			return water_formed_rocks[index % water_formed_rocks.size()];
		}
		else offset += water_formed_rocks.size();

		if (organicPresent && index < offset + organic_rocks.size()) {
			return organic_rocks[index % organic_rocks.size()];
		}
		else offset += organic_rocks.size();

		if (includeOrganicMaterials && index < offset + organic_materials.size()) {
			return organic_materials[index % organic_materials.size()];
		}
		else offset += organic_materials.size();

		return "";
	}


	static string get_random_substrate(PrecipitationType type, int64_t spike, bool low_spot, bool has_plants) {
		
		bool is_wet = type == PrecipitationType::HURRICANE || type == PrecipitationType::RAIN_FOREST || (type == PrecipitationType::TEMPERATE && low_spot);
		bool is_high_and_dry = !low_spot && (type == PrecipitationType::TEMPERATE || type == PrecipitationType::SEMI_ARID || type == PrecipitationType::DESERT);
		bool is_high_no_plants = !low_spot && !has_plants;

		if (is_wet && has_plants) {
			return BMaterial::organic_materials[spike % organic_materials.size()];
		}
		else if (is_high_and_dry || is_high_no_plants) {
			return BMaterial::high_dry_substrates[spike % high_dry_substrates.size()];
		}
		else {
			return BMaterial::dry_substrates[spike % dry_substrates.size()];
		}

	}

	static string path(string material) {
		return format("assets/textures/{}.png", material);
	}

	static string get_random_wood(vector<WoodMaterialType> possible_types, int64_t seed) {
		if (possible_types.empty()) return BMaterial::DEFAULT;

		WoodMaterialType type = possible_types[Random::randInt(
			seed,
			0,
			static_cast<int64_t>(possible_types.size())
		)];
		const vector<string>* materials = nullptr;

		switch (type) {
			case WoodMaterialType::WOOD:
				materials = &BMaterial::wood;
				break;
			case WoodMaterialType::FIBERS:
				materials = &BMaterial::fibers;
				break;
			case WoodMaterialType::SPONGE:
				materials = &BMaterial::sponge;
				break;
			case WoodMaterialType::VEIN_STONE:
				materials = &BMaterial::vein_stone;
				break;
			case WoodMaterialType::RESIN:
				materials = &BMaterial::resin;
				break;
		}

		if (materials == nullptr || materials->empty()) return BMaterial::DEFAULT;
		return (*materials)[Random::randInt(
			seed + 1,
			0,
			static_cast<int64_t>(materials->size())
		)];
	}

	static string get_random_leaf(int64_t seed) {
		return leaves[Random::randInt(seed, 0, leaves.size())];
	};

};
