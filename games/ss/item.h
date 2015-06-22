#ifndef _ITEM_H_
#define _ITEM_H_

#define ITEM_INGREDIENT_NUM 4
#define RESEARCH_INGREDIENT_NUM 4 

typedef enum {
    ITT_EMPTY = -1,
    ITT_ERROR = 0,

    ITT_BEAMGUN1 = 1000,
    ITT_BEAMGUN2 = 1001,
    ITT_BEAMGUN3 = 1002,
    ITT_BEAMGUN4 = 1003,            
    
    ITT_BLASTER = 1100,
    ITT_HEAL_BLASTER = 1110,

    ITT_DEBRI_SOIL = 2000,
    ITT_DEBRI_ROCK = 2001,
    ITT_DEBRI_IRONORE = 2010,
    ITT_DEBRI_RAREMETALORE = 2020,
    ITT_DEBRI_HARDROCK = 2030,

    // glossaries
    ITT_APPLE = 3000,
    ITT_ENERGY_PARTICE = 3005,
    ITT_MICROBE = 3010,
    ITT_WEED_SEED = 3020,
    ITT_TREE_SEED = 3030,
    ITT_ARTIFACT = 3100,
    ITT_HYPER_PARTICLE = 3110,
    ITT_DARK_MATTER_PARTICLE = 3120,        
    ITT_IRON_PLATE = 3200,
    ITT_RAREMETAL_CRYSTAL = 3210,
    ITT_BATTERY1 = 3300,
    ITT_BATTERY2 = 3301,    
    ITT_SHIELD = 3400,
    ITT_ACCELERATOR = 3500,
    ITT_SHOVEL = 3600,
    ITT_HP_POTION = 3700,
    
    
    // put to use
    ITT_BRICK_PANEL = 4000,
    ITT_TURRET = 4040,

    ITT_REACTOR = 4100,
    ITT_POLE = 4110,
    ITT_EXCHANGE = 4120,
    ITT_CABLE = 4121,
    ITT_PORTAL = 4122,
    ITT_FENCE = 4130,



} ITEMTYPE;

class Ingredient {
public:
    ITEMTYPE itt;
    int n;
    Ingredient() : itt(ITT_EMPTY), n(0) {}
};

class ItemConf {
public:
    ITEMTYPE itt;
    char name[32];
    TileDeck *deck;
    int index;
    int stack_max;

    bool puttable;
    Ingredient ingredients[ITEM_INGREDIENT_NUM];
    Ingredient research_ingredients[ITEM_INGREDIENT_NUM];
    int required_ene;
    int dur;
    int level;
    int is_beamgun;
    int hp_recover_amount;
    bool is_passive_mod;
    ItemConf( ITEMTYPE itt, int lv, const char *nm, TileDeck *deck, int index, int stack_max, bool puttable ) : itt(itt), deck(deck),index(index), stack_max(stack_max), puttable(puttable), required_ene(0), dur(0), level(lv), is_beamgun(false), hp_recover_amount(0), is_passive_mod(false) {
        snprintf(name,sizeof(name),"%s", nm );
    }
    ItemConf() : itt(ITT_ERROR){}
    void addIngredient( ITEMTYPE itt, int n );
    void addResearchIngredient( ITEMTYPE itt, int n );

    static ItemConf *getItemConf( ITEMTYPE itt );
    static ItemConf *getByIndex( int index );
    static ItemConf *getCraftable( int ind );
    static int countCraftable();
};

class ResearchIngredient : public Ingredient {
public:
    int current;
    ResearchIngredient() : Ingredient(), current(0) {}
};


class ResearchState {
public:
    ITEMTYPE itt;
    ResearchIngredient ingrs[4];
    bool locked;
    ResearchState() : itt(ITT_EMPTY), locked(true) {}
    bool isComplete();
    static void unlock( ITEMTYPE itt );
    static ResearchState *getResearchState( ITEMTYPE itt );
    static bool itemIsLocked( ITEMTYPE itt );
    static void unlockAll();
};

#define ITEM_CONF_NUM 128
extern ResearchState g_research_state[ITEM_CONF_NUM];
void resetResearchStateLocked();



int getBeamgunEne( ItemConf *itc );



class ItemDump {
public:
    int itt;
    int num;
    int dur;
};

void setupItemConf();

class Item {
public:
    ITEMTYPE itt;
    int num;
    int dur;
    ItemConf *conf;
    Item(){init();}
    void init(){ itt = ITT_EMPTY; num = 0; conf = NULL; }
    void set( ITEMTYPE itt, int n, int dur ){
        this->itt = itt;
        this->num = n;
        this->dur = dur;
        this->conf = ItemConf::getItemConf(itt);
    }
    void set( ITEMTYPE itt, int n ) {
        this->itt = itt;
        this->num = n;
        this->conf = ItemConf::getItemConf(itt);        
        this->dur = this->conf->dur;
    }
    bool isDamaged() {
        if( conf->dur == 0 ) return false; else return ( conf->dur > dur );
    }
    
    bool isEmpty(){
        return ( itt == ITT_EMPTY );
    }
    void setEmpty(){
        itt = ITT_EMPTY;
        num = 0;
        conf = NULL;
    }
    void dec(int n){
        assert( num>=n);
        num -= n;
        if(num==0){
            itt = ITT_EMPTY;
            conf = NULL;
        }
    }
    int room() {
        return conf->stack_max - num;
    }
    bool validate() {
        bool ret;
        if( itt == ITT_ERROR ) {
            ret = false;
        } else if( itt == ITT_EMPTY ) {
            ret = ( num == 0 && (conf == NULL || conf->itt == ITT_EMPTY ) ); 
        } else {
            ret = ( num > 0 && conf != NULL );
        }
        if(!ret) print("Item::validate: fail. itt:%d num:%d dur:%d conf:%p(%d)", itt, num, dur, conf, conf ? conf->itt : 0 );
        return ret;
    }
    void dump() {
        print("itt:%d num:%d dur:%d conf:%p", itt, num, dur, conf );
    }
};


ITEMTYPE blockTypeToItemId( BLOCKTYPE bt );
BLOCKTYPE itemIDToBlockType( ITEMTYPE itt );


class ItemOwner {
public:
    Item *items;
    int item_num;

    int stack_multiply;
    
    ItemOwner( int num, int stackmul );
    int countItem(ITEMTYPE itt);
    int findItemType(ITEMTYPE itt );
    bool decItem(ITEMTYPE itt, int n );
    bool incItem(ITEMTYPE itt, int n, int ofs_index, bool dry );
    bool incItemIndex( int ind, ITEMTYPE itt, int n );
    void dumpItem();
    void decItemIndex( int index, int n );
    void clearItemID(ITEMTYPE id);
    void clearItems();
    void swapItem( int from_index, int to_index );
    bool  merge( ItemOwner *to_merge );

    virtual void onItemUpdated( int index ) {};
};


#endif
