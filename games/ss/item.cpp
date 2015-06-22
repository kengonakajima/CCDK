#include "moyai/client.h"
#include "conf.h"
#include "ids.h"
#include "atlas.h"
#include "item.h"
#include "dimension.h"
#include "net.h"
#include "globals.h"




static ItemConf item_confs[ITEM_CONF_NUM];
static ItemConf *sorted_item_confs[ITEM_CONF_NUM];
ResearchState g_research_state[ITEM_CONF_NUM];

void setupItemConf(){
    int ind=0;

    // weapons
    item_confs[ind] = ItemConf( ITT_BEAMGUN1, 0, "BEAMGUN LV1", g_base_deck, B_ATLAS_ITEM_BEAMGUN1, 1, false );
    item_confs[ind].addIngredient( ITT_IRON_PLATE, 3 );
    item_confs[ind].required_ene = 100;
    item_confs[ind].dur = 10000;
    item_confs[ind].is_beamgun = true;
    item_confs[ind].addResearchIngredient( ITT_IRON_PLATE, 20 );
    ind++;
    
    item_confs[ind] = ItemConf( ITT_BEAMGUN2, 1, "BEAMGUN LV2", g_base_deck, B_ATLAS_ITEM_BEAMGUN2, 1, false );
    item_confs[ind].addIngredient( ITT_IRON_PLATE, 10 );
    item_confs[ind].addIngredient( ITT_RAREMETAL_CRYSTAL, 5 );
    item_confs[ind].required_ene = 400;
    item_confs[ind].dur = 10000;
    item_confs[ind].is_beamgun = true;
    item_confs[ind].addResearchIngredient( ITT_IRON_PLATE, 100 );
    item_confs[ind].addResearchIngredient( ITT_RAREMETAL_CRYSTAL, 20 );
    item_confs[ind].addResearchIngredient( ITT_ARTIFACT, 10 );
    ind++;
    
    item_confs[ind] = ItemConf( ITT_BEAMGUN3, 3, "BEAMGUN LV3", g_base_deck, B_ATLAS_ITEM_BEAMGUN3, 1, false );
    item_confs[ind].addIngredient( ITT_IRON_PLATE, 30 );
    item_confs[ind].addIngredient( ITT_RAREMETAL_CRYSTAL, 20 );
    item_confs[ind].addIngredient( ITT_ARTIFACT, 10 );
    item_confs[ind].addIngredient( ITT_HYPER_PARTICLE, 1 );
    item_confs[ind].required_ene = 1000;
    item_confs[ind].dur = 10000;
    item_confs[ind].is_beamgun = true;
    item_confs[ind].addResearchIngredient( ITT_BEAMGUN2, 10 );
    item_confs[ind].addResearchIngredient( ITT_ARTIFACT, 40 );
    item_confs[ind].addResearchIngredient( ITT_HYPER_PARTICLE, 15 );
    ind++;
    
    item_confs[ind] = ItemConf( ITT_BEAMGUN4, 5, "BEAMGUN LV4", g_base_deck, B_ATLAS_ITEM_BEAMGUN4, 1, false );
    item_confs[ind].addIngredient( ITT_IRON_PLATE, 9 );
    item_confs[ind].addIngredient( ITT_RAREMETAL_CRYSTAL, 5 );
    item_confs[ind].addIngredient( ITT_ARTIFACT, 20 );
    item_confs[ind].addIngredient( ITT_HYPER_PARTICLE, 5 );
    item_confs[ind].required_ene = 2000;
    item_confs[ind].dur = 10000;
    item_confs[ind].is_beamgun = true;
    item_confs[ind].addResearchIngredient( ITT_BEAMGUN3, 10 );
    item_confs[ind].addResearchIngredient( ITT_ARTIFACT, 100 );
    item_confs[ind].addResearchIngredient( ITT_HYPER_PARTICLE, 20 );
    item_confs[ind].addResearchIngredient( ITT_DARK_MATTER_PARTICLE, 2 );
    ind++;
    
    item_confs[ind] = ItemConf( ITT_BLASTER, 0, "BLASTER", g_base_deck, B_ATLAS_ITEM_BLASTER, DEFAULT_STACK_MAX, false );
    item_confs[ind].addIngredient( ITT_IRON_PLATE, 1 );
    item_confs[ind].addIngredient( ITT_RAREMETAL_CRYSTAL, 1 );
    item_confs[ind].required_ene = 100;
    item_confs[ind].addResearchIngredient( ITT_IRON_PLATE, 20 );
    item_confs[ind].addResearchIngredient( ITT_RAREMETAL_CRYSTAL, 10 );
    item_confs[ind].addResearchIngredient( ITT_ARTIFACT, 1 );    
    ind++;

    item_confs[ind] = ItemConf( ITT_HEAL_BLASTER, 1, "HEALING BLASTER", g_base_deck, B_ATLAS_ITEM_HEAL_BLASTER, DEFAULT_STACK_MAX, false );
    item_confs[ind].addIngredient( ITT_IRON_PLATE, 2 );
    item_confs[ind].addIngredient( ITT_RAREMETAL_CRYSTAL, 1 );
    item_confs[ind].addIngredient( ITT_MICROBE, 1 );
    item_confs[ind].required_ene = 200;
    item_confs[ind].addResearchIngredient( ITT_IRON_PLATE, 40 );
    item_confs[ind].addResearchIngredient( ITT_RAREMETAL_CRYSTAL, 20 );
    item_confs[ind].addResearchIngredient( ITT_MICROBE, 20 );
    item_confs[ind].addResearchIngredient( ITT_ARTIFACT, 20 );
    item_confs[ind].addResearchIngredient( ITT_HYPER_PARTICLE, 2 );
    ind++;    
    
    
    // glossaries
    item_confs[ind] = ItemConf( ITT_APPLE, 0, "APPLE", g_base_deck, B_ATLAS_ITEM_APPLE, DEFAULT_STACK_MAX, false );
    item_confs[ind].hp_recover_amount = PC_MAXHP/4;
    ind++;
    item_confs[ind++] = ItemConf( ITT_ENERGY_PARTICE, 0, "ENERGY PARTICLE", g_base_deck, B_ATLAS_ITEM_ENERGY_PARTICLE, DEFAULT_STACK_MAX, false);
    item_confs[ind++] = ItemConf( ITT_MICROBE, 0, "MICROBE PARTICLE", g_base_deck, B_ATLAS_MICROBE_PARTICLE, DEFAULT_STACK_MAX, false );

    item_confs[ind] = ItemConf( ITT_WEED_SEED, 1, "WEED SEED", g_base_deck, B_ATLAS_WEED_SEED, DEFAULT_STACK_MAX, true );
    item_confs[ind].addIngredient( ITT_DEBRI_SOIL, 1 );
    item_confs[ind].addIngredient( ITT_MICROBE, 1 );
    item_confs[ind].required_ene = 10;
    item_confs[ind].addResearchIngredient( ITT_MICROBE, 20 );
    item_confs[ind].addResearchIngredient( ITT_DEBRI_SOIL, 20 );
    item_confs[ind].addResearchIngredient( ITT_ARTIFACT, 5 );        
    ind++;

    item_confs[ind] = ItemConf( ITT_HP_POTION, 1, "HP DRINK", g_base_deck, B_ATLAS_ITEM_HP_POTION, DEFAULT_STACK_MAX, false );
    item_confs[ind].addIngredient( ITT_IRON_PLATE, 1 );
    item_confs[ind].addIngredient( ITT_RAREMETAL_CRYSTAL, 1 );
    item_confs[ind].addIngredient( ITT_MICROBE, 1 );
    item_confs[ind].hp_recover_amount = PC_MAXHP/2;
    item_confs[ind].required_ene = 200;
    item_confs[ind].addResearchIngredient( ITT_IRON_PLATE, 10 );
    item_confs[ind].addResearchIngredient( ITT_RAREMETAL_CRYSTAL, 10 );
    item_confs[ind].addResearchIngredient( ITT_MICROBE, 40 );
    item_confs[ind].addResearchIngredient( ITT_ARTIFACT, 20 );
    ind++;
        
    
    item_confs[ind] = ItemConf( ITT_TREE_SEED, 1, "TREE SEED", g_base_deck, B_ATLAS_TREE_SEED, DEFAULT_STACK_MAX, true );
    item_confs[ind].addIngredient( ITT_DEBRI_SOIL, 10 );
    item_confs[ind].addIngredient( ITT_DEBRI_ROCK, 1 );    
    item_confs[ind].addIngredient( ITT_MICROBE, 1 );
    item_confs[ind].required_ene = 100;
    item_confs[ind].addResearchIngredient( ITT_DEBRI_SOIL, 50 );
    item_confs[ind].addResearchIngredient( ITT_DEBRI_ROCK, 10 );
    item_confs[ind].addResearchIngredient( ITT_MICROBE, 30 );
    item_confs[ind].addResearchIngredient( ITT_ARTIFACT, 3 );
    ind++;

    item_confs[ind++] = ItemConf( ITT_ARTIFACT, 0, "ALIEN ARTIFACT", g_base_deck, B_ATLAS_ITEM_ARTIFACT, DEFAULT_STACK_MAX, false );
    item_confs[ind++] = ItemConf( ITT_HYPER_PARTICLE, 0, "HYPER PARTICLE", g_base_deck, B_ATLAS_ITEM_HYPER_PARTICLE, DEFAULT_STACK_MAX, false );
    item_confs[ind++] = ItemConf( ITT_DARK_MATTER_PARTICLE, 0, "DARK MATTER PARTICLE", g_base_deck, B_ATLAS_ITEM_DARK_MATTER_PARTICLE, DEFAULT_STACK_MAX, false );    
    
    item_confs[ind] = ItemConf( ITT_IRON_PLATE, 0, "IRON PLATE", g_base_deck, B_ATLAS_ITEM_IRON_PLATE, DEFAULT_STACK_MAX, false );
    item_confs[ind].addIngredient( ITT_DEBRI_IRONORE, 1 );
    item_confs[ind].required_ene = 50;
    item_confs[ind].addResearchIngredient( ITT_DEBRI_IRONORE, 10 );
    ind++;
    
    item_confs[ind] = ItemConf( ITT_RAREMETAL_CRYSTAL, 0, "RARE METAL CRYSTAL", g_base_deck, B_ATLAS_ITEM_RAREMETAL_CRYSTAL, DEFAULT_STACK_MAX, false );
    item_confs[ind].addIngredient( ITT_DEBRI_RAREMETALORE, 1 );
    item_confs[ind].required_ene = 100;
    item_confs[ind].addResearchIngredient( ITT_DEBRI_RAREMETALORE, 10 );
    ind++;
    
    item_confs[ind] = ItemConf( ITT_BATTERY1, 1, "BATTERY LV1", g_base_deck, B_ATLAS_ITEM_BATTERY1, 1, false );
    item_confs[ind].addIngredient( ITT_IRON_PLATE, 10 );
    item_confs[ind].addIngredient( ITT_RAREMETAL_CRYSTAL, 20 );
    item_confs[ind].dur = 2000;
    item_confs[ind].required_ene = 500;
    item_confs[ind].is_passive_mod = true;    
    item_confs[ind].addResearchIngredient( ITT_IRON_PLATE, 100 );
    item_confs[ind].addResearchIngredient( ITT_RAREMETAL_CRYSTAL, 200 );
    item_confs[ind].addResearchIngredient( ITT_ARTIFACT, 30 );
    ind++;

    item_confs[ind] = ItemConf( ITT_BATTERY2, 4, "BATTERY LV2", g_base_deck, B_ATLAS_ITEM_BATTERY2, 1, false );
    item_confs[ind].addIngredient( ITT_IRON_PLATE, 10 );
    item_confs[ind].addIngredient( ITT_RAREMETAL_CRYSTAL, 50 );
    item_confs[ind].addIngredient( ITT_ARTIFACT, 10 );
    item_confs[ind].addIngredient( ITT_HYPER_PARTICLE, 2 );
    item_confs[ind].dur = 4000;
    item_confs[ind].required_ene = 2000;
    item_confs[ind].is_passive_mod = true;
    item_confs[ind].addResearchIngredient( ITT_BATTERY1, 20 );
    item_confs[ind].addResearchIngredient( ITT_ARTIFACT, 50 );
    item_confs[ind].addResearchIngredient( ITT_HYPER_PARTICLE, 20 );
    item_confs[ind].addResearchIngredient( ITT_DARK_MATTER_PARTICLE, 2 );
    ind++;
    
    
    item_confs[ind] = ItemConf( ITT_SHIELD, 3, "SHIELD", g_base_deck, B_ATLAS_ITEM_SHIELD, 1, false );
    item_confs[ind].addIngredient( ITT_IRON_PLATE, 20 );
    item_confs[ind].addIngredient( ITT_RAREMETAL_CRYSTAL, 10 );
    item_confs[ind].addIngredient( ITT_HYPER_PARTICLE, 1 );    
    item_confs[ind].dur = 1000;
    item_confs[ind].required_ene = 1000;
    item_confs[ind].is_passive_mod = true;    
    item_confs[ind].addResearchIngredient( ITT_IRON_PLATE, 200 );
    item_confs[ind].addResearchIngredient( ITT_RAREMETAL_CRYSTAL, 200 );
    item_confs[ind].addResearchIngredient( ITT_ARTIFACT, 100 );
    item_confs[ind].addResearchIngredient( ITT_HYPER_PARTICLE, 10 );
    ind++;
    
    item_confs[ind] = ItemConf( ITT_ACCELERATOR, 3, "ACCELERATOR", g_base_deck, B_ATLAS_ITEM_ACCELERATOR, 1, false );
    item_confs[ind].addIngredient( ITT_IRON_PLATE, 10 );
    item_confs[ind].addIngredient( ITT_RAREMETAL_CRYSTAL, 10 );
    item_confs[ind].addIngredient( ITT_HYPER_PARTICLE, 2 );
    item_confs[ind].dur = 40000;
    item_confs[ind].required_ene = 500;
    item_confs[ind].is_passive_mod = true;
    item_confs[ind].addResearchIngredient( ITT_IRON_PLATE, 100 );
    item_confs[ind].addResearchIngredient( ITT_RAREMETAL_CRYSTAL, 100 );
    item_confs[ind].addResearchIngredient( ITT_HYPER_PARTICLE, 10 );
    item_confs[ind].addResearchIngredient( ITT_DARK_MATTER_PARTICLE, 1 );
    ind++;
    
    item_confs[ind] = ItemConf( ITT_SHOVEL, 1, "SNOW SHOVEL", g_base_deck, B_ATLAS_ITEM_SHOVEL, 1, false );
    item_confs[ind].addIngredient( ITT_IRON_PLATE, 10 );
    item_confs[ind].dur = 500;
    item_confs[ind].required_ene = 50;
    item_confs[ind].addResearchIngredient( ITT_IRON_PLATE, 50 );
    item_confs[ind].addResearchIngredient( ITT_ARTIFACT, 1 );
    ind++;        

    
    // debris
    item_confs[ind++] = ItemConf( ITT_DEBRI_SOIL, 0, "SOIL DEBRI", g_base_deck, BT_SOIL, DEFAULT_STACK_MAX, true );
    item_confs[ind++] = ItemConf( ITT_DEBRI_ROCK, 0, "ROCK DEBRI", g_base_deck, BT_ROCK, DEFAULT_STACK_MAX, true );
    item_confs[ind++] = ItemConf( ITT_DEBRI_IRONORE, 0, "IRON ORE", g_base_deck, BT_IRONORE, DEFAULT_STACK_MAX, true );
    item_confs[ind++] = ItemConf( ITT_DEBRI_RAREMETALORE, 0, "RARE METAL ORE", g_base_deck, BT_RAREMETALORE, DEFAULT_STACK_MAX, true );
    item_confs[ind++] = ItemConf( ITT_DEBRI_HARDROCK, 0, "HARD ROCK DEBRI", g_base_deck, BT_HARDROCK, DEFAULT_STACK_MAX, true );
    
    // put to use
    item_confs[ind] = ItemConf( ITT_BRICK_PANEL, 0, "BRICK PANEL", g_base_deck, B_ATLAS_ITEM_BRICK_PANEL, DEFAULT_STACK_MAX, true );
    item_confs[ind].addIngredient( ITT_DEBRI_SOIL, 2 );
    item_confs[ind].required_ene = 25;
    item_confs[ind].addResearchIngredient( ITT_DEBRI_SOIL, 30 );
    ind++;;

    item_confs[ind] = ItemConf( ITT_REACTOR, 1, "ENERGY REACTOR", g_base_deck, BT_REACTOR_ACTIVE, DEFAULT_STACK_MAX, true );
    item_confs[ind].addIngredient( ITT_IRON_PLATE, 10 );
    item_confs[ind].addIngredient( ITT_RAREMETAL_CRYSTAL, 3 );
    item_confs[ind].required_ene = 500;
    item_confs[ind].addResearchIngredient( ITT_IRON_PLATE, 50 );
    item_confs[ind].addResearchIngredient( ITT_RAREMETAL_CRYSTAL, 20 );
    item_confs[ind].addResearchIngredient( ITT_ARTIFACT, 10 );
    ind++;
    
    item_confs[ind] = ItemConf( ITT_POLE, 1, "POWER POLE", g_base_deck, BT_POLE, DEFAULT_STACK_MAX, true );
    item_confs[ind].addIngredient( ITT_IRON_PLATE, 5 );
    item_confs[ind].required_ene = 100;
    item_confs[ind].addResearchIngredient( ITT_IRON_PLATE, 100 );
    item_confs[ind].addResearchIngredient( ITT_ARTIFACT, 10 );
    ind++;
    
    item_confs[ind] = ItemConf( ITT_EXCHANGE, 2, "EXCHANGE", g_base_deck, BT_EXCHANGE_ACTIVE, DEFAULT_STACK_MAX, true );
    item_confs[ind].addIngredient( ITT_IRON_PLATE, 50 );
    item_confs[ind].addIngredient( ITT_RAREMETAL_CRYSTAL, 10 );
    item_confs[ind].addIngredient( ITT_ARTIFACT, 1 );
    item_confs[ind].required_ene = 500;
    item_confs[ind].addResearchIngredient( ITT_IRON_PLATE, 200 );
    item_confs[ind].addResearchIngredient( ITT_RAREMETAL_CRYSTAL, 100 );
    item_confs[ind].addResearchIngredient( ITT_ARTIFACT, 30 );
    item_confs[ind].addResearchIngredient( ITT_HYPER_PARTICLE, 4 );
    ind++;
    
    item_confs[ind] = ItemConf( ITT_CABLE, 2, "NETWORK CABLE", g_base_deck, BT_CABLE, DEFAULT_STACK_MAX, true );
    item_confs[ind].addIngredient( ITT_IRON_PLATE, 2 );
    item_confs[ind].addIngredient( ITT_RAREMETAL_CRYSTAL, 1 );
    item_confs[ind].required_ene = 50;
    item_confs[ind].addResearchIngredient( ITT_IRON_PLATE, 100 );
    item_confs[ind].addResearchIngredient( ITT_RAREMETAL_CRYSTAL, 50 );
    item_confs[ind].addResearchIngredient( ITT_ARTIFACT, 5 );
    ind++;
    
    item_confs[ind] = ItemConf( ITT_PORTAL, 4, "PORTAL", g_base_deck, BT_PORTAL_ACTIVE, DEFAULT_STACK_MAX, true );
    item_confs[ind].addIngredient( ITT_IRON_PLATE, 10 );
    item_confs[ind].addIngredient( ITT_RAREMETAL_CRYSTAL, 10 );
    item_confs[ind].addIngredient( ITT_ARTIFACT, 10 );
    item_confs[ind].addIngredient( ITT_HYPER_PARTICLE, 1 );
    item_confs[ind].required_ene = 500;
    item_confs[ind].addResearchIngredient( ITT_REACTOR, 20 );
    item_confs[ind].addResearchIngredient( ITT_ARTIFACT, 50 );
    item_confs[ind].addResearchIngredient( ITT_HYPER_PARTICLE, 10 );
    item_confs[ind].addResearchIngredient( ITT_DARK_MATTER_PARTICLE, 2 );
    ind++;
    
    item_confs[ind] = ItemConf( ITT_TURRET, 3, "TURRET", g_base_deck, BT_TURRET_ACTIVE, DEFAULT_STACK_MAX, true );
    item_confs[ind].addIngredient( ITT_IRON_PLATE, 20 );
    item_confs[ind].addIngredient( ITT_RAREMETAL_CRYSTAL, 3 );
    item_confs[ind].required_ene = 200;
    item_confs[ind].addResearchIngredient( ITT_IRON_PLATE, 100 );
    item_confs[ind].addResearchIngredient( ITT_RAREMETAL_CRYSTAL, 50 );
    item_confs[ind].addResearchIngredient( ITT_ARTIFACT, 30 );
    item_confs[ind].addResearchIngredient( ITT_HYPER_PARTICLE, 2 );
    ind++;
    
    item_confs[ind] = ItemConf( ITT_FENCE, 2, "FENCE", g_base_deck, BT_FENCE, DEFAULT_STACK_MAX, true );
    item_confs[ind].addIngredient( ITT_IRON_PLATE, 2 );
    item_confs[ind].required_ene = 50;
    item_confs[ind].addResearchIngredient( ITT_IRON_PLATE, 50 );
    item_confs[ind].addResearchIngredient( ITT_ARTIFACT, 10 );
    item_confs[ind].addResearchIngredient( ITT_HYPER_PARTICLE, 1 );
    ind++;            
    
    item_confs[ind] = ItemConf( ITT_EMPTY, 999999999, "end of the list", NULL, 0, 0, true ); // 番兵
    ind++;

    // Sort for easy crafting
    int item_num = ind - 1; // end of the list
    SorterEntry sorter[ elementof(item_confs) ];
    for(int i=0;i<item_num;i++) {
        sorter[i].val = item_confs[i].level;
        sorter[i].ptr = & item_confs[i];
    }
    quickSortF( sorter, 0, item_num-1 );
    for(int i=0;i<item_num;i++) {
        sorted_item_confs[i] = (ItemConf*) sorter[i].ptr;
    }

    // Everything is locked when game started
    for(int i=0;i<item_num;i++) {
        ItemConf *itc = sorted_item_confs[i];
        g_research_state[i].itt = itc->itt;
        for(int j=0;j<elementof(itc->ingredients);j++ ) {
            if( itc->research_ingredients[j].itt > ITT_ERROR ) {
                g_research_state[i].ingrs[j].itt = itc->research_ingredients[j].itt;
                g_research_state[i].ingrs[j].n = itc->research_ingredients[j].n;
            }                 
        }
    }
}

