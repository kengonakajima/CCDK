#ifndef _PWGRID_H_
#define _PWGRID_H_


class PowerEquipmentDump {
public:
    int type;
    int powergrid_id;
    Pos2 at;
    int ene_unit;
};

class PowerNodeDump {
public:
    int id;
    int powergrid_id;
    Pos2 at;
    Pos2 dests[2];
};
class PowerGridDump {
public:
    int id;
    int ene;
};

class PowerSystemDump {
public:
    int nodes_used;
    PowerNodeDump nodes[4096];
    int grids_used;
    PowerGridDump grids[1024];
    int equipments_used;
    PowerEquipmentDump equipments[4096];
};

class PowerGrid;
class PowerNode {
public:
    int id;
    PowerGrid *parent;
    Pos2 at;
    Pos2 dests[2];
    PowerNode *next;
    PowerNode *prev;
    PowerNode(Pos2 at);
    void addDest( Pos2 to );
    int countDest();
};

typedef enum {
    PEQT_INVAL = 0,
    PEQT_REACTOR = 1,
    PEQT_MOTHERSHIP = 2,
    PEQT_EXCHANGE = 3,
} POWEREQUIPMENTTYPE;

class PowerEquipmentNode {
public:
    POWEREQUIPMENTTYPE type;
    Pos2 at;    
    int ene_unit;
    PowerEquipmentNode *next;
    PowerEquipmentNode *prev;
    PowerEquipmentNode( POWEREQUIPMENTTYPE type, Pos2 at, int ene_unit ) : type(type), at(at), ene_unit(ene_unit), next(NULL), prev(NULL) {}    
};

class PowerGrid {
public:
    int id;
    int ene;
    double accum_time, decay_at;
    double last_usage_at; // accum_time. Last time of using energy.
    double last_gen_at; // accum_time. Last time of generating energy.
    static const int MAXENE = 20000;
    static const int ENE_PER_ARM = 2;
    static const int MOTHERSHIP_ENE_PER_POLL = 10;
    static const int EXCHANGE_ENE_MAX = 50000;        
    PowerNode *node_top;
    PowerGrid *next;
    PowerEquipmentNode *equip_top;
    PowerGrid( int id );
    ~PowerGrid();
    PowerNode *findNode( Pos2 at );
    PowerNode *addNode( Pos2 at );
    PowerNode *findNodeById( int id );
    PowerEquipmentNode *findPowerEquipmentNode(Pos2 at);
    void use(int amount) { ene -= amount; last_usage_at = accum_time; }
    int countNodes();
    int countPowerEquipments();
    void dumpNodes();    
    void repaint();
    Color getColor();
    void poll( double dt );
    void clearDestination( Pos2 dest, Pos2 *cleared, int *cleared_n );
    bool ensureEquipment( POWEREQUIPMENTTYPE type, Pos2 at, int ene_unit);
    PowerEquipmentNode *findEquipment( Pos2 at );
    int getMaxNodeId();
    void modEne( int diff, bool local );
};

class PowerSystem {
public:
    PowerGrid *grid_top;
    PowerSystem();
    PowerGrid *findGrid( Pos2 at );
    PowerGrid *findGridById( int id );
    PowerGrid *addGrid();
    int addNode( Pos2 p );
    PowerNode *findNode( Pos2 p );
    PowerEquipmentNode *findEquipment( Pos2 p );
    bool ensureEquipment( Pos2 p );

    void mergeGrid( PowerGrid *remain, PowerGrid *remove );
    void deleteGrid( PowerGrid *g );
    void dumpGrids();
    void poll( double dt );
    int getMaxGridId();

    void clear();
    
    // for saving
    bool dumpAll( PowerSystemDump *dump );
    void applyDump( PowerSystemDump *dump );
    
};


#endif
