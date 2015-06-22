#include "moyai/client.h"

#include "ids.h"
#include "dimension.h"
#include "conf.h"
#include "item.h"
#include "util.h"
#include "net.h"
#include "field.h"
#include "mapview.h"
#include "atlas.h"
#include "char.h"
#include "pwgrid.h"
#include "enemy.h"
#include "effect.h"
#include "hud.h"
#include "pc.h"
#include "globals.h"



Chunk::Chunk( int chx, int chy ) :  chx(chx), chy(chy), changed(true) {
    // ground
    ggrid = new Grid(CHUNKSZ,CHUNKSZ);
    ggrid->setDeck( g_base_deck );
    ggrid->setFragmentShader( g_eye_col_replacer );
    gprop = new Prop2D();
    gprop->setLoc( chx * PIXEL_PER_CHUNK, chy * PIXEL_PER_CHUNK );
    gprop->setScl(PPC,PPC);
    gprop->addGrid(ggrid);
    gprop->tex_epsilon = 1.0 / 512.0 / 2.0; // quarter pixel
    g_ground_layer->insertProp(gprop);

    // electricity
    egrid = new Grid(CHUNKSZ,CHUNKSZ);
    egrid->setDeck( g_base_deck );
    eprop = new Prop2D();
    eprop->setLoc( chx * PIXEL_PER_CHUNK, chy * PIXEL_PER_CHUNK );
    eprop->setScl(PPC,PPC);
    eprop->addGrid(egrid);
    g_electricity_layer->insertProp(eprop);
    

    // fluid
    fgrid = new Grid(CHUNKSZ,CHUNKSZ);
    fgrid->setDeck( g_base_deck );
    fgrid->setFragmentShader( g_eye_col_replacer );
    fprop = new Prop2D();
    fprop->setLoc( chx * PIXEL_PER_CHUNK, chy * PIXEL_PER_CHUNK );
    fprop->setScl(PPC,PPC);
    fprop->addGrid(fgrid);
    fprop->tex_epsilon = 0.0001;
    g_fluid_layer->insertProp(fprop);


    // block
    bgrid = new Grid(CHUNKSZ,CHUNKSZ);
    bgrid->setDeck( g_base_deck );
    bprop = new Prop2D();
    bprop->addGrid(bgrid);
    bprop->setScl(PPC,PPC);
    bprop->setLoc( chx * PIXEL_PER_CHUNK, chy * PIXEL_PER_CHUNK );
    bgrid->setFragmentShader(g_eye_col_replacer);
    g_block_layer->insertProp(bprop);

    // info
    igrid = new Grid(CHUNKSZ,CHUNKSZ);
    igrid->setDeck( g_ascii_deck );
    iprop = new Prop2D();
    iprop->addGrid(igrid);
    iprop->setScl(PPC,PPC);
    iprop->setLoc( chx * PIXEL_PER_CHUNK, chy * PIXEL_PER_CHUNK );
    g_info_layer->insertProp(iprop);
    
}

Chunk::~Chunk() {
    bprop->to_clean = true;
    fprop->to_clean = true;    
    gprop->to_clean = true;
    iprop->to_clean = true;
}

int calcConnectedBlockIndex( int x, int y, BLOCKTYPE bt ) {

    static const int d_ind[16] = { // TDLR
#if 0        
        6,    // 0000
        32+4, // 000R
        32+5, // 00L0
        32+7, // 00LR
        4,    // 0D00
        0,    // 0D0R
        1,    // 0DL0
        32+3, // 0DLR
        5,    // T000
        32+0, // T00R
        32+1, // T0L0
        32+2, // T0LR
        32+6, // TD00
        2,    // TD0R
        3,    // TDL0
        7,    // TDLR
#else
        0,    // 0000
        1,    // 000R
        3,    // 00L0
        2,    // 00LR
        4,    // 0D00
        7,    // 0D0R
        8,    // 0DL0
        12,   // 0DLR
        6,    // T000
        9,    // T00R
        10,   // T0L0
        15,   // T0LR
        5,    // TD00
        14,   // TD0R
        13,   // TDL0
        11,   // TDLR
        
#endif        
    };
    
    Cell *rc = g_fld->get(x+1,y);
    Cell *lc = g_fld->get(x-1,y);
    Cell *tc = g_fld->get(x,y+1);
    Cell *dc = g_fld->get(x,y-1);    

    int bit = 0;
    if( rc && rc->bt == bt ) bit += 1;
    if( lc && lc->bt == bt ) bit += 2;
    if( dc && dc->bt == bt ) bit += 4;
    if( tc && tc->bt == bt ) bit += 8;
    return d_ind[bit];
}

// Make sure a cell has a pole
void ensurePole( Pos2 polepos ) {
    Prop *cur = g_roof_layer->prop_top;
    while(cur){
        Pole *pole = (Pole*) cur;
        if( pole->p == polepos ) { 
            return;
        }
        cur = cur->next;
    }
    //    print("mapview: no polestem! allocating: %d %d", polepos.x, polepos.y );
    //
    Pole *pole = new Pole(polepos);

    pole->updateCables();
}

// Make sure to delete cables that are going in to a pole when deleting the pole.
void deleteCableToPole( Pos2 polepos ) { 
    Prop *cur = g_roof_layer->prop_top;
    while(cur) {
        Pole *pole = (Pole*) cur;
        for(int i=0;i<elementof(pole->destcache);i++) {
            if( pole->destcache[i] == polepos ) {
                //                print("HHHHHHHHHHHAve to clean cable to %d,%d  pole_at:%d,%d", polepos.x, polepos.y, pole->p.x, pole->p.y );
                pole->destcache[i].x = pole->destcache[i].y = -1;
                pole->updateCables();
                break;
            }
        }
        cur = cur->next;
    }
}