//
int ItemConf::countCraftable() {
    int cnt=0;
    for(int i = 0; i < elementof(sorted_item_confs); i++ ) {
        if( sorted_item_confs[i] && sorted_item_confs[i]->itt > ITT_ERROR && sorted_item_confs[i]->ingredients[0].itt > ITT_ERROR ) cnt++;
    }
    return cnt;
}

// 
ItemConf *ItemConf::getCraftable( int ind ) {
    int out_ind=0;
    for(int i = 0; i < elementof(sorted_item_confs); i++ ) {
        if( sorted_item_confs[i] ) {
            if( sorted_item_confs[i]->itt > ITT_ERROR && sorted_item_confs[i]->ingredients[0].itt > ITT_ERROR ) {
                if( ind == out_ind ) {
                    return sorted_item_confs[i];
                }
                out_ind ++;
            }
        } 
    }
    return NULL;
}

ItemConf * ItemConf::getItemConf( ITEMTYPE itt ){
    for(int i=0;i<elementof(item_confs);i++){
        ItemConf *itc = & item_confs[i];
        if(itc->itt == itt) return itc;
        if(itc->itt == -1) return NULL;
    }
    return NULL;
}

ItemConf * ItemConf::getByIndex( int index ) {
    for(int i=0;i<elementof(item_confs);i++){
        ItemConf *itc = & item_confs[i];
        if( itc->index == index ) {
            return itc;
        }
    }
    assertmsg(false, "ItemConf::getByIndex: invalid index:%d", index );
    return NULL;
}
void ItemConf::addIngredient( ITEMTYPE itt, int n ) {
    for(int i=0;i<elementof(ingredients);i++) {
        if( ingredients[i].itt == ITT_EMPTY ) {
            ingredients[i].itt = itt;
            ingredients[i].n = n;
            break;
        }
    }
}
void ItemConf::addResearchIngredient( ITEMTYPE itt, int n ) {
    for(int i=0;i<elementof(research_ingredients);i++) {
        if( research_ingredients[i].itt == ITT_EMPTY ) {
            research_ingredients[i].itt = itt;
            research_ingredients[i].n = n;
            break;
        }
    }
}

