#ifndef _ENEMY_H_
#define _ENEMY_H_

class PC;

typedef enum {
    EXT_NONE = 0, // Don't explode
    EXT_CREATURE = 1,
    EXT_MACHINE = 2,
    EXT_BOSS = 3,
} EXPLOSIONTYPE;

#define FLYING true
#define SWIMMING true
#define NOT_FLYING false
#define NOT_SWIMMING false

enum {
    INTVL_IND_HYPER = 0, // Common in every enemies
    INTVL_IND_AUTOSYNC = 1, // Common in every enemies auto sync (network)
    // Below are used in each Enemy, so the same index is reused.
    INTVL_IND_BUILDER_CHECKBUILD = 2, 
    INTVL_IND_FIREGENERATOR_SHOOT = 2, 
    INTVL_IND_BIRD_SOUND = 2, 
    INTVL_IND_DEFENDER_SHOOT = 2,
    INTVL_IND_CORE_SHOOT = 2,
};

class Enemy : public Char {
public:
    Vec2 v;    
    int hp,maxhp; 
    float fly_height;
    bool flying;
    bool swimming;
    Prop2D *shadow_prop;
    double timeout;
    bool beam_hits;
    bool machine;
    ITEMTYPE reward_itt;
    EXPLOSIONTYPE explosion_type;
    BLOCKTYPE bt_can_enter;
    float body_size;
    CATEGORY hit_category;
    bool push_pc;
    double hyper_until;
    bool enable_hyper_effect;
    int fly_on_kill_num;
    int egg_on_kill_num;
    bool enable_durbar;
    bool enable_move;
    Vec2 target;
    ENEMYTYPE enemy_type;
    float net_auto_sync_move_interval_sec;
    int base_score;
    
    Enemy( Vec2 lc, TileDeck *dk, bool flying, bool swimming, float fly_height, int client_id=0, int internal_id = 0);
    ~Enemy();
    virtual bool charPoll( double dt );
    virtual bool enemyPoll( double dt ) { return true; }

    virtual void notifyHitBeam( Beam *b, int dmg );
    virtual void onBeam( Beam *b, int dmg ) {}
    virtual void onPushPC( PC *pc ) {}
    virtual void onKill() {}
    virtual void onTimeout() {}
    virtual void recvRemoteKnockback() {}
    
    bool applyDamage( int dmg );
    
    inline bool hitWithFlyHeight( Prop2D *p, float sz ) {
        return ( loc.x + sz > p->loc.x - sz ) && ( loc.y + fly_height + sz > p->loc.y - sz ) &&
            ( loc.x - sz < p->loc.x + sz ) && ( loc.y + fly_height - sz < p->loc.y + sz );        
    }

    static Enemy *getNearestShootable( Vec2 from );
    void ensureShadow();

    void checkLocalSyncMoveSend();
};



#define KNOCKBACK_DELAY 0.2

class Worm : public Enemy {
public:
    bool shooter;
    double shoot_at;    
    double last_hit_at;
    double rest_until;
    double turn_at;
    
    Worm( Vec2 lc, int client_id=0, int internal_id=0 );
    virtual bool enemyPoll( double dt );
    virtual void onBeam( Beam *b, int dmg );
};
class Egg : public Enemy {
public:
    double hatch_at;
    Egg(Vec2 lc, int client_id=0, int internal_id=0);
    virtual bool enemyPoll(double dt);
};

class Fly : public Enemy {
public:
    double turn_at;
    Fly( Vec2 lc, int client_id=0, int internal_id=0 );
    virtual bool enemyPoll(double dt);
};

typedef enum {
    BLT_INVAL=0,
    BLT_POISONBALL = 1,
    BLT_SPARIO,
    BLT_SIG,
    BLT_BELO,
    BLT_FIREBALL,
    BLT_FIRE,
    BLT_ROTFIRE,
    BLT_SNOWBALL,
    BLT_TORNADO,
    BLT_SHRIMP_DEBRI,
    BLT_BOUNCER,
    BLT_CRYSTAL_BEAM,
} BULLETTYPE;



class Bullet : public Enemy {
public:
    int hit_pc_damage;
    int hit_wall_damage;
    BULLETTYPE bullet_type;
    float round_vel;
    float round_dia;    
    float hit_beam_size;
    float friction;
    float rot_speed;
    
    virtual bool enemyPoll( double dt );

    Bullet( BULLETTYPE blt, Vec2 at, Vec2 to, int client_id=0, int internal_id=0 );
    ~Bullet();
    static void shootAt( BULLETTYPE blt, Vec2 from, Vec2 at );
    static void shootFanAt( BULLETTYPE blt, Vec2 from, Vec2 at, int side_n, float pitch );
    virtual void onTouchWall( Vec2 nextloc, int hitbits, bool nxok, bool nyok );
    Beam *checkHitBeam();
};