// Chunk changed. Call this once and update status of Grid.
void Chunk::update() {
    for(int y=0;y<CHUNKSZ;y++){
        for(int x=0;x<CHUNKSZ;x++){
            int cell_x = x + chx * CHUNKSZ, cell_y = y + chy * CHUNKSZ;
            Cell *c = g_fld->get( cell_x, cell_y );

            if( c->isVent() ) {
                int ind = B_ATLAS_VENT_NORMAL;
                if( c->content >= VENT_READY_CONTENT_THRES ) {
                    ind = B_ATLAS_VENT_EXPLODING;
                }
                ggrid->set(x,y, ind);
            } else if( c->gt == GT_CORE ) {
                Cell *uc = g_fld->get( cell_x, cell_y + 1 );
                Cell *dc = g_fld->get( cell_x, cell_y - 1 );
                Cell *rc = g_fld->get( cell_x + 1, cell_y );
                int ind = c->gt;
                if(dc && dc->gt == GT_CORE ){
                    if( rc && rc->gt == GT_CORE ) ind = B_ATLAS_CORE_LT; else ind = B_ATLAS_CORE_RT;
                } else if(uc && uc->gt == GT_CORE ) {
                    if( rc && rc->gt == GT_CORE ) ind = B_ATLAS_CORE_LD; else ind = B_ATLAS_CORE_RD;
                }
                ggrid->set(x,y, ind );
            } else if( c->gt == GT_SHIFTER ) {
                bool uvrot=false,xfl=false;
                switch(c->dir) {
                case DIR_UP: uvrot =true; break;
                case DIR_DOWN: uvrot = true; xfl = true; break;
                case DIR_RIGHT: break;
                case DIR_LEFT: xfl = true; break;
                case DIR_NONE:
                    break;
                }
                ggrid->set(x,y, c->gt);
                ggrid->setUVRot(x,y,uvrot);
                ggrid->setXFlip(x,y,xfl);
            } else if( c->gt == GT_PIT ) {
                Cell *uc = g_fld->get(cell_x, cell_y+1);
                int ind = c->gt;
                if(uc && uc->gt != GT_PIT ) ind = B_ATLAS_CLIFF;
                ggrid->set(x,y, ind );
            } else {
                ggrid->set(x,y, c->gt);
                ggrid->setXFlip(x,y,false);
                ggrid->setYFlip(x,y,false);
                ggrid->setUVRot(x,y,false);                
            }
            
            

            if( c->bt ) {
                Vec2 texofs(0,0);
                bgrid->setTexOffset(x,y, &texofs );
            }
            

            if( c->bt == BT_CELL ) {
                int ind = B_ATLAS_CELL_BASE + calcConnectedBlockIndex( cell_x, cell_y, BT_CELL );
                bgrid->set(x,y, ind );
            } else if( c->bt == BT_SOIL ) {
                int ind = B_ATLAS_SOIL_BASE + calcConnectedBlockIndex( cell_x, cell_y, BT_SOIL );
                bgrid->set(x,y, ind );
            } else if( c->bt == BT_ROCK ) {
                int ind = B_ATLAS_ROCK_BASE + calcConnectedBlockIndex( cell_x, cell_y, BT_ROCK );
                bgrid->set(x,y, ind );                
            } else if( c->bt == BT_TREE ) {
                ChunkStat *cs = g_fld->getChunkStat(cell_x/CHUNKSZ,cell_y/CHUNKSZ);
                if(cs && cs->isTundra() ) {
                    bgrid->set(x,y,B_ATLAS_TUNDRA_TREE );
                } else {
                    if( c->content > TREE_CONTENT_RIPE_THRES ) {
                        bgrid->set(x,y,B_ATLAS_RIPE_TREE);
                    } else {
                        bgrid->set(x,y,c->bt);
                    }
                }
                
            } else if( c->bt == BT_REACTOR_ARM ) {
                bool xfl = false, uvrot = false;
                switch(c->dir) {
                case DIR_LEFT: break;
                case DIR_RIGHT: xfl = true; break;
                case DIR_UP: uvrot = true; xfl = true; break;
                case DIR_DOWN: uvrot = true; break;
                default:
                    break;
                }
                bgrid->setXFlip(x,y,xfl);
                bgrid->setUVRot(x,y,uvrot);
                bgrid->set(x,y,BT_REACTOR_ARM);
            } else if( c->bt == BT_POLE ) {
                ensurePole( Pos2(cell_x, cell_y) );
                bgrid->set(x,y,BT_POLE);
            } else if( c->bt == BT_CABLE ) {
                bool uvrot, xflip, yflip;
                int rel_ind;
#define CALC_CABLE_FLAGS( x,y, fn ) { \
                    Cell *uc = g_fld->get( x, y + 1 ); \
                    Cell *dc = g_fld->get( x, y - 1 ); \
                    Cell *rc = g_fld->get( x + 1, y ); \
                    Cell *lc = g_fld->get( x - 1, y ); \
                    int ind = 0; /* 4bits: UDRL  */ \
                    if( uc && uc->fn() ) ind += 8; \
                    if( dc && dc->fn() ) ind += 4;  \
                    if( rc && rc->fn() ) ind += 2; \
                    if( lc && lc->fn() ) ind += 1; \
                    int rel_inds[16] = {  0,0,0,0, 0,1,1,2, 0,1,1,2, 0,2,2,3 }; \
                    bool rots[16] =    {  0,0,0,0, 1,0,0,0, 1,0,0,0, 1,1,1,0 }; \
                    bool xfls[16] =    {  0,0,0,0, 0,0,1,0, 0,0,1,0, 0,0,0,0 }; \
                    bool yfls[16] =    {  0,0,0,0, 0,0,0,0, 0,1,1,1, 0,1,0,0 }; \
                    uvrot = rots[ind]; \
                    rel_ind = rel_inds[ind]; \
                    xflip = xfls[ind]; \
                    yflip = yfls[ind]; \
                }

                CALC_CABLE_FLAGS( cell_x, cell_y, isCableConnecting );
                    
                int base = B_ATLAS_CABLE_INACTIVE_LR;
                if( c->content > 0 ) base = B_ATLAS_CABLE_ACTIVE_LR;

                bgrid->set( x,y, base + rel_ind );
                bgrid->setUVRot(x,y, uvrot );
                bgrid->setXFlip(x,y, xflip );
                bgrid->setYFlip(x,y, yflip );
            } else if( c->bt >= BT_ENEMY_BUILDING_START && c->bt <= BT_ENEMY_BUILDING_END ) {
                if( c->bt == BT_DEROTA ) {
                    bgrid->set(x,y,B_ATLAS_DEROTA);
                } else if( c->bt == BT_CAGE ) {
                    int ind = B_ATLAS_CAGE;
                    switch( (BOSSTYPE) c->content) {
                    case BST_DEFAULT: 
                    case BST_GIREV:
                        break;
                    case BST_TAKWASHI: ind = B_ATLAS_TAKWASHI_IN_CAGE; break;
                    case BST_REPAIRER: ind = B_ATLAS_REPAIRER_IN_CAGE; break;
                    case BST_BUILDER: ind = B_ATLAS_BUILDER_IN_CAGE; break;
                    case BST_CHASER: ind = B_ATLAS_CHASER_IN_CAGE; break;
                    case BST_VOIDER: ind = B_ATLAS_VOIDER_IN_CAGE; break;
                    case BST_DEFENDER: ind = B_ATLAS_DEFENDER_IN_CAGE; break;
                    }
                    bgrid->set(x,y,ind );
                } else if(c->bt == BT_ENEMY_FRAME ) {
                    Cell *uc = g_fld->get( cell_x, cell_y + 1 );
                    Cell *dc = g_fld->get( cell_x, cell_y - 1 );
                    Cell *rc = g_fld->get( cell_x + 1, cell_y );
                    Cell *lc = g_fld->get( cell_x - 1, cell_y );
                    int ind = 0; // 4bits: UDRL
                    if( uc && uc->isEnemyFrameEnabler() ) ind += 8;
                    if( dc && dc->isEnemyFrameEnabler() ) ind += 4;
                    if( rc && rc->isEnemyFrameEnabler() ) ind += 2;
                    if( lc && lc->isEnemyFrameEnabler() ) ind += 1;
                    int inds[16] = {
                        B_ATLAS_ENEMY_FRAME_CENTER,     // 0 no frame
                        B_ATLAS_ENEMY_FRAME_RIGHT,      // 1 have only on left side
                        B_ATLAS_ENEMY_FRAME_LEFT,       // 2 have only on right side
                        B_ATLAS_ENEMY_FRAME_LEFT_RIGHT, // 3 have on left and right side
                        B_ATLAS_ENEMY_FRAME_TOP,        // 4 have only on down side
                        B_ATLAS_ENEMY_FRAME_RIGHT_TOP,  // 5 have on down and left side
                        B_ATLAS_ENEMY_FRAME_LEFT_TOP,   // 6 down and right side
                        B_ATLAS_ENEMY_FRAME_TOP,        // 7 except top
                        B_ATLAS_ENEMY_FRAME_BOTTOM,     // 8 only top
                        B_ATLAS_ENEMY_FRAME_RIGHT_BOTTOM,     // 9 top and left
                        B_ATLAS_ENEMY_FRAME_LEFT_BOTTOM,     // 10 top and right
                        B_ATLAS_ENEMY_FRAME_BOTTOM,     // 11 except down
                        B_ATLAS_ENEMY_FRAME_TOP_BOTTOM, // 12 top and down
                        B_ATLAS_ENEMY_FRAME_RIGHT,     // 13 except right
                        B_ATLAS_ENEMY_FRAME_LEFT,    // 14 except left
                        B_ATLAS_ENEMY_FRAME_CENTER,     // 15 all direction               
                    };
                    bgrid->set(x,y, inds[ind] );
                } else if(c->bt == BT_MONO_EYE){
                    int relind = 3-c->getEyeOpenness();
                    bgrid->set(x,y,B_ATLAS_MONO_EYE_OPEN + relind );
                } else if(c->bt == BT_COMBI_EYE) {
                    int relind = 3-c->getEyeOpenness();
                    bgrid->set(x,y,B_ATLAS_COMBI_EYE_OPEN + relind );
                } else if(c->bt == BT_PANEL_EYE) {
                    int relind = 3-c->getEyeOpenness();                    
                    bgrid->set(x,y,B_ATLAS_PANEL_EYE_OPEN + relind );
                } else if(c->bt == BT_ROUND_EYE) {
                    int relind = 3-c->getEyeOpenness();
                    bgrid->set(x,y,B_ATLAS_ROUND_EYE_OPEN + relind );
                } else if( c->bt == BT_SHIELD_EYE ) {
                    bgrid->set(x,y,B_ATLAS_SHIELD_EYE );
                } else if( c->bt == BT_ENEMY_EYE_DESTROYED) {
                    bgrid->set(x,y,B_ATLAS_ENEMY_EYE_DESTROYED );
                } else if( c->bt == BT_CHASER_EYE ) {
                    int relind = 3-c->getEyeOpenness();
                    bgrid->set(x,y,B_ATLAS_CHASER_EYE_OPEN + relind );
                } else if( c->bt == BT_BUILDER_EYE ) {
                    int relind = 3-c->getEyeOpenness();
                    bgrid->set(x,y,B_ATLAS_BUILDER_EYE_OPEN + relind );
                } else if( c->bt == BT_BARRIERGEN ) {
                    bgrid->set(x,y, B_ATLAS_BARRIERGEN);
                } else if( c->bt == BT_BARRIER ) {
                    Cell *uc = g_fld->get( cell_x, cell_y + 1 );
                    Cell *dc = g_fld->get( cell_x, cell_y - 1 );
                    Cell *rc = g_fld->get( cell_x + 1, cell_y );
                    Cell *lc = g_fld->get( cell_x - 1, cell_y );                    
                    bool ud=false,lr=false;
                    ud = ( uc && (uc->bt == BT_BARRIER||uc->bt==BT_BARRIERGEN) ) || ( dc && (dc->bt == BT_BARRIER||dc->bt==BT_BARRIERGEN) );
                    lr = ( rc && (rc->bt == BT_BARRIER||rc->bt==BT_BARRIERGEN) ) || ( lc && (lc->bt == BT_BARRIER||lc->bt==BT_BARRIERGEN) );
                    if( ud && lr ) {
                        bgrid->set(x,y, B_ATLAS_BARRIER_CROSS);
                        bgrid->setUVRot(x,y,false);
                    } else if( lr ) {
                        bgrid->set(x,y, B_ATLAS_BARRIER_LR);
                        bgrid->setUVRot(x,y,false);
                    } else if( ud ) {
                        bgrid->set(x,y, B_ATLAS_BARRIER_LR);
                        bgrid->setUVRot(x,y,true);
                    } else {
                        bgrid->set(x,y, Grid::GRID_NOT_USED );
                    }
                } else if( c->bt == BT_HYPERGEN ) {
                    bgrid->set(x,y,B_ATLAS_HYPERGEN);
                } else if( c->bt == BT_FLYGEN ) {
                    bgrid->set(x,y,B_ATLAS_FLYGEN );
                } else if( c->bt == BT_SLIMEGEN ) {
                    bgrid->set(x,y,B_ATLAS_SLIMEGEN );
                } else if( c->bt == BT_BIRDGEN ) {
                    bgrid->set(x,y,B_ATLAS_BIRDGEN );                    
                } else if( c->bt == BT_COPTERGEN ) {
                    bgrid->set(x,y,B_ATLAS_COPTERGEN );
                } else if( c->bt == BT_CORE_COVER ) {
                    bgrid->set(x,y,B_ATLAS_CORE_COVER );
                } else if( c->bt == BT_GRAY_GOO ) {
                    bgrid->set(x,y,B_ATLAS_GRAY_GOO );
                } else {
                    print("enemy building %d is not implemented", c->bt );
                }
                // enemy block end
            } else if( c->bt == BT_FIREGEN ) {
                int relind = 0;
                switch(c->dir) {
                case DIR_UP: relind = 1; break;
                case DIR_DOWN: relind = 3; break;
                case DIR_LEFT: relind = 0; break;
                case DIR_RIGHT: relind = 2; break;
                default: assertmsg(false, "inval");
                }
                if( c->content > FIREGEN_READY_CONTENT ) relind += 4;
                bgrid->set(x,y,B_ATLAS_FIREGEN_BASE + relind);
            } else if( c->isEarthBlock() || c->isPlayerBuilding() || c->bt == BT_SNOW ){
                bgrid->set(x,y, c->bt );
            } else if( c->bt == BT_IVY ) {
                int ind = B_ATLAS_IVY_SINGLE; 
                if( c->isConnected(DIR_LEFT) && c->isConnected(DIR_DOWN) ) ind = B_ATLAS_IVY_LD;
                else if( c->isConnected(DIR_RIGHT) && c->isConnected(DIR_DOWN) ) ind = B_ATLAS_IVY_RD;
                else if( c->isConnected(DIR_RIGHT) && c->isConnected(DIR_UP) ) ind = B_ATLAS_IVY_RU;
                else if( c->isConnected(DIR_LEFT) && c->isConnected(DIR_UP) ) ind = B_ATLAS_IVY_LU;
                else if( c->isConnected(DIR_LEFT) && c->isConnected(DIR_RIGHT) ) ind = B_ATLAS_IVY_LR;
                else if( c->isConnected(DIR_UP) && c->isConnected(DIR_DOWN) ) ind = B_ATLAS_IVY_UD;
                else if( c->isConnected(DIR_RIGHT) ) ind = B_ATLAS_IVY_R;
                else if( c->isConnected(DIR_LEFT) ) ind = B_ATLAS_IVY_L;
                else if( c->isConnected(DIR_DOWN) ) ind = B_ATLAS_IVY_D;
                else if( c->isConnected(DIR_UP) ) ind = B_ATLAS_IVY_U;

                bgrid->set(x,y, ind );
            } else if( c->bt == BT_AIR ){
                bgrid->set(x,y,Grid::GRID_NOT_USED);
            } else if( c->bt == BT_BOMBFLOWER ) {
                int ind = B_ATLAS_BOMBFLOWER_NORMAL;
                if( c->content > 0 ) ind = B_ATLAS_BOMBFLOWER_MAD;
                bgrid->set(x,y,ind);
            } else if( c->bt == BT_BEEHIVE ) {
                int ind = B_ATLAS_BEEHIVE_NORMAL;
                if( c->content > 0 ) ind = B_ATLAS_BEEHIVE_MAD;
                bgrid->set(x,y,ind);
            } else if( c->bt == BT_CRYSTAL_BEAMER ) {
                int relind=0;
                switch(c->dir) {
                case DIR_UP: relind = 1; break;
                case DIR_DOWN: relind = 3; break;
                case DIR_LEFT: relind = 0; break;
                case DIR_RIGHT: relind = 2; break;
                default: assertmsg(false, "inval");
                }
                int ind = ( c->content >= PAIRBARRIER_CONTENT_THRES ) ? B_ATLAS_CRYSTAL_BEAMER_ACTIVE_BASE : B_ATLAS_CRYSTAL_BEAMER_INACTIVE_BASE;
                bgrid->set(x,y, ind + relind );
            } else if( c->bt == BT_CRYSTAL || c->bt == BT_WEAK_CRYSTAL ) {
                bgrid->set(x,y, c->bt );
            } else if( c->bt == BT_PAIRBARRIER ) {
                bgrid->set(x,y,  c->content >= PAIRBARRIER_CONTENT_THRES ? B_ATLAS_PAIRBARRIER_ACTIVE : B_ATLAS_PAIRBARRIER_INACTIVE );
            } else { 
                print("can't render bt:%d", c->bt );
                bgrid->set(x,y,Grid::GRID_NOT_USED);
            }

            if( c->damage > 0 ) {
                int maxhp = c->getMaxTopHP();
                DurBar::ensure( Pos2(cell_x,cell_y),  maxhp - c->damage, maxhp, g_durbar_deck );
            } else {
                DurBar::clear( Pos2(cell_x,cell_y) );
            }

            if( c->moisture > 0 ) {
                ggrid->setColor( x,y,Color(1,1,1,0.95) );
            } else {
                ggrid->setColor( x,y,Color(1,1,1,1) );
            }

            if( c->powergrid_id > 0 ) {
                egrid->set(x,y, B_ATLAS_ELECTRICITY_AREA );
                egrid->setColor( x,y, Color(1,1,1,0.15) );
            } else {
                egrid->set(x,y, Grid::GRID_NOT_USED );
            }

#if 0            
            if( c->powergrid_id > 0 ) {
                igrid->set(x,y, c->powergrid_id );
                igrid->setColor(x,y, Color(1,1,1,0.3) );
            } else {
                igrid->set(x,y, Grid::GRID_NOT_USED );
            }
#endif
            


#if 0            
            if( c->hyper_count > 0 ) {
                igrid->set(x,y, c->hyper_count );
                igrid->setColor(x,y, Color(1,1,1,0.3) );
            } else {
                igrid->set(x,y, Grid::GRID_NOT_USED );
            }
#endif
            
#if 0
            if( c->moisture > 0 ) {
                igrid->set(x,y, c->moisture % 0xf);
                igrid->setColor(x,y, Color(1,1,1,0.3) );
            }
#endif            

#if 0
            if( c->bt == BT_IVY ) {
                print("ivy: xy:%d,%d dir:%d", x,y,c->dir);
                igrid->set(x,y, (int)c->dir );
                igrid->setColor(x,y, Color(1,1,1,0.6) );                
            } else {
                igrid->set(x,y, Grid::GRID_NOT_USED);
            }
#endif
                
#if 0
            if( c->content > 0 ) {
                igrid->set(x,y, c->content );
                igrid->setColor(x,y, Color(1,1,1,0.3) );                
            } else {
                igrid->set(x,y, Grid::GRID_NOT_USED);
            }
#endif
            

#if 0
            if( c->fortress_id > 0 ) {
                igrid->set(x,y, c->fortress_id & 0xf );
                igrid->setColor(x,y, Color(1,1,1,0.3) );
            } else {
                igrid->set(x,y, Grid::GRID_NOT_USED );
            }
#endif

#if 0
            // grid id
            if( c->bt == BT_POLE ) {
                Pos2 p(cell_x,cell_y);
                PowerGrid *g = g_ps->findGrid(p);
                if(g) {
                    igrid->set(x,y, g->id);
                    igrid->setColor(x,y, Color(1,1,1,0.3) );
                } else {
                    igrid->set(x,y, Grid::GRID_NOT_USED );
                }
            } else {
                igrid->set(x,y, Grid::GRID_NOT_USED );
            }
#endif
                
#if 0
            // node id
            if( c->bt == BT_POLE ) {
                Pos2 p(cell_x,cell_y);
                PowerGrid *g = g_ps->findGrid(p);
                if(g) {
                    PowerNode *n = g->findNode(p);
                    if(n) {
                        igrid->set(x,y, n->id);
                        igrid->setColor(x,y, Color(1,1,1,0.3) );
                    } else {
                        igrid->set(x,y, Grid::GRID_NOT_USED );                        
                    }
                } else {
                    igrid->set(x,y, Grid::GRID_NOT_USED );
                }
            } else {
                igrid->set(x,y, Grid::GRID_NOT_USED );
            }
#endif            
            
        }
    }
}


