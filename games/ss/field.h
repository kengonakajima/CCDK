#ifndef _FIELD_H_
#define _FIELD_H_

#define PIXEL_PER_CELL 24
#define PPC PIXEL_PER_CELL

#define CHUNKSZ 16

typedef enum  {
    FTS_NOT_EYE = 0,
    FTS_IDLE = 1,
    FTS_ACTIVE = 2,
    FTS_MAD = 3,
} FTSTATE;

typedef enum {
    OPENNESS_CLOSED = 0,
    OPENNESS_ALMOST_CLOSED = 1,
    OPENNESS_ALMOST_OPEN = 2,
    OPENNESS_OPEN = 3,
} OPENNESS;

class Cell {
public:
    GROUNDTYPE gt;
    BLOCKTYPE bt;
    SURFACETYPE st;    


    int damage;  // wall damage. not HP.  Add on attack, and be desroyed when reached maxhp.
    unsigned short randomness;
    static const int RANDOMNESS_MAX = 65536;
    static const int CONNECT_R = 1, CONNECT_L = 2, CONNECT_U = 4, CONNECT_D = 8, CONNECT_NONE=0; // use operator '|' to dir
    short content; // How many/much things are stored in a block?
    short moisture; // Spreads from water
    DIR dir; 
    int powergrid_id; // 0:no power, >=1: have powergrid
    int fortress_id; // 0:no fortress. >=1: eye of fortress.
    int ftstate;
    int hyper_count;
    bool untouched; // True if the block is destroyed by PC.

    
    //
    inline void clearConnect() {
        content = CONNECT_NONE;
    }
    inline void setConnect( DIR d ) {
        switch(d) {
        case DIR_UP: content |= CONNECT_U; break;
        case DIR_DOWN: content |= CONNECT_D; break;
        case DIR_RIGHT: content |= CONNECT_R; break;
        case DIR_LEFT: content |= CONNECT_L; break;
        case DIR_NONE: content = CONNECT_NONE; break;
        }
    }
    inline bool isConnected( DIR d ) {
        switch(d) {
        case DIR_UP: return content & CONNECT_U; 
        case DIR_DOWN: return content & CONNECT_D; 
        case DIR_RIGHT: return content & CONNECT_R;
        case DIR_LEFT: return content & CONNECT_L;
        case DIR_NONE: return false;
        default: return false;
        }
    }
    inline bool isReactor() {
        return bt == BT_REACTOR_INACTIVE || bt == BT_REACTOR_ACTIVE || bt == BT_REACTOR_ARM;
    }
    inline bool isSnowGrowable() {
        return gt == GT_TUNDRA || gt == GT_SNOW;
    }
    inline bool isWall(bool enemy) {
        if( enemy ) {
            if( bt == BT_BARRIER )return false;
        }
        
        switch(bt) {
        case BT_INVAL:
        case BT_AIR:
        case BT_ENEMY_EYE_DESTROYED:
        case BT_CABLE:
        case BT_BUILDER_EYE: // Builder can go over this
        case BT_POLE:
            return false;
        case BT_CELL:            
        case BT_SOIL:
        case BT_SNOW:
        case BT_IVY:
        case BT_BOMBFLOWER:
        case BT_BEEHIVE:
        case BT_ROCK:
        case BT_TREE:
        case BT_CRYSTAL:
        case BT_CRYSTAL_BEAMER:            
        case BT_WEAK_CRYSTAL:
        case BT_PAIRBARRIER:
        case BT_MOTHERSHIP:
        case BT_IRONORE:
        case BT_RAREMETALORE:
        case BT_DEROTA:
        case BT_CAGE:
        case BT_ENEMY_FRAME:
        case BT_MONO_EYE:
        case BT_COMBI_EYE:
        case BT_PANEL_EYE:
        case BT_ROUND_EYE:
        case BT_SHIELD_EYE:
        case BT_CHASER_EYE:
        case BT_REACTOR_INACTIVE:
        case BT_REACTOR_ACTIVE:
        case BT_REACTOR_ARM:
        case BT_EXCHANGE_INACTIVE:
        case BT_EXCHANGE_ACTIVE:
        case BT_PORTAL_INACTIVE:
        case BT_PORTAL_ACTIVE:
        case BT_TURRET_INACTIVE:
        case BT_TURRET_ACTIVE:
        case BT_FENCE:
        case BT_EYE_END:
        case BT_BARRIERGEN:
        case BT_BARRIER:
        case BT_HYPERGEN:
        case BT_FLYGEN:
        case BT_HARDROCK:
        case BT_FIREGEN:
        case BT_COPTERGEN:
        case BT_SLIMEGEN:
        case BT_BIRDGEN:
        case BT_CORE_COVER:
        case BT_GRAY_GOO:
            return true;
        default:
            return false;
        }
        return false;
    }
    inline bool isBigBossInCage() {
        BOSSTYPE bst = (BOSSTYPE) content;
        if( bt == BT_CAGE && bst == BST_GIREV ) return true; // TODO: add new bosses
        return false;
    }
    inline bool isPlayerBuilding() {
        return (bt >= BT_PLAYER_BUILDING_START && bt <= BT_PLAYER_BUILDING_END );
    }
    inline bool isEnemyBuilding() {
        return ( bt >= BT_ENEMY_BUILDING_START && bt <= BT_ENEMY_BUILDING_END );
    }
    inline bool isEnemyEye() {
        return ( bt >= BT_EYE_START && bt <= BT_EYE_END );
    }

