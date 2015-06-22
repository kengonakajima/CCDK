#ifndef _PC_H_
#define _PC_H_

class Debri;
class Enemy;

class PCDump {
public:
    int shoot_sound_index;
    int hair_base_index;
    int face_base_index;
    int body_base_index;
    int beam_base_index;
    int total_milestone_cleared;
    ItemDump items[PC_ITEM_NUM];
    int hp, ene;
    char nickname[PC_NICKNAME_MAXLEN+1];
    long long score;
    int recovery_count_left;
    unsigned int last_recovery_at_sec;
    PCDump() {}
};

enum {
    INTVL_IND_PC_FOOTSMOKE = 0,
    INTVL_IND_PC_ENEHEAT = 1,
    INTVL_IND_PC_SHOW_NAME = 2,
    INTVL_IND_PC_TOOHOT = 3,
    INTVL_IND_PC_ALARM_LOWHP = 4,
    INTVL_IND_PC_CHARGE_PWGRID = 5,
    INTVL_IND_PC_WARP_SOUND = 6,
};

class PC : public Char, public ItemOwner {
public:
    Vec2 ideal_v; // 1 per PPC
    DIR dir;
    float body_size;
    Vec2 shoot_v;
    int selected_item_index; 

    int hp, maxhp;
    int ene, maxene; // calculated from item
    int armor, maxarmor; // calculated from item

    double last_charge_at;

    bool enable_ai;

    bool warping;
    DIR warp_dir;

    double knockback_until;
    Vec2 knockback_v;

    Vec2 history_loc[64];

    bool enable_hpbar, enable_enebar;

    // no main sprite.
    Prop2D *equip_prop; // Draw order depends on move direction.
    Prop2D *body_prop; // 
    Prop2D *face_prop; //
    Prop2D *hair_prop; //

    double died_at;

    // stats
    int total_milestone_cleared;
    
    // Customize look and sounds 
    int shoot_sound_index;
    int hair_base_index, face_base_index, body_base_index, beam_base_index;
    char nickname[PC_NICKNAME_MAXLEN+1];
    double last_network_update_at; // Clean if no packets from network for threshold time

    static const int ENE_CHAIN_FOLLOW_THRES = 20;
    int energy_chain_heat_count; // To show up follow window, you have to keep shooting.

    int user_id; // initialized by PARTY protocol
    long long score;
    int recovery_count_left;
    unsigned int last_recovery_at_sec; // time_t.  Increment this count after 24 hours from the last recovery

    CharGridTextBox *nktb;
    double last_insert_history_at;
    
    PC( Vec2 lc );
    virtual bool charPoll( double dt );


    void refreshStatus();
    bool addItemByDebri( Debri *d );
    virtual void onItemUpdated( int index ) {print("pc:onitemupdated:%d",index); }; // -1:reduced number. Update GUI only on LocalChar.
    virtual void onStatusUpdated() {}; 
    virtual void onTouchWall( Vec2 nextloc, int hitbits, bool nxok, bool nyok ) {};
    virtual void onPushed( int dmg, Enemy *e ) {};
    virtual void onTooHot() {}
    virtual bool pcPoll( double dt ) { return true; }
    //    bool tryAction( Vec2 direction );
    void moveSelection( DIR dir );
    bool isPuttableSelected();
    bool isDiggableSelected();
    bool isItemSelected( ITEMTYPE itt );
    
    void onAttacked(int dmg, Enemy *e );

    int charge( int e );

    bool hasEnough( ITEMTYPE itt, int num );
    bool isCraftable( ItemConf *prodconf, int *possible_num );

    bool craft( ItemConf *prodconf);

    void calcEnhancement( int *battery_capacity, int *armor, int *maxarmor );

    void startWarp( Pos2 from, DIR to_dir );

    static PC *getNearestPC( Vec2 from );
    void recoverHP( int amount );

    void wearSuitItem( ITEMTYPE itt, int amount );
    void wearBattery();

    void clearHistory( Vec2 at );
    void insertHistory( Vec2 lc );
    Vec2 getFromHistory();
    bool findAcceleratorInSuit();
    float getShootIntervalSec();
    void respawn();
    void suicide();
    void abandonItem( int ind );
    
    static Vec2 calcEquipPosition(DIR d);

    void dump( PCDump *out );
    void applyPCDump( PCDump *in );

    void setNickname( const char *s );
    void addInitialItems();    
    virtual void onDelete();

    bool isSwappable( int i0, int i1 );
    int countBeamguns();
};

class LocalPC : public PC {
public:
    Vec2 last_touch_wall_at;
    bool can_put_block_as_wall;
    bool can_put_block_to_remove_water;
    Vec2 last_put_debri_at;
    LocalPC( Vec2 lc );
    bool tryAction( Vec2 direction );

    CharGridTextBox *leaving_tb;
    virtual void onItemUpdated( int index );
    virtual void onStatusUpdated();
    virtual void onTouchWall( Vec2 nextloc, int hitbits, bool nxok, bool nyok ) ;
    virtual void onPushed( int dmg, Enemy *e );
    virtual void onTooHot();
    virtual bool pcPoll( double dt );

    void resetPutBlockFlags() { can_put_block_as_wall = can_put_block_to_remove_water = true; last_put_debri_at = Vec2(0,0); }
    void modScore( int score );
};

#endif
