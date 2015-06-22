
#include "moyai/client.h"

#include "dimension.h"

#include "ids.h"
#include "conf.h"
#include "item.h"
#include "util.h"
#include "net.h"
#include "field.h"
#include "pwgrid.h"
#include "mapview.h"
#include "globals.h"

//////////////////////

PowerNode::PowerNode( Pos2 at ) : at(at), next(NULL), prev(NULL) {
    for(int i=0;i<elementof(dests);i++) {
        dests[i].x = dests[i].y = -1;
    }
}
PowerGrid::PowerGrid( int id ) : id(id), ene(0), accum_time(0), decay_at(0), last_usage_at(0), last_gen_at(0), node_top(NULL), next(NULL), equip_top(NULL) {
}

// Release all nodes included in a grid
PowerGrid::~PowerGrid() {
    PowerNode *cur = node_top;
    while(cur) {
        PowerNode *to_delete = cur;
        cur = cur->next;
        delete to_delete;
    }
    PowerEquipmentNode *cureq = equip_top;
    while(cureq) {
        PowerEquipmentNode *to_delete = cureq;
        cureq = cureq->next;
        delete to_delete;        
    }
}
Color PowerGrid::getColor() {
    static Color empty( 1,0,0,1 );
    static Color notfull(1,1,0,1);    
    static Color full(0.5,1,1,1);
    if( ene == 0 ) return empty;
    if( ene == MAXENE ) return full;
    return notfull;
}
PowerNode *PowerGrid::findNode( Pos2 at ) {
    PowerNode *cur = node_top;
    while(cur) {
        if(cur->at == at ) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}
PowerEquipmentNode *PowerGrid::findPowerEquipmentNode( Pos2 at ) {
    PowerEquipmentNode *cur = equip_top;
    while(cur) {
        if(cur->at == at ) {
            return cur;
        }
    }
    return NULL;
}
int PowerGrid::getMaxNodeId() {
    int maxid = 0;
    PowerNode *cur = node_top;
    while(cur) {
        if(cur->id > maxid ) maxid = cur->id;
        cur = cur->next;
    }
    return maxid;
}
PowerNode *PowerGrid::addNode( Pos2 at ) {
    PowerNode *n = findNode(at);
    if(n) return n;

    n = new PowerNode(at);
    n->id = getMaxNodeId()+1;
    n->parent = this;
    if(node_top) {
        n->next = node_top;
        n->next->prev = n;
    }
    n->prev = NULL;
    node_top = n;
    print("grid[%d] : addNode: added at %d,%d ene:%d", id, at.x, at.y, countNodes(), ene );
    return n;
}
int PowerGrid::countPowerEquipments() {
    int n=0;
    PowerEquipmentNode *cur = equip_top;
    while(cur) {
        n++;
        cur = cur->next;
    }
    return n;
}
int PowerGrid::countNodes() {
    int n=0;
    PowerNode *cur = node_top;
    while(cur) {
        n++;
        cur = cur->next;
    }
    return n;
}
void PowerGrid:: dumpNodes() {
    print("dumpNodes:");
    PowerNode *cur = node_top;
    while(cur) {
        print("  node group:%d id:%d pos:(%d,%d) dest0:(%d,%d) dest1:(%d,%d)",
              id, cur->id, cur->at.x, cur->at.y, cur->dests[0].x, cur->dests[0].y, cur->dests[1].x, cur->dests[1].y );
        cur = cur->next;
    }
    print("dumpNodes end") ;   
}

PowerSystem::PowerSystem() : grid_top(NULL) {

}
PowerGrid *PowerSystem::findGrid( Pos2 at ) {
    PowerGrid *g = grid_top;
    while(g) {
        PowerNode *node = g->findNode(at);
        if(node) {
            return g;
        }
        if( g->findEquipment(at) ) {
            return g;
        }
        g = g->next;
    }

    return NULL;
}
int PowerSystem::getMaxGridId() {
    int maxid=0;
    PowerGrid *g = grid_top;
    while(g) {
        if( g->id > maxid )maxid = g->id;
        g = g->next;
    }
    return maxid;
}
PowerGrid *PowerSystem::addGrid() {
    print("addGrid");
    PowerGrid *ng = new PowerGrid( getMaxGridId()+1 );
    ng->next = grid_top ;
    grid_top = ng;
    return ng;
}

// Returns new id of node.  0 on failure
int PowerSystem::addNode( Pos2 p ) { 
    Pos2 cands[2];
    int ncand=2;
    g_fld->findPoleNeighbours( p, cands, &ncand );
#if 0
    print("powersystem: addnode: p:%d,%d  ncand: %d", p.x, p.y, ncand );
#endif    
    if(ncand==1) {
        PowerGrid *g0 = g_ps->findGrid( cands[0] );
        if( !g0 ) {
            // According to timing of synchronization, you may not have a powergrid but have in field.
            // In this case it cleans pole on the field. 
            Cell *c = g_fld->get(cands[0]);
            if(c&& c->bt == BT_POLE ) { c->bt = BT_AIR; g_fld->notifyChanged(c); }
            print("cleared stray pole at %d,%d", cands[0].x, cands[0].y);
            return 0;
        }
        if( g0->countNodes() < MAX_NODE_IN_A_GRID ) {
            PowerNode *newnode = g0->addNode(p);
            newnode->addDest( cands[0] );
            return g0->id;
        } else {
            print("powersystem: addnode 1: grid id:%d reached max node.", g0->id );
            return 0;
        }
    } else if( ncand == 2 ) {
        PowerGrid *g0 = g_ps->findGrid( cands[0] );
        PowerGrid *g1 = g_ps->findGrid( cands[1] );

        if(!g0) {
            Cell *c = g_fld->get(cands[0]);
            if(c&& c->bt == BT_POLE ) { c->bt = BT_AIR; g_fld->notifyChanged(c); }
            print("cleared stray pole at %d,%d", cands[0].x, cands[0].y );
            return 0;
        }
        if(!g1) {
            Cell *c = g_fld->get(cands[1]);
            if(c&& c->bt == BT_POLE ) { c->bt = BT_AIR; g_fld->notifyChanged(c); }
            print("cleared stray pole at %d,%d", cands[1].x, cands[1].y );
            return 0;            
        }
        
        
        if( g0 == g1 ) {
            if( g0->countNodes() < MAX_NODE_IN_A_GRID ) {
                PowerNode *newnode = g0->addNode(p); // sorted from near to distant. [0] is the nearest.
                newnode->addDest( cands[0] );
                return g0->id;
            } else {
                print("powersystem: addnode 2: grid id:%d reached max node.", g0->id );
                return 0;                
            }
        } else {
            // Connecting different grids.
            // Making sure total node of new grid doesn't have excess number of node.
            if( ( g0->countNodes() + g1->countNodes() ) < MAX_NODE_IN_A_GRID ) {
                PowerNode *newnode = g0->addNode(p);
                newnode->addDest( cands[0] );
                newnode->addDest( cands[1] );
                g_ps->mergeGrid( g0, g1 );
                return g0->id;
            } else {
                print("powersystem: addnode 3: grid id:%d and id:%d reached max node.", g0->id, g1->id );
                return 0;                
            }
        }
        

    } else {
        PowerGrid *g = g_ps->addGrid();
        g->addNode(p);
        print("powersystem: addnode: no grid to connect! adding a grid id:%d", g->id );
        return g->id;
    }
}
PowerNode *PowerGrid::findNodeById( int id ) {
    PowerNode *cur = node_top;
    while(cur) {
        if( cur->id == id ) return cur;
        cur = cur->next;
    }
    return NULL;
}

void PowerNode::addDest( Pos2 to ) {
    for(int i=0;i<elementof(dests);i++) {
        if( dests[i] == to ) return;
        //
        if( dests[i].x == -1 ) {
            dests[i] = to;
            print("pnode: addDest: added [%d] %d,%d", i, to.x, to.y );
            return;
        }
    }
    assertmsg( false, "can't add more than 2 nodes");
}
int PowerNode::countDest() {
    int cnt=0;
    for(int i=0;i<elementof(dests);i++) {
        if( dests[i].x >= 0 ) cnt++;
    }
    return cnt;
}
PowerNode *PowerSystem::findNode( Pos2 p) {
    PowerGrid *curg = grid_top;
    while(curg) {
        PowerNode *n = curg->findNode(p);
        if(n) return n;
        curg = curg->next;
    }
    return NULL;
}


void PowerSystem::mergeGrid( PowerGrid *remain, PowerGrid *remove ) {
    print("mergeGrid");
    // add all nodes of a pwgrid to remaining pwgrid and delete.
    PowerNode *cur = remove->node_top;
    while(cur) {
        PowerNode *newnode = remain->addNode( cur->at );
        for(int i=0;i<elementof(cur->dests);i++) {
            if( cur->dests[i].x >= 0 ) newnode->addDest( cur->dests[i] );            
        }
        cur = cur->next;
    }

    // delete a grid from list
    deleteGrid(remove);

    // modify ground
    remain->repaint();    
}

void PowerSystem::deleteGrid( PowerGrid *g ) {
    if(g==grid_top) {
        grid_top = g->next;
        delete g;
        return;
    } else {
        PowerGrid *prev = NULL;
        PowerGrid *cur = grid_top;
        while(cur) {
            if( cur == g ) {
                prev->next = cur->next;
                delete g;
                return;
            }
            prev = cur;
            cur = cur->next;
        }
    }
    assert(false); // bug
}

void PowerGrid::repaint() {
    PowerNode *cur = node_top;
    while(cur) {
        g_fld->setPowerGrid( cur->at, id );
        cur = cur->next;
    }

}


void PowerSystem::poll(double dt) {
    PowerGrid *cur = grid_top;
    while(cur){
        cur->poll(dt);
        cur = cur->next;
    }    
}
void PowerGrid::poll( double dt ) {
    accum_time += dt;
    if( accum_time > decay_at ) {
        //        print("powergrid poll. ene:%d",ene);
        //        print("PowerGrid::poll: id:%d nodes:%d eqs:%d",id, countNodes(), countPowerEquipments() );
        decay_at = accum_time + POWERGRID_DECAY_INTERVAL;
        
        // decay and generating power
        ene--;
        if(ene<0)ene=0;

        // Fix stray pole
        PowerNode *cur = node_top;
        while(cur) {
            Cell *c = g_fld->get( cur->at );
            assert(c);
            if( c->powergrid_id != 0 && c->powergrid_id != id ) {
                //                CHUNKLOADSTATE ls = g_fld->getChunkLoadState( cur->at );
                //                if( ls == CHUNKLOADSTATE_LOADED ) {
                print("found stray pole at %d,%d gid:%d. convert to gid:%d", cur->at.x, cur->at.y, c->powergrid_id, id );
                g_fld->setPowerGrid( cur->at, id );
                // fixed, saving
                g_fld->saveAt(c, 4 );                                    
            } 
            cur = cur->next;
        }

        // All equipments to a node
        int prevene = ene;
        PowerEquipmentNode *cureq = equip_top;
        while(cureq) {
            switch( cureq->type ) {
            case PEQT_INVAL:
                assert(false);
                break;
            case PEQT_REACTOR:
            case PEQT_MOTHERSHIP:                
                ene += cureq->ene_unit;
                if(ene>MAXENE)ene=MAXENE;
                break;
            case PEQT_EXCHANGE:
                break;
            }
            cureq = cureq->next;
        }
        if( ene != prevene ) {
            int diff = ene - prevene;
            modEne( diff, true ); // sync
        }
    }
}
// Sync realtime on local modificaion
void PowerGrid::modEne( int diff, bool local ) {
    int prev_ene = ene;
    ene += diff;
    if( ene < 0 ) ene = 0;
    if( ene > MAXENE ) ene = MAXENE;
    if( ene > prev_ene ) {
        last_gen_at = accum_time;
    }
    if( local && ene != prev_ene ) realtimePowerGridSend( id, ene - prev_ene );
    //        print("powergrid %d generates %d ene. total:%d", id, gen_ene, ene );
}

PowerGrid *PowerSystem::findGridById( int id ) {
    PowerGrid *cur = grid_top;
    while(cur){
        if(cur->id==id) return cur;
        cur = cur->next;
    }
    return NULL;
}


void PowerSystem::dumpGrids() {
    PowerGrid *cur = grid_top;
    while(cur) {
        cur->dumpNodes();
        cur = cur->next;
    }
}

// cleared:実際にdestから削除したpoleの位置
void PowerGrid::clearDestination( Pos2 dest, Pos2 *cleared, int *cleared_num ) {
    int out_i = 0;
    PowerNode *cur = node_top;
    while(cur) {
        for(int i=0;i<elementof(cur->dests);i++) {
            if( cur->dests[i] == dest ) {
                cleared[out_i] = cur->at;
                out_i++;
                cur->dests[i].x = cur->dests[i].y = -1;
            }
        }
        cur = cur->next;
    }
    *cleared_num = out_i;
}
void PowerSystem::clear() {
    PowerGrid *curgrid = grid_top;
    while(curgrid) {
        PowerGrid *to_delete = curgrid;
        curgrid = curgrid->next;
        delete to_delete;
    }
    grid_top = NULL;
}

// Returns false if data size is too big to dump. true on success.
bool PowerSystem::dumpAll( PowerSystemDump *dump ) {
    print("===== PowerSystem::dumpAll called");
    int ni=0, gi=0,eqi=0;
    PowerGrid *curgrid = grid_top;
    while(curgrid) {
        PowerNode *curnode = curgrid->node_top;
        if( gi >= elementof(dump->grids) ) {
            print("PowerSystem::dumpAll: grids full!" );
            return false;
        }
        while(curnode) {
            if( ni >= elementof(dump->nodes) ) {
                print("PowerSystem::dumpAll: nodes full!" );
                return false;
            }
            if(dump) {
                dump->nodes[ni].id = curnode->id;
                dump->nodes[ni].powergrid_id = curnode->parent->id;
                dump->nodes[ni].at = curnode->at;
                for(int i=0;i<elementof(curnode->dests);i++) dump->nodes[ni].dests[i] = curnode->dests[i];
            }
#if 0            
            print("pwdump: node(%d,%d) id:%d gid:%d dest:(%d,%d) (%d,%d)",
                  curnode->at.x, curnode->at.y, curnode->id, curnode->parent->id, curnode->dests[0].x, curnode->dests[0].y,
                  curnode->dests[1].x, curnode->dests[1].y );
#endif            
            ni++;
            curnode = curnode->next;
        }

        PowerEquipmentNode *eqnode = curgrid->equip_top;
        while(eqnode) {
            if( eqi >= elementof(dump->equipments) ) {
                print("PowerSystem::dumpAll: equipment full!");
                return false;
            }
            if(dump) {
                dump->equipments[eqi].at = eqnode->at;
                dump->equipments[eqi].powergrid_id = curgrid->id;
                dump->equipments[eqi].type = eqnode->type;
                dump->equipments[eqi].ene_unit = eqnode->ene_unit;
                eqi++;
            }
#if 0            
            print("pwdump: equip(%d,%d) gid:%d unit:%d type:%d", eqnode->at.x, eqnode->at.y, curgrid->id, eqnode->ene_unit, eqnode->type );
#endif            
            eqnode = eqnode->next;
        }
        if(dump) {
            dump->grids[gi].id = curgrid->id;
            dump->grids[gi].ene = curgrid->ene;
        }
        print("pwdump: grid id:%d ene:%d", curgrid->id, curgrid->ene );
        gi++;
        curgrid = curgrid->next;
    }
    if(dump) {
        dump->grids_used = gi;
        dump->nodes_used = ni;
        dump->equipments_used = eqi;
    }
    
    print("pwdump: grids_used:%d nodes_used:%d equipments_used:%d", gi, ni, eqi );
    print("==========");
    return true;
}

void PowerSystem::applyDump( PowerSystemDump *dump ) {
    print("======== PowerSystem::applyDump called. grid:%d node:%d equipment:%d", dump->grids_used, dump->nodes_used, dump->equipments_used );
    for(int i=0;i<dump->grids_used;i++) {
        PowerGrid *ng = addGrid();
        ng->id = dump->grids[i].id;
        ng->ene = dump->grids[i].ene;
    }
    for(int i=0;i<dump->nodes_used;i++){
        PowerGrid *g = findGridById( dump->nodes[i].powergrid_id );
        assert(g);
        PowerNode *nn = g->addNode( dump->nodes[i].at );
        nn->id = dump->nodes[i].id;
        for(int k=0;k<elementof(nn->dests);k++) {
            nn->dests[k] = dump->nodes[i].dests[k];
        }
    }
    for(int i=0;i<dump->equipments_used;i++) {
        PowerGrid *g = findGridById( dump->equipments[i].powergrid_id );
        assert(g);
        g->ensureEquipment( (POWEREQUIPMENTTYPE) dump->equipments[i].type,
                            dump->equipments[i].at,
                            dump->equipments[i].ene_unit );
    }
    print("============");
}

// Returns true if correctly added
bool PowerSystem::ensureEquipment( Pos2 p ) {
    Cell *c = g_fld->get(p);
    if(!c)return false;
    if( c->powergrid_id == 0 ) return false; // no need to do anything if a cell is not connected to a grid
    PowerGrid *grid = findGridById( c->powergrid_id );
    if(!grid) {
        print("PowerSystem::ensureEquipment: pgid %d is not found at %d,%d", c->powergrid_id, p.x, p.y );
        return false;
    }
        
    if( c->bt == BT_REACTOR_ACTIVE ) {
        int arm_num = g_fld->countReactorArm(p);
        return grid->ensureEquipment( PEQT_REACTOR, p, arm_num * PowerGrid::ENE_PER_ARM );
    } else if( c->bt == BT_MOTHERSHIP ) {
        return grid->ensureEquipment( PEQT_MOTHERSHIP, p, PowerGrid::MOTHERSHIP_ENE_PER_POLL );
    } else if( c->bt == BT_EXCHANGE_ACTIVE ) {
        return grid->ensureEquipment( PEQT_EXCHANGE, p, 0 );
    } else {
        assert(false);
    }
    return false;
}
PowerEquipmentNode *PowerGrid::findEquipment( Pos2 at ) {
    PowerEquipmentNode *cur = equip_top;
    while(cur) {
        if(cur->at == at ) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

// Returns true if successfully added
bool PowerGrid::ensureEquipment( POWEREQUIPMENTTYPE type, Pos2 at, int ene_unit) {
    PowerEquipmentNode *e = findEquipment(at);
    if(e) {
        //        print("ensureEquipment: found at %d,%d type:%d eu:%d", at.x, at.y, type, ene_unit );
        e->ene_unit = ene_unit;
        return false;
    }
    print("ensureEquipment: (%d,%d) type:%d is not found in pwgrid %d, adding.", at.x, at.y, type, id );
    e = new PowerEquipmentNode( type, at, ene_unit);
    e->next = equip_top;
    equip_top = e;
    e->prev = NULL;
    return true;
}