    inline bool isEnemyFrameEnabler() {
        return ( bt == BT_ENEMY_FRAME || isEnemyEye() || bt == BT_ENEMY_EYE_DESTROYED|| gt == GT_CORE );
    }
    inline bool isEnemyBulletHit() {
        if( bt == BT_CELL ) return false;
        return isWall(true) && (!isEnemyBuilding() );
    }
    inline bool isBlockPuttable() {
        return bt == BT_AIR && ( gt != GT_CORE ) && (gt != GT_PIT );
    }
    inline bool isGroundBeamHit() {
        return (gt == GT_ENHANCER ) || (gt == GT_NUCLEUS) || (gt == GT_CORE);
    }
    inline bool isBeamHit() {
        if( isEnemyEye() ) {
            if( bt == BT_SHIELD_EYE ) return true; else return getEyeOpenness() == OPENNESS_OPEN;
        } else if( bt == BT_ENEMY_FRAME ) {
            return false;
        } else {
            return ( (isWall(false)&& (!isPlayerBuilding())) || isGroundBeamHit() || (bt==BT_BARRIER) ||(bt==BT_FIREGEN));
        }
    }
    inline bool isBeamAvoidOnShoot() {
        return isWall(false) && isEnemyBuilding()==false;
    }
    inline bool canDig() {
        return gt == GT_SNOW || gt == GT_JUNGLE || st == ST_WEED || st == ST_ROCK;
    }
    inline bool isImmutableAgainstBeam() {
        return bt == BT_BARRIER || bt == BT_CRYSTAL || bt == BT_CORE_COVER || (bt==BT_CRYSTAL_BEAMER && content < PAIRBARRIER_CONTENT_THRES);
    }
    inline bool isAbsolutelyImmutable() {
        return bt == BT_PAIRBARRIER || bt == BT_CRYSTAL || bt == BT_CRYSTAL_BEAMER;
    }
    inline bool isBlastableGround() {
        return gt != GT_LAVA_COLD && gt != GT_LAVA_HOT && gt != GT_VENT && gt != GT_VENTCORE && gt != GT_FIREGEN
            && gt != GT_CORE && gt != GT_PIT;
    }
    static const int DIGGED_NONE = 0;
    static const int DIGGED_PLANT = 1;
    static const int DIGGED_EARTH = 2;    
    inline int dig() {
        int result = DIGGED_NONE;
        if(gt==GT_SNOW){
            gt = GT_TUNDRA;
            result = DIGGED_PLANT;
        } else if( gt==GT_JUNGLE) {
            gt = GT_SOIL;
            result = DIGGED_PLANT;
        }
        if( st == ST_WEED ) {
            st = ST_NONE;
            gt = GT_ROCK;            
            result = DIGGED_PLANT;
        } else if( st == ST_ROCK ) {
            st = ST_NONE;
            gt = GT_ROCK;
            result = DIGGED_EARTH;
        }
        return result;
    }
    inline bool isExchange() {
        return ( bt == BT_EXCHANGE_ACTIVE || bt == BT_EXCHANGE_INACTIVE );
    }
    inline bool isVent() {
        return (gt == GT_VENT || gt == GT_VENTCORE );
    }
    inline bool isPortal() {
        return( bt == BT_PORTAL_ACTIVE || bt == BT_PORTAL_INACTIVE );
    }
    inline bool isCableConnecting() {
        return (bt == BT_CABLE || isPortal() );
    }
    inline bool isCoreCircuit() {
        return isCoreSwitch() || isCoreCable();
    }
    inline bool isCoreSwitch() {
        return st == ST_CORE_SWITCH_INACTIVE || st == ST_CORE_SWITCH_ACTIVE;
    }
    inline bool isCoreCable() {
        return st == ST_CORE_CABLE_INACTIVE || st == ST_CORE_CABLE_ACTIVE;
    }
    inline bool isCoreCableConnecting() {
        return ( isCoreCable() || bt == BT_CORE_COVER || isCoreSwitch() );
    }
    inline bool isCable() {
        return (bt == BT_CABLE );
    }

    inline bool isEarthBlock() {
        return bt == BT_SOIL || bt == BT_ROCK || bt == BT_IRONORE || bt == BT_RAREMETALORE || bt == BT_HARDROCK || bt == BT_FIREGEN || bt == BT_SNOW;
    }
    inline bool isEarthGround() {
        return gt == GT_GRASS || gt == GT_SOIL || gt == GT_SAND || gt == GT_DEEP || gt == GT_LAVA_COLD ||
            gt == GT_ROCK || gt == GT_SNOW || gt == GT_JUNGLE;
    }
    inline bool isWalkableSurface() {
        return isWater() == false && st != ST_BLOODWATER && st != ST_ROCK;
    }
    inline bool isWalkableGround() {
        return gt != GT_PIT;
    }
    inline bool isWalkable(bool enemy) {
        return (!isWall(enemy)) && isWalkableSurface() && isWalkableGround();
    }
    inline bool isFlyable(bool enemy) {
        if(enemy) {
            if( bt == BT_TREE || bt == BT_IVY || bt == BT_BEEHIVE ) return true;
        }
        return !isWall(enemy);
    }
    inline bool isMossable() {
        return ( gt == GT_SOIL || gt == GT_SAND || gt == GT_GRASS || gt == GT_TUNDRA || gt == GT_JUNGLE ) ;        
    }
    inline bool isPlantable() {
        return isMossable() || gt == GT_SNOW;
    }
    