void Chunk::updateMovers() {
    for(int y=0;y<CHUNKSZ;y++){
        for(int x=0;x<CHUNKSZ;x++){
            int cell_x = x + chx * CHUNKSZ, cell_y = y + chy * CHUNKSZ;
            Cell *c = g_fld->get( cell_x, cell_y );

            if(c->isWater() ) {
                int center_ind = ( c->st == ST_WATER ) ? B_ATLAS_WATER_CENTER : B_ATLAS_BLOODWATER_CENTER;
                fgrid->set( x,y, center_ind );
                Vec2 ofsv(0,0);
                ofsv.x = sin( g_now_time*2)/4;
                ofsv.y = 0;
                fgrid->setColor( x,y, Color(1,1,1,0.7) );                
                fgrid->setTexOffset( x,y, &ofsv );
            } else if( c->st == ST_WEED || c->st == ST_ENERGIUM ||
                       c->st == ST_HYPERLINE || c->st == ST_GROUNDWORK || c->st == ST_ROCK ||
                       c->st == ST_ICE 
                       ) {
                fgrid->set( x,y, c->st );
                Vec2 ofsv(0,0);
                fgrid->setTexOffset( x,y, &ofsv );
                fgrid->setColor( x,y, Color(1,1,1,1) );
            } else if( c->st == ST_CORE_CABLE_INACTIVE || c->st == ST_CORE_CABLE_ACTIVE ) {
                bool uvrot, xflip, yflip;
                int rel_ind;
                CALC_CABLE_FLAGS( cell_x, cell_y, isCoreCableConnecting );
                int ind = c->st == ST_CORE_CABLE_INACTIVE ? B_ATLAS_CORE_CABLE_INACTIVE_BASE : B_ATLAS_CORE_CABLE_ACTIVE_BASE;
                ind += rel_ind;
                fgrid->set(x,y,ind);
                fgrid->setXFlip( x,y, xflip );
                fgrid->setYFlip( x,y, yflip );
                fgrid->setUVRot( x,y, uvrot );
            } else if( c->st == ST_CORE_SWITCH_INACTIVE || c->st == ST_CORE_SWITCH_ACTIVE ) {
                fgrid->set( x,y, c->st );
            } else {
                fgrid->set( x,y, Grid::GRID_NOT_USED );
                fgrid->setXFlip( x,y, false );
                fgrid->setYFlip( x,y, false );
                fgrid->setUVRot( x,y, 0 );                
            }
        }
    }
}