int getBeamgunEne( ItemConf *itc ) {
    switch(itc->itt ) {
    case ITT_BEAMGUN1:
        return 1;
    case ITT_BEAMGUN2:
        return 4;
    case ITT_BEAMGUN3:
        return 16;
    case ITT_BEAMGUN4:
        return 64;
    default:
        assertmsg(false, "invalid itemid:%d",itc->itt);
    }
    return 0;
}

/////////////


ResearchState * ResearchState::getResearchState( ITEMTYPE itt ){
    for(int i=0;i<elementof(g_research_state);i++){
        ResearchState *rs = & g_research_state[i];
        if(rs->itt == itt) return rs;
        if(rs->itt == -1) return NULL;
    }
    return NULL;
}

void ResearchState::unlock( ITEMTYPE itt ) {
    ResearchState *rs = ResearchState::getResearchState(itt);
    assert(rs);
    rs->locked = false;
}
void ResearchState::unlockAll() {
    for(int i=0;i<elementof(g_research_state);i++) {
        ResearchState *rs = & g_research_state[i];
        if(rs->itt == -1 )break;
        unlock(rs->itt);
    }
}

bool ResearchState::itemIsLocked( ITEMTYPE itt ) {
    ResearchState *rs = ResearchState::getResearchState(itt);
    assert(rs);
    return rs->locked;
}