    inline bool isToRepair() {
        return (bt == BT_ENEMY_EYE_DESTROYED) || ( st == ST_GROUNDWORK ) ;
    }
    inline bool isFlyingEnemyEnterable() {
        return (bt == BT_CELL || bt == BT_SLIMEGEN || bt == BT_FLYGEN || bt == BT_COPTERGEN || bt == BT_FIREGEN || isEnemyBuilding() );
    }
    inline bool isEnterable(bool enemy, bool flying, bool swimming, BLOCKTYPE bt_can_enter ) {
        if( enemy ) {
            if(flying) {
                return isFlyable(enemy) || isFlyingEnemyEnterable() || ( bt_can_enter != BT_AIR && bt == bt_can_enter);
            } else if( swimming ){
                return isWalkable(enemy) || ( bt_can_enter != BT_AIR && bt == bt_can_enter ) || ( isWater() );
            } else { 
                return isWalkable(enemy) || ( bt_can_enter != BT_AIR && bt == bt_can_enter );
            }
        } else {
            return isWalkable(false);
        }
    }
    inline bool isDestroyable() {
        return bt == BT_SOIL || bt == BT_ROCK;
    }

    inline float getWalkSpeedRate() {
        if( st == ST_ICE ) {
            return 1.0f;
        } else { 
            switch(gt) {
            case GT_VENTCORE:
            case GT_VENT:
            case GT_LAVA_HOT:
            case GT_FIREGEN:
                return 0.2f;

            case GT_SNOW:
                return 0.35f;
            
            case GT_SAND:
            case GT_JUNGLE:
                return 0.5f;
            case GT_LAVA_COLD:             
            case GT_SOIL:
            case GT_TUNDRA:
                return 0.75f;
            case GT_BRICK_PANEL: return 1.5f;
            default:
                return 1.0f;
            }
        }
    }
    inline bool isTooHot() {
        return ( gt == GT_LAVA_HOT || gt == GT_VENT || gt == GT_VENTCORE || gt == GT_FIREGEN );
    }
    inline bool isWater() {
        return st == ST_WATER || st == ST_BLOODWATER;
    }
    inline bool tryChangeGrass() {
        if( isWater() == false && isMossable() && gt != GT_GRASS ){
            gt = GT_GRASS;
            return true;
        }
        return false;
    }
    
    inline int getMaxBlockHP() {
        switch(bt) {
        case BT_CELL: return 4;
        case BT_SOIL: return 8;
        case BT_SNOW: return 1;
        case BT_IVY: return 10;
        case BT_BOMBFLOWER: return 9;
        case BT_BEEHIVE: return 20;
        case BT_TREE: return 7;
        case BT_FLYGEN: return 3;
        case BT_SLIMEGEN: return 100;
        case BT_BIRDGEN: return 100;
        case BT_COPTERGEN: return 50;
        case BT_ROCK: return 100;
        case BT_HARDROCK: return 2000;
        case BT_IRONORE: return 8;
        case BT_RAREMETALORE: return 8;
        case BT_DEROTA: return 10;
        case BT_CAGE: return 10;
        case BT_BARRIERGEN: return 30;
        case BT_MONO_EYE: return 50;
        case BT_PANEL_EYE: return 100;
        case BT_ROUND_EYE: return 300;
        case BT_BUILDER_EYE: return 400;
        case BT_COMBI_EYE: return 500;
        case BT_CHASER_EYE: return 200;
        case BT_SHIELD_EYE: return 1500;
        case BT_HYPERGEN: return 4000;
        case BT_FIREGEN: return 300;
        case BT_GRAY_GOO: return 10;
        case BT_WEAK_CRYSTAL: return 100;

        case BT_ENEMY_FRAME:
        case BT_CRYSTAL:
        case BT_CRYSTAL_BEAMER:            
        case BT_PAIRBARRIER:
            return 0; // can't destroy

        case BT_POLE:
        case BT_REACTOR_ACTIVE:            
        case BT_REACTOR_INACTIVE:
        case BT_REACTOR_ARM:                        
        case BT_EXCHANGE_ACTIVE:
        case BT_EXCHANGE_INACTIVE:
        case BT_TURRET_ACTIVE:
        case BT_TURRET_INACTIVE:
        case BT_PORTAL_ACTIVE:
        case BT_PORTAL_INACTIVE:
            return 300;
        case BT_FENCE:
            return 2000;
        default: return 0;
        }
    }
    inline int getMaxGroundHP() {
        switch(gt) {
        case GT_NUCLEUS: return 50;
        case GT_ENHANCER: return 10;
        case GT_CORE: return 10000;
        default:
            return 0;
        }
    }
    // HP of the top thing on a cell
    inline int getMaxTopHP() {
        if( isWall(false) || bt == BT_BUILDER_EYE ) {
            return getMaxBlockHP();
        } else {
            return getMaxGroundHP();
        }
    }

    inline void setCage( BOSSTYPE bst ) {
        bt = BT_CAGE;
        content = (int) bst;
    }


    inline int calcEyeInterval() { return 20 + (randomness%40); }
    // 0:closed 1:half open 2:almost open 3:open