////////////////

MapView::MapView( int w, int h ) {
    
    chw = w / CHUNKSZ;
    chh = h / CHUNKSZ;
    
    size_t sz = sizeof(Chunk*) * chw * chh;
    chunks = (Chunk**) MALLOC( sz );
    memset( chunks, 0, sz );

    active_chunks = (Chunk**) MALLOC( sz );
    memset( active_chunks, 0, sz );
    
}

void MapView::poll( Vec2 center, float zoom_rate ) {
    //    scroll_ofs = gprop->accum_time - floor(gprop->accum_time);
    float xrange = SCRW/zoom_rate/2/PIXEL_PER_CHUNK, yrange = SCRH/zoom_rate/2/PIXEL_PER_CHUNK;
//    print("xr:%f yr:%f", xrange, yrange );
    int minchx = (int)(center.x/PIXEL_PER_CHUNK) - xrange - 1;
    int minchy = (int)(center.y/PIXEL_PER_CHUNK) - yrange - 1;
    int maxchx = (int)(center.x/PIXEL_PER_CHUNK) + xrange + 1;
    int maxchy = (int)(center.y/PIXEL_PER_CHUNK) + yrange + 1;

    //        print("xx: %d %d %d %d %d %d",minchx, minchy, maxchx, maxchy, chw, chh );
    for(int y=minchy;y<=maxchy;y++) {
        if(y<0|| y>=chh)continue;
        for(int x=minchx;x<=maxchx;x++){
            if( x<0||x>=chw)continue;

            int ind = x + y * chw;
            Chunk *chk = chunks[ind];
            if(!chk) {
                //                print("alloc new chunk:%d,%d",x,y);
                chk = chunks[ind] = allocateChunk(x,y);
                chk->changed = true;
                g_fld->setChunkRevealed(x,y,true);
                if(g_current_project_id>0) {
                    realtimeRevealSend(x,y);
                }
            }
            if( chk->changed ) {
                chk->update();
                chk->changed = false;
            }
            chk->updateMovers();
            
            // Adjust polygon size slightly to avoid having too wide niches between polygons
            if( g_zoom_rate < 1.0 ) {
                chk->fgrid->enfat_epsilon = 0.0000001;
            } else {
                chk->fgrid->enfat_epsilon = 0;
            }
        }
    }
    checkCleanChunks( minchx, minchy, maxchx, maxchy );


    
}