bool ResearchState::isComplete() {
    bool completed = true;
    for(int i=0;i<elementof(ingrs);i++) {
        //        print("ING: t:%d cur:%d n:%d", ingrs[i].itt, ingrs[i].current, ingrs[i].n );
        if( ingrs[i].itt > ITT_EMPTY && ingrs[i].current < ingrs[i].n ) {
            completed = false;
        }
    }
    return completed;
}
void resetResearchStateLocked() {
    for(int i=0;i<elementof(g_research_state);i++) {
        g_research_state[i].locked = true;
    }
}
/////////////////////
ItemOwner::ItemOwner( int n, int stackmul ) : stack_multiply(stackmul) {
    size_t sz = sizeof(Item) * n;
    items = (Item*) MALLOC( sz );
    for(int i=0;i<n;i++) items[i].init();
    item_num = n;
    
}


int ItemOwner::countItem( ITEMTYPE itt ) {
    int tot=0;
    for(int i=0;i<item_num;i++){
        if( items[i].isEmpty() == false && items[i].itt == itt ) tot += items[i].num;
    }
    return tot;
}



// Set dry to true to check-only
bool ItemOwner::incItem(ITEMTYPE itt, int n, int ofs_index, bool dry ) {
    ItemConf *itc = ItemConf::getItemConf(itt);
    assert(itc);
    // Check room first and then actually add
    int space=0;
    for(int i=ofs_index;i<item_num;i++){
        if(items[i].isEmpty() ){
            space += itc->stack_max*stack_multiply;
        } else if( items[i].itt == itt && items[i].num < itc->stack_max*stack_multiply ){
            space += itc->stack_max*stack_multiply - items[i].num;
        }
    }
    if( space < n ){
        //        print("stackable item is full.. require:%d space:%d", n, space );
        return false;
    }
    // Found a room. Use up existing slot and then use next slot
    for(int i=ofs_index;i<item_num;i++){
        if(items[i].itt == itt && items[i].num < itc->stack_max*stack_multiply ){
            int d = itc->stack_max*stack_multiply - items[i].num;
            int to_add;
            if( n <= d ){
                to_add = n;
            } else {
                to_add = d;
            }
            if(!dry){
                items[i].num += to_add;
                onItemUpdated(i);
            }
            n -= to_add;
            break;
        }
    }
    if( n == 0 ){
        onItemUpdated(-1);
        return true;
    } 
    // Have more. Find next rooms..
    for(int i=ofs_index;i<item_num;i++){
        if(items[i].isEmpty() || ( items[i].itt == itt && items[i].num < itc->stack_max*stack_multiply ) ){
            int to_add ;
            if(n>itc->stack_max*stack_multiply){
                to_add = itc->stack_max*stack_multiply;
            } else {
                to_add = n;
            }
            if(!dry){
                items[i].set(itt,to_add, itc->dur );
                onItemUpdated(i);
            }
            n -= to_add;
            if(n==0)break;
        }
    }

    assertmsg(n==0, "bug n is:%d dry:%d", n, dry  );
    if(!dry){
        onItemUpdated(-1);
    }
    return true;

}
bool ItemOwner::incItemIndex( int ind, ITEMTYPE itt, int n ) {
    if( items[ind].itt == itt ) {
        if( items[ind].room() >= n  ) {
            items[ind].num += n;
            onItemUpdated(ind);
            return true;
        } else {
            return false;
        }
    } else if( items[ind].isEmpty() ) {
        ItemConf *itc = ItemConf::getItemConf(itt);
        items[ind].set(itt,n,itc->dur);
        return true;
    } else {
        return false;
    }
}