    inline OPENNESS getEyeOpenness() {
        if( ftstate == FTS_IDLE ) {
            return OPENNESS_CLOSED;
        } else if( ftstate == FTS_ACTIVE ) {
            int interval = calcEyeInterval();
            // interval0 = 0
            // closed
            int thres1 = interval*0.1;
            // almost_closed
            int thres2 = interval*0.2;
            // almost_open
            int thres3 = interval*0.3;
            // open
            int thres4 = interval*0.4;
            // open
            int thres5 = interval*0.5;
            // open
            int thres6 = interval*0.6;
            // open
            int thres7 = interval*0.7;
            // almost_open
            int thres8 = interval*0.8;
            // almost_closed
            int thres9 = interval*0.9;
            // closed
            if( content < thres1 ) return OPENNESS_CLOSED;
            if( content >= thres1 && content < thres2 ) return OPENNESS_ALMOST_CLOSED;
            if( content >= thres2 && content < thres3 ) return OPENNESS_ALMOST_OPEN;
            if( content >= thres3 && content < thres4 ) return OPENNESS_OPEN;
            if( content >= thres4 && content < thres5 ) return OPENNESS_OPEN;
            if( content >= thres5 && content < thres6 ) return OPENNESS_OPEN;
            if( content >= thres6 && content < thres7 ) return OPENNESS_OPEN;
            if( content >= thres7 && content < thres8 ) return OPENNESS_ALMOST_OPEN;
            if( content >= thres8 && content < thres9 ) return OPENNESS_ALMOST_CLOSED;
            return OPENNESS_CLOSED;
        } else if( ftstate == FTS_MAD ) {
            return OPENNESS_OPEN;
        } else {
            return OPENNESS_CLOSED;
        }
    }
    bool isToCloseEye() {
        int interval = calcEyeInterval();
        if( content > interval ) {
            return true;
        } else {
            return false;
        }
    }
    inline int diffWayToPC() {
        if( isEnemyEye() || bt == BT_ENEMY_FRAME || bt == BT_ENEMY_EYE_DESTROYED ) {
            return 1;
        } else {
            return isWalkable(true) ? 1 : 100;    
        }
    }
    bool isGUIEnabler() {
        return ( isExchange() || isPortal() || bt == BT_MOTHERSHIP );
    }
    bool isCoreCableActivator() {
        return st == ST_CORE_CABLE_ACTIVE || st == ST_CORE_SWITCH_ACTIVE;
    }

    void tryPutCrystalBeamer( DIR d ) {
        if(isEnemyBuilding()==false) {
            bt = BT_CRYSTAL_BEAMER;
            dir = d;
        }
    }
    void putShifter( DIR d ) {
        gt = GT_SHIFTER;
        dir = d;
    }
    