Chunk *MapView::getChunk( int chx, int chy ) {
    if( chx<0||chy<0||chx>=chw||chy>=chh) return NULL;
    int ind = chx + chy * chw;
    return chunks[ind];
} 
Chunk *MapView::allocateChunk( int chx, int chy) {
    int vacant_ind = -1;
    for(int i=0;i<chw*chh;i++){
        if( active_chunks[i] == NULL ) {
            vacant_ind = i;
            break;
        }
    }
    assert( vacant_ind >= 0 );
    Chunk *chk = new Chunk( chx, chy );
    active_chunks[vacant_ind] = chk;

    return chk;
}
void MapView::checkCleanChunks( int minchx, int minchy, int maxchx, int maxchy ) {
    // Clean a chunk that is far distance and still active
    for(int i=0;i<chw*chh;i++){
        Chunk *chk = active_chunks[i];
        if(chk &&
           (( chk->chx < minchx ) ||
            ( chk->chx > maxchx ) ||
            ( chk->chy < minchy ) ||
            ( chk->chy > maxchy )   )
           ){
            chunks[chk->chx + chk->chy*g_fld->chw]=NULL;
            delete chk;
            active_chunks[i] = NULL;
        }
    }
}

void MapView::notifyChanged( int chx, int chy ) {
    Chunk *chk = getChunk(chx,chy);
    if(chk) {
        chk->changed = true;
    }
}
void MapView::notifyChanged( Vec2 at ) {
    notifyChanged( at.x / PPC / CHUNKSZ, at.y / PPC / CHUNKSZ );
}
void MapView::notifyChangedAll() {
   for(int i=0;i<chw*chh;i++){
        Chunk *chk = active_chunks[i];
        if(chk) {
            chk->changed = true;
        }
   }
}