void ItemOwner::swapItem( int from_index, int to_index ){
    items[from_index].dump();    
    assert(items[from_index].validate());
    assert(items[to_index].validate());
    
    Item tmp = items[from_index];
    assert( tmp.validate() );
    items[from_index].set( items[to_index].itt, items[to_index].num, items[to_index].dur );
    items[from_index].dump();
    assert(items[from_index].validate());    
    items[to_index].set( tmp.itt, tmp.num, tmp.dur );
    assert(items[to_index].validate());    
}

void ItemOwner::decItemIndex( int index, int n ){
    assert( index >= 0 && index < item_num );
    assert( items[index].num > 0 );
    items[index].dec(n);
    onItemUpdated(-1);
}
bool ItemOwner::decItem(ITEMTYPE itt, int n ){
    ItemConf *itc = ItemConf::getItemConf(itt);
    assert(itc);

    // Check if you have enough items first
    int tot=0;
    for(int i=0;i< item_num; i++){
        if(items[i].itt==itt ) tot += items[i].num;
    }
    if(n>tot){
        print("decitem: not enough");
        return false;
    }
    
    // Use from slot that have less than stack_max, and then go to fully stacked slot
    for(int i=0;i<item_num;i++){
        if( items[i].itt == itt && items[i].num < (itc->stack_max*stack_multiply) ){
            int to_dec;
            if( items[i].num >= n ){
                to_dec = n;
            } else {
                to_dec = items[i].num;
            }
            items[i].dec(to_dec);
            n -= to_dec;
        }
    }
    if( n == 0 ){
        onItemUpdated(-1);
        return true;
    }
    for(int i=0;i<item_num;i++){
        if( items[i].itt == itt ){
            int to_dec;
            if( items[i].num < n ){
                to_dec = items[i].num;
            } else {
                to_dec = n;
            }
            items[i].dec(to_dec);
            n -= to_dec;
            if(n==0)break;
        }            
    }

    assert( n==0 );
    onItemUpdated(-1);
    return true;
}




void ItemOwner::clearItems(){
    for(int i=0;i<item_num;i++){
        items[i].setEmpty();
    }
}

int ItemOwner::findItemType(ITEMTYPE itt ){
    for(int i=0;i<item_num;i++){
        if( ! items[i].isEmpty() && items[i].itt == itt ){
            return i;
        } 
    }
    return -1;
}

// Returns false and quit if item is full.
bool ItemOwner::merge( ItemOwner *to_merge ) {
    for(int i=0;i< to_merge->item_num; i++ ) {
        Item *itm_to_add = & ( to_merge->items[i] );
        if( itm_to_add->isEmpty() == false ) {
            print("ItemOwner::merging itt:%d num:%d", itm_to_add->itt, itm_to_add->num );
            if( incItem( itm_to_add->itt, itm_to_add->num, PC_SUIT_NUM, false ) == false ) {
                return false;
            }
        }
    }
    return true;
}