    inline Color toColor() {
        unsigned int ret=0x0;


        switch(gt) {
        case GT_INVAL: ret = 0xffffff; break;
        case GT_GRASS: ret = 0x2a5634; break;
        case GT_SOIL: ret = 0x4d4110; break;
        case GT_SAND: ret = 0xb86e28; break;
        case GT_NUCLEUS: ret = 0xff0000; break;
        case GT_ENHANCER: ret = 0xff00ff; break;
        case GT_ROCK: ret = 0x323232; break;
        case GT_BRICK_PANEL: ret = 0x6b2918; break;
        case GT_DEEP: ret = 0x172233; break;
        case GT_VENT: ret = 0xff8a00; break;
        case GT_VENTCORE: ret = 0xff8a00; break;
        case GT_LAVA_COLD: ret = 0x383838; break;
        case GT_LAVA_HOT: ret = 0xc33f1d; break;
        case GT_FIREGEN: ret = 0xc33f1d; break;
        case GT_TUNDRA: ret = 0x56502a; break;
        case GT_SNOW: ret = 0xb9d5d5; break;
        case GT_JUNGLE: ret = 0x346e34; break;
        case GT_SHRIMPGEN: ret = 0x262626; break;
        case GT_CORE: ret = 0xff0000; break;
        case GT_PIT: ret = 0x010000; break;
        case GT_SHIFTER: ret = 0x208f8f; break;
        case GT_TELEPORTER_INACTIVE: ret = 0x823322; break;
        case GT_TELEPORTER_ACTIVE: ret = 0xff0000; break;
        case GT_DUNGEON: ret = 0x323322; break;
        }

        switch(st) {
        case ST_NONE: break;
        case ST_WATER:
            if( gt != GT_DEEP ) ret = 0x0000ff;
            break;
        case ST_BLOODWATER: ret = 0x800000; break;
        case ST_WEED: ret = 0x296b29; break;
        case ST_ENERGIUM: ret = 0x009759; break;
        case ST_HYPERLINE: ret = 0xff0000; break;
        case ST_GROUNDWORK: ret =0x010101; break;
        case ST_ROCK: ret = 0x707070; break;
        case ST_ICE: ret = 0x78aafc; break;
        case ST_CORE_CABLE_INACTIVE: ret = 0xff0000; break;
        case ST_CORE_CABLE_ACTIVE: ret = 0xff0000; break;
        case ST_CORE_SWITCH_INACTIVE: ret = 0xff0000; break;
        case ST_CORE_SWITCH_ACTIVE: ret = 0xff0000; break;                        
        }

        
        switch(bt) {
        case BT_INVAL:
        case BT_AIR:
            break;
        case BT_ROCK: ret = 0x919191; break;
        case BT_SOIL: ret = 0x8f551f; break;
        case BT_SNOW: ret = 0xeeffff; break;
        case BT_IVY: ret = 0x296b29; break;
        case BT_BOMBFLOWER: ret = 0xcf2ff5; break;
        case BT_BEEHIVE: ret = 0xc37300; break;
        case BT_CELL: ret = 0xff0000; break;
        case BT_TREE: ret = 0x489848; break;
        case BT_CRYSTAL: ret = 0xeaff00; break;
        case BT_WEAK_CRYSTAL: ret = 0x909090; break;
        case BT_PAIRBARRIER: ret = 0x00ffff; break;
        case BT_MOTHERSHIP:
        case BT_REACTOR_ACTIVE:            
        case BT_REACTOR_INACTIVE:
        case BT_REACTOR_ARM:
        case BT_POLE:
        case BT_EXCHANGE_ACTIVE:            
        case BT_EXCHANGE_INACTIVE:
        case BT_CABLE:
        case BT_PORTAL_ACTIVE:
        case BT_PORTAL_INACTIVE:
        case BT_TURRET_ACTIVE:
        case BT_TURRET_INACTIVE:
        case BT_FENCE:
            ret = 0xffff00; break;
        case BT_IRONORE: ret = 0x2b6f92; break;
        case BT_RAREMETALORE: ret = 0xc9d90f; break;

        case BT_ENEMY_BUILDING_START:  // BT_DEROTA
        case BT_CAGE:
        case BT_ENEMY_FRAME:
        case BT_MONO_EYE:
        case BT_COMBI_EYE:
        case BT_PANEL_EYE:
        case BT_ROUND_EYE:
        case BT_SHIELD_EYE:
        case BT_CHASER_EYE:
        case BT_BUILDER_EYE:
        case BT_ENEMY_EYE_DESTROYED:
        case BT_ENEMY_BUILDING_END:
        case BT_FLYGEN:
        case BT_COPTERGEN:
        case BT_SLIMEGEN:
        case BT_BIRDGEN:
            ret = 0xff0000;
            break;
        case BT_BARRIERGEN:
        case BT_BARRIER:
            ret = 0x00ffff;
            break;
            
        case BT_HYPERGEN:
            ret = 0xff00ff;
            break;
        case BT_HARDROCK: ret = 0x261111; break;
        case BT_FIREGEN: ret = 0xc33f1d; break;
        case BT_PLAYER_BUILDING_END: ret = 0x00ff00; break;
        case BT_CORE_COVER: ret = 0xff0000; break;
        case BT_GRAY_GOO: ret = 0xff00ff; break;
        case BT_CRYSTAL_BEAMER: ret = 0x00ff00; break;
        }
        return Color(ret);
    }
    bool isBrightColor() {
        Color c = toColor();
        return ( c.r > 0.6 && c.g > 0.6 && c.b > 0.6 );
    }
    bool isReactorArmPuttable() {
        return ( bt == BT_AIR && st == ST_ENERGIUM );
    }
};

typedef enum {
    SIMDIR_XPYP = 0,
    SIMDIR_XNYN = 1,
    SIMDIR_XNYP = 2,
    SIMDIR_XPYN = 3,
} SIMDIR;

class Debri;

typedef enum {
    LOCKSTATE_INIT = 0,
    LOCKSTATE_SENT = 1, // Resend if timed out
    LOCKSTATE_GOT = 2,
} LOCKSTATE;

class ToSim {
public:
    LOCKSTATE lock_state; // Simulation is only possible after retrieving a lock from backend
    static const int LOCK_RETRY_INTERVAL_SEC = 2;
    static const int LOCK_KEEP_INTERVAL_SEC = 2;
    double last_lock_sent_at; // Used by keep and lock
    static const int SNAPSHOT_SYNC_INTERVAL_SEC = 10; // interval to send snapshot sync
    double last_snapshot_sync_at; // Send snapshot sync to other game servers with a regular interval
    static const int SAVE_DELAY = 10; // Interval of saving chunks to DB
    double last_save_at; // Save chunk to db
    double last_important_change_at; // Skip saving on minor changes.
    static const int IMAGE_UPDATE_INTERVAL_SEC = 10;
    double last_image_update_at;
    
    int cnt; // Means used if cnt >=1
    int chx, chy;
    void clear() {
        lock_state = LOCKSTATE_INIT;
        last_snapshot_sync_at = 0;
        last_save_at = 0;
        last_lock_sent_at = 0;
        last_important_change_at = 0;
        last_image_update_at = 0;
        cnt = 0;
        chx = chy = -1;
    }
};



class ResourceDepositDump {
public:
    ItemDump items[DEPOSIT_ITEM_NUM];
};

class ResourceDeposit : public ItemOwner {
public:
    ResourceDeposit() : ItemOwner( DEPOSIT_ITEM_NUM, 100000 ) {}
    void dump( ResourceDepositDump *dump );
    void applyDump( ResourceDepositDump *dump );
};




class ChunkStat {
public:
    int simcnt;
    int tundra, iron, raremetal, water;
    ChunkStat() : simcnt(0), tundra(0),iron(0),raremetal(0),water(0) {}
    bool isTundra() {
        return ( tundra > 10 );
    }
};

typedef enum {
    CHUNKLOADSTATE_INIT = 0, // Initialized a chunk.
    CHUNKLOADSTATE_LOADING = 1, // Now loading from storage
    CHUNKLOADSTATE_LOADED = 3, // Loaded, but no lock yet
    CHUNKLOADSTATE_OUT_OF_FIELD = 4, // Don't need to load
} CHUNKLOADSTATE;