//////////////////

DurBar::DurBar(Pos2 at, TileDeck *dk ) : Prop2D(), at(at), target_char_id(0) {
    setDeck(dk);
    setIndex(0);
    setLoc( at.x * PPC + PPC/2, at.y * PPC + PPC );
    setScl(24,4);
    tex_epsilon = 0.01;
    g_durbar_layer->insertProp(this);
}
bool DurBar::prop2DPoll( double dt ) {
    if( accum_time > last_update + 2 ){
        return false;
    }
    if( target_char_id ) { 
        Enemy *e = (Enemy*) g_char_layer->findPropById(target_char_id);
        if(e) loc = e->loc + Vec2(0,PPC);
    }
    return true;
}

DurBar* DurBar::ensure( Pos2 at, int curhp, int maxhp, TileDeck *dk ) {
    if( curhp < 0 ) curhp = 0;
    Prop *cur = g_durbar_layer->prop_top;
    while(cur) {
        DurBar *b = (DurBar*)cur;
        if(b->at == at && b->deck == dk ) {
            b->update( curhp, maxhp );
            return b;
        }
        cur = cur->next;
    }
    // Not initialized. create new one.
    DurBar *b = new DurBar(at, dk );
    b->update( curhp, maxhp );
    return b;
}
DurBar* DurBar::ensure( Char *tgt, int curhp, int maxhp, TileDeck *dk ) {
    if( curhp < 0 ) curhp = 0;
    Prop *cur = g_durbar_layer->prop_top;
    while(cur) {
        DurBar *b = (DurBar*)cur;
        if(b->target_char_id == tgt->id && b->deck == dk ) {
            b->update( curhp, maxhp );
            return b;
        }
        cur = cur->next;
    }
    DurBar *b = new DurBar(Pos2(-1,-1), dk);
    b->update(curhp, maxhp);
    b->target_char_id = tgt->id;
    return b;
}

