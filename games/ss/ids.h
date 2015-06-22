#ifndef _IDS_H_
#define _IDS_H_

typedef enum {
    BMT_NONE = 0,
    BMT_DESERT = 1,
    BMT_VOLCANO = 2,
    BMT_TUNDRA = 3,
    BMT_JUNGLE = 4,
    BMT_LAKE = 5,
    BMT_DUNGEON = 6,
    BMT_MOUNTAIN = 7,
} BIOMETYPE;

typedef enum {
    CAT_INVAL,
    CAT_PC,
    CAT_BUBBLE,
    CAT_ENEMY,
    CAT_BEAM,
    CAT_BLASTER,
    CAT_DEBRI,
    CAT_FLAG,
    CAT_BUILDER,
    CAT_VOLCANIC_BOMB,
    CAT_BOMBFLOWER_SEED,
    CAT_PC_PART,
} CATEGORY;


typedef enum {
    GT_INVAL = 0,
    GT_GRASS = 1,
    GT_SOIL = 2,
    GT_SAND = 3,
    GT_NUCLEUS = 5,
    GT_ENHANCER = 6,
    GT_BRICK_PANEL = 7,
    GT_DEEP = 8,
    GT_LAVA_COLD = 9,
    GT_LAVA_HOT = 10,
    GT_VENT = 11,
    GT_VENTCORE = 12,
    GT_FIREGEN = 13,
    GT_ROCK = 16,
    GT_TUNDRA = 17,
    GT_SNOW = 18,
    GT_JUNGLE = 19,
    GT_SHRIMPGEN = 20,
    GT_CORE = 21,
    GT_PIT = 22,
    GT_SHIFTER = 23, // Uses dir
    GT_TELEPORTER_INACTIVE = 24,
    GT_TELEPORTER_ACTIVE = 25,
    GT_DUNGEON = 26,
} GROUNDTYPE;

typedef enum {
    BT_INVAL = -1,
    BT_AIR = 0,

    BT_PLAYER_BUILDING_START = 32,
    BT_MOTHERSHIP = 32,
    BT_REACTOR_INACTIVE = 33,
    BT_REACTOR_ACTIVE = 34,
    BT_REACTOR_ARM = 35,
    BT_POLE = 36,
    BT_EXCHANGE_INACTIVE = 37,
    BT_EXCHANGE_ACTIVE = 38,
    BT_CABLE = 39,
    BT_PORTAL_INACTIVE = 40,
    BT_PORTAL_ACTIVE = 41,
    BT_TURRET_INACTIVE = 42,
    BT_TURRET_ACTIVE = 43,
    BT_FENCE = 44,
    BT_PLAYER_BUILDING_END = 45,

    
    BT_ROCK = 64,
    BT_SOIL = 65,
    BT_SNOW = 66,
    BT_IVY = 67, // Use 'content' member to store connectivity
    BT_CELL = 68,    
    BT_TREE = 69, // Use 'content' to have an apple or not
    BT_CRYSTAL = 70, // undestroyable
    BT_PAIRBARRIER = 71, // undestroyable but can be made sleep for some time
    BT_WEAK_CRYSTAL = 72, // destructable but hard
    
    
    BT_IRONORE = 80,
    BT_RAREMETALORE = 81,

    BT_HARDROCK = 96,
    BT_FIREGEN = 97, // use dir
    BT_BOMBFLOWER = 98, // use 'content' for indicating state
    BT_BEEHIVE = 99, // use 'content' for indicating state
    BT_CRYSTAL_BEAMER = 100,

    // buildings
    BT_ENEMY_BUILDING_START = 128,
    BT_DEROTA = 128,
    BT_ENEMY_FRAME = 129,
    BT_ENEMY_EYE_DESTROYED = 130,
    BT_CAGE = 131,
    BT_BARRIERGEN = 132,
    BT_BARRIER = 133,
    BT_HYPERGEN = 134,
    BT_FLYGEN = 135,
    BT_COPTERGEN = 136,
    BT_SLIMEGEN = 137,
    BT_BIRDGEN = 138,
    BT_CORE_COVER = 139,
    BT_GRAY_GOO = 140,
    
    // eyes
    BT_EYE_START = 144,
    BT_MONO_EYE = 144,
    BT_COMBI_EYE = 145,
    BT_PANEL_EYE = 146,
    BT_ROUND_EYE = 147,
    BT_SHIELD_EYE = 148,
    BT_CHASER_EYE = 149,
    BT_BUILDER_EYE = 150,
    BT_EYE_END = 144+15,
    BT_ENEMY_BUILDING_END = 128+31,
} BLOCKTYPE;

typedef enum {
    ST_NONE = 0,
    ST_WATER = 128,
    ST_BLOODWATER = 129,
    ST_WEED = 130,
    ST_ENERGIUM = 131,
    ST_HYPERLINE = 132,
    ST_GROUNDWORK = 133,
    ST_ROCK = 134,
    ST_ICE = 135,
    ST_CORE_CABLE_INACTIVE = 136,
    ST_CORE_CABLE_ACTIVE = 137,
    ST_CORE_SWITCH_INACTIVE = 138,
    ST_CORE_SWITCH_ACTIVE = 139,
} SURFACETYPE;




// Color in html 4.01
typedef enum {
    CC_BLACK = 0,
    CC_LOWBLUE = 1,
    CC_LOWGREEN = 2,
    CC_LOWCYAN = 3,
    CC_LOWRED = 4,
    CC_LOWMAGENTA = 5,
    CC_BROWN = 6,
    CC_LIGHTGRAY = 7,
    CC_DARKGRAY = 8,
    CC_HIGHBLUE = 9,
    CC_HIGHGREEN = 10,
    CC_HIGHCYAN = 11,
    CC_HIGHRED = 12,
    CC_HIGHMAGENTA = 13,
    CC_YELLOW = 14,
    CC_WHITE = 15    
} COLORCODE;

typedef enum {
    BST_DEFAULT = 0,
    BST_GIREV = 1,
    BST_TAKWASHI = 2,
    BST_REPAIRER = 3,
    BST_BUILDER = 4,
    BST_CHASER = 5,
    BST_VOIDER = 6,
    BST_DEFENDER = 7,
} BOSSTYPE;


#endif