class Environment {
public:
    BIOMETYPE bmt;
    Rect rect;
    Environment( BIOMETYPE bmt, Rect r ) : bmt(bmt), rect(r) {}
    Environment() : bmt(BMT_NONE) {}
};


class PowerEquipmentNode;
class Field {
public:
    int width, height;
    Cell *cells;
    int chw, chh;
    Vec2 loc_max;

    ToSim to_sim[1024];
    static const int SAVE_CHUNK_PER_SIM = 2;
    int saved_chunk_this_sim;

    // sim 
    int simchunk_per_poll;
    int sim_counter;
    double last_simclock_at;
    
    int sim_clock_count;

    // stats
    int cur_chunk_change_cnt, last_chunk_change_cnt;

    bool *revealed;
    ChunkStat *chstats;
    CHUNKLOADSTATE *load_state;
    unsigned int load_poll_count;

    int generate_step, generate_counter;
    
    int fortress_id_generator;
    
    FlagCandidate flag_cands[MILESTONE_MAX];

    Environment envs[256];

    Pos2 build_fortress_log[2000];
    int build_fortress_log_num;
    
    //    
    Field( int w, int h );
    size_t getRevealBufferSize();
    void clear();
    void setupFlagCands();
    inline Cell *get( int x, int y) {
        if( x<0 || y<0 || x >= width || y >= height ) return NULL;
        return & cells[ x + y * width ];
    }
    inline Cell *get( Pos2 p ) {
        return get( p.x, p.y );
    }
    inline void getCellXY(Cell *c, int *x, int *y ) { 
        int d_addr = ((char*)c) - ((char*) &cells[0] );
        int ind = d_addr / sizeof(Cell);
        assertmsg( ind >= 0 && ind < width*height, "getCellXY: the cell is not in this field?" );
        int mod = d_addr % sizeof(Cell);
        assert(mod==0);
        *x = ind % width;
        *y = ind / width;
    }
    inline Pos2 getCellPos(Cell *c) {
        int x,y;
        getCellXY(c,&x,&y);
        return Pos2(x,y);
    }
    inline Cell *get( Vec2 v ) {
        return get( (int)v.x/PPC, (int)v.y/PPC);
    }
    inline Cell *getDir( Cell *c, DIR d ) {
        int x,y;
        getCellXY( c, &x, &y );
        int dx,dy;
        dirToDXDY( d, &dx, &dy );
        return get( Pos2( x+dx,y+dy) );
    }
    inline Cell *getDia( Vec2 center, float dia ) {
        Vec2 rel = Vec2(0,0).randomize( dia * 1.414 ).normalize( range(0,dia) );
        return get(center+rel);
    }

    int countCableConnection( Cell *c, bool include_exchange );


    // Generation takes more than 10 seconds, so make it asynchronous.
    void startGenerate();
    bool asyncGenerate();
    void generateSync();
    bool isGenerateStarted();
    
    void statistics();
    void generateBiomeGround( Rect rect, BIOMETYPE bmt );
    void generateBiomeBlocks( Rect rect, BIOMETYPE bmt, float distance_to_rp );
    void generateCommon();
    void blurBiomeGround( Rect rect, DIR d );
    void blurBiomeBlock( Rect rect, DIR d );    
    void sim( bool paused );
    void simStepChunk( bool paused );
    inline void simCell(int x, int y, bool *chg, bool *uchg, bool *dchg, bool *rchg, bool *lchg );
    bool simChunk(int chx, int chy);
    Pos2 getRespawnPoint();
    Pos2 getMotherShipPoint();
    Pos2 getFirstFortressPoint();

    void notifyChanged( int x, int y, bool send_far = false );
    void notifyChunkChanged( int chx, int chy );
    void notifyChunkChanged( int chx, int chy, int w, int h );    
    inline void notifyChanged( Cell *c, bool send_far = false ) {
        int cx,cy;
        getCellXY(c,&cx,&cy);
        notifyChanged(cx,cy,send_far);
    }
    inline void notifyChanged5( Cell *c, bool send_far = false ) {
        Pos2 at = getCellPos(c);
        notifyChanged(at.x,at.y, send_far );
        notifyChanged(at.x+1,at.y, send_far );
        notifyChanged(at.x,at.y+1, send_far );
        notifyChanged(at.x-1,at.y, send_far );
        notifyChanged(at.x,at.y-1, send_far );        
    }
    void notifyChanged( Pos2 at ) { notifyChanged(at.x,at.y); }
    void notifyChangedAround( Pos2 at, int r );     // no sync
    void notifyChunkToSave( int chx, int chy );
    void notifyChunkToSave( Cell *c ) {
        Pos2 p = getCellPos(c);
        notifyChunkToSave( p.x/CHUNKSZ, p.y/CHUNKSZ );
    }
    void checkSaveAllUnsaved( double wait );
    
    //
    static const int WALKABLE_BIT_HIT_RT = 8;
    static const int WALKABLE_BIT_HIT_LT = 4;
    static const int WALKABLE_BIT_HIT_RB = 2;
    static const int WALKABLE_BIT_HIT_LB = 1;
    
    int getEnterableBits( Vec2 at, float sz, bool enemy, bool flying, bool swimming, BLOCKTYPE bt_can_enter);
    bool damage( Vec2 at, int dmg, int *consumed, Char *by ); 
    bool checkGatherDebri( Debri *d );