void DurBar::clear( Pos2 at ) {
    Prop *cur = g_durbar_layer->prop_top;
    while(cur) {
        DurBar *b = (DurBar*)cur;
        if(b->at == at ) {
            b->to_clean = true;
            return;
        }
        cur = cur->next;
    }    
}
void DurBar::clear( Char *tgt ) {
    Prop *cur = g_durbar_layer->prop_top;
    while(cur) {
        DurBar *b = (DurBar*)cur;
        if(b->target_char_id == tgt->id ) {
            b->to_clean = true;
            return;
        }
        cur = cur->next;
    }    
    
}
void DurBar::update( int curhp, int maxhp ) {
    float r = (float) curhp  / (float) maxhp;
    int ind = r * 32;
    if( ind >= 32 ) ind = 31;
    setIndex(ind);
    last_update = accum_time;
}


/////////////////////

Pole::Pole( Pos2 p ) : Prop2D(), p(p) {
    setLoc( toCellCenter( Vec2(p.x*PPC,p.y*PPC) ) );
    setDeck(g_base_deck);
    setIndex( B_ATLAS_POLE_STEM );
    setScl(PPC);
    draw_offset.y = POLE_STEM_Y_DIFF;
    g_roof_layer->insertProp(this);
}

// 
bool Pole::prop2DPoll( double dt) {
    Cell *c = g_fld->get(loc);
    assert( c  );
    if( c->bt != BT_POLE ) {
        Pos2 p(loc.x/PPC, loc.y/PPC);
        print("mapview: Pole: bt_pole is not found at %d,%d",p.x,p.y);
        deleteCableToPole(p);
        return false;
    }

    PowerGrid *g = g_ps->findGridById( c->powergrid_id );
    if(!g) {
        print("pole %d,%d: pgid %d is not found ", p.x, p.y , c->powergrid_id );
        return true;
    }

    int nprim = getPrimNum();
    for(int i=0;i<nprim;i++){
        Prim *p = getPrim(i);
        p->color = g->getColor();
    }

    if( g->last_gen_at > g->accum_time - 1 && updateInterval(0, 1.0f) ) {
        createChargeEffect( loc + Vec2(0,24), 1, false);
    }

    return true;
}
void Pole::addCable(Pos2 rel_to ) {
    Vec2 from(0,PPC-2);
    Vec2 to = Vec2( rel_to.x*PPC,rel_to.y*PPC+PPC-2);
    Color c(0.5,1,0.7,1);
    catenaryCurve( this, from, to, c, 2 );
}