class Takwashi : public Enemy {
public:
    // Stop for some time and shoot fast. By this it makes a weakpoint.
    double shoot_start, shoot_end, last_shoot_at;
    Vec2 shoot_tgt;
    bool to_shoot;
    Takwashi( Vec2 lc, int client_id=0, int internal_id=0 );
    virtual bool enemyPoll(double dt);
};


class Repairer : public Enemy {
public:
    double turn_at;
    double spark_at;
    Repairer( Vec2 lc, int client_id=0, int internal_id=0 );
    virtual bool enemyPoll(double dt);
};

class Chaser : public Enemy {
public:
    double reset_target_at;
    double shoot_at;
    float thres;
    double accel_at;
    Chaser( Vec2 lc, int client_id=0, int internal_id=0 );
    virtual bool enemyPoll(double dt);
};

class Builder : public Enemy {
public:
    double turn_at;
    bool building;
    DIR build_dir;
    int build_cnt;
    double build_after_at;
    Builder( Vec2 lc, int client_id=0, int internal_id=0 );
    virtual bool enemyPoll(double dt);
    virtual void onTouchWall( Vec2 nextloc, int hitbits, bool nxok, bool nyok );    
};


class Girev : public Enemy {
public:
    double shoot_at;
    double turn_at;
    double repairer_at;
    Girev( Vec2 lc, int client_id=0, int internal_id=0 );
    virtual bool enemyPoll(double dt);
    
    
};

class HyperPutter : public Enemy {
public:
    Pos2 goalpos;
    DIR curdir;
    HyperPutter( Vec2 lc, int client_id=0, int internal_id=0 );
    virtual bool enemyPoll(double dt);
};

class Voider : public Enemy {
public:
    double erase_at;
    Voider( Vec2 lc, int client_id=0, int internal_id=0 );
    virtual bool enemyPoll(double dt);
};

class VolcanicBomb : public Flyer {
public:
    VolcanicBomb( Vec2 lc, int client_id=0, int internal_id=0 );
    virtual bool flyerPoll( double dt );
    virtual void land();
};

class FireGenerator : public Enemy {
public:
    double shoot_at;
    FireGenerator( Vec2 lc, int client_id=0, int internal_id=0 );
    virtual bool enemyPoll( double dt );
    void shoot();
};


class FireCopter : public Enemy {
public:
    float vel;
    double gen_child_at;
    double turn_at;
    static const int CHILD_NUM = 8;
    int childids[CHILD_NUM];
    FireCopter( Vec2 lc, int client_id=0, int internal_id=0 );
    ~FireCopter();
    virtual bool enemyPoll( double dt );
    virtual void onKill();
};
class SnowSlime : public Enemy {
public:
    float vel;
    double turn_at;
    float fly_vy;
    double shoot_at;
    SnowSlime( Vec2 lc, int client_id=0, int internal_id=0 );
    virtual bool enemyPoll( double dt );
};

class Bird : public Enemy {
public:
    double turn_at;
    Bird( Vec2 lc, int client_id=0, int internal_id=0 );
    virtual bool enemyPoll(double dt);
};

class Shrimp : public Enemy {
public:
    double turn_at;
    double start_at; // Rest in a water at the beginning.
    Shrimp( Vec2 lc, int client_id=0, int internal_id=0 );
    virtual bool enemyPoll( double dt );
    virtual void onKill();
    virtual void onTimeout();
    void blast( bool is_local );
};

class BombFlowerSeed : public Flyer {
public:
    BombFlowerSeed( Vec2 lc, int client_id=0, int internal_id=0 );
    virtual bool flyerPoll( double dt );
    void land();
};

class Bee : public Enemy {
public:
    double turn_at;
    Bee( Vec2 lc, int client_id=0, int internal_id=0 );
    virtual bool enemyPoll( double dt );
};

class Defender : public Enemy {
public:
    double turn_at;    
    Defender(Vec2 lc, int client_id=0, int internal_id=0 );
    virtual bool enemyPoll( double dt );
};

class Teleporter : public Enemy {
public:
    double start_at; // Invisible at the first creation.
    double fade_at; // Start point of fading out.
    bool shot;
    Teleporter( Vec2 lc, int client_id=0, int internal_id=0 );
    virtual bool enemyPoll(double dt);
    virtual void onTouchWall( Vec2 nextloc, int hitbits, bool nxok, bool nyok );    
};

class CoreBall : public Enemy {
public:
    double shoot_start_at;
    double turn_at;
    CoreBall( Vec2 lc, int client_id=0, int internal_id=0 );
    virtual bool enemyPoll( double dt );
};



#endif