    bool setToSim( int chx, int chy, int cnt );
    ToSim *getToSim( int chx, int chy );
    inline bool setToSim( Vec2 at, int cnt ) { return setToSim( at.x / PPC / CHUNKSZ, at.y / PPC / CHUNKSZ, cnt ); }
    int countToSim( int *lock_num );
    bool setLockGot( int chx, int chy );

    void burn( Vec2 at, Char *by );

    int scatterS( Pos2 center, int dia, SURFACETYPE st, int n, bool put_on_water, float velocity_per_put );
    int scatterGSelectG( Pos2 center, int dia, GROUNDTYPE gt_toput, GROUNDTYPE gt_toselect, int n )    ;
    int scatterBSelectGatAir( Pos2 center, int dia, GROUNDTYPE gt_toselect, BLOCKTYPE bt_toput, int n );
    int scatterGSelectS( Pos2 center, int dia, SURFACETYPE st_toselect, GROUNDTYPE gt_toput, int n );
    int scatterBSelectSatAir( Pos2 center, int dia, SURFACETYPE st_toselect, BLOCKTYPE bt_toput, int n );

    void findPoleNeighbours( Pos2 from, Pos2 *out, int *outnum );
    void setPowerGrid(Pos2 at, int id);
    void clearPowerGrid( Pos2 at, int to_clean_id );
    int setBlockCircleOn( Pos2 center, float dia, BLOCKTYPE on, BLOCKTYPE toput, float fill_rate );
    Image *dumpNewImage( bool use_reveal = false );
    Vec2 findSafeLoc(Vec2 center);
    bool searchPlayerBuilding( Pos2 center, int distance_cell, Pos2 *outp );
    bool findEnemyAttackTarget( Vec2 center, Vec2 *tgt, float distance, float building_target = 15 );
    bool isValidPos( Pos2 p );
    void get8(Pos2 center, Cell *out[8] );
    void get4(Pos2 center, Cell *out[4] );
    void getCorner4( Vec2 center, float sz, Cell **lb, Cell **rb, Cell **lt, Cell **rt );
    void getRectCorner(Vec2 center, float sz, Cell *out[4]);
    void checkPutBarrier( Pos2 from, DIR dir, int maxlen, int contvol );
    void buildLongFence( Pos2 from );
    int checkBarrierGen( Pos2 from, DIR dir, int maxlen );
    bool checkReactorPuttable(Pos2 at);
    void buildRandomGrounder( Pos2 at );
    void buildRiver( Pos2 from, int nloop, SURFACETYPE st );
    void fillRectSG( Rect r, SURFACETYPE st, GROUNDTYPE gt );
    void fillRectG( Rect r, GROUNDTYPE gt, bool to_notify = false );
    void fillRectB( Rect r, BLOCKTYPE bt );
    void fillRectFortressId( Rect r, int fid );
    void fillRectCage( Rect r, BOSSTYPE bst );
    void fillCircleGSB( Pos2 center, float dia, GROUNDTYPE gt, SURFACETYPE st, BLOCKTYPE bt );
    bool isClearPlace( Rect r );    
    void buildVent( Pos2 center );
    void getConnectedGround( Pos2 center, PosSet *outset, GROUNDTYPE gt );
    int paintEdgeGB( Rect r, GROUNDTYPE gt, BLOCKTYPE bt );
    int paintEdgeBB( Rect r, BLOCKTYPE bt_from, BLOCKTYPE bt_to, GROUNDTYPE gt_avoid );
    void paintContentInsideG( Pos2 from, GROUNDTYPE gt, int content );
    void explodeVent( Pos2 center ) ;
    void scatterLava( Pos2 center );
    void putFiregen( Pos2 at, DIR d );
    void buildFiregenCrack( Pos2 from );
    void meltSnow( Vec2 at );
    bool tryPutBlockNearestOnAir(Vec2 lc, BLOCKTYPE bt );
    int countBlock( Vec2 center, float dia, BLOCKTYPE bt );
    void tryPutIvy( Pos2 at );
    void destroyConnectedIvy(Cell *c);
    void scatterIvy( Pos2 center, int dia, int n );
    void destroyCore( Cell *c );
    void clearEnemyFrame( Pos2 center, int dia );
    void fillPitCollidor( Pos2 from, DIR dir );
    void edgeRectB( Rect r, BLOCKTYPE bt );
    void orthoLineGSB( Pos2 from, Pos2 to, int w, GROUNDTYPE gt, SURFACETYPE st, BLOCKTYPE bt, bool overwrite_enemy_building, bool convert_crystal );
    void edgeRectCrystalBeamer( Rect r, int mod, int mgn, bool reverse );
    void edgeRectShifter( Rect r, int mgn, bool reverse );
    void buildMegaResourceSite( Pos2 p, BLOCKTYPE bt, int dia, float rock_rate );
    int scatterBonEarthGround( Pos2 center, int dia, BLOCKTYPE bt_toput, int n );
    int countReactorArm( Pos2 p );
    bool findNearestBlock( Pos2 from, BLOCKTYPE bt, int dia, Pos2 *out );
    PowerEquipmentNode *findNearestExchange( Pos2 from );
    void setEnvironment( Rect r, BIOMETYPE bmt );
    Environment *getEnvironment( Pos2 at );
    bool isFlagPuttableCell( Pos2 p );
    