void catenaryCurve( Prop2D *p, Vec2 from, Vec2 to, Color c, float width ) {
    Vec2 quarter = (from*0.75+to*0.25);
    quarter.y -= 6;
    p->addLine( from, quarter, c );
    Vec2 mid = (from + to)/2;
    mid.y -= 8;
    p->addLine( quarter, mid, c );
    Vec2 q3 = (from*0.25+to*0.75);
    q3.y -= 6;
    p->addLine( mid, q3, c );
    p->addLine( q3, to, c );
}

void Pole::updateCables() {
    clearPrims();
    
    // power cable line
    PowerNode *pn = g_ps->findNode(p);
    if(pn) {
        for(int i=0;i<elementof(pn->dests);i++) {
            if( i< elementof( destcache) ) {
                destcache[i] = pn->dests[i];
            }
            if( pn->dests[i].x >= 0 ) {
                addCable( pn->dests[i] - p );
                //                print("addcbl from:%d,%d to:%d,%d",p.x,p.y, pn->dests[i].x, pn->dests[i].y );
            }
        }
    }
}
/////////////////
// Show links between exchange terminals
ExchangeLink g_exlinks[128]; // TODO: Enough? this could be adjustable.

void clearExchangeLinks() {
    for(int i=0;i<elementof(g_exlinks);i++) {
        g_exlinks[i] = ExchangeLink();
    }
}
ExchangeLink *findExchangeLink( Pos2 at ) {
    for(int i=0;i<elementof(g_exlinks);i++) {
        if( g_exlinks[i].from == at ) {
            return & g_exlinks[i];
        }
    }
    return NULL;
}

int getNearExchangeLinks( Pos2 from, Pos2 out[3] ) {
    SorterEntry sents[elementof(g_exlinks)];
    int si = 0;
    for(int i=0;i<elementof(g_exlinks);i++) {
        if( g_exlinks[i].from.x >= 0 && g_exlinks[i].from != from ) {
            sents[si].val = from.len(g_exlinks[i].from );
            sents[si].ptr = &g_exlinks[i];
            si++;
        }
    }
    quickSortF( sents, 0, si-1);
    // quickSortF sorts the smallest element first.
    for(int i=0;i<MIN(3,si);i++){
        out[i] = ((ExchangeLink*) sents[i].ptr)->from;
    }
    if(si>2) return 2; else return si;
}

void addExchangeLink( Pos2 at ) {
    ExchangeLink *l = findExchangeLink(at);
    if(l)return;
    for(int i=0;i<elementof(g_exlinks);i++) {
        if( g_exlinks[i].from.x == -1 ) {
            g_exlinks[i].from = at;
            Pos2 p[3];
            int n = getNearExchangeLinks( at, p );
            for(int j=0;j<n;j++) g_exlinks[i].to[j] = p[j];
            print("adding exlink: %d,%d - %d,%d %d,%d %d,%d", at.x, at.y, p[0].x, p[0].y, p[1].x, p[1].y, p[2].x, p[2].y );
            return;
        } 
    }    
}

void refreshExchangeLinks() {
    clearExchangeLinks();
    PowerGrid *pg = g_ps->grid_top;
    while(pg){
        PowerEquipmentNode *pe = pg->equip_top;
        while(pe) {
            if( pe->type == PEQT_EXCHANGE ) {
                addExchangeLink( pe->at );
            }
            pe = pe->next;
        }
        pg = pg->next;
    }

    for(int k=0;k<2;k++) {
        for(int i=0;i<3;i++) {
            Pos2 p[3];
            int n;
            if( g_exlinks[i].from.x >= 0 ) {
                n = getNearExchangeLinks( g_exlinks[i].from, p );
                for(int j=0;j<n;j++) g_exlinks[i].to[j] = p[j];
                print("adding exlink 2: n:%d, %d,%d - %d,%d %d,%d %d,%d",
                      n, g_exlinks[i].from.x, g_exlinks[i].from.y, p[0].x, p[0].y, p[1].x, p[1].y, p[2].x, p[2].y );            
            }
        }
    }
}

void exchangeEffect( Pos2 at ) {
    ExchangeLink *l = findExchangeLink(at);
    if(l) {
        int i = irange(0,elementof(l->to));
        if( l->to[i].x >= 0 ) {
            Vec2 from = toCellCenter(l->from), to = toCellCenter(l->to[i]);
            if( range(0,100)<50) {
                Vec2 tmp = from;
                from = to;
                to = tmp;
            }
            createExchangeLinkBall( from,to);
        }
    }
}
// Show more particle if it's near to PC
void pollExchangeEffect() {
    Pos2 from = vec2ToPos2( g_pc->loc );
    // near ones
    if( range(0,100) < 10 ) {
        Pos2 nears[10];
        int ni=0;
        float thres = 50;
        for(int i=0;i<elementof(g_exlinks);i++) {
            if( g_exlinks[i].from.x>=0 && from.len(g_exlinks[i].from) < thres ) {
                nears[ni] = g_exlinks[i].from;
                ni++;
            }
        }
        int ind = irange(0,ni);
        exchangeEffect( nears[ind] );
    }
    // distant ones
    if( range(0,100) < 10) {
        for(int i=0;i<10;i++) {
            int ind = range(0,elementof(g_exlinks));
            if( g_exlinks[ind].from.x >= 0 ) {
                exchangeEffect( g_exlinks[ind].from );
                break;
            }
        }
    }

    
}