    // Revealing
    void setChunkRevealed( int chx, int chy, bool flg );
    bool getChunkRevealed( int chx, int chy );
    void revealRange( int chx, int chy, int mgn );
    
    // Fortress
    static const int FORTRESS_MAX_DIA = 20;
    bool buildFortress( bool put_ft_id, Pos2 lbpos, const char **buf, int linenum, Pos2 *center );
    bool buildFortressByLevel( Pos2 center, float minlv, float maxlv );
    bool buildFortressCoreDefender( Pos2 center, int hint );
    void buildGrounderByLevel( Pos2 center, float minlv, float maxlv );    
    void destroyFortressEye( Cell *c );
    int countFortressEyes( Pos2 center, int dia, int ft_id, int *initial_eyes );
    int wipeFortress( Pos2 center, int dia, bool mad );
    int hitFortress( Pos2 lbpos, int w, int h );
    void appendBuildFortressLog(Pos2 at);
    
    // deposit/exchange
    void setExchange( Cell *c );
    void setPortal( Cell *c );
    

    // Flags
    FlagCandidate *findNearestFlagCand( Pos2 from );
    bool clearFlag( Pos2 p );
    bool debugClearFlag();

    // Boss
    void buildBossContainer( Pos2 lb, int size, BOSSTYPE bst );
    int generateBoss( Cell *cell_destroyed );

    // Dungeon
    void buildDungeon( Pos2 center, int sz );
    Rect buildEntranceRoom( Rect rect, DIR dir, MazeNode *node );
    Rect buildRandomRoom( Rect rect, MazeNode *node );
    Rect buildCorridorRoom( Rect rect );
    Rect buildFortressRoom( Rect rect );
    Rect buildCoreRoom( Rect rect );        
    void buildCorridorGS( Pos2 from, Pos2 to, int w, DIR dir, GROUNDTYPE gt, SURFACETYPE st, bool overwrite_enemy_building = false );
    void putCoveredCore( Pos2 lb );
    void orthoOutlineBSelectG( Pos2 from, Pos2 to, int w, GROUNDTYPE gt_toselect, BLOCKTYPE bt_toput );
    void fillShifter( Rect rect );
    void buildRoomGate( Pos2 at, DIR todir );
    
    // Stats
    ChunkStat *getChunkStat( Pos2 p ) { return getChunkStat( p.x/CHUNKSZ, p.y/CHUNKSZ); }
    ChunkStat *getChunkStat(int chx, int chy );
    void updateChunkStat( int chx, int chy );
    void dumpChunkStat();

    // Networking
    void pollNetworkCache();
    int calcChunkLoadState(Pos2 at);
    CHUNKLOADSTATE getChunkLoadState( Pos2 at );
    bool isChunkLoaded( Vec2 at );
    void setChunkLoadState( Pos2 lb, CHUNKLOADSTATE state );
    void applyFlagCands( ProjectInfo *pinfo );
    void copyFlagCands( ProjectInfo *pinfo );
    void saveAt( Cell *c, int dia =1);
    void saveAt( Pos2 p, int dia=1);
};



inline Pos2 vec2ToPos2( Vec2 v ) {
    return Pos2( v.x / PPC, v.y / PPC );
}

inline Vec2 pos2ToVec2( Pos2 p ) {
    return Vec2( p.x * PPC, p.y * PPC );
}
inline Vec2 toCellCenter( Vec2 at ) {
    return Vec2( (int)(at.x / PPC) * PPC + PPC/2,
                 (int)(at.y / PPC) * PPC + PPC/2
                 );
}
inline Vec2 toCellCenter( Pos2 p ) {
    return toCellCenter( Vec2( p.x*PPC, p.y*PPC) );
}
inline Vec2 toCellCenter( int x, int y ) {
    return toCellCenter( Vec2(x*PPC,y*PPC) );
}

inline Vec2 randomDirToVec2() {
    int dx,dy;
    dirToDXDY( randomDir(), &dx, &dy );
    return Vec2(dx,dy);
}
inline Vec2 dirToVec2( DIR d ) {
    int dx,dy;
    dirToDXDY(d, &dx, &dy );
    return Vec2(dx,dy);
}
inline Pos2 dirToPos2( DIR d ) {
    int dx,dy;
    dirToDXDY(d, &dx, &dy );
    return Pos2(dx,dy);
}
inline DIR vec2ToDir( Vec2 v ) {
    if( v.x == 0 && v.y == 0 ) return DIR_NONE;
    float r = atan2( v.y, v.x ); // 0~M_PI, 0~-M_PI
    if( r >= M_PI*-0.25 && r <= M_PI*0.25 ) return DIR_RIGHT;
    if( r >= M_PI*0.25 && r <= M_PI*0.75 ) return DIR_UP;
    if( r >= M_PI*0.75 || r <= M_PI*-0.75 ) return DIR_LEFT;
    if( r <= M_PI*0.25 && r >= M_PI*-0.75 ) return DIR_DOWN;
    return DIR_NONE;
}
inline BOSSTYPE getRandomSingleCagedBoss() {
    BOSSTYPE bosses[] = { BST_TAKWASHI, BST_REPAIRER, BST_BUILDER, BST_CHASER, BST_VOIDER, BST_DEFENDER };
    return choose(bosses);
}

int BlockToScore( BLOCKTYPE bt );
int GroundToScore( GROUNDTYPE gt );

#endif
