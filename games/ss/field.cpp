#include "moyai/client.h"
#include "dimension.h"
#include "conf.h"
#include "ids.h"
#include "noise.h"
#include "item.h"
#include "util.h"
#include "net.h"
#include "field.h"
#include "mapview.h"
#include "char.h"
#include "hud.h"
#include "pc.h"
#include "atlas.h"
#include "pwgrid.h"
#include "effect.h"
#include "enemy.h"
#include "globals.h"

int GroundToScore( GROUNDTYPE gt ) {
    switch(gt) {
    case GT_NUCLEUS: return 2000;
    case GT_ENHANCER: return 1500;
    case GT_SHRIMPGEN: return 2000;
    case GT_CORE: return 200000;
    default: return 0;
    }
}

int BlockToScore( BLOCKTYPE bt ) {
    switch(bt) {
    case BT_ROCK: return 100;
    case BT_SOIL: return 30;
    case BT_SNOW: return 0;
    case BT_IVY: return 30;
    case BT_CELL: return 10;
    case BT_TREE: return 20;
    case BT_CRYSTAL: return 0;
    case BT_PAIRBARRIER: return 0;
    case BT_WEAK_CRYSTAL: return 1000;
    
    
    case BT_IRONORE: return 200;
    case BT_RAREMETALORE: return 200;

    case BT_HARDROCK: return 500;
    case BT_FIREGEN: return 1000;
    case BT_BOMBFLOWER: return 100;
    case BT_BEEHIVE: return 1000;
    case BT_CRYSTAL_BEAMER: return 0;

    // Enemy ground structures
    case BT_DEROTA: return 500;
    case BT_ENEMY_FRAME: return 0;
    case BT_ENEMY_EYE_DESTROYED: return 0;
    case BT_CAGE: return 500;
    case BT_BARRIERGEN: return 500;
    case BT_BARRIER: return 0;
    case BT_HYPERGEN: return 20000;
    case BT_FLYGEN: return 500;
    case BT_COPTERGEN: return 2000;
    case BT_SLIMEGEN: return 1500;
    case BT_BIRDGEN: return 3000;
    case BT_CORE_COVER: return 0;
    case BT_GRAY_GOO: return 300;
    
    // eyes
    case BT_MONO_EYE: return 2000;
    case BT_COMBI_EYE: return 8000;
    case BT_PANEL_EYE: return 5000;
    case BT_ROUND_EYE: return 6000;
    case BT_SHIELD_EYE: return 7000;
    case BT_CHASER_EYE: return 5000;
    case BT_BUILDER_EYE: return 8000;
    default: return 0;
    }
}
        
/////////////////////

Field::Field( int w, int h ) : width(w), height(h), chw(w/CHUNKSZ), chh(h/CHUNKSZ), loc_max(w*PPC,h*PPC),
                               saved_chunk_this_sim(0),
                               simchunk_per_poll(200), sim_counter(0), last_simclock_at(0), sim_clock_count(0),
                               cur_chunk_change_cnt(0), last_chunk_change_cnt(0), 
                               generate_step(0), generate_counter(0), fortress_id_generator(0),
                               build_fortress_log_num(0)
{
    size_t sz = sizeof(Cell) * w * h;
    cells = (Cell*) MALLOC( sz );

    sz = getRevealBufferSize();
    revealed = (bool*) MALLOC(sz);

    sz = sizeof(ChunkStat) * chw * chh;
    chstats = (ChunkStat*) MALLOC(sz);

    sz = sizeof(CHUNKLOADSTATE) * chw * chh;
    load_state = (CHUNKLOADSTATE*) MALLOC(sz);

}
size_t Field::getRevealBufferSize() {
    return sizeof( revealed[0] ) * chw * chh;
}

void Field::clear(){
    memset( cells, 0, width*height*sizeof(Cell) );
    for( int y=0;y<height;y++){
        for(int x=0;x<width;x++){
            Cell *c = get(x,y);
            c->bt = BT_AIR;
            c->gt = GT_SOIL;
            c->st = ST_NONE;
            c->randomness = range(0,Cell::RANDOMNESS_MAX);
            c->damage = 0;
            c->content = 0;
            c->moisture = 0;
            c->dir = DIR_NONE;
            c->powergrid_id = 0;
            c->fortress_id = 0;
            c->ftstate = FTS_NOT_EYE;
            c->hyper_count = 0;
            c->untouched = true;
        }
    }

    memset( revealed, 0, chw * chh * sizeof(bool) );
    memset( chstats, 0, chw * chh * sizeof(ChunkStat) );
    memset( load_state, 0, chw * chh * sizeof(CHUNKLOADSTATE) );

    sim_clock_count = 0;
    load_poll_count = 0;

    for(int i=0;i<elementof(to_sim);i++)  to_sim[i].clear();

    memset( envs, 0, sizeof(envs) );
    memset( build_fortress_log, 0, sizeof(build_fortress_log));
    build_fortress_log_num = 0;
    print("field init done");
}
bool Field::isFlagPuttableCell( Pos2 p ) {
    if( p.x<=5 || p.y<=5 || p.x >= FIELD_W-5 || p.y >= FIELD_H-5) return false;
    Cell *c = get(p), *uc = get(p.up()), *dc = get(p.down()), *lc = get(p.left()), *rc = get(p.right());
    if(c->gt == GT_PIT || uc->gt == GT_PIT || dc->gt == GT_PIT || lc->gt == GT_PIT || rc->gt == GT_PIT ) return false;
    if(c->isAbsolutelyImmutable() || dc->isAbsolutelyImmutable() || uc->isAbsolutelyImmutable() || rc->isAbsolutelyImmutable()
       || lc->isAbsolutelyImmutable() ) return false;
    return true;
}


void Field::setupFlagCands() {
    int mgn = 10;
    for(int i=0;i<elementof(flag_cands);i++) {
        Pos2 p( irange(mgn,width-mgn), irange(mgn,height-mgn) );
        if( p.len( getRespawnPoint() ) < 50 ) { i--; continue; }
        if( isFlagPuttableCell(p) == false ) {i--; continue;}
        flag_cands[i].finished = false;
        flag_cands[i].pos = p;
    }

    const int FIRST_FLAG_INDEX = 0;
    const int CORE_FLAG_INDEX_START = 1; // A field has 4 cores. So use index 1,2,3,4
    const int FORTRESS_FLAG_INDEX_START = 5;
    
    // Fortress
    for(int i=0;i<MILESTONE_MAX/2;i++) {
        int log_ind = irange(0, build_fortress_log_num );
        Pos2 p = build_fortress_log[log_ind];
        if( isFlagPuttableCell(p)) {
            flag_cands[FORTRESS_FLAG_INDEX_START+i].pos = p;
        }
    }
    
    // Cores
    int core_ind=0;
    for(int y=1;y<FIELD_H-1;y++) {
        for(int x=1;x<FIELD_W-1;x++) {
            Cell *c = get(x,y);
            Cell *uc = get(x,y+1), *lc = get(x-1,y);
            if(c && c->gt == GT_CORE && uc && uc->gt != GT_CORE && lc && lc->gt != GT_CORE ) {
                // Core must have a flag
                flag_cands[CORE_FLAG_INDEX_START+core_ind].pos = Pos2(x,y);
                core_ind++;
                assert(core_ind<=4);
            }
        }
    }

    // The first flag is fixed
    flag_cands[FIRST_FLAG_INDEX].pos = getFirstFortressPoint() + Pos2(4,4);
}

void Field::startGenerate() {
    generate_step = 1;
    generate_counter = 0;
}
bool Field::isGenerateStarted() {
    return generate_step > 0;
}
// Returns true if generation is finished.
bool Field::asyncGenerate() {
    // Divide to 4x4 = 16
    int unit_w = width / 8;
    int unit_h = height / 8;
    assert( width % unit_w == 0 );
    assert( height % unit_h == 0 );

    // VOLCANO : v  TUNDRA : t JUNGLE:j DUNGEON:D DESERT : d
    const char *biomes[8] = {
        "tmvtltvv",
        "vtmlmlvm",
        "vddjdttm",
        "djllmvdt",
        "mdllvdml",
        "djjddtdv",
        "ljmdvvjm",
        "dmvvtdjm",
    };
    int maxbx = 8, maxby = 8;
    
    switch( generate_step ) {
    case 1: // init
        clear();
        generate_step++;
        return false;
    case 2: // GT
    case 3: // blur GT
    case 4: // BT
    case 5: // blur BT
        {

            int bx = generate_counter % maxbx;
            int by = generate_counter / maxby;
            generate_counter ++;
            if( generate_counter == (maxbx * maxby + 1) ) {
                print("generate step %d finished", generate_step );
                generate_step ++;
                generate_counter = 0;
                return false;
            }
#define DEBUG_QUICK_GENERATE 0
            
#if DEBUG_QUICK_GENERATE
            if( bx >= 1 || by >= 1 ) return false;
#endif
            
            int lbx = bx * unit_w;
            int lby = by * unit_h;
            Rect r( Pos2(lbx,lby), Pos2( lbx + unit_w-1, lby + unit_h-1) );
            char ch = biomes[7-by][bx];
            BIOMETYPE bmt = BMT_DESERT;
            switch(ch) {
            case 'v': bmt = BMT_VOLCANO; break;
            case 'j': bmt = BMT_JUNGLE; break;
            case 't': bmt = BMT_TUNDRA; break;
            case 'd': bmt = BMT_DESERT; break;
            case 'l': bmt = BMT_LAKE; break;
            case 'm': bmt = BMT_MOUNTAIN; break;
            default:break;
            }
            if(generate_step==2) {
                setEnvironment( r, bmt );
                generateBiomeGround( r, bmt );
            } else if(generate_step==3) {
                if( r.min.x > 0 || r.min.y > 0 ) {
                    blurBiomeGround(r,DIR_DOWN);
                    blurBiomeGround(r,DIR_LEFT);                        
                }
            } else if(generate_step==4) {
                float l = r.center().len( getRespawnPoint() );
                generateBiomeBlocks( r, bmt, l );
            } else if(generate_step==5) {
                if( r.min.x > 0 || r.min.y > 0 ) {
                    blurBiomeBlock(r,DIR_DOWN);
                    blurBiomeBlock(r,DIR_LEFT);
                }
            }
        }
        return false;
    case 6: 
        generateCommon();
        generate_step++;
        return false;
    case 7: // clean and finish
        setupFlagCands();        
        {
            Pos2 rp = getRespawnPoint();
            revealRange(rp.x/CHUNKSZ,rp.y/CHUNKSZ,2);
        }
        statistics();
        generate_step++;
        print("generate finished" );            
        return false;
    case 8:
        if( g_enable_debug_menu ) {
            Image *img = g_fld->dumpNewImage();
            img->writePNG( "dumped.png" );
            delete img;
        }

        generate_step++;
        return false;
    case 9:
        assert( flag_cands[0].pos.x != 0 && flag_cands[0].pos.y != 0 );
        return true;
    }
    return false;
}
void Field::generateSync() {
    startGenerate();
    while(true) {
        if( asyncGenerate() == true ) break;
    }
}
void Field::revealRange( int chx, int chy, int mgn ) {
    for(int y = chy - mgn; y <= chy + mgn; y ++ ) {
        for(int x = chx - mgn; x <= chx + mgn; x ++ ) {
            setChunkRevealed(x,y,true);
        }
    }
}
void Field::statistics() {
    int eye=0, iron=0, raremetal=0, hypergen=0, ebld=0, ene=0;
    for(int y=0;y<height;y++) {
        for(int x=0;x<width;x++) {
            Cell *c = get(x,y);
            if( c->isEnemyEye() ) eye++;
            if( c->bt == BT_IRONORE ) iron++;
            else if( c->bt == BT_RAREMETALORE ) raremetal++;
            else if( c->bt == BT_HYPERGEN ) hypergen++;
            else if( c->isEnemyBuilding() ) ebld ++;
            if( c->st == ST_ENERGIUM ) ene++;
        }
    }
    print( "Field::statistics: iron:%d raremetal:%d ene:%d eye:%d hypergen:%d ebld:%d ", iron, raremetal, ene, eye, hypergen, ebld );
}

void Field::blurBiomeGround( Rect rect, DIR d ) {
    //    print("blurBiome: min:%d,%d", rect.min.x, rect.min.y );
    Pos2 candpos[1024];
    int canddia[1024];
    GROUNDTYPE candgt[1024];
    SURFACETYPE candst[1024];
    int candn = 0;
    
    int n = rect.width();
    for(int i=0;i<n;i++){
        Pos2 bottomp = rect.getRandomEdge(d);
        int mgn = 7;
        Pos2 dp( irange(-mgn,mgn+1), irange(-mgn,mgn+1) );        
        Cell *c = get(bottomp+dp);
        if(c) {
            candpos[candn] = bottomp + dp;
            canddia[candn] = irange(1,mgn-2);
            candgt[candn] = c->gt;
            candst[candn] = c->st;
            candn++;
            if( candn >= elementof(candpos) ) break;
        }
    }
    for(int i=0;i<candn;i++) {
        fillCircleGSB( candpos[i], canddia[i], candgt[i], candst[i], BT_AIR );
    }
}
void Field::blurBiomeBlock( Rect rect, DIR d ) {
    int n = rect.width();
    for(int i=0;i<n;i++){
        Pos2 bottomp = rect.getRandomEdge(d);
        int mgn = 7;
        Pos2 dp( irange(-mgn,mgn+1), irange(-mgn,mgn+1) );
        setBlockCircleOn( bottomp+dp, irange(1,5), BT_SNOW, BT_AIR, 1.0 ); 
    }
}

void Field::generateBiomeGround( Rect rect, BIOMETYPE bmt ) {    
    int w = rect.width();
    int h = rect.height();
    //    print("generateBiome: w,h:%d,%d min:(%d,%d) bmt:%d",w,h, rect.min.x, rect.min.y,bmt );
    
    // Ground, Surface, and Blocks
    NoiseTable *nt = new NoiseTable(w,h);
    nt->noise( 32,0,1 );
    NoiseTable *wt = new NoiseTable(w,h);
    wt->noise( 32,0,1 );

    for( int y=0;y<h;y++){
        for(int x=0;x<w;x++){
            Pos2 p( rect.min.x + x, rect.min.y + y);
            Cell *c = get(p);

            float r;
            float deep_water_thres = 0.7;
            float water_thres = 0.6;
            float shore_thres = 0.55;
            float land_thres = 0.35;
            
            if( bmt == BMT_JUNGLE ) {
                water_thres = 0.55;
                shore_thres = 0.52;
                land_thres = 0.35;
            } else if( bmt == BMT_LAKE ) {
                water_thres = 0.40; // More water
                shore_thres = 0.52;
                land_thres = 0.35;
            }


            // 水面
            r = wt->get(x,y);
            if( r > deep_water_thres ) {
                switch( bmt ) {
                case BMT_JUNGLE: c->st = ST_BLOODWATER; break;
                default: c->st = ST_WATER; break;
                }
                c->gt = GT_DEEP;
            } else if( r > water_thres ) {
                switch( bmt ) {
                case BMT_JUNGLE: c->st = ST_BLOODWATER; break;
                default: c->st = ST_WATER; break;                
                }
            } else if( r > shore_thres  ) {
                if(bmt == BMT_TUNDRA || bmt == BMT_MOUNTAIN ) {
                    c->gt = GT_SNOW;
                } else {
                    c->gt = GT_SAND;
                }
            } else if( r > land_thres ){
                switch(bmt) {
                case BMT_VOLCANO:
                    c->gt = GT_LAVA_COLD;
                    break;
                case BMT_TUNDRA:
                case BMT_MOUNTAIN:
                    c->gt = GT_SNOW;
                    break;
                case BMT_JUNGLE:
                    c->gt = GT_JUNGLE;
                    break;
                default:
                    c->gt = GT_SOIL;
                    break;
                }
            } else {
                if( bmt == BMT_JUNGLE ) {
                    c->gt = GT_SOIL;
                } else {
                    c->gt = GT_ROCK;
                }
            }
            
            // Walls
            if( c->st == ST_NONE ) {
                r = nt->get(x,y);            
                if( bmt == BMT_MOUNTAIN ) {
                    if( r > 0.7 ) {
                        c->bt = BT_HARDROCK;
                    } else if( r > 0.55 ) {
                        c->bt = BT_ROCK;
                    } else if( r > 0.45 ) {
                        c->bt = BT_SOIL;
                    } else {
                        c->bt = BT_AIR;
                    }
                } else {
                    if( r > 0.7) {
                        c->bt = BT_ROCK;
                    } else if( r > 0.6) {
                        c->bt = BT_SOIL;
                    } else {
                        c->bt = BT_AIR;
                    }
                }
            }
        }
    }

    // Rivers
    int nriver = (h*w)/10000, river_built=0;
    if( bmt == BMT_JUNGLE )nriver *= 3;
    for(int i=0;i<nriver*100;i++) {
        Pos2 p = rect.getRandomPos();
        Cell *c = get(p);
        if(c && c->isWater() && c->gt == GT_DEEP ) {
            if( p.len(getRespawnPoint()) < 40 ) continue;
            buildRiver(p, irange(200,1000), bmt == BMT_JUNGLE ? ST_BLOODWATER : ST_WATER );
            river_built ++;
            if( river_built >= nriver ) break;
        }
    }
}
void Field::generateBiomeBlocks( Rect rect, BIOMETYPE bmt, float distance_to_rp ) {
    float distrate = 1.0 + ( distance_to_rp / 400.0); // 6 times when distance is 2048
    float halfdistrate = 1.0 + ( distance_to_rp / 800.0 ); // 3.4 times when distance is 2048
    float tripledistrate = 1.0 + ( distance_to_rp / 1600 ); // 2.2 times when distance is 2048
    
    int w = rect.width();
    int h = rect.height();
    // Power supply
    int nenergium = (h*w)/7000;
    for(int i=0;i<nenergium;i++) {
        Pos2 p = rect.getRandomPos();
        float dia = range(2,8);
        int n = range(5,dia*dia/5);
        scatterS( p, dia, ST_ENERGIUM, n, false, 0 );
    }
    // resource: iron and raremetal
    int nmine = (h*w)/5000;
    for(int i=0;i<nmine;i++) {
        Pos2 p = rect.getRandomPos();
        float dia = range(1,7);
        int n = range(1,4);
        BLOCKTYPE toput = BT_IRONORE;
        if( range(0,100) < 30 ) {
            toput = BT_RAREMETALORE;
        }
        
        for(int j=0;j<n;j++) {
            Pos2 dp( irange(-dia,dia), irange(-dia,dia) );
            setBlockCircleOn( p+dp, dia, BT_INVAL, toput, range(0.3,1.0) );
        }
    }


    if( bmt == BMT_DESERT || bmt == BMT_LAKE ) {
        // cells
        int ncell = (h*w)/1000 * halfdistrate;
        for(int i=0;i<ncell;i++) {
            Pos2 center = rect.getRandomPos();
            if( center.len(getRespawnPoint()) <20)continue;
            int nuc_per_colony = irange(1, irange(2,5) );
            for(int j=0;j<nuc_per_colony;j++) {
                Pos2 p = center + Pos2(0,0).randomize(5);
                Cell *c = get(p);
                if(c && c->bt == BT_AIR ) {
                    c->gt = GT_NUCLEUS;
                }
            }
        }
    
        // flygen
        int nflygen = (h*w)/1000 * distrate;
        for(int i=0;i<nflygen;i++) {
            Pos2 p = rect.getRandomPos();
            if( p.len(getRespawnPoint()) > 20 ) {
                for(int j=1;j<5;j++) {
                    Cell *c = get( p + Pos2( irange(-3,4),irange(-3,4) ) );
                    if(c && c->bt == BT_AIR && c->st == ST_NONE ) {
                        c->bt = BT_FLYGEN;
                    }            
                }
            }
        }

        // st_rocks  砂漠では散ってる
        int nrocksite = (h*w)/ 3000 * halfdistrate;
        for(int i=0;i<nrocksite;i++){
            Pos2 p = rect.getRandomPos();
            if( p.len(getRespawnPoint()) < 20 ) continue;
            int dia = irange(5,12);
            scatterS( p, dia, ST_ROCK, irange(2,100), false, 10 );
        }
    } else if( bmt == BMT_VOLCANO ) {
        // vents
        int nvents = (h*w) / 2000;
        for(int i=0;i<nvents;i++) {
            Pos2 center = rect.getRandomPos();
            if( center.len(getRespawnPoint()) < 30 ) continue;
            Rect r( center + Pos2(-1,-1), center + Pos2(1,1) );
            if( isClearPlace(r) ) {
                buildVent(center);
            }
        }

        // block firegens
        int nfiregens = (h*w) / 1000 * halfdistrate;
        for(int i=0;i<nfiregens;i++) {
            Pos2 center = rect.getRandomPos();
            if( center.len(getRespawnPoint()) < 30 ) continue;
            putFiregen(center, randomDir());
        }

        // st_rocks  more in volcanos
        int nrocksite = (h*w)/ 4000;
        for(int i=0;i<nrocksite;i++){
            Pos2 center = rect.getRandomPos();
            if( center.len(getRespawnPoint()) < 20 ) continue;
            int dia = irange(3,8);
            scatterS( center, dia, ST_ROCK, irange(30,200), false, 2 );
        }

        // gt firegens
        int ngrfiregens = (h*w) / 1000;
        for(int i=0;i<ngrfiregens;i++) {
            Pos2 center = rect.getRandomPos();
            // 2 types
            if( (i%2)==0) {
                int dia = irange(4,14);
                int num = irange(1,10);
                scatterGSelectG( center, dia, GT_FIREGEN, GT_LAVA_COLD, num );
            } else {
                buildFiregenCrack(center);
            }
        }

        // copter
        int ncoptergen = (h*w) / 1000 * distrate;
        for(int i=0;i<ncoptergen;i++) {
            Pos2 center = rect.getRandomPos();
            int num = irange(1,4);
            for(int j=0;j<num;j++) {
                Pos2 p = center + Pos2( irange(-4,5), irange(-4,5) );
                Cell *c = get(p);
                if( c && c->gt == GT_LAVA_COLD && c->bt == BT_AIR && c->st == ST_NONE ) {
                    c->bt = BT_COPTERGEN;
                }
            }
        }
    } else if( bmt == BMT_TUNDRA || bmt == BMT_MOUNTAIN ) {
        // Small ponds
        int nponds = (h*w) / 1000;
        for(int i=0;i<nponds;i++) {
            Pos2 center = rect.getRandomPos();
            int dia = irange(2,10);
            int n = dia*dia*4;
            SURFACETYPE st = range(0,100)<50 ? ST_WATER : ST_ICE;
            
            scatterS( center, dia, st, n, true, 3);
        }
        // Trees
        int ntrees = (h*w) / 200;
        for(int i=0;i<ntrees;i++) {
            Pos2 center = rect.getRandomPos();
            int dia = irange(2,10);
            int n = dia*dia/4;            
            scatterBSelectGatAir( center, dia, GT_SNOW, BT_TREE, n );                    
        }

        // Ice
        int nices = (h*w) / 100;
        for(int i=0;i<nices;i++) {
            Pos2 p = rect.getRandomPos();
            Cell *c =get(p);
            if(c->st == ST_WATER ) c->st = ST_ICE;
        }

        // Snow wall
        NoiseTable *st = new NoiseTable(w,h);
        st->noise( 32,0,1 );
        for( int y=0;y<h;y++){
            for(int x=0;x<w;x++){
                Cell *c = get( rect.min + Pos2(x,y));
                float r = st->get(x,y);
                if( r > 0.5 && c->st == ST_NONE && c->bt == BT_AIR ) {
                    c->bt = BT_SNOW;
                }
            }
        }
        // slimegen
        int nslimegen = (h*w) / 1000 * distrate;
        for(int i=0;i<nslimegen;i++) {
            Pos2 p = rect.getRandomPos();
            int dia = irange(2,7), n = irange(1,10);
            scatterBSelectGatAir( p, dia, GT_SNOW, BT_SLIMEGEN, n);
        }

        // birdgen
        int nbirdgen = (h*w) / 2000 * distrate;
        for(int i=0;i<nbirdgen;i++) {
            Pos2 p = rect.getRandomPos();
            int dia = irange(2,8), n = irange(1,4);
            scatterBSelectGatAir( p, dia, GT_SNOW, BT_BIRDGEN, n);
        }
    } else if( bmt == BMT_JUNGLE ) {
        // shrimpgen
        int nshrimpgen = (h*w) / 300 * distrate;
        for(int i=0;i<nshrimpgen;i++) {
            Pos2 p = rect.getRandomPos();
            int dia = irange(2,10), n = irange(2,8);
            scatterGSelectS( p, dia, ST_BLOODWATER, GT_SHRIMPGEN, n );
        }
        // Massive trees
        int ntrees = (h*w) / 100;
        for(int i=0;i<ntrees;i++) {
            Pos2 center = rect.getRandomPos();
            int dia = irange(2,10);
            int n = dia*dia/4;            
            scatterBSelectSatAir( center, dia, ST_NONE, BT_TREE, n );                    
        }
        // Many ivy trees
        int nivys = (h*w) / 300;
        for(int i=0;i<nivys;i++) {
            Pos2 center = rect.getRandomPos();
            int dia = irange(5,15);
            int n = dia*dia/2;
            scatterIvy( center, dia, n );
        }
        // bombflowers
        int nbombflowers = (h*w) / 300 * halfdistrate;
        for(int i=0;i<nbombflowers;i++){
            Pos2 center = rect.getRandomPos();
            int dia = irange(2,10);
            int n = dia*dia/5;
            scatterBSelectSatAir( center, dia, ST_NONE, BT_BOMBFLOWER, n );
        }
        // Bee hives
        int nbeehives = (h*w) / 300 * distrate;
        for(int i=0;i<nbeehives;i++){
            Pos2 center = rect.getRandomPos();
            int dia = irange(2,10);
            int n = dia*dia/5;
            scatterBSelectSatAir( center, dia, ST_NONE, BT_BEEHIVE, n );
        }
    }


    // long fences
    int nlongfence = (h*w)/15000;
    for(int i=0;i<nlongfence;i++) {
        Pos2 p = rect.getRandomPos();
        if( p.len( getRespawnPoint() ) > 200 ) {
            buildLongFence(p);
        }
    }
    
    // fortress
    int n_ft = w*h/7000 * tripledistrate;
    for(int i=0;i<n_ft;i++) {
        Pos2 at = rect.min + Pos2( irange(20,w-20), irange(20,h-20) );
        float maxlv = 0.3 + distance_to_rp / 1500.0f;
        if( at.len(getRespawnPoint()) < 40 ||
            at.len(getFirstFortressPoint()) < 40 ) continue;
        
        bool put_ok = buildFortressByLevel( at, 0, maxlv );
        if(put_ok) appendBuildFortressLog(at);
    }
        
    // Ground enemy structures
    int n_gd = w*h/1000 * halfdistrate;
    for(int i=0;i<n_gd;i++) {
        Pos2 at = rect.min + Pos2( irange(20,w-20), irange(20,h-20) );
        if( at.len(getRespawnPoint()) > 30 ) {
            float maxlv = 0.3 + distance_to_rp / 1500.0f;
            buildGrounderByLevel( at, 0, maxlv );
        }
    }
}
void Field::appendBuildFortressLog(Pos2 at) {
    if(build_fortress_log_num < elementof(build_fortress_log) ) {
        build_fortress_log[build_fortress_log_num] = at;
        build_fortress_log_num ++;
    }    
}

void Field::generateCommon() {
    // Huge mines
    int nsite = (width*height) / (256*256);
    for(int i = 0; i < nsite; i++ ) {
        Pos2 p( irange(20,width-20), irange(20,height-20) );
        if( p.len(getRespawnPoint()) < 200 ) continue;
        if( irange(0,100) < 70 ) {
            int sz = irange(5,20);
            if( range(0,100) < 10 ) sz *= 2;
            buildMegaResourceSite(p, BT_IRONORE, sz, range(0,0.5));
        } else {
            int sz = irange(4,15);
            if( range(0,100) < 10 ) sz *= 1.5;            
            buildMegaResourceSite(p, BT_RAREMETALORE, sz, range(0,0.5) );
        }
    }
    int nenesite = (width*height) / (1024*1024/2);
    for(int i=0;i<nenesite;i++) {
        Pos2 center( irange(20,width-20), irange(20,height-20) );
        if( center.len(getRespawnPoint()) < 200 ) continue;        
        int dia = irange(10,30);
        int partn = irange(2,20);
        int mgn = irange(10,50);
        for(int j=0;j<partn;j++) {
            Pos2 p = center + Pos2( irange(-mgn,mgn), irange(-mgn,mgn));
            int n = irange(30,100);            
            scatterS( p, dia, ST_ENERGIUM, n, false, range(0,15) );
        }
    }
    
    // 1 big dungeon
    buildDungeon( Pos2( width-400,height-400), 32*15 );
    // 3 small dungeons
    buildDungeon( Pos2(width/2,height/2), 32*7 );
    buildDungeon( Pos2(width/2,300), 32*5 );
    buildDungeon( Pos2(300,height/2), 32*5 );
    //    buildDungeon( Pos2(200,200), 32*7 );                
    
    // Hypergen
    int n_hg = (width*height) / 6400;
    for(int i=0;i<n_hg;i++ ) {
        Pos2 at( irange(20,width-20), irange(20,height-20) );
        if( at.len(getRespawnPoint()) < 300 ) continue;
        float l = at.len( getRespawnPoint() );
        l /= (float)width;
        if( l > 1.0 ) l = 1.0;
        if( range(0,1) < l ) {
            Cell *c = get(at);
            if(c && c->bt == BT_AIR ) {
                c->bt = BT_HYPERGEN;
                c->gt = GT_ROCK;
            }
        }
    }

    
    // Safety zone
    Pos2 rp = getRespawnPoint();

    int dia = 15;
    for( int y=rp.y-dia;y<rp.y+dia;y++){
        for(int x=rp.x-dia;x<rp.x+dia;x++){
            float l = ::len(rp.x,rp.y,x,y);
            Cell *c = get(x,y);
            if(!c)continue;
            if( l < dia ) { 
                c->bt = BT_AIR;
                c->st = ST_NONE;
            }
        }
    }

    // Plants nearby spawn point
    int ntree = 20;
    for(int i=0;i<ntree;i++){
        int m = 20;
        Pos2 p = rp + Pos2( irange(-m,m),irange(-m,m));
        if(p.len(rp)<3)continue;
        Cell *c = get(p);
        if( c && c->bt == BT_AIR && c->st == ST_NONE ) {
            if( range(0,1) < 0.2 ) {
                c->bt = BT_TREE;
            } else {
                c->st = ST_WEED;
            }
        }
    }

    

    // First fortress is located at fixed position
    {
        Pos2 first_ft_p = getFirstFortressPoint();
        const char *ft_first[] = {
            "M",
        };
        buildFortress( true, first_ft_p, ft_first, elementof(ft_first), NULL );    
    }
    
    Cell *c = get( getMotherShipPoint());
    c->bt = BT_MOTHERSHIP;


    // Initial resource
    setBlockCircleOn( rp+Pos2(-11,21), 4, BT_INVAL, BT_IRONORE, 1.0 );
    setBlockCircleOn( rp+Pos2(-10,12), 3, BT_INVAL, BT_RAREMETALORE, 1.0 );        

    c = get( rp + Pos2(0,-4) );
    c->st = ST_ENERGIUM;
    c = get( rp + Pos2(1,-5) );
    c->st = ST_ENERGIUM;
    c = get( rp + Pos2(2,-5) );
    c->st = ST_ENERGIUM;
    c = get( rp + Pos2(4,-6) );
    c->st = ST_ENERGIUM;

    c = get( rp + Pos2(14,-6) );
    c->st = ST_ENERGIUM;
    c = get( rp + Pos2(18,-6) );
    c->st = ST_ENERGIUM;                    

}


void Field::sim( bool paused ){
    for(int i=0;i<simchunk_per_poll;i++){
        simStepChunk( paused );
    }
    saved_chunk_this_sim = 0;
}

void debugValidateChunkAt( Pos2 lb ) {
    int emptycnt=0;
    for(int y=lb.y;y<lb.y+CHUNKSZ;y++) {
        for(int x=lb.x;x<lb.x+CHUNKSZ;x++) {
            Cell *c = g_fld->get(x,y);
            assert(c);
            if(c->bt == BT_AIR && c->gt == GT_SOIL && c->st == ST_NONE ) emptycnt++;
        }
    }
    int threscnt = CHUNKSZ*CHUNKSZ - 5;
    if( emptycnt >= threscnt ) {
        g_log->printf( WHITE, "EMPTYCHUNK cnt(%d) at lb:%d,%d",emptycnt,lb.x,lb.y);
        //        assert(false);
    }
}

// Process one step of chunk simulation
void Field::simStepChunk( bool paused ){
    sim_counter ++;
    int to_sim_i = sim_counter % elementof(to_sim);
    
    if( to_sim_i == 0 ) {
        double step_time = now() - last_simclock_at;
        if( step_time > FIELD_MIN_INTERVAL_SEC ){
            simchunk_per_poll += 10;
            if( simchunk_per_poll > 500 )simchunk_per_poll = 500;
        } else {
            simchunk_per_poll -= 30;
            if( simchunk_per_poll < 10 ) simchunk_per_poll = 10;
        }
        last_simclock_at = now();
        sim_clock_count ++;
    
        last_chunk_change_cnt = cur_chunk_change_cnt;
        cur_chunk_change_cnt = 0;
    }

    ToSim *ts = & to_sim[ to_sim_i ];
    if( ts->cnt == 0 ) {
    } else {
        if( ts->lock_state == LOCKSTATE_GOT || isDBNetworkActive() == false ) { // Perform simulation in offline mode too
            if( paused == false ) { 
                ts->cnt --;
                if( simChunk( ts->chx, ts->chy ) ) {
                    cur_chunk_change_cnt ++;
                }
                ChunkStat *cs = getChunkStat( ts->chx, ts->chy );
                if(cs) cs->simcnt ++;
                if( ( cs->simcnt % 20 ) == 0 ) {
                    updateChunkStat(ts->chx, ts->chy);
                }
            }
            if( ts->cnt == 0 ) {
                //                print("to_sim got 0 at (%d,%d), saving.",ts->chx, ts->chy );
                if( saved_chunk_this_sim < SAVE_CHUNK_PER_SIM ) {
                    saved_chunk_this_sim ++;
                    prt("s ");
                    realtimeUnlockGridSend( ts->chx, ts->chy );
                    saveAt( Pos2(ts->chx*CHUNKSZ, ts->chy*CHUNKSZ), 0);
                    ts->clear();
                } else {
                    prt("_s ");
                }
            } else {
                double nt = now();
                if( ts->last_lock_sent_at < nt - ToSim::LOCK_KEEP_INTERVAL_SEC ) {
                    ts->last_lock_sent_at = nt;
                    realtimeLockKeepGridSend( ts->chx, ts->chy );
                }
                if( ts->last_snapshot_sync_at < nt - ToSim::SNAPSHOT_SYNC_INTERVAL_SEC ) {
                    ts->last_snapshot_sync_at = nt + range(0, ToSim::SNAPSHOT_SYNC_INTERVAL_SEC/2); // Slightly adjust timing to get smoother
                    CHUNKLOADSTATE ls = getChunkLoadState(Pos2(ts->chx*CHUNKSZ,ts->chy*CHUNKSZ));
                    if( ls == CHUNKLOADSTATE_LOADED ) {
                        debugValidateChunkAt( Pos2(ts->chx*CHUNKSZ,ts->chy*CHUNKSZ) );
                        realtimeSyncChunkSnapshotSend(ts->chx, ts->chy);
                    }
                }
                if( ts->last_save_at < ts->last_important_change_at - ToSim::SAVE_DELAY ) {
                    Pos2 lb(ts->chx*CHUNKSZ, ts->chy*CHUNKSZ);
                    CHUNKLOADSTATE ls = getChunkLoadState(lb);
                    if( saved_chunk_this_sim < SAVE_CHUNK_PER_SIM && ls == CHUNKLOADSTATE_LOADED ) {
                        ts->last_save_at = now();
                        print("save chunk %d,%d after change", ts->chx, ts->chy );
                        debugValidateChunkAt(lb);
                        saveAt( lb, 0);
                        saved_chunk_this_sim ++;
                    } else {
                        print("__s ");
                    }
                }
                if( ts->last_image_update_at < nt - ToSim::IMAGE_UPDATE_INTERVAL_SEC ) {
                    ts->last_image_update_at = nt;
                    CHUNKLOADSTATE lds = getChunkLoadState( Pos2(ts->chx*CHUNKSZ,ts->chy*CHUNKSZ));
                    if( lds == CHUNKLOADSTATE_LOADED ) {
                        bool rev = g_fld->getChunkRevealed( ts->chx, ts->chy );
                        if(rev) dbUpdateChunkImageSend( g_current_project_id, ts->chx, ts->chy );
                    }
                }
            }
        } else if( ts->lock_state == LOCKSTATE_INIT ) {
            if( realtimeLockGridSend( ts->chx, ts->chy ) ) {
                ts->lock_state = LOCKSTATE_SENT;
                ts->last_lock_sent_at = now();
            }
        } else if( ts->lock_state == LOCKSTATE_SENT ) {
            if( ts->last_lock_sent_at < now() - ToSim::LOCK_RETRY_INTERVAL_SEC ) {
                realtimeLockGridSend( ts->chx, ts->chy );
            }
        }
    }
}

inline void Field::simCell(int x, int y, bool *chg, bool *uchg, bool *dchg, bool *rchg, bool *lchg ) {
    Cell *c = get(x,y);

    Cell *uc = get(x,y+1);
    Cell *dc = get(x,y-1);
    Cell *rc = get(x+1,y);
    Cell *lc = get(x-1,y);
    
    // sim bt 
    if( c->bt != BT_AIR ) {
        if( c->damage > 0 ) {
            if( (sim_clock_count + x + y) % 19 == 0 ) { 
                c->damage --;
                *chg = true; // durbar
            }
        }
        switch( c->bt ) {
        case BT_CAGE: 
        case BT_IVY:
            // To avoid content is reset
            break;
        case BT_PAIRBARRIER:
            {
                int prev_content = c->content;
                c->content ++;
                if( c->content >= PAIRBARRIER_CONTENT_THRES && prev_content < PAIRBARRIER_CONTENT_THRES ) {
                    *chg = true; // To render
                }

                // Build barrier
                if( c->content >= PAIRBARRIER_CONTENT_THRES ) {
                    Pos2 p(x,y);
                    checkPutBarrier( p, DIR_RIGHT, BARRIER_INTERVAL, 6 ); 
                    checkPutBarrier( p, DIR_LEFT, BARRIER_INTERVAL, 6 ); 
                    checkPutBarrier( p, DIR_DOWN, BARRIER_INTERVAL, 6 );
                    checkPutBarrier( p, DIR_UP, BARRIER_INTERVAL, 6 ); 
                }
            }

            break;
        case BT_CELL:
            {
                if( c->content > 0 ) { // no seeable change 
                    int consume = 1 + (c->randomness % 3);
                    c->content -= consume;
                    if( c->content < 0 ) c->content = 0;
                }
                // Change ground
                if( c->gt == GT_SOIL || c->gt == GT_SAND || c->gt == GT_GRASS ) {
                    if( (c->randomness % 32) == 0 && c->gt != GT_ENHANCER ) {
                        c->gt = GT_ENHANCER;
                        *chg = true;
                    } 
                }

                
                if( c->content > 10 ) {
                    // spread to next cell
                    Cell *nc4[4];
                    get4(Pos2(x,y),nc4);
                    Cell *nc = nc4[ irange(0,4) ];
                    if(nc && nc->bt == BT_AIR && nc->isWalkableSurface() ) {
                        nc->bt = BT_CELL;
                        notifyChanged(nc);
                    }

                    // Worms out
                    if( range(0,100) < 1 ) new Worm(toCellCenter(Vec2(x*PPC,y*PPC)).randomize(PPC/2) );
                }

                // Move to next cells
                int thr = 10;
                if( rc && rc->bt == BT_CELL && rc->content < c->content - thr-1 ) { c->content -= thr; rc->content += thr; }  
                if( lc && lc->bt == BT_CELL && lc->content < c->content - thr-1 ) { c->content -= thr; lc->content += thr; } 
                if( uc && uc->bt == BT_CELL && uc->content < c->content - thr-1 ) { c->content -= thr; uc->content += thr; } 
                if( dc && dc->bt == BT_CELL && dc->content < c->content - thr-1 ) { c->content -= thr; dc->content += thr; } 
            }
            break;
        case BT_TREE:
            {
                if( c->isPlantable() == false ) {
                    c->bt = BT_AIR;
                    c->content = 0;
                    *chg = true;
                    break;
                }
                
                // Greening near
                if(c->moisture>0){
                    Cell *gc = getDia( Vec2(x*PPC,y*PPC), 10*PPC );
                    if(gc) {
                        *chg |= gc->tryChangeGrass();
                    }
                }
                
                // Generate new tree
                if( c->moisture > 0  && (c->randomness%8) > 4 ) {
                    c->content += c->moisture / 5;
                    if( c->content > TREE_CONTENT_MAX ) {
                        int tcnt = 0;
                        Cell *around[8];
                        get8( Pos2(x,y), around );
                        for(int i=0;i<8;i++) {
                            if( around[i] && around[i]->bt == BT_TREE ) tcnt++;
                        }
                        if( tcnt < 4) {
                            Cell *nc = around[ irange(0,8) ];
                            if(nc && nc->isPlantable() && nc->bt == BT_AIR && nc->moisture > 0 && nc->st == ST_NONE && toCellCenter(x,y).len(g_pc->loc) > PPC*2 ) {
                                nc->bt = BT_TREE;
                                nc->content = 0;
                                c->content /= 2;
                                notifyChanged(nc);
                            }
                        }
                    }
                }
            }
            break;
        case BT_REACTOR_INACTIVE:
#define CHECKARM(tgt,d,flg) if( tgt && tgt->isReactorArmPuttable() ) { \
                tgt->bt = BT_REACTOR_ARM; tgt->dir = d; flg = true; \
                c->bt = BT_REACTOR_ACTIVE; *chg = true; }

            CHECKARM( uc, DIR_DOWN, *uchg );
            CHECKARM( dc, DIR_UP, *dchg );
            CHECKARM( rc, DIR_LEFT, *rchg );
            CHECKARM( lc, DIR_RIGHT, *lchg );
            if( c->st == ST_ENERGIUM ) { c->bt = BT_REACTOR_ACTIVE; *chg = true; }
            if(c->content > 0 ) { c->content = 0; *chg = true; }
            break;
        case BT_REACTOR_ACTIVE:
            g_ps->ensureEquipment( Pos2(x,y) );
            // PC can get energy by directy touching a reactor.
            c->content ++;
            if( c->content > 100 ) c->content = 100;
            break;

        case BT_DEROTA:
            {
                int mod = (c->hyper_count>0 ? 2 : 6 );
                if( c->hyper_count > 0 && (c->content%11==0) ) {
                    createHyperSpark( toCellCenter(x,y).randomize(PPC/2));
                }
                c->content ++;
                if( ( c->content % mod ) == 0 ) {
                    Vec2 center = toCellCenter(Vec2(x*PPC,y*PPC));
                    Vec2 tgt;
                    bool to_shoot = findEnemyAttackTarget( center, &tgt, MACHINE_SHOOT_DISTANCE );
                    if(to_shoot) {
                        Bullet::shootAt( BLT_SPARIO, center, tgt );
                    }
                }
            }
            break;
        case BT_MONO_EYE:
        case BT_COMBI_EYE:
        case BT_PANEL_EYE:
        case BT_ROUND_EYE:
        case BT_CHASER_EYE:
        case BT_BUILDER_EYE:
            {
                OPENNESS openness_orig = c->getEyeOpenness();
                //                print("EYE!!! %d,%d  t:%d st:%d",x,y, c->bt, c->ftstate );

                if( c->ftstate == FTS_IDLE ) {
                    c->ftstate = FTS_ACTIVE; 
                }

                if( c->hyper_count > 0 && (c->content%11==0) ) {
                    createHyperSpark( toCellCenter(x,y).randomize(PPC/2));
                }
                if( c->hyper_count > 0 && c->damage > 0 ) {
                    c->damage -= 3;
                    if( c->damage < 0 ) c->damage = 0;
                }
                c->content++;
                OPENNESS cur_openness = c->getEyeOpenness();
                if( cur_openness == OPENNESS_OPEN ) {
                    Vec2 center = toCellCenter(Vec2(x*PPC,y*PPC));
                    Vec2 tgt;
                    bool to_shoot = findEnemyAttackTarget( center, &tgt, MACHINE_SHOOT_DISTANCE );
                    int mod3_hyper = c->hyper_count > 0 ? 1 : 3;
                    int mod2_hyper = c->hyper_count > 0 ? 1 : 2;

                    if( to_shoot ) { 
                        if(c->bt == BT_COMBI_EYE && (c->content % mod3_hyper == 0 ) ) {
                            Bullet::shootFanAt( BLT_SPARIO, center, tgt, 2, 0.2 );
                        } else if( c->bt == BT_MONO_EYE && (c->content % mod3_hyper) == 0  ){
                            Vec2 at = toCellCenter(Vec2(x*PPC,y*PPC)) + Vec2(-PPC/4,PPC/4);                        
                            Bullet::shootAt( BLT_SPARIO, at, tgt );
                        } else if( c->bt == BT_PANEL_EYE ) {
                            int fan_n = ( c->hyper_count > 0 ) ? 2 : 1;
                            Bullet::shootFanAt( BLT_SIG, center, tgt, fan_n, 0.15 );
                        } else if( c->bt == BT_ROUND_EYE && (c->content% mod2_hyper == 0) ) {
                            Bullet::shootAt( BLT_BELO, center, tgt );
                        } else if( c->bt == BT_CHASER_EYE ) {
                            if( range(0,100) < (( c->hyper_count > 0 ) ? 30 : 10) ) {
                                new Chaser(center);
                            }
                        } else if( c->bt == BT_BUILDER_EYE ) {
                            if( range(0,100) < (( c->hyper_count > 0 ) ? 30 : 10 ) ) {
                                new Builder(center);
                            }
                        }
                    }                    
                }
                if( c->content > c->calcEyeInterval() ) {
                    c->content = 0;
                }                
                if( openness_orig != cur_openness ) *chg = true;
            }
            
        case BT_EXCHANGE_INACTIVE:
            if( c->powergrid_id > 0 ) {
                c->bt = BT_EXCHANGE_ACTIVE;
                *chg = true;
            }
            if(g_ps->ensureEquipment(Pos2(x,y)) ) {
                refreshExchangeLinks();
            }
            break;
        case BT_EXCHANGE_ACTIVE:
            if( c->powergrid_id == 0 ) {
                c->bt = BT_EXCHANGE_INACTIVE;
                *chg = true;
            }
            if( g_ps->ensureEquipment(Pos2(x,y)) ) {
                refreshExchangeLinks();
            }
            break;
        case BT_PORTAL_INACTIVE:
            if( c->powergrid_id > 0 ) {
                c->bt = BT_PORTAL_ACTIVE;
                *chg = true;
            }
            break;
        case BT_PORTAL_ACTIVE:
            if( c->powergrid_id == 0 ) {
                c->bt = BT_PORTAL_INACTIVE;
                *chg = true;
            }
            break;
        case BT_CABLE:
            break;
        case BT_TURRET_INACTIVE:
            if( c->powergrid_id > 0 ) {
                c->bt = BT_TURRET_ACTIVE;
                *chg = true;
            }
            break;
        case BT_TURRET_ACTIVE:
            if( c->powergrid_id == 0 ) {
                c->bt = BT_TURRET_INACTIVE;
                *chg = true;
            } else {
                // Consume energy
                PowerGrid *grid = g_ps->findGridById( c->powergrid_id );
                if( grid && grid->ene > 0 ) {
                    Vec2 at = toCellCenter( Vec2(x*PPC,y*PPC) );
                    Char *e = Enemy::getNearestShootable(at );
                    if(e && at.len(e->loc) < TURRET_RANGE ) { 
                        grid->ene --;
                        new Beam( at, e->loc, BEAMTYPE_NORMAL, B_ATLAS_PC_BEAM, 1, 0 );
                    }
                }

            }
            break;
        case BT_FENCE:
            if( c->damage > 0 ) {
                int to_dec = 10;
                if( to_dec > c->damage ) to_dec = c->damage;
                c->damage -= to_dec;
                if( to_dec > 0 ) *chg = true;
            }
            break;
        case BT_BARRIERGEN:
            {
                c->content ++;
                if( c->content > 15 ) c->content = 0;
                Pos2 p(x,y);
                switch( c->content % 4 ) {
                case 0: checkPutBarrier( p, DIR_RIGHT, BARRIER_INTERVAL, 10 ); break;
                case 1: checkPutBarrier( p, DIR_LEFT, BARRIER_INTERVAL, 10 ); break;
                case 2: checkPutBarrier( p, DIR_DOWN, BARRIER_INTERVAL, 10 ); break;
                case 3: checkPutBarrier( p, DIR_UP, BARRIER_INTERVAL, 10 ); break;
                }
                break;
            }
        case BT_BARRIER:
            c->content --;
            if( c->content <= 0 ) {
                c->bt = BT_AIR;
                *chg = true;
            }
            break;
        case BT_HYPERGEN:
            c->content ++;
            if( c->content > 10 ) {
                c->content = 0;
                new HyperPutter( toCellCenter(x,y) );
            }
            break;
        case BT_FLYGEN:
            {
                int nn = 50;
                int rr = nn + (c->randomness % nn);
                c->content ++;
                Vec2 at = toCellCenter(x,y);
                if( (c->content % rr) == 0 && at.len(g_pc->loc) < 100*PPC ) {
                    int n = irange(1,5);
                    for(int i=0;i<n;i++) {
                        new Fly( toCellCenter(x,y));
                    }
                }
            }
            break;
        case BT_COPTERGEN:
            {
                int add = 1 + (c->randomness%5);
                c->content += add;
                if( c->content > COPTERGEN_MAX_CONTENT ) {
                    c->content = 0;
                    new FireCopter( toCellCenter(x,y) );
                }
            }
            break;
        case BT_SLIMEGEN:
            {
                int add = 1 + (c->randomness%5);
                c->content += add;
                if( c->content > SLIMEGEN_MAX_CONTENT ) {
                    c->content = 0;
                    new SnowSlime( toCellCenter(x,y));
                }
            }
            break;
        case BT_BIRDGEN:
            {
                int add = 1 + (c->randomness%5);
                c->content += add;
                if( c->content > BIRDGEN_MAX_CONTENT ) {
                    c->content = 0;
                    new Bird( toCellCenter(x,y));
                }                
            }
            break;
        case BT_MOTHERSHIP:
            g_ps->ensureEquipment( Pos2(x,y) );
            soundPlayAt( g_beep_sounds[ irange(0,4) ], toCellCenter(x,y),4 );
            if( sim_clock_count % 2 == 0 ) {
                Vec2 at = toCellCenter(x,y).randomize(PPC/2);
                if( range(0,100) > 70 ) {
                    createSpark( at, 1 );
                } else if( range(0,100) > 50 ) {
                    createSmokeEffect(at);
                }
            }
            break;
        case BT_FIREGEN:
            {
                int prev_content = c->content;
                int add = 1 + (c->randomness %3);
                c->content += add;
                if( c->content > FIREGEN_MAX_CONTENT ) {
                    c->content = 0;
                    *chg = true;
                    new FireGenerator( toCellCenter(x,y) );
                } else if( c->content > FIREGEN_READY_CONTENT ) {
                    if( prev_content <= FIREGEN_READY_CONTENT ) {
                        *chg = true; // Render that this firegen is ready and danger
                    }
                }
            }
            break;
        case BT_SNOW:
            if( c->moisture == 0 && (c->isSnowGrowable()) ) {
                int mod = (c->randomness + sim_clock_count) % 128;
                if( mod == 0 ) {
                    if(uc && uc->bt == BT_AIR ) {uc->bt = BT_SNOW;*uchg = true;}
                    if(dc && dc->bt == BT_AIR ) {dc->bt = BT_SNOW;*dchg = true;}
                    if(lc && lc->bt == BT_AIR ) {lc->bt = BT_SNOW;*lchg = true;}
                    if(rc && rc->bt == BT_AIR ) {rc->bt = BT_SNOW;*rchg = true;}                    
                } 
            }
            break;
        case BT_BOMBFLOWER:
            if( c->content > 0 ) {
                c->content ++;
                if( c->content > 10 ) {
                    Vec2 at = toCellCenter(x,y);
                    createBuildingExplosion(at);
                    soundPlayAt( g_cutweed_sound, at,1);
                    for(int i=0;i<3;i++) createLeafEffect(at);
                    c->bt = BT_AIR;
                    *chg = true;
                    int n = irange(2,8);
                    for(int i=0;i<n;i++) new BombFlowerSeed(toCellCenter(x,y));
                }
            }
            break;
        case BT_BEEHIVE:
            if( c->content > 0 ) {
                c->content --;
                if( c->content == 0 ) {
                    *chg = true; // Color changes.
                }
                Vec2 at = toCellCenter(x,y);
                int n = 1 + ( c->randomness % 3 );
                for(int i=0;i<n;i++) new Bee(at.randomize(PPC/3));
                
            }
            break;
        case BT_CORE_COVER:
            {
                Cell *cells[4];
                get4(Pos2(x,y),cells);
                for(int i=0;i<4;i++) {
                    if(cells[i] && cells[i]->st == ST_CORE_CABLE_ACTIVE ) {
                        c->bt = BT_AIR;
                        *chg = true;
                    }
                }
                    
            }
            break;
        case BT_GRAY_GOO:
            {
                int add = 1 * ( c->hyper_count > 0 ? 3 : 1 );
                
                c->content += add;
                if( c->hyper_count > 0 && ((x+y+sim_clock_count)%11==0) ) {
                    createHyperSpark( toCellCenter(x,y).randomize(PPC/2));
                }
                if( c->content > GRAY_GOO_INTERVAL ) {
                    c->content = 0;
                    int dx,dy;
                    dirToDXDY( randomDir(), &dx, &dy );
                    Pos2 p(x+dx,y+dy);
                    Cell *nc = get(p);
                    if(nc && nc->isBlockPuttable() && nc->isWater() == false ) {
                        nc->bt = BT_GRAY_GOO;
                        notifyChanged(nc);
                    }
                        
                }
            }
            break;
        case BT_CRYSTAL_BEAMER:
            {
                int prev_content = c->content;
                c->content ++;
                if( prev_content < PAIRBARRIER_CONTENT_THRES && c->content >= PAIRBARRIER_CONTENT_THRES ) {
                    *chg = true; // Render active beamer
                }
                if( c->content >= PAIRBARRIER_CONTENT_THRES ) {
                    if( (x+y+sim_clock_count) % 32 == 0 ) {
                        Vec2 dv = dirToVec2(c->dir);
                        Vec2 from = toCellCenter(x,y);
                        Bullet::shootAt( BLT_CRYSTAL_BEAM, from + dv.normalize(PPC), from + dv.normalize(PPC*2) );
                    }
                }
            }
            break;
        case BT_POLE:
            {
                // Destroy by itself when found to be stray
                PowerGrid *pg = g_ps->findGridById( c->powergrid_id );
                if(!pg) {
                    PowerNode *pn = g_ps->findNode(Pos2(x,y));
                    if(pn) {
                        print("simcell: found stray pole at %d,%d old id:%d. converting.",x,y, c->powergrid_id );
                        setPowerGrid( Pos2(x,y), pn->parent->id );
                        saveAt(c, 4 );
                        *chg = true;
                    } else {
                        // not found, destroy
                        print("simcell: found truely stray pole at %d,%d, clean?", x,y );
#if 0                        
                        c->bt = BT_AIR;
                        clearPowerGrid( Pos2(x,y), c->powergrid_id );
                        *chg = true;
                        saveAt(c, 4 );
#endif                        
                    }
                }
            }
            break;
        default:
            c->content = 0; // changed, but nothing viewable
            break;
        }
    }

    // sim gt
    switch(c->gt) {
    case GT_NUCLEUS:
        if( c->bt == BT_AIR ) {
            c->bt = BT_CELL;
        } else if( c->bt == BT_CELL ) {
            int dn = (c->randomness % 200)+1;
            c->content += dn;
            if( c->content > 1000 ) c->content = 1000; // nothing viewable
            if( range(0,100) < 10 ) createCellBubbleRed( toCellCenter(x,y) );
        }        
        break;
    case GT_ENHANCER:
        if( range(0,100) < 10 ) createCellBubbleBlue( toCellCenter(x,y) );
        break;
    case GT_SAND:
    case GT_SOIL:        
        break;
    case GT_VENTCORE:
        {
            bool ready = ( c->content > VENT_READY_CONTENT_THRES );
            int speed = 1 + (c->randomness % 4);
            c->content += speed;            
            if( c->content > VENT_CONTENT_MAX ) {
                c->content = 0;
                explodeVent( Pos2(x,y));
                paintContentInsideG( Pos2(x,y), GT_VENT, c->content );
                soundPlayAt( g_volcano_explode_sound, toCellCenter(Pos2(x,y)),1);                
            } else if( c->content > VENT_READY_CONTENT_THRES ) {
                if( !ready ) {
                    paintContentInsideG( Pos2(x,y), GT_VENT, c->content );
                }
                soundPlayAt( g_volcano_ready_sound, toCellCenter(Pos2(x,y)),1);
            }
        }
        break;
    case GT_LAVA_HOT:
        if( ( (c->randomness+sim_clock_count) % 64 ) == 0 ) {
            c->gt = GT_LAVA_COLD;
            *chg = true;
        }
        if( c->st == ST_WATER && ((c->randomness+sim_clock_count) % 8 == 0 ) ) {
            createSmokeEffect( toCellCenter(x,y));
        }
        break;
    case GT_FIREGEN:
        {
            int mod = ( c->randomness + sim_clock_count ) % 48;
            if( mod == 0 ) {
                createSmokeEffect( toCellCenter(x,y) );
            } else if( mod == 24 ) {
                Vec2 at = toCellCenter(x,y);
                Bullet::shootAt( BLT_FIRE, at, at.randomize(PPC) );
            }
        }
        break;
    case GT_TUNDRA:
        {
            int mod = ( c->randomness + sim_clock_count ) % 1024;
            if( mod == 0 ) {
                ChunkStat *cs = getChunkStat(x/CHUNKSZ,y/CHUNKSZ);
                if(cs->tundra > 10 ) {
                    c->gt = GT_SNOW;
                    *chg = true;
                }
            }
        }
        break;
    case GT_SNOW:
        if( c->st == ST_WATER ) {
            c->gt = GT_TUNDRA;
            *chg = true;
        }
        break;
    case GT_SHRIMPGEN:
        {
            int mod = ( c->randomness + sim_clock_count ) % 48;
            if(mod==0){
                new Shrimp( toCellCenter(x,y));
            }
        }
        break;
    case GT_JUNGLE:
        if( c->st == ST_NONE && c->bt == BT_AIR && ( c->randomness % 256) > 200 ) {
            c->st = ST_WEED;
            *chg = true;
        }
        break;
    case GT_CORE:
        {
            // Destroy 4 cells at the same time. Only left-down cell are simulated
            if(uc && uc->gt == GT_CORE && rc && rc->gt == GT_CORE && lc && dc ) {
                Cell *ruc = get(x+1,y+1);
                if( uc->bt == BT_AIR && rc->bt == BT_AIR && ruc && ruc->bt == BT_AIR && c->bt == BT_AIR ) {
                    c->content ++;
                    if( (c->content % 3 ) == 0 ) { 
                        soundPlayAt( g_core_sound, toCellCenter(x,y),1);
                    }
                    int thr = 30;
                    if( c->hyper_count > 0 ) thr = 15;
                    if( c->content > thr ) {
                        c->content = 0;
                        Vec2 center = toCellCenter(x,y)+Vec2(PPC/2,PPC/2);
                        new CoreBall(center);
                    }
                }
            }
            // Recover quickly
            if( c->damage > 0 ) {
                c->damage -= 10;
                if(c->damage<0) c->damage = 0;
            }
        }
        break;
    case GT_TELEPORTER_ACTIVE:
        if( c->bt == BT_AIR ) {
            c->content ++;
            if( c->content > TELEPORTER_GEN_LATENCY ) {
                c->gt = GT_TELEPORTER_INACTIVE;
                c->content = 0;
                *chg = true;
                new Teleporter( toCellCenter(x,y));
            }
        }
    default:
        break;
    }

    // sim st
    switch(c->st) {
    case ST_HYPERLINE:
        c->hyper_count = 15;
        break;
    case ST_WEED:
        if(c->moisture>0){
            Cell *gc = getDia( Vec2(x*PPC,y*PPC), 4*PPC );
            if(gc) *chg |= gc->tryChangeGrass();
        }
        if( c->isPlantable() == false ) {
            c->st = ST_NONE;
            c->content = 0;
            *chg = true;
        }
        break;
    case ST_WATER:
        c->moisture = WATER_MOISTURE_MAX;
        break;
    case ST_ICE:
        c->moisture = WATER_MOISTURE_MAX;        
        {
            int mod = (c->randomness + sim_clock_count ) % 64;
            if(mod==0) {
                if(uc && uc->st == ST_WATER ) { uc->st = ST_ICE; *uchg = true; }
                if(dc && dc->st == ST_WATER ) { dc->st = ST_ICE; *dchg = true; }
                if(lc && lc->st == ST_WATER ) { lc->st = ST_ICE; *lchg = true; }
                if(rc && rc->st == ST_WATER ) { rc->st = ST_ICE; *rchg = true; }                
            }
        }
        break;
    case ST_CORE_CABLE_INACTIVE:
        {
            Cell *cells[4];
            get4(Pos2(x,y),cells);
            for(int i=0;i<4;i++) {
                if( cells[i] && cells[i]->isCoreCableActivator() ) {
                    c->st = ST_CORE_CABLE_ACTIVE;
                    *chg = true;
                }
            }
        }
        break;
    case ST_CORE_SWITCH_INACTIVE:
        {
            if( c->isEnemyBuilding() == false && c->isWall(true) == false ) {
                c->st = ST_CORE_SWITCH_ACTIVE;
                soundPlayAt( g_switch_sound, toCellCenter(x,y),1);
                *chg = true;
            }
        }
        break;
    case ST_CORE_CABLE_ACTIVE:
        break;
    default:
        break;
    }

    
    // moisture:
    if( c->moisture > 0 ) {
        //        if( c->moisture > 0 ) c->moisture --;        これがあると、常に変化し続けになってしまう。
        if(uc && uc->moisture < c->moisture-1){ uc->moisture = c->moisture-1; *uchg = true; }
        if(dc && dc->moisture < c->moisture-1){ dc->moisture = c->moisture-1; *dchg = true; }
        if(rc && rc->moisture < c->moisture-1){ rc->moisture = c->moisture-1; *rchg = true; }
        if(lc && lc->moisture < c->moisture-1){ lc->moisture = c->moisture-1; *lchg = true; }
    }

    // Hyper 
    if( c->hyper_count > 0 ) {
        c->hyper_count --; 
        if(uc && uc->hyper_count < c->hyper_count-1){ uc->hyper_count = c->hyper_count-1; } // 今のところ見た目の変化は無し
        if(dc && dc->hyper_count < c->hyper_count-1){ dc->hyper_count = c->hyper_count-1; }
        if(rc && rc->hyper_count < c->hyper_count-1){ rc->hyper_count = c->hyper_count-1; }
        if(lc && lc->hyper_count < c->hyper_count-1){ lc->hyper_count = c->hyper_count-1; }        
    }

    //
    if( *chg && isRealtimeNetworkActive() )  realtimeSyncCellSend(c,'C');
    if( *rchg && isRealtimeNetworkActive() ) realtimeSyncCellSend(rc,'R');
    if( *lchg && isRealtimeNetworkActive() ) realtimeSyncCellSend(lc,'L');
    if( *uchg && isRealtimeNetworkActive() ) realtimeSyncCellSend(uc,'U');
    if( *dchg && isRealtimeNetworkActive() ) realtimeSyncCellSend(dc,'D');
}

bool Field::simChunk(int chx, int chy){

    int x0 = chx*CHUNKSZ, y0 = chy*CHUNKSZ;
    int x1 = x0 + CHUNKSZ, y1 = y0 + CHUNKSZ;

    bool chk_chg=false, uchk_chg=false, dchk_chg=false, rchk_chg=false, lchk_chg=false;

    for(int y=y0;y<y1;y++){
        for(int x=x0;x<x1;x++){
            bool chg=0,uchg=0,dchg=0,rchg=0,lchg=0; 
            simCell(x,y,&chg,&uchg,&dchg,&rchg,&lchg); 
            chk_chg |= chg;
            if(rchg) { if(x==x1-1) rchk_chg = true; else chk_chg = true; } 
            if(lchg) { if(x==x0) lchk_chg = true; else chk_chg = true; } 
            if(uchg) { if(y==y1-1) uchk_chg = true; else chk_chg = true; } 
            if(dchg) { if(y==y0)  dchk_chg = true; else chk_chg = true; }
        }
    }
    
    
    if( chk_chg ) { notifyChunkChanged( chx, chy ); }
    if( uchk_chg ) { notifyChunkChanged( chx, chy+1 ); }
    if( dchk_chg ) { notifyChunkChanged( chx, chy-1 ); }
    if( rchk_chg ) { notifyChunkChanged( chx+1, chy ); }
    if( lchk_chg ) { notifyChunkChanged( chx-1, chy );  }

    
    return chk_chg || uchk_chg || dchk_chg || rchk_chg || lchk_chg;
}

Pos2 Field::getRespawnPoint() {
    return Pos2( 24,24 );
}
Pos2 Field::getMotherShipPoint() {
    return getRespawnPoint() + Pos2(-5,-5);
}
Pos2 Field::getFirstFortressPoint() {
    return getRespawnPoint() + Pos2(2,35);
}


// Returns 4 bits: RT-LT-RB-LB
int Field::getEnterableBits( Vec2 at, float sz, bool enemy, bool flying, bool swimming, BLOCKTYPE bt_can_enter ) {
    assertmsg( sz < PPC, "too big" );

    int out = 0;
    Cell *rtc = get( at + Vec2( sz, sz ) );
    Cell *ltc = get( at + Vec2( -sz, sz ) );        
    Cell *rbc = get( at + Vec2( sz, -sz ) );
    Cell *lbc = get( at + Vec2( -sz, -sz ) );

    if( (!rtc) || (!rtc->isEnterable(enemy,flying,swimming,bt_can_enter)) ) out += WALKABLE_BIT_HIT_RT;
    if( (!ltc) || (!ltc->isEnterable(enemy,flying,swimming,bt_can_enter)) ) out += WALKABLE_BIT_HIT_LT;
    if( (!rbc) || (!rbc->isEnterable(enemy,flying,swimming,bt_can_enter)) ) out += WALKABLE_BIT_HIT_RB;
    if( (!lbc) || (!lbc->isEnterable(enemy,flying,swimming,bt_can_enter)) ) out += WALKABLE_BIT_HIT_LB;

    return out;
}


void increaseScore( Char *by, int add ) {
    if(!by)return;
    if(by->category == CAT_BEAM ) {
        Beam *beam = (Beam*) by;
        if( beam->shooter_id == g_pc->id ) g_pc->modScore(add);
    } else if(by->category== CAT_BLASTER) {
        Blaster *bl = (Blaster*)by;
        if(bl->shooter_id == g_pc->id ) g_pc->modScore(add);
    }
}
// Enemies pass NULL to cosumed.
// Returns true when hit and stop, false when going through (penetrating)
bool Field::damage( Vec2 at, int dmg, int *consumed, Char *by ) {
    Cell *c = get(at);
    if(!c)return false;

    
    if( c->isEarthBlock() ) {
        bool gen_debri = ( c->bt != BT_FIREGEN && c->bt != BT_SNOW );
        int fin_dmg = dmg;
        int last = c->getMaxBlockHP() - c->damage;
        if( last < fin_dmg ) fin_dmg = last;
        c->damage += fin_dmg;
        if(consumed) *consumed = fin_dmg;
        notifyChanged(c,true);
        if( c->damage >= c->getMaxBlockHP() ) {
            if(c->untouched) increaseScore(by,BlockToScore(c->bt));
            c->damage = 0;
            int orig_bt = c->bt;
            c->bt = BT_AIR;
            c->content = 0;

            int n = irange(1,4);
            if( orig_bt == BT_IRONORE || orig_bt == BT_RAREMETALORE ) {
                if( range(0,100) < 20 ) n *= 3;
                if( c->untouched == false ) n = 1; 
            } else {
                if( c->untouched == false ) n = 0;      
            }
            
            // BT_AIR first, because debri hits on wall and stop right away
            if(gen_debri) for(int i=0;i<n;i++) new Debri( toCellCenter(at), orig_bt );
            soundPlayAt(g_dig_sound,at,1);
            notifyChanged(c, true );
            notifyChangedAround(getCellPos(c),1);            
            createDigSmoke( toCellCenter(at), 1 );
            notifyChunkToSave(c);
        }
        return true;
    } else if( c->bt == BT_BARRIER || c->bt == BT_CRYSTAL ){ // undestructable, no response
        if(consumed) *consumed = dmg;
        notifyChunkToSave(c);
        return true;
    } else if( c->bt == BT_PAIRBARRIER || c->bt == BT_CRYSTAL_BEAMER ) {  // undestructable, but have response
        if(consumed) *consumed = dmg;
        if(consumed && c->content >= PAIRBARRIER_CONTENT_THRES ) soundPlayAt( g_switch_off_sound, at,1);
        if(consumed) {
            c->content = 0;
            notifyChanged(c);
            // Don't need to save
        }
        return true;
    } else if( c->bt == BT_CELL || c->bt == BT_TREE || c->bt == BT_FLYGEN || c->bt == BT_COPTERGEN || c->bt == BT_SLIMEGEN ||
               c->bt == BT_BIRDGEN || c->bt == BT_IVY || c->bt == BT_BOMBFLOWER || c->bt == BT_BEEHIVE || c->bt == BT_WEAK_CRYSTAL ) {
        int fin_dmg = dmg;
        int last = c->getMaxBlockHP() - c->damage;
        if( last < fin_dmg ) fin_dmg = last;
        c->damage += fin_dmg;        
        if(consumed) *consumed = fin_dmg;
        notifyChanged(c,true);

        if( c->bt == BT_BOMBFLOWER && c->content == 0 ) {
            c->content = 1;
            notifyChanged(c,true);
        }
        if( c->bt == BT_BEEHIVE && c->content==0) {
            soundPlayAt( g_beestart_sound, at,1);
            c->content = BEEHIVE_BEE_NUM;
            notifyChanged(c,true);
        }
        
        // Destroy
        if( c->damage >= c->getMaxBlockHP() ) {
            increaseScore(by,BlockToScore(c->bt));            
            c->damage = 0;
            if( c->bt == BT_TREE && c->content > TREE_CONTENT_RIPE_THRES ) {
                ChunkStat *cs = getChunkStat( at.x/PPC/CHUNKSZ, at.y/PPC/CHUNKSZ);
                if(cs && cs->isTundra() == false ) {
                    new Debri( toCellCenter(at), B_ATLAS_ITEM_APPLE );
                }
            } else if( c->bt == BT_FLYGEN ){
                new Debri( toCellCenter(at), B_ATLAS_MICROBE_PARTICLE );
            } else if( c->bt == BT_COPTERGEN || c->bt == BT_SLIMEGEN || c->bt == BT_BIRDGEN ) {
                if(range(0,100)<50) {
                    new Debri( toCellCenter(at), B_ATLAS_ITEM_RAREMETAL_CRYSTAL );
                } else {
                    new Debri( toCellCenter(at), B_ATLAS_MICROBE_PARTICLE );
                }
            } else if( c->bt == BT_BEEHIVE ) {
                new Debri( toCellCenter(at), B_ATLAS_MICROBE_PARTICLE );                
            }
            if( c->bt == BT_FLYGEN ) {
                int n = (c->hyper_count > 0) ? 20 : 4;
                for(int i=0;i<n;i++) {
                    new Fly(toCellCenter(at));
                }
            }

            if( c->bt == BT_IVY ) {
                destroyConnectedIvy(c);
            } else if( c->bt == BT_BOMBFLOWER || c->bt == BT_TREE ) {
                for(int i=0;i<3;i++) createLeafEffect(at);
            } else if( c->bt == BT_WEAK_CRYSTAL ) {
                new Debri( toCellCenter(at), B_ATLAS_ITEM_IRON_PLATE );
                if(range(0,100)<50) new Debri( toCellCenter(at), B_ATLAS_ITEM_RAREMETAL_CRYSTAL );
            }

            if( c->bt == BT_COPTERGEN || c->bt == BT_TREE || c->bt == BT_WEAK_CRYSTAL ) {
                soundPlayAt(g_destroy_building_sound,at,1);
                createBuildingExplosion( toCellCenter(at) );
            } else if( c->bt != BT_IVY ){
                if( c->bt == BT_CELL )  soundPlayAt(g_killcreature_sound,at,1); else soundPlayAt(g_wormkill_sound,at,1);
                createDigSmoke( toCellCenter(at), 1 );                        
            }
            
            //
            c->content = 0;
            c->damage = 0;
            c->bt = BT_AIR;
            notifyChanged(c,true);
            notifyChunkToSave(c);
        }
        return true;

    } else if( (c->bt >= BT_ENEMY_BUILDING_START && c->bt <= BT_ENEMY_BUILDING_END) || c->isPlayerBuilding() ) {
        int fin_dmg = dmg;
        int maxdmg = c->getMaxBlockHP();
        if( maxdmg > 0 ) {
            int last = maxdmg - c->damage;
            if( last < fin_dmg ) fin_dmg = last;
            c->damage += fin_dmg;
            if(consumed) *consumed = fin_dmg;
            notifyChanged(c,true);
            if( c->damage >= c->getMaxBlockHP() ) {
                Vec2 tgt;
                if( c->hyper_count > 0 && findEnemyAttackTarget(at,&tgt, MACHINE_SHOOT_DISTANCE ) ) {
                    if( c->bt == BT_GRAY_GOO ) {
                        Bullet::shootFanAt( BLT_SPARIO, toCellCenter(at), tgt, 1, M_PI/8.0 );                        
                    } else {
                        Bullet::shootFanAt( BLT_SPARIO, toCellCenter(at), tgt, 8, M_PI/8.0 );
                    }
                }

                print("DESTROYED G: ftid:%d fts:%d",c->fortress_id, c->ftstate );
                increaseScore(by,BlockToScore(c->bt));
                // Destroying ground structure
                if( c->fortress_id > 0 && c->ftstate != FTS_NOT_EYE ) {
                    c->bt = BT_ENEMY_EYE_DESTROYED;
                    destroyFortressEye(c);
                } else {
                    if( c->bt == BT_CAGE ) {
                        if( generateBoss(c) == 0 ) {
                            if( c->isCoreCircuit() == false  ) c->st = ST_GROUNDWORK;
                        }
                    } else if( c->isPlayerBuilding() ) {
                        // no groundwork for PC building
                        c->bt = BT_AIR;
                        notifyChanged(c,true);
                        saveAt(c);
                    } else {
                        bool to_make_debri = true;
                        if( c->bt == BT_GRAY_GOO ) to_make_debri = false;
                        if( c->isCoreCircuit() == false && c->bt != BT_GRAY_GOO ) c->st = ST_GROUNDWORK;
                        if( c->bt == BT_HYPERGEN ) {
                            int n = irange(1,3);
                            for(int i=0;i<n;i++) new Debri( toCellCenter(at), B_ATLAS_ITEM_HYPER_PARTICLE );
                        } 
                        
                        c->bt = BT_AIR;                                                
                        if( range(0,100)<50 && to_make_debri ) {
                            const int cands[] = {
                                B_ATLAS_ITEM_IRON_PLATE,B_ATLAS_ITEM_IRON_PLATE,B_ATLAS_ITEM_IRON_PLATE,
                                B_ATLAS_ITEM_ENERGY_PARTICLE,B_ATLAS_ITEM_ENERGY_PARTICLE,B_ATLAS_ITEM_ENERGY_PARTICLE,
                                B_ATLAS_ITEM_RAREMETAL_CRYSTAL
                            };
                            new Debri( toCellCenter(at), choose(cands) );
                        }
                    }
                }
                c->damage = c->content = 0;
                
                notifyChanged(c,true);
                notifyChunkToSave(c);            
                soundPlayAt( g_destroy_building_sound, at,1 );
                createBuildingExplosion( toCellCenter(at) );
            }
        }
        return true;
    } else if( c->gt == GT_ENHANCER || c->gt == GT_NUCLEUS || c->gt == GT_CORE ) { // Ground after blocks

        // Can shoot at core only when all cover is cleared
        if( c->gt == GT_CORE ) {
            Cell *cells[4];
            get4( Pos2(at.x/PPC,at.y/PPC), cells );
            for(int i=0;i<4;i++) {
                if( cells[i] && cells[i]->bt == BT_CORE_COVER ) {
                    return false;
                }
            }
        }
        
        // No destructable blocks. Look at ground.
        int fin_dmg = dmg;
        int last = c->getMaxGroundHP() - c->damage;
        if( last < fin_dmg ) fin_dmg = last;
        c->damage += fin_dmg;
        if(consumed) *consumed = fin_dmg;
        notifyChanged(c,true);                
        if( c->damage >= c->getMaxGroundHP() ) {
            increaseScore(by,GroundToScore(c->gt));
            c->damage = c->content = 0;
            if( c->gt == GT_ENHANCER || c->gt == GT_NUCLEUS ) {
                new Debri( toCellCenter(at), B_ATLAS_MICROBE_PARTICLE );
                c->gt = GT_ROCK;
                notifyChanged(c,true);
            } else if( c->gt == GT_CORE ) {
                destroyCore(c);
                clearEnemyFrame( Pos2(at.x/PPC,at.y/PPC), 3 );
            }

            soundPlayAt( g_destroy_building_sound, at,1);
            createBuildingExplosion( toCellCenter(at) );
        }
        notifyChunkToSave(c);
        return true;            
    }
    return false;
}

// Returns true when success
bool Field::checkGatherDebri( Debri *d ) {
    if( d->loc.len( g_pc->loc ) < PPC ) {        
        if( g_pc->addItemByDebri(d) ) {
            soundPlayAt(g_pickup_sound,d->loc,1);
            return true;
        }
    }
    return false;
}

// Save with some delay. Avoid too many chunks saved in short time
void Field::notifyChunkToSave( int chx, int chy ) {
    ToSim *ts = getToSim(chx,chy);
    if(ts) ts->last_important_change_at = now();
}

void Field::notifyChunkChanged( int chx, int chy ) {
    if( chx>= 0 && chy >= 0 && chx < chw && chy < chh ) {
        g_mapview->notifyChanged( chx, chy );
    }
}
void Field::notifyChunkChanged( int chx, int chy, int w, int h ) {
    for(int y=chy;y<chy+h;y++) {
        for(int x=chx;x<chx+w;x++) {
            notifyChunkChanged(x,y);
        }
    }
}

// r: half of the length of edge of a squre
void Field::notifyChangedAround( Pos2 at, int r ) {
    int minchx = (at.x - r)/CHUNKSZ;
    int maxchx = (at.x + r)/CHUNKSZ;
    int minchy = (at.y - r)/CHUNKSZ;
    int maxchy = (at.y + r)/CHUNKSZ;
    for(int chy=minchy;chy<=maxchy;chy++) {
        for(int chx=minchx;chx<=maxchx;chx++) {
            notifyChunkChanged(chx,chy);
            
        }
    }    
}

// A cell is changed
void Field::notifyChanged( int x, int y, bool send_far ) {
    notifyChunkChanged( x / CHUNKSZ, y / CHUNKSZ );
    if( isRealtimeNetworkActive() ) {
        Cell *c = get(x,y);
        realtimeSyncCellSend( c, 'N', send_far );
    }
}

void Field::setChunkRevealed( int chx, int chy, bool flg ) {
    if( chx>=0 && chy>=0 && chx < chw && chy < chh ) {
        int ind = chx + chy * chw;
        revealed[ind] = flg;
    }
}
bool Field::getChunkRevealed( int chx, int chy ) {
    if( chx>=0 && chy>=0 && chx < chw && chy < chh ) {
        int ind = chx + chy * chw;
        return revealed[ind];
    } else {
        return false;
    }
}

// Returns true when success. If array is full, false
bool Field::setToSim( int chx, int chy, int cnt ) {
    if( chx<0||chy<0||chx>=chw||chy>=chh) return false;

    for(int i=0;i<elementof(to_sim);i++) {
        if( to_sim[i].chx == chx && to_sim[i].chy == chy ) {
            to_sim[i].cnt = cnt;
            //            if( chx <=1 && chy <=1 )print("HHHHHHHHHHHHHH:%d,%d",chx,chy);
            return true;
        }
    }

    for(int i=0;i<elementof(to_sim);i++) {
        if( to_sim[i].cnt == 0 ) {
            to_sim[i].chx = chx;
            to_sim[i].chy = chy;
            to_sim[i].cnt = cnt;
            return true;
        }
    }
    return false;
}
int Field::countToSim( int *lock_n ) {
    if(lock_n) *lock_n = 0;
    int out=0;
    for(int i=0;i<elementof(to_sim);i++) {
        if( to_sim[i].cnt > 0 ) {
            out++;
            if(lock_n && to_sim[i].lock_state == LOCKSTATE_GOT ) (*lock_n)++;
        }
    }
    return out;
}
// TODO: scan..
ToSim *Field::getToSim( int chx, int chy ) {
    for(int i=0;i<elementof(to_sim);i++) {
        if( to_sim[i].cnt > 0 && to_sim[i].chx == chx && to_sim[i].chy == chy ) {
            return &to_sim[i];
        }
    }
    return NULL;
}

void Field::burn( Vec2 at, Char *by ) {
    Vec2 d[4] = { Vec2(1,1), Vec2(1,-1), Vec2(-1,1), Vec2(-1,-1) };
    for(int i=0;i<elementof(d);i++) {
        Vec2 v = at + (d[i] * (PPC/2) );
        Cell *c = get(v);
        if( !c ) continue;

        if( c->isGroundBeamHit() ) {
            damage( at, BLAST_DAMAGE, NULL, by ); 
        } else if( c->isBlastableGround() ) {
            c->gt = GT_ROCK;
        }
        if( c->st == ST_HYPERLINE || c->st == ST_GROUNDWORK || c->st == ST_ROCK || c->st == ST_WEED ) {            
            c->st = ST_NONE;
            createBuildingExplosion( v );
            soundPlayAt( g_destroy_building_sound, at,1 );
        }
        
        notifyChanged(c);


    }
}

// Put a ground tile only on a ground type
int Field::scatterGSelectG( Pos2 center, int dia, GROUNDTYPE gt_toput, GROUNDTYPE gt_toselect, int n ) {
    int putn = 0;
    for(int i=0;i<n;i++){
        Pos2 randp( irange(-dia,dia+1), irange(-dia,dia+1) );                
        Cell *c = get( center + randp );
        if(!c)continue;
        if(c->gt == gt_toselect) {
            c->gt = gt_toput;
            putn ++;
        }
    }
    return putn;
}
// put a block only on a ground type 
int Field::scatterBSelectGatAir( Pos2 center, int dia, GROUNDTYPE gt_toselect, BLOCKTYPE bt_toput, int n ) {
    int putn = 0;
    for(int i=0;i<n;i++){
        Pos2 randp( irange(-dia,dia+1), irange(-dia,dia+1) );
        Pos2 p = center + randp;
        if( p.len(center) > dia ) continue;
        Cell *c = get(p);
        if(!c)continue;
        if(c->gt == gt_toselect && c->bt == BT_AIR ) {
            c->bt = bt_toput;
            putn++;
        }
    }
    return putn;
}
int Field::scatterGSelectS( Pos2 center, int dia, SURFACETYPE st_toselect, GROUNDTYPE gt_toput, int n ) {
    int putn = 0;
    for(int i=0;i<n;i++){
        Pos2 randp( irange(-dia,dia+1), irange(-dia,dia+1) );
        Pos2 p = center + randp;
        if( p.len(center) > dia ) continue;
        Cell *c = get(p);
        if(!c)continue;
        if(c->st == st_toselect ) {
            c->gt = gt_toput;
            putn++;
        }
    }
    return putn;    
}
int Field::scatterBSelectSatAir( Pos2 center, int dia, SURFACETYPE st_toselect, BLOCKTYPE bt_toput, int n ) {
    int putn = 0;
    for(int i=0;i<n;i++){
        Pos2 randp( irange(-dia,dia+1), irange(-dia,dia+1) );
        Pos2 p = center + randp;
        if( p.len(center) > dia ) continue;
        Cell *c = get(p);
        if(!c)continue;
        if(c->st == st_toselect && c->bt == BT_AIR ) {
            c->bt = bt_toput;
            putn++;
        }
    }
    return putn;    
}


int Field::scatterS( Pos2 center, int dia, SURFACETYPE st, int n, bool put_on_water, float velocity_per_put ) {
    int putn = 0;

    Vec2 vel = Vec2(0,0).randomize(velocity_per_put);
    Vec2 curdiff;
    for(int i=0;i<n;i++){
        Pos2 randp( irange(-dia,dia+1), irange(-dia,dia+1) );
        Pos2 diffp(curdiff.x/PPC, curdiff.y/PPC);
        Pos2 p = center + randp + diffp;
        if( p.len(center+diffp) > dia )continue;
        Cell *c = get(p);
        curdiff += vel;
        if(!c)continue;

        
        bool can_put = true;

        if( c->st == ST_WATER ) {
            if( put_on_water == false ) can_put = false;
        }

        if( can_put ) {
            c->st = st;            
        }

        putn ++;
    }
    return putn;
}

// Returns most distant first (sort from distant to close)
void Field::findPoleNeighbours( Pos2 from, Pos2 *out, int *outnum ) {
    int r = POLE_NEIGHBOUR_THRES;
    Pos2 cands[20];
    int ncand=0;
    SorterEntry sents[elementof(cands)];
    for(int y=-r;y<=r;y++) {
        for(int x=-r;x<=r;x++) {
            Pos2 p = from + Pos2(x,y);
            Cell *c = get(p);
            if(c && c->bt == BT_POLE && p != from ) {
                cands[ncand] =  p;
                sents[ncand].val = p.len(from);
                sents[ncand].ptr = & cands[ncand];
                ncand++;
                if(ncand == elementof(cands) ) break;
            }
        }
        if(ncand == elementof(cands) ) break;        
    }
    
    quickSortF( sents, 0,ncand-1);

    int fin_ind = -1;
    for(int i=0;i<ncand && i < *outnum ; i++ ) {
        Pos2 *p = (Pos2*) sents[i].ptr;
        out[i] = *p;
        fin_ind = i;
    }
    *outnum  = fin_ind + 1;
}

void Field::clearPowerGrid( Pos2 at, int to_clean_id ) {
    int r = POLE_ELECTRICITY_AREA;
    for(int y=-r; y<=r;y++ ) {
        for(int x=-r; x<=r;x++ ) {
            Cell *c = get(at.x+x,at.y+y);
            if(c && c->powergrid_id == to_clean_id ) c->powergrid_id = 0;
        }
    }
    notifyChangedAround( at, r );
    
}
void Field::setPowerGrid(Pos2 at, int id) {
    int r = POLE_ELECTRICITY_AREA;
    for(int y=-r; y<=r;y++ ) {
        for(int x=-r; x<=r;x++ ) {
            Cell *c = get(at.x+x,at.y+y);
            if(c && c->powergrid_id != id ) {
                c->powergrid_id = id;
                notifyChanged(c);
            }
        }
    }
}

int Field::setBlockCircleOn( Pos2 center, float dia, BLOCKTYPE on, BLOCKTYPE toput, float fill_rate ) {
    int put = 0;
    for(int y=-dia-1;y<=dia+1;y++) {
        for(int x=-dia-1;x<=dia+1;x++) {
            Pos2 at = center + Pos2( x, y );
            Cell *c = get(at);
            if(!c)continue;
            bool block_ok = true;
            if( on == BT_INVAL ) block_ok = true; else block_ok = ( on == c->bt );
            if( block_ok && range(0,1) < fill_rate && at.len(center) < dia ) {
                c->bt = toput;
                put++;
            }
        }
    }
    return put;
}

Image *Field::dumpNewImage( bool use_reveal ) {
    Image *img = new Image();
    img->setSize(width,height);
    // Field
    for(int y=0;y<height;y++) {
        for(int x=0;x<width;x++) {
            Cell *c = get(x,y);
            Color col = c->toColor();
            if( use_reveal ) {
                int chx = x/CHUNKSZ, chy = y/CHUNKSZ;
                if( getChunkRevealed(chx,chy) == false ) {
                    col = Color(0,0,0,1);
                }
            }
            img->setPixel(x,height-1-y, col);
        }
    }
    // Flags
    for(int i=0;i<elementof(flag_cands);i++) {
        int x = flag_cands[i].pos.x, y = FIELD_H - 1 - flag_cands[i].pos.y;
        if(isFlagPuttableCell(flag_cands[i].pos) == false ) print( "dumpNewImage: invalid flag:%d,%d", flag_cands[i].pos.x, flag_cands[i].pos.y);
        //        print("flag %d: XY:%d,%d", i, flag_cands[i].pos.x, flag_cands[i].pos.y );
        for(int j=-8;j<=8;j++) {
            if(j==0)continue;
            img->setPixel(x+j,y+j, Color(0,1,1,1) );
            img->setPixel(x+j,y-j, Color(0,1,1,1) );
        }
    }
    return img;
}

// Set bst to 0 then default enemies will stored
void Field::buildBossContainer( Pos2 lb, int size, BOSSTYPE bst ) {
    if( isValidPos(lb) == false || isValidPos( lb + Pos2(size,size) ) == false ) return;
    // Can't put just next to other Cages
    for(int y=lb.y-1;y<lb.y+size+1;y++) {
        for(int x=lb.x-1;x<lb.x+size+1;x++) {
            Cell *c = get(x,y);
            if( !c ) return;
            if( c->bt == BT_CAGE ) return;
        }
    }
            
    for(int y=lb.y;y<lb.y+size;y++) {
        for(int x=lb.x;x<lb.x+size;x++) {
            Cell *c = get(x,y);
            c->bt = BT_CAGE;
            c->content = (int)bst; // Each cell have information about what to generate when destroyed
        }
    }
}


bool Field::buildFortress( bool put_ft_id, Pos2 lbpos, const char **buf, int linenum, Pos2 *center ) {

    // Count X size by looking at the first line of fortress data string
    int longest = 0;
    for(int i=0;i<linenum;i++) {
        int l = strlen(buf[i]);
        if( l > longest ) longest = l;
    }

    int fid;
    if( (fid=hitFortress( lbpos, longest, linenum ) ) ) {
        //        print("buildFortress: hit at(%d,%d) longest:%d linenum:%d fid:%d", lbpos.x, lbpos.y, longest, linenum , fid );
        return false;
    }
    if(center) *center = lbpos + Pos2(longest/2,linenum/2);
    
    // Count blocks for bonus items
    for(int y=lbpos.y;y<=lbpos.y+linenum;y++) {
        for(int x=lbpos.x;x<=lbpos.x+longest;x++) {
            Cell *c = get(x,y);
            if(c && c->bt == BT_CRYSTAL) {
                print("buildFortress: found crystal");
                return false;
            }
        }
    }

    if( put_ft_id ) fortress_id_generator ++;
    int ft_id = fortress_id_generator;
    
    for(int yy=0;yy<linenum;yy++) {
        int y = linenum - 1 - yy;
        for(int x=0;x<longest;x++) {
            Pos2 at = lbpos + Pos2( x,y );
            char ch = buf[yy][x];
            BLOCKTYPE bt = BT_AIR;
            int content = 0;
            if(ch==0)break;
            switch(ch) {
            case 'M': bt = BT_MONO_EYE; break;
            case 'C': bt = BT_COMBI_EYE; break;
            case 'P': bt = BT_PANEL_EYE; break;
            case 'R': bt = BT_ROUND_EYE; break;
            case 'S': bt = BT_SHIELD_EYE; break;
            case '+': bt = BT_ENEMY_FRAME; break;
            case '*': bt = BT_ENEMY_EYE_DESTROYED; break;
            case 'd': bt = BT_DEROTA; break;
            case 'b': bt = BT_BARRIERGEN; break;
            case 't': bt = BT_CAGE; content = (int)BST_TAKWASHI; break;
            case 'g': bt = BT_CAGE; content = (int)BST_GIREV; break;
            case 'c': bt = BT_CAGE; content = (int)BST_CHASER; break;
            case 'r': bt = BT_CAGE; content = (int)BST_REPAIRER; break;
            case 'u': bt = BT_CAGE; content = (int)BST_BUILDER; break;
            case 'v': bt = BT_CAGE; content = (int)BST_VOIDER; break;
            case 'f': bt = BT_CAGE; content = (int)BST_DEFENDER; break;
            case 'H': bt = BT_CHASER_EYE; break;
            case 'B': bt = BT_BUILDER_EYE; break;
            case '$': bt = BT_CRYSTAL; break;
            case 'w': bt = BT_WEAK_CRYSTAL; break;
            case '!':  {
                bt = BT_CAGE;
                content = (int) getRandomSingleCagedBoss(); 
                break;
            }
            case '?': {
                static const BLOCKTYPE eyes[] = { BT_MONO_EYE, BT_COMBI_EYE, BT_PANEL_EYE, BT_ROUND_EYE, BT_SHIELD_EYE, BT_CHASER_EYE };
                bt = choose(eyes);
                break;
            }
            default:
                break;
            }
            Cell *c = get(at);
            assertmsg( c, "invalid pos:%d,%d lbpos:%d,%d", at.x,at.y, lbpos.x, lbpos.y );
            c->bt = bt;
            c->content = content;
            if(c->isEnemyBuilding() && c->gt == GT_PIT ) c->gt = GT_DUNGEON;
            if(c->isEnemyEye() ) c->ftstate = FTS_IDLE;

            if( put_ft_id ) c->fortress_id = ft_id; // Set fortress id even in nothing placed, for fortress-to-fortress collision

        }
    }
    // Put frames around eyes
    for(int y=-1;y<linenum+1;y++) {
        for(int x=-1;x<longest+1;x++) {
            Pos2 at = lbpos + Pos2(x,y);
            Cell *c = get(at);
            if(c && c->isEnemyEye() && c->bt != BT_BUILDER_EYE ) {
                Cell *cells[8];
                get8(at,cells);
                for(int i=0;i<8;i++) {
                    if( cells[i] == NULL ) continue;
                    if( cells[i]->bt == BT_AIR || cells[i]->isEarthBlock() || cells[i]->bt == BT_BARRIERGEN ||
                        cells[i]->bt == BT_CAGE || cells[i]->bt == BT_DEROTA || cells[i]->bt == BT_SNOW || cells[i]->bt == BT_TREE ||
                        cells[i]->bt == BT_IVY || cells[i]->bt == BT_CELL || cells[i]->bt == BT_FIREGEN || cells[i]->bt == BT_FLYGEN ||
                        cells[i]->bt == BT_SLIMEGEN || cells[i]->bt == BT_COPTERGEN || cells[i]->bt == BT_BIRDGEN 
                        ) {
                        cells[i]->bt = BT_ENEMY_FRAME;
                        if(put_ft_id) cells[i]->fortress_id = ft_id;
                        if( cells[i]->gt == GT_PIT ) cells[i]->gt = GT_DUNGEON;
                    }
                }
            }
        }
    }
    return true;
}

// minlv, maxlv : 0 ~ 1
bool Field::buildFortressByLevel( Pos2 center, float minlv, float maxlv ) {
    const char *ft_mono1[] = {
        "M"
    };
    const char *ft_mono1_with_cage4[] = {
        "!   !",
        " +++ ",
        " +M+ ",
        " +++ ",
        "!   !",
    };
    const char *ft_single_random[] = {
        "?"
    };
    const char *ft_double_random_v[] = {
        "??"
    };
    const char *ft_double_random_h[] = {
        "?",
        "?",
    };
    const char *ft_single_random_with_derotas[] = {
        "d   d",
        "     ",
        "  ?  ",
        "     ",
        "d   d",
    };

    const char *ft_single_builder[] = {
        "+M+++B+++M+",
    };
    const char *ft_quad_builder[] = {
        " B ",
        "BSB",
        " B ",
    };
    

    const char *ft_mono4[] = {
        "MM",
        "MM",
    };
    const char *ft_panel4[] = {
        "PP",
        "PP",
    };
    
    const char *ft_cross_random[] = {
        " M ",
        "S?S",
        " M ",
    };
    const char *ft_combi4[] = {
        "CC",
        "CC",
    };
    const char *ft_panel4_mono4[] = {
        "M  M",
        " ?P ",
        " P? ",
        "M  M",
    };
    const char *ft_long_mono4[] = {
        "    M    ",
        "    +    ",
        "    +    ",
        "    +    ",        
        "M+++?+++M",
        "    +    ",
        "    +    ",
        "    +    ",
        "    M    ",        
    };
    const char *ft_long_builder4[] = {
        "     +B+     ",
        "     +++     ",        
        "     +++     ",        
        "      +      ",
        "      +      ",
        "+++   +   +++",        
        "B+++++?+++++B",
        "+++   +   +++",
        "      +      ",
        "      +      ",
        "     +++     ",
        "     +++     ",        
        "     +B+     ",        
    };    
    const char *ft_single_random_with_barrier[] = {
        "b   b",
        "     ",
        "  ?  ",
        "     ",
        "b   b",
    };
    const char *ft_square_middle[] = {
        "M++++?++++M",
        "+         +",
        "+         +",
        "+         +",
        "+         +",
        "?          ",
        "+         +",
        "+         +",
        "+         +",
        "+         +",
        "M++++?++++M",        
    };
    const char *ft_square_large[] = {
        "M++++++?++++++M",
        "+             +",
        "+             +",
        "+             +",
        "+   R+++++R   +",
        "+   +     +   +",
        "+   +     +   +",
        "    +  C  +   ?",
        "+   +     +   +",
        "+   +     +   +",
        "+   R+++++R   +",
        "+             +",
        "+             +",
        "+             +",
        "M++++++?++++++M",                
    };

    
    const char **cands[] = {
        ft_mono1, ft_mono1_with_cage4,
        ft_single_random, ft_mono4, ft_panel4, ft_cross_random,
        ft_combi4, ft_panel4_mono4, ft_long_mono4, ft_single_random_with_derotas,
        ft_double_random_v, ft_double_random_h,
        ft_single_random_with_barrier,
        ft_single_builder, ft_quad_builder,
        ft_long_builder4,
        ft_square_middle, ft_square_large,
    };
    int linenums[] = {
        elementof(ft_mono1), elementof(ft_mono1_with_cage4),        
        elementof(ft_single_random), elementof(ft_mono4), elementof(ft_panel4), elementof(ft_cross_random),
        elementof(ft_combi4), elementof(ft_panel4_mono4), elementof(ft_long_mono4), elementof(ft_single_random_with_derotas),
        elementof(ft_double_random_v), elementof(ft_double_random_h),
        elementof(ft_single_random_with_barrier),
        elementof(ft_single_builder), elementof(ft_quad_builder),
        elementof(ft_long_builder4),
        elementof(ft_square_middle), elementof(ft_square_large)
    };
    int levels[] = {
        0, 1,
        0, 1, 1, 1,
        2, 2, 2, 0,
        1, 1,
        1,
        2, 3,
        4,
        3, 4,
    };

#define CHOOSE_FT_CAND() \
    int indcands[ elementof(levels) ]; \
    int indcand_n=0; \
    int iminlv = minlv * 4, imaxlv = maxlv * 4;\
    for(int i=0;i<elementof(levels);i++) { \
        if( levels[i] >= iminlv && levels[i] <= imaxlv ) { \
            indcands[indcand_n] = i; \
            indcand_n ++; \
        } \
    } \
    int ind; \
    if( indcand_n == 0 ) ind = irange(0,elementof(levels)); else ind = indcands[ irange(0, indcand_n) ]; 

    CHOOSE_FT_CAND();
    Pos2 lbpos = center - Pos2( strlen( cands[ind][0] )/2, linenums[ind]/2 );
    return buildFortress( true, lbpos, cands[ind], linenums[ind], NULL );
}
// lv : 0~1
void Field::buildGrounderByLevel( Pos2 center, float minlv, float maxlv ) {
    const char *ft_derota_array[] = {
        "d d d",
        "     ",
        "d d d",
        "     ",        
        "d d d",
    };
    const char *ft_single_derota[] = {
        "d",
    };
    const char *ft_single_weakcrystal[] = {
        "w",
    };
    const char *ft_quad_weakcrystal[] = {
        "w w",
        "   ",
        "w w",
    };
        
    const char *ft_derota8_h[] = {
        "dddddddd"
    };
    const char *ft_derota8_v[] = {
        "d","d","d","d", "d","d","d","d"
    };
    const char *ft_derota_cross[] = {
        " d ",
        "ddd",
        " d ",
    };
    const char *ft_double_derota_h[] = {
        "dd",
    };
    const char *ft_double_derota_angle[] = {
        "d ",
        " d"
    };
    const char *ft_double_derota_v[] = {
        "d",
        "d"
    };
    const char *ft_pair_derota_h[] = {
        "d!"
    };
    const char *ft_cross_derota[] = {
        " ! ",
        "!d!",
        " ! "
    };
    const char *ft_quad_derota[] = {
        "dd",
        "dd",
    };
    const char *ft_cage1_random[] = {
        "!",
    };
    const char *ft_cage4_random[] = {
        "! !",
        "   ",
        "! !"
    };
    const char *ft_cage4_repairer[] = {
        "r r",
        "   ",
        "r r"        
    };
        
    const char *ft_cage3_girev1[] = {
        "ggg",
        "ggg",
        "ggg",
    };
    const char *ft_cage3_girev1_with_tak[] = {
        "t   t",
        " ggg ",
        " ggg ",
        " ggg ",
        "t   t",
    };    
    const char *ft_cage3_girev4[] = {
        "ggg   ggg",
        "ggg   ggg",
        "ggg   ggg",
        "         ",
        "         ",
        "         ",
        "ggg   ggg",
        "ggg   ggg",
        "ggg   ggg",        
    };
    const char *ft_cage4_defender[] = {
        "f f",
        "   ",
        "f f",
    };
    const char *ft_cage9_defender[] = {
        "f f f",
        "     ",
        "f f f",
        "     ",
        "f f f",        
    };


    const char **cands[] = {
        ft_derota_array, ft_single_derota, ft_double_derota_h, ft_double_derota_v,
        ft_derota_array, ft_single_derota, ft_double_derota_h, ft_double_derota_v,         
        ft_quad_derota, ft_double_derota_angle,
        ft_derota8_v, ft_derota8_h,
        ft_cage1_random, ft_cage4_random,
        ft_cage3_girev1, ft_cage3_girev1_with_tak, ft_cage3_girev4,
        ft_pair_derota_h, ft_cross_derota, ft_cage4_repairer,
        ft_derota_cross,
        ft_cage4_defender, ft_cage9_defender,
        ft_single_weakcrystal, ft_quad_weakcrystal,
    };
    int linenums[] = {
        elementof(ft_derota_array), elementof(ft_single_derota), elementof(ft_double_derota_h), elementof(ft_double_derota_v),
        elementof(ft_derota_array), elementof(ft_single_derota), elementof(ft_double_derota_h), elementof(ft_double_derota_v),        
        elementof(ft_quad_derota), elementof(ft_double_derota_angle),
        elementof(ft_derota8_v), elementof(ft_derota8_h),        
        elementof(ft_cage1_random), elementof(ft_cage4_random),
        elementof(ft_cage3_girev1), elementof(ft_cage3_girev1_with_tak), elementof(ft_cage3_girev4),
        elementof(ft_pair_derota_h), elementof(ft_cross_derota), elementof(ft_cage4_repairer),
        elementof(ft_derota_cross),
        elementof(ft_cage4_defender), elementof(ft_cage9_defender),
        elementof(ft_single_weakcrystal), elementof(ft_quad_weakcrystal),        
    };
    int levels[] = {
        2, 0, 1,1,
        2, 0, 1,1,        
        1, 0, 
        3, 3,
        1,  2,
        2, 3, 3,
        1, 2, 1,
        1,
        3,4,
        0,3,
    };
    
    CHOOSE_FT_CAND();
    Pos2 lbpos = center - Pos2( strlen(cands[ind][0])/2, linenums[ind]/2 );
    //    print("imaxlv:%d %d,%d maxlv:%f",imaxlv, lbpos.x, lbpos.y, maxlv );
    buildFortress( false, lbpos, cands[ind], linenums[ind], NULL );
}
bool Field::buildFortressCoreDefender( Pos2 center, int hint ) {
    const char *ft_cross[] = {
        "   P  ",
        "   C  ",
        "  SCS ",        
        "PCCRCCP",
        "  SCS ",
        "   C  ",
        "   P  ",        
    };
    const char *ft_builder[] = {
        " B  B ",
        "BR++RB",
        " +SS+ ",
        " +SS+ ",
        "BR++RB",
        " B  B ",
    };
    const char *ft_chaser[] = {
        "     HHH     ",
        "   HH + HH   ",
        "  H   +   H  ",
        " H    +    H ",
        " H    +    H ",
        "H   r + r   H",
        "H+++++R+++++H",
        "H   r + r   H",
        " H    +    H ",
        " H    +    H ",
        "  H   +   H  ",
        "   HH + HH   ",
        "     HHH     ",        
    };
    const char *ft_random[] = {
        "?   ?   $   ?   ?",
        "        $        ",
        "        $        ",
        "        $        ",
        "?   ?   $   ?   ?",
        "        $        ",
        "                 ",
        "        C        ",
        "$$$$$$ CCC $$$$$$$",
        "        C        ",
        "                 ",
        "        $        ",
        "?   ?   $   ?   ?",
        "        $        ",
        "        $        ",
        "        $        ",
        "?   ?   $   ?   ?",
    };

    const char **cands[] = { ft_cross, ft_builder, ft_chaser, ft_random };
    int linenums[] = { elementof(ft_cross), elementof(ft_builder), elementof(ft_chaser), elementof(ft_random) };
    int lens[] = { strlen( ft_cross[0]), strlen( ft_builder[0]), strlen( ft_chaser[0]), strlen(ft_random[0]) };

    int ind = hint % elementof(cands);
    Pos2 lbpos = center - Pos2(  lens[ind]/2, linenums[ind]/2 );
    bool res = buildFortress( true, lbpos, cands[ind], linenums[ind], NULL );
    if(!res) {
        print("buildFortressCoreDefender: can't build FT at %d,%d!", lbpos.x, lbpos.y );
    }
    return res;
    
}
int Field::countFortressEyes( Pos2 center, int dia, int ft_id, int *initial_eyes ) {
    int n=0,inin=0;
    for(int y=center.y-dia;y<=center.y+dia;y++) {
        for(int x=center.x-dia;x<=center.x+dia;x++) {
            Cell *c = get(x,y);
            if(c && c->fortress_id == ft_id ) {
                if( c->isEnemyEye() ) n++;
                if( c->ftstate != FTS_NOT_EYE ) inin++;
            }
        }
    }
    if( initial_eyes ) *initial_eyes = inin;
    return n;
}

void Field::destroyFortressEye( Cell *c ) {
    int cx,cy;
    getCellXY(c, &cx, &cy);
    print("destroyFortressEye: %d,%d  id:%d",cx,cy, c->fortress_id );

    int initial_eyes;
    int cur_eyes = countFortressEyes( Pos2(cx,cy), FORTRESS_MAX_DIA, c->fortress_id, &initial_eyes );
    if( cur_eyes == 0 ) {
        print("WIPED!");
        stopSituationBGM();
        stopEffect();
        int score = (1+initial_eyes/2) * initial_eyes * 1000;
        g_pc->modScore( score );
        
        
        g_wipe_fortress_sound->play();
        int ftsize = wipeFortress( Pos2(cx,cy), FORTRESS_MAX_DIA,false);
        realtimeEventSend( EVT_FORTRESS_DESTROYED, g_pc->nickname, ftsize );
        hudFortressDestroyMessage( g_pc->nickname, ftsize, true );

    } else if( cur_eyes <  (initial_eyes/3+2) ) {
        print("GO MAD!");
        wipeFortress( Pos2(cx,cy), FORTRESS_MAX_DIA, true);
    } else {
        print("EYES LEFT" );
    }
}

// Returns nuber of Eyes
int Field::wipeFortress( Pos2 center, int dia, bool mad ) {
    Cell *cc = get(center);
    Pos2 lb = center - Pos2(dia,dia);
    Pos2 rt = center + Pos2(dia,dia);
    int eye_n = 0;
    for( int y = lb.y; y <= rt.y; y++ ) {
        for(int x = lb.x; x <= rt.x; x++ ) {
            Cell *c = get(x,y);
            if(!c)continue;
            if( c->ftstate != FTS_NOT_EYE ) eye_n ++;
            if(mad) {
                // Go mad
                if( c->isEnemyEye() ) {
                    c->ftstate = FTS_MAD;
                }
            } else if( c->fortress_id == cc->fortress_id ) {
                // Destroyed
                if( c->bt == BT_ENEMY_FRAME || c->bt == BT_ENEMY_EYE_DESTROYED ) {
                    Vec2 at = toCellCenter( Vec2(x*PPC,y*PPC) );
                    createBuildingExplosion( at );
                    if( c->bt == BT_ENEMY_EYE_DESTROYED ) {
                        new Debri( at, B_ATLAS_ITEM_ARTIFACT ) ;
                    }
                    if( range(0,100) < 40 ) {
                        new Debri( at, B_ATLAS_ITEM_IRON_PLATE );
                    }
                    if( range(0,100) < 20 ) {
                        new Debri( at, B_ATLAS_ITEM_RAREMETAL_CRYSTAL );
                    }
                    c->bt = BT_AIR;
                    c->gt = GT_ROCK;
                    notifyChanged(c);                    
                } else if( c->isEnemyBuilding() ) {
                    damage( toCellCenter( x,y ), 400, NULL, NULL ); // Damage on everything around, including enemy buildings that aren't eyes
                }
            }
        }
    }
    return eye_n;
}
// Fortress to fortress hit
int Field::hitFortress( Pos2 lb, int w, int h ) {
    for(int y=lb.y;y<=lb.y+h;y++) {
        for(int x=lb.x;x<=lb.x+w;x++) {
            Cell *c = get(x,y);
            if( c && c->fortress_id != 0 ) return c->fortress_id;
        }
    }
    return 0;
}

// Count number of cables next four directions of a cell
int Field::countCableConnection( Cell *c, bool include_exchange ) {
    Cell *uc = getDir( c, DIR_UP );
    Cell *dc = getDir( c, DIR_DOWN );
    Cell *rc = getDir( c, DIR_RIGHT );
    Cell *lc = getDir( c, DIR_LEFT );
    int cnt=0;
    if( include_exchange ) {
        if(uc && uc->isCableConnecting()) cnt++;
        if(dc && dc->isCableConnecting()) cnt++;
        if(rc && rc->isCableConnecting()) cnt++;
        if(lc && lc->isCableConnecting()) cnt++;                
    } else {
        if(uc && uc->isCable()) cnt++;
        if(dc && dc->isCable()) cnt++;
        if(rc && rc->isCable()) cnt++;
        if(lc && lc->isCable()) cnt++;        
    }
    
    return cnt;    
}


void Field::setExchange( Cell *c ) {
    c->bt = BT_EXCHANGE_INACTIVE;
    c->damage = 0;
    notifyChanged(c);
    saveAt(c);
}
void Field::setPortal( Cell *c ) {
    c->bt = BT_PORTAL_INACTIVE;
    c->damage = 0;
    notifyChanged(c);
    saveAt(c);    
}


Vec2 Field::findSafeLoc(Vec2 center) {
    for(int i=1;i<10000;i++) {
        Vec2 at = center.randomize(i * PPC/2);
        Cell *atc = get(toCellCenter(at));
        if( atc && atc->bt == BT_AIR && atc->st == ST_NONE ) {
            return at;
        }
    }
    print("findSafeLoc: couldn't find near %f,%f", center.x, center.y );
    return center;
}

FlagCandidate *Field::findNearestFlagCand( Pos2 from ) {
    float minl = 99999999999;
    FlagCandidate *out = NULL;
    for(int i=0;i<elementof(flag_cands);i++) {
        if( flag_cands[i].finished == false ) {
            float l = from.len(flag_cands[i].pos);
            if( l < minl ) {
                minl = l;
                out = & flag_cands[i];
            }
        }
    }
    return out;
}
bool Field::clearFlag( Pos2 p ) {
    for(int i=0;i<elementof(flag_cands);i++) {
        if( flag_cands[i].pos == p && flag_cands[i].finished == false ) {
            flag_cands[i].finished = true;
            print("clearFlag: finished %d,%d", flag_cands[i].pos.x, flag_cands[i].pos.y );
            return true;
        }
    }
    print("can't find flag at %d,%d", p.x, p.y );
    return false;
}
bool Field::debugClearFlag() {
    for(int i=0;i<elementof(flag_cands);i++) {
        if( flag_cands[i].finished == false ) {
            flag_cands[i].finished = true;
            print("clearFlag: finishing flag %d (%d,%d)", i, flag_cands[i].pos.x, flag_cands[i].pos.y );
            return true;
        }
    }
    return false;
}

bool Field::searchPlayerBuilding( Pos2 center, int distance_cell, Pos2 *outp ) {
    Pos2 lb( center.x - distance_cell, center.y - distance_cell );
    Pos2 rt( center.x + distance_cell, center.y + distance_cell );

    float minl = 99999999999;
    bool found_any = false;
    for(int y=lb.y;y<=rt.y;y++) {
        for(int x=lb.x;x<=rt.x;x++) {
            Cell *c = get(x,y);
            if(c && c->isPlayerBuilding() ) {
                Pos2 p(x,y);
                float l = p.len(center);
                if( l < minl ) {
                    minl = l;
                    *outp = p;
                    found_any = true;
                }
            }
        }
    }
    return found_any;
}
// Returns true if there is something to shoot at (PC and Player buildings)
bool Field::findEnemyAttackTarget( Vec2 center, Vec2 *tgt, float char_distance, float building_distance ) {
    PC *nearestpc = PC::getNearestPC(center); // TODO: avoid simple scanning using cache
    if(nearestpc && center.len(nearestpc->loc) < char_distance ) {
        *tgt = nearestpc->loc;
        return true;
    }
    Pos2 p;
    if( searchPlayerBuilding( Pos2(center.x/PPC,center.y/PPC), building_distance, &p) ) { 
        *tgt = toCellCenter(Vec2( p.x*PPC, p.y*PPC));
        return true;
    }
    return false;
}

bool Field::isValidPos( Pos2 p ) {
    if( p.x <0 || p.y < 0 || p.x >= width || p.y >= height ) return false; else return true;
}

// cell_destroyed: Cell that is actually destroyed.
// Boss enemy will appear always from the center of destroyed cages.
// Returns size of the group of cages
int Field::generateBoss( Cell *cell_destroyed ) {
    int x,y;
    getCellXY(cell_destroyed, &x, &y);
    int maxsz = 5;
    int maxx = x, minx = x, maxy = y, miny = y;
    for(int i=0;i<maxsz;i++) {
        Cell *c = get(x+i,y);
        if(c && c->isBigBossInCage() ) maxx = x+i; else break;
    }
    for(int i=0;i<maxsz;i++) {
        Cell *c = get(x-i,y);
        if(c && c->isBigBossInCage() ) minx = x-i; else break;
    }
    for(int i=0;i<maxsz;i++) {
        Cell *c = get(x,y+i);
        if(c && c->isBigBossInCage() ) maxy = y+i; else break;
    }
    for(int i=0;i<maxsz;i++) {
        Cell *c = get(x,y-i);
        if(c && c->isBigBossInCage() ) miny = y-i; else break;
    }

    print("generateBoss: XY:%d,%d minxy:%d,%d maxy:%d,%d", x,y, minx,miny, maxx, maxy );

    BOSSTYPE bst = BST_DEFAULT;
    for(int y=miny;y<=maxy;y++) {
        for(int x=minx;x<=maxx;x++) {
            Cell *c = get(x,y);
            c->bt = BT_AIR;
            c->gt = GT_ROCK;
            c->st = ST_GROUNDWORK;
            bst = (BOSSTYPE) c->content;
            c->content = 0;
            createBuildingExplosion( toCellCenter(x,y) );
            notifyChanged(c);
        }
    }
    Vec2 center = (Vec2(minx*PPC,miny*PPC) + Vec2((maxx+1)*PPC,(maxy+1)*PPC))/2.0f;

    switch(bst) {
    case BST_GIREV:
        new Girev( center);
        break;
    case BST_TAKWASHI:
    case BST_DEFAULT:        
        new Takwashi( center );
        break;
    case BST_CHASER:
        new Chaser( center);
        break;
    case BST_BUILDER:
        new Builder(center);
        break;
    case BST_REPAIRER:
        new Repairer(center);
        break;
    case BST_VOIDER:
        new Voider(center);
        break;
    case BST_DEFENDER:
        new Defender(center);
        break;
    }
    return maxx-minx;
}

// Get 8 pointers except center
void Field::get8(Pos2 center, Cell *out[8] ) {
    out[0] = get(center.add(0,1)); // T
    out[1] = get(center.add(1,1)); // RT
    out[2] = get(center.add(1,0)); // R
    out[3] = get(center.add(1,-1)); // RD
    out[4] = get(center.add(0,-1)); // D
    out[5] = get(center.add(-1,-1)); // LD
    out[6] = get(center.add(-1,0)); // L
    out[7] = get(center.add(-1,1)); // LT
}
void Field::get4(Pos2 center, Cell *out[4] ) {
    out[0] = get(center.add(0,1)); // T
    out[1] = get(center.add(1,0)); // R
    out[2] = get(center.add(0,-1)); // D
    out[3] = get(center.add(-1,0)); // L
}
void Field::getCorner4( Vec2 center, float sz, Cell **lb, Cell **rb, Cell **lt, Cell **rt ) {
    *lb = get( center + Vec2(-sz,-sz) );
    *rb = get( center + Vec2(sz,-sz) );
    *lt = get( center + Vec2(-sz,sz) );
    *rt = get( center + Vec2(sz,sz) );
}


// Returns distance(in cell) to the place if found.
int Field::checkBarrierGen( Pos2 from, DIR dir, int maxlen ) {
    Pos2 cur = from;
    int found_pair_i = -1;
    for(int i=0;i<maxlen;i++) {
        cur = cur + dirToPos2(dir);
        Cell *c = get(cur);
        if(!c)return -1;
        
        if( c->bt == BT_AIR || c->bt == BT_BARRIER ) {
            continue;
        } else if( c->bt == BT_BARRIERGEN || c->bt == BT_PAIRBARRIER ) {
            found_pair_i = i;
            break;
        } else {
            return -1;
        }
    }
    return found_pair_i;
}
void Field::checkPutBarrier( Pos2 from, DIR dir, int maxlen, int contentvol ) {
    int dx,dy;
    dirToDXDY(dir,&dx,&dy);

    int found_pair_i;
    if( (found_pair_i = checkBarrierGen(from,dir,maxlen)) < 0 ) return ;
    Pos2 cur = from;
    for(int i=0;i<found_pair_i;i++) {
        cur = cur + Pos2(dx,dy);
        Cell *c = get(cur);
        if( c->bt != BT_BARRIER ) {
            c->bt = BT_BARRIER;
            notifyChanged(c);
        } 
        c->content = contentvol;
    }
}
bool Field::checkReactorPuttable( Pos2 at ) {
    Cell *c = get(at);
    
    if(c&&c->st==ST_NONE) {
        Cell *uc = get(at+Pos2(0,1));
        Cell *dc = get(at+Pos2(0,-1));
        Cell *rc = get(at+Pos2(1,0));
        Cell *lc = get(at+Pos2(-1,0));
        return ( (uc && uc->isReactorArmPuttable()) || (dc && dc->isReactorArmPuttable()) ||
                 (lc && lc->isReactorArmPuttable()) || (rc && rc->isReactorArmPuttable()) );
    } else {
        return false;
    }
}
void Field::buildLongFence( Pos2 from) {
    DIR curdir = randomDir();
    int maxloop = irange(80,300);
    int loopcnt = 0;
    Pos2 curp = from;
    int interval = 10;
    while(1) {
        loopcnt++;
        if( loopcnt > maxloop ) break;
        curp = curp + dirToPos2(curdir);
        Cell *c = get(curp);
        if( (loopcnt%interval) == 0 ) {
            if(c && (c->bt == BT_AIR||c->isEarthBlock() ) ) {
                c->bt = BT_BARRIERGEN;
                interval = irange(7,11);
                if( range(0,100) < 10 ) {
                    if(range(0,100)<50) curdir = rightDir(curdir); else curdir = leftDir(curdir);
                }
            }
        } else {
            if(c && (c->bt == BT_FLYGEN || c->isEarthBlock() )  ) {
                c->bt = BT_AIR;
            }
        }
    }
}

void Field::buildRandomGrounder( Pos2 at) {
    Cell *c = g_fld->get(at);
    if( c && c->isBlockPuttable() ) {
        switch( irange(0,10) ) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:            
            c->bt = BT_DEROTA;
            break;
        case 6:
            c->bt = BT_BARRIERGEN;
            break;
        case 7:
        case 8:
        case 9:
            c->setCage( getRandomSingleCagedBoss() );
            break;
        }
        soundPlayAt(g_repair_done_sound,toCellCenter(at),1);
        notifyChanged(c);
    }
}

void Field::buildRiver( Pos2 from, int nloop, SURFACETYPE st ) {
    Vec2 cur( from.x*PPC, from.y*PPC);
    Vec2 v = Vec2(0,0).randomize(PPC/2);
    int minw, maxw;
    for(int i=0;i<nloop;i++){
        minw = 1;
        maxw = (nloop-i) / 100;
        if( maxw == 0 ) break;
        cur += v;
        v += Vec2(0,0).randomize(4);
        v = v.normalize(PPC/2);
        Pos2 center(cur.x/PPC, cur.y/PPC);
        int w = irange(minw,maxw+1);
        Rect r( center, center + Pos2(w,w) );
        fillRectSG(r, st, GT_SAND );
    }
}
void Field::fillRectSG( Rect r, SURFACETYPE st, GROUNDTYPE gt ) {
    for(int y=r.min.y; y <= r.max.y; y++ ) {
        for(int x=r.min.x; x <= r.max.x; x++) {
            Cell *c = get(x,y);
            if(c) {
                c->st = st;
                c->gt = gt;
            }
        }
    }
}

void Field::fillRectG( Rect r, GROUNDTYPE gt, bool to_notify ) {
    for(int y=r.min.y; y <= r.max.y; y++ ) {
        for(int x=r.min.x; x <= r.max.x; x++) {
            Cell *c = get(x,y);
            if(c) {
                c->gt = gt;
                if(to_notify) notifyChanged(c);
            }
        }
    }
}

void Field::fillRectB( Rect r, BLOCKTYPE bt ) {
    for(int y=r.min.y; y <= r.max.y; y++ ) {
        for(int x=r.min.x; x <= r.max.x; x++) {
            Cell *c = get(x,y);
            if(c) {
                c->bt = bt;
            }
        }
    }
}

void Field::fillRectFortressId( Rect r, int fid ) {
    for(int y=r.min.y; y <= r.max.y; y++ ) {
        for(int x=r.min.x; x <= r.max.x; x++) {
            Cell *c = get(x,y);
            if(c) {
                c->fortress_id = fid;
            }
        }
    }
}


void Field::fillRectCage( Rect r, BOSSTYPE bst ) {
    for(int y=r.min.y; y <= r.max.y; y++ ) {
        for(int x=r.min.x; x <= r.max.x; x++) {
            Cell *c = get(x,y);
            if(c) {
                c->setCage(bst);
            }
        }
    }
}

void Field::edgeRectB( Rect r, BLOCKTYPE bt ) {
    for(int y=r.min.y; y <= r.max.y; y++ ) {
        for(int x=r.min.x; x <= r.max.x; x++) {
            if( x > r.min.x && x < r.max.x && y > r.min.y && y < r.max.y ) continue;
            Cell *c = get(x,y);
            if(c) {
                c->bt = bt;
            }
        }
    }    
}

void Field::fillCircleGSB( Pos2 center, float dia, GROUNDTYPE gt, SURFACETYPE st, BLOCKTYPE bt ) {
    int minx = center.x - dia, maxx = center.x + dia;
    int miny = center.y - dia, maxy = center.y + dia;
    for(int y=miny; y <= maxy; y++ ) {
        for(int x=minx; x <= maxx; x++) {
            Pos2 p(x,y);
            float l = center.len(p);
            if( l > dia ) continue;
            Cell *c = get(p);
            if(c) {
                c->gt = gt;
                c->st = st;
                c->bt = bt;
            }
        }
    }    
}

bool Field::isClearPlace( Rect r ) {
    for(int y=r.min.y; y <= r.max.y; y++ ) {
        for(int x=r.min.x; x <= r.max.x; x++) {
            Cell *c = get(x,y);
            if(!c)return false;
            if( c->bt != BT_AIR)return false;
            if( c->isWall(false) ) return false;
            if( c->isWalkableSurface() == false ) return false;
        }
    }
    return true;
}

// Paint the vent then put hardrocks around
void Field::buildVent( Pos2 center ) {
    int n = irange(1,7);
    for(int i=0;i<n;i++) {
        Pos2 p = center.randomize(4);
        fillCircleGSB( p, irange(2,4), GT_VENT, ST_NONE, BT_AIR );
    }

    Rect r( center + Pos2(-10,-10), center + Pos2(10,10) );
    paintEdgeGB( r, GT_VENT, BT_HARDROCK );
    paintEdgeBB( r, BT_HARDROCK, BT_HARDROCK, GT_VENT );

    Cell *corec = get(center);
    if(corec && corec->gt == GT_VENT ) {
        corec->gt = GT_VENTCORE;
    }
}
// used for building vent
int Field::paintEdgeGB( Rect r, GROUNDTYPE gt, BLOCKTYPE bt ) {
    int cnt = 0;
    for(int y=r.min.y; y <= r.max.y; y++ ) {
        for(int x=r.min.x; x <= r.max.x; x++) {
            Cell *c = get(x,y);
            if(!c)continue;
            if( c->gt == gt ) continue;
            Cell *uc = get(x,y+1), *dc = get(x,y-1), *rc = get(x+1,y), *lc = get(x-1,y);
            if( (uc && uc->gt == gt ) || (dc && dc->gt == gt ) || (rc && rc->gt == gt) || (lc && lc->gt == gt ) ) {
                c->bt = bt;
                cnt++;
            }
        }
    }
    return cnt;
}
// used for building vent
int Field::paintEdgeBB( Rect r, BLOCKTYPE bt_from, BLOCKTYPE bt_toset, GROUNDTYPE gt_avoid ) {
    PosSet ps( r.area() );
    for(int y=r.min.y; y <= r.max.y; y++ ) {
        for(int x=r.min.x; x <= r.max.x; x++) {
            Cell *c = get(x,y);
            if(!c)continue;
            if( c->bt == bt_from ) continue;
            if( c->gt == gt_avoid ) continue;
            Cell *uc = get(x,y+1), *dc = get(x,y-1), *rc = get(x+1,y), *lc = get(x-1,y);
            if( (uc && uc->bt == bt_from ) || (dc && dc->bt == bt_from ) || (rc && rc->bt == bt_from) || (lc && lc->bt == bt_from ) ) {
                ps.add(Pos2(x,y));
            }
        }
    }
    for(int i=0;i<ps.nused;i++) {
        Cell *c = get(ps.ary[i]);
        c->bt = bt_toset;
    }
    return ps.nused;
}
// used for building vent
void Field::getConnectedGround( Pos2 center, PosSet *outset, GROUNDTYPE gt ) {
    outset->add(center);

    int maxiter = 30;
    for(int j=0;j<maxiter;j++ ) {
        int prev_nused = outset->nused;
        for(int i=0;i<outset->nused;i++) {
            Pos2 p = outset->ary[i];
            Pos2 up = p+Pos2(0,1), dp = p+Pos2(0,-1), rp = p+Pos2(1,0), lp = p+Pos2(-1,0);
            Cell *uc = get(up), *dc = get(dp), *lc = get(lp), *rc = get(rp);
            
            if(uc && uc->gt == gt && outset->find(up) == false ) {
                outset->add(up);
            }
            if(dc && dc->gt == gt && outset->find(dp) == false ) {
                outset->add(dp);
            }
            if(lc && lc->gt == gt && outset->find(lp) == false ) {
                outset->add(lp);
            }
            if(rc && rc->gt == gt && outset->find(rp) == false ) {
                outset->add(rp);
            }
        }
        if( prev_nused == outset->nused ) {
            break;
        }
    }
}
// used for building vent
void Field::paintContentInsideG( Pos2 from, GROUNDTYPE gt, int content ) {
    PosSet ps(1024);
    getConnectedGround( from, &ps, GT_VENT );
    for(int i=0;i<ps.nused;i++){
        Pos2 p = ps.ary[i];
        Cell *c = get(p);
        if(c && c->gt == GT_VENT) {
            c->content = content;
            notifyChanged(c);
        }
    }
}
void Field::explodeVent( Pos2 center ) {
    PosSet ps(1024);
    getConnectedGround( center, &ps, GT_VENT );
    for(int i=0;i<ps.nused;i++) {
        Pos2 p = ps.ary[i];
        //        Bullet::shootAt( BLT_SPARIO, Vec2(p.x*PPC,p.y*PPC), Vec2(p.x*PPC,p.y*PPC).randomize(PPC) );
        new VolcanicBomb( Vec2(p.x*PPC,p.y*PPC) );
    }
        
}

void Field::scatterLava( Pos2 center ) {
    for(int y=center.y-1; y <= center.y+1; y++ ) {
        for(int x=center.x-1; x <= center.x+1; x++) {
            Cell *c = g_fld->get(x,y);
            if(c && c->gt != GT_LAVA_HOT && c->isVent() == false ) {
                c->gt = GT_LAVA_HOT;
                if( c->bt == BT_CELL ) {
                    c->bt = BT_AIR;
                    c->content = 0;
                }
                g_fld->notifyChanged(c);
            }
        }
    }
                
}

void Field::putFiregen( Pos2 at, DIR d ) {
    Cell *c = get(at);
    if(c && (c->bt == BT_AIR || c->isEarthBlock() ) ) {
        c->bt = BT_FIREGEN;
        c->dir = d;
    }
}

void Field::buildFiregenCrack( Pos2 from ) {
    Vec2 cur = toCellCenter(from);
    Vec2 v = Vec2(0,0).randomize(1).normalize(PPC/2);
    int n = irange(4,100);
    for(int i=0;i<n;i++) {
        cur += v;
        v += Vec2(0,0).randomize(PPC/4);
        Cell *c = get(cur);
        if(c&&c->gt == GT_LAVA_COLD) {
            c->gt = GT_FIREGEN;
        }
    }
}

void Field::updateChunkStat( int chx, int chy ) {
    int tundra=0, iron=0, raremetal=0, water=0;
    for(int y=chy*CHUNKSZ;y<(chy+1)*CHUNKSZ;y++) {
        for(int x=chx*CHUNKSZ;x<(chx+1)*CHUNKSZ;x++) {
            Cell *c = get(x,y);
            if( c->gt == GT_TUNDRA || c->gt == GT_SNOW ) tundra++;
            if( c->st == ST_WATER ) water++;
            if( c->bt == BT_IRONORE ) iron++;
            if( c->bt == BT_RAREMETALORE ) raremetal++;
        }
    }
    ChunkStat *cs = getChunkStat(chx,chy);
    cs->tundra = tundra;
    cs->iron = iron;
    cs->raremetal = raremetal;
    cs->water = water;
}
ChunkStat *Field::getChunkStat(int chx, int chy ) {
    if( chx <0 || chy<0 || chx>=chw || chy >= chh ) return NULL;
    
    int ind = chx + chy * chw;
    return & chstats[ind];
}
void Field::dumpChunkStat() {
    for(int chy=0;chy<chh;chy++) {
        for(int chx=0;chx<chw;chx++) {
            ChunkStat *cs = & chstats[chx+chy*chw];
            prt("%d", cs->isTundra() ? 1 : 0 );
        }
        print("");
    }
}

void Field::meltSnow( Vec2 at ) {
    Cell *c = get(at);
    if(!c)return;
    
    if( c->gt == GT_SNOW ) {
        c->gt = GT_TUNDRA;
        notifyChanged(c);
        createSmokeEffect(at);
    }

    // Don't melt nearby a PC for avoiding stuck
    if(at.len(g_pc->loc) >PPC*2 ){
        if( c->st == ST_ICE ) {
            c->st = ST_WATER;
            notifyChanged(c);
            createSmokeEffect(at);
        }
    }
}

// Randomly
bool Field::tryPutBlockNearestOnAir(Vec2 center, BLOCKTYPE bt ) {
    for(int i=0;i<30;i++) {
        Vec2 at = center.randomize( i * 4 );
        Cell *c = get(at);
        if(c && c->bt == BT_AIR ) {
            c->bt = bt;
            notifyChanged(c);
            return true;
        }
    }
    return false;
}

int Field::countBlock( Vec2 center, float dia, BLOCKTYPE bt ) {
    int cnt = 0;
    Pos2 p( center.x/PPC, center.y/PPC);
    for(int y=p.y-dia;y <=p.y+dia;y++) {
        for(int x=p.x-dia;x<=p.x+dia; x++ ) {
            Cell *c = get(x,y);
            if(c && c->bt == bt ) cnt++;
        }
    }
    return cnt;
}

// 
void Field::getRectCorner(Vec2 center, float sz, Cell *out[4]) {
    out[0] = get( center + Vec2(sz,sz) ); // TR
    out[1] = get( center + Vec2(sz,-sz) ); // DR
    out[2] = get( center + Vec2(-sz,sz) ); // TL
    out[3] = get( center + Vec2(-sz,-sz) ); // DL
}


void Field::tryPutIvy( Pos2 at ) {
    int l = irange(1,12);
    Cell *c = get(at);
    if(c && c->bt==BT_AIR) {
        c->bt = BT_IVY;
        c->clearConnect();
    } else {
        return;
    }

    if(l==1) return;

    Pos2 p = at;
    DIR d = randomDir();
    for(int i=0;i<l;i++) {
        Cell *c = get(p);
        Pos2 np = p + Pos2::fromDir(d);
        Cell *nc = get(np);
        if( nc && nc->bt == BT_AIR ){ //&& nc->isWater() == false ) {
            nc->bt = BT_IVY;
            c->setConnect( d );
            nc->setConnect( reverseDir(d) );
        } else {
            break;
        }
        p = np;
        if( range(0,100) < 30 ) d = rightDir(d);
    }
}
// Assuming all IVYs are short
void Field::destroyConnectedIvy(Cell *c) {
    int x,y;
    getCellXY(c,&x,&y);
    for(int i=0;i<3;i++)createLeafEffect( toCellCenter(x,y));
    c->bt = BT_AIR;
    notifyChanged(c);
    soundPlayAt( g_cutweed_sound, toCellCenter(x,y),1);
    if( c->isConnected(DIR_RIGHT) ) {
        Cell *rc = get(x+1,y);
        if(rc && rc->bt == BT_IVY ) destroyConnectedIvy(rc);
    }
    if( c->isConnected(DIR_LEFT) ) {
        Cell *lc = get(x-1,y);
        if(lc && lc->bt == BT_IVY ) destroyConnectedIvy(lc);        
    }
    if( c->isConnected(DIR_UP) ) {
        Cell *uc = get(x,y+1);
        if(uc && uc->bt == BT_IVY ) destroyConnectedIvy(uc);
    }
    if( c->isConnected(DIR_DOWN) ) {
        Cell *dc = get(x,y-1);
        if(dc && dc->bt == BT_IVY ) destroyConnectedIvy(dc);
    }
}

void Field::scatterIvy( Pos2 center, int dia, int n  ) {
    for( int i = 0; i < n; i++ ) {
        Pos2 p = center.randomize(dia) ;
        if(p.len(center) > dia) continue;
        tryPutIvy(p);
    }
}

void Field::destroyCore( Cell *c ) {
    int x,y;
    getCellXY(c,&x,&y);
    
    c->gt = GT_ROCK;
    createBuildingExplosion( toCellCenter(x,y) );
    
    

    Cell *cells[8];
    get8( Pos2(x,y), cells );
    for(int i=0;i<8;i++) {
        if(cells[i] && cells[i]->gt == GT_CORE ) {
            new Debri( toCellCenter(x,y), B_ATLAS_ITEM_DARK_MATTER_PARTICLE );
            cells[i]->gt = GT_ROCK;
            int x,y;
            getCellXY( cells[i], &x, &y);
            createBuildingExplosion( toCellCenter(x,y));
        }
    }
    stopEffect();
    g_wipe_fortress_sound->play();
    hudCoreDestroyMessage( g_pc->nickname, true );
}

void Field::clearEnemyFrame( Pos2 center, int dia ) {
    for(int y=center.y-dia; y <= center.y+dia; y++ ) {
        for(int x=center.x-dia; x <= center.x+dia; x++ ) {
            Cell *c = get(x,y);
            if(c && c->bt == BT_ENEMY_FRAME ) {
                c->bt = BT_AIR;
                c->gt = GT_ROCK;
                notifyChanged(c);
            }
        }
    }
}




// Always square shaped
// Room width is always odd-numbered, because need the center core room
void Field::buildDungeon( Pos2 center, int sz ) {
    int roomsz = 32;
    int roomnum = sz/roomsz;
    if( (roomnum%2)==0) roomnum++;
    print("buildDungeon: roomnum:%d", roomnum );
    assert( roomnum >= 5 ); // center is core, next 8 rooms to the center is a core defender fortress
    
    Maze *mz = new Maze(roomnum, roomnum);

    // entrance
    mz->setConnection( 1, 0, DIR_DOWN, 'E' ); // Entrance in down direction  (デバッグ効率用)      
    mz->setConnection( irange(0,mz->width), 0, DIR_DOWN, 'E' ); // down direction
    mz->setConnection( irange(0,mz->width), mz->height-1, DIR_UP, 'E' ); // up 
    mz->setConnection( 0, irange(0,mz->height), DIR_LEFT, 'E' ); // left
    mz->setConnection( mz->width-1, irange(0,mz->height), DIR_RIGHT, 'E' ); // right

    mz->makeAllConnected();
    
    // Center core room(C) has 4 collidor direction (up,down,right,left)
    // C:core F:fortress R:random  E:Entrance    default is random(=0)
    //  R R R R R
    //    | | |
    //  R-F-R-F-R
    //    | | |
    //  R-R-C-R-R
    //    | | |
    //  R-F-R-F-R
    //    | | |
    //  R R R R R

    mz->setConnection4( mz->width/2, mz->height/2, 'C' );
    mz->setConnection4( mz->width/2-1, mz->height/2, 0 );
    mz->setConnection4( mz->width/2+1, mz->height/2, 0 );
    mz->setConnection4( mz->width/2, mz->height/2-1, 0 );
    mz->setConnection4( mz->width/2, mz->height/2+1, 0 );
    mz->setConnection4( mz->width/2+1, mz->height/2+1, 'F' );
    mz->setConnection4( mz->width/2+1, mz->height/2-1, 'F' );
    mz->setConnection4( mz->width/2-1, mz->height/2+1, 'F' );    
    mz->setConnection4( mz->width/2-1, mz->height/2-1, 'F' );            

    mz->makeAllConnectionsPair();

    // Room type and connection setting is finished!
    mz->dump();

    // Paint all background
    Pos2 lbpos = center - Pos2(sz/2,sz/2);
    Pos2 rtpos = center + Pos2(sz/2,sz/2);
    fillRectSG( Rect(lbpos,rtpos), ST_NONE,GT_PIT );
    fillRectB( Rect(lbpos,rtpos), BT_AIR );

    setEnvironment( Rect( lbpos, rtpos), BMT_DUNGEON );
    
    // Blur the edges of dungeon to be natural
    {
        int nrect = sz;
        int mgn = 12;
        Rect r(lbpos, rtpos );
        for(int i=0;i<nrect;i++) {
            Pos2 p = r.getRandomEdge( randomDir() );
            Rect r( p - Pos2( irange(0,mgn), irange(0,mgn) ),
                    p + Pos2( irange(0,mgn), irange(0,mgn) ) );
            fillRectSG( r, ST_NONE, GT_PIT );
            fillRectB( r, BT_AIR );            
        }
    }

    // Floating small rooms
    {
        int nbg = (sz*sz)/500;
        int bgsz = 3;
        for(int i=0;i<nbg;i++) {
            Pos2 p = center + Pos2( irange(-sz/2,sz/2), irange(-sz/2,sz/2) );
            Rect r( p - Pos2( irange(0,bgsz), irange(0,bgsz ) ),
                    p + Pos2( irange(0,bgsz), irange(0,bgsz ) ) );
            fillRectG( r, GT_DUNGEON );
            float rnd = range(0,100);

            if( rnd < 30 ) {
                fillRectB( r, BT_DEROTA );
            } else if( rnd < 50 ) {
                fillRectCage( r, getRandomSingleCagedBoss() );
            }
        }
    }
    
    // Build!
    int ft_cnt=0;
    Pos2 ft_centers[4];
    Pos2 core_center;
    for(int ry=0;ry<mz->height;ry++) {
        for(int rx=0;rx<mz->width;rx++) {
            Pos2 room_lb = lbpos + Pos2( rx * roomsz , ry * roomsz  );
            Pos2 room_rt = lbpos + Pos2( rx * roomsz + roomsz - 1, ry * roomsz + roomsz - 1 );
            Rect rect(room_lb, room_rt);
            MazeNode *node = mz->get(rx,ry);
            if( node->type == 'E' ) {
                if(rx==0) {
                    node->rect = buildEntranceRoom( rect, DIR_LEFT, node );
                } else if(ry==0) {
                    node->rect = buildEntranceRoom( rect, DIR_DOWN, node );
                } else if(rx==mz->width-1) {
                    node->rect = buildEntranceRoom( rect, DIR_RIGHT, node );
                } else if(ry==mz->height-1) {
                    node->rect = buildEntranceRoom( rect, DIR_UP, node );
                }
            } else if( node->type == 'F' ) {
                node->rect = buildFortressRoom( rect );
                buildFortressCoreDefender( rect.center(), ft_cnt );
                ft_centers[ft_cnt] = rect.center();
                ft_cnt++;
            } else if( node->type == 'C' ) {
                node->rect = buildCoreRoom( rect );
                core_center = rect.center();
            } else if( node->type == 0 ) {
                if( range(0,100) < 30 ) {
                    node->rect = buildCorridorRoom( rect );
                } else {
                    node->rect = buildRandomRoom( rect, node );
                }
            }
        }
    }


    // Collidor. Need to place only right and down side. Avoiding dupe.
    for(int ry=0;ry<mz->height;ry++) {
        for(int rx=0;rx<mz->width;rx++) {
            MazeNode *node = mz->get(rx,ry);
            if( node->con_right ) {
                MazeNode *rnode = mz->get(rx+1,ry);
                if(rnode) {
                    Pos2 from = node->rect.getCenterEdge(DIR_RIGHT);
                    Pos2 to = rnode->rect.getCenterEdge(DIR_LEFT);
                    GROUNDTYPE gt = GT_DUNGEON;
                    if(range(0,100)<20) gt = GT_TELEPORTER_INACTIVE;
                    buildCorridorGS( from, to, 2, DIR_RIGHT, gt, ST_NONE );
                }
            }
            if( node->con_down ) {
                MazeNode *dnode = mz->get(rx,ry-1);
                if(dnode) {
                    Pos2 from = node->rect.getCenterEdge(DIR_DOWN);
                    Pos2 to = dnode->rect.getCenterEdge(DIR_UP);
                    GROUNDTYPE gt = GT_DUNGEON;
                    if(range(0,100)<20) gt = GT_TELEPORTER_INACTIVE;                    
                    buildCorridorGS( from, to, 2, DIR_DOWN, gt, ST_NONE );
                }
            }
        }
    }

    // Write collidor and put cable
    {
        assert( ft_cnt == 4 );    
        int minx = mini( mini( ft_centers[0].x, ft_centers[1].x ), mini( ft_centers[2].x, ft_centers[3].x ) );
        int miny = mini( mini( ft_centers[0].y, ft_centers[1].y ), mini( ft_centers[2].y, ft_centers[3].y ) );
        int maxx = maxi( maxi( ft_centers[0].x, ft_centers[1].x ), maxi( ft_centers[2].x, ft_centers[3].x ) );
        int maxy = maxi( maxi( ft_centers[0].y, ft_centers[1].y ), maxi( ft_centers[2].y, ft_centers[3].y ) );
        Pos2 ft_lb( minx, miny ), ft_rb(maxx,miny), ft_lt(minx,maxy), ft_rt(maxx,maxy);

        Cell *c;        
        buildCorridorGS( core_center + Pos2(0,-1), ft_lb, 1, DIR_DOWN, GT_DUNGEON, ST_CORE_CABLE_INACTIVE );
        buildCorridorGS( core_center + Pos2(2,0), ft_rb, 1, DIR_RIGHT, GT_DUNGEON, ST_CORE_CABLE_INACTIVE );
        buildCorridorGS( core_center + Pos2(-1,1), ft_lt, 1, DIR_LEFT, GT_DUNGEON, ST_CORE_CABLE_INACTIVE );
        buildCorridorGS( core_center + Pos2(1,2), ft_rt, 1, DIR_UP, GT_DUNGEON, ST_CORE_CABLE_INACTIVE );
        c = get( ft_centers[0] ); c->st = ST_CORE_SWITCH_INACTIVE;
        c = get( ft_centers[1] ); c->st = ST_CORE_SWITCH_INACTIVE;
        c = get( ft_centers[2] ); c->st = ST_CORE_SWITCH_INACTIVE;
        c = get( ft_centers[3] ); c->st = ST_CORE_SWITCH_INACTIVE;        
    }    
}


// Returns a rectangle of a real room
// Room has the minimum size 
Rect Field::buildRandomRoom( Rect rect, MazeNode *node ) {
    Pos2 center = rect.center();
    //    center = center.randomize(10);
    
    int rw = irange(10,28), rh = irange(10,28);
    Pos2 lb = center - Pos2(rw/2,rh/2);
    Pos2 rt = center + Pos2(rw/2,rh/2);
    Rect rr(lb,rt);

    fillRectB( rr, BT_AIR );
    
    float r = range(0,100);
    if( r < 10 ) {
        // teleporter room
        fillRectSG( rr, ST_NONE, GT_TELEPORTER_INACTIVE );
    } else if( r < 40 ){
        // shifter room
        fillRectSG( rr, ST_NONE, GT_DUNGEON );        
        fillShifter(rr);
    } else {
        //  normal room
        fillRectSG( rr, ST_NONE, GT_DUNGEON );
    }

    // Inside a room
    r = range(0,100);
    if( r < 30 ) {
        // Stems at the 4 corner of a room
        int w = irange(1,3);
        Pos2 lb = rr.in( 0.2, 0.2 ), rt = rr.in( 0.8, 0.8 ), lt = rr.in( 0.2, 0.8 ), rb = rr.in( 0.8, 0.2 );        
        BLOCKTYPE bt = BT_CRYSTAL;
        switch(irange(0,3)) {
        case 0: bt = BT_WEAK_CRYSTAL; break;
        case 1: bt = BT_GRAY_GOO; break;
        }
        fillRectB( Rect(lb,lb+Pos2(w,w)), bt );
        fillRectB( Rect(rt,rt+Pos2(-w,-w)).translate(1,1), bt );
        fillRectB( Rect(lt,lt+Pos2(w,-w)).translate(0,1), bt );
        fillRectB( Rect(rb,rb+Pos2(-w,w)).translate(1,0), bt );
    } else if( r < 50 ) {
        // Cage room
        int cnt=0, mod = irange(1,5);
        BOSSTYPE bst = BST_DEFENDER;
        if( range(0,100) < 30 ) bst = BST_CHASER;
        
        Pos2 lb = rr.in(0.2,0.2), rt = rr.in(0.8,0.8) + Pos2(1,1);
        for(int y=lb.y;y<=rt.y;y++){
            for(int x=lb.x;x<=rt.x;x++) {
                Cell *c = get(x,y);
                if(c&&c->isBlockPuttable() && (cnt%mod)==0  ) {
                    c->setCage( bst );
                }
                cnt++;
            }
        }
    } else if( r < 60 ) {
        // Barrier room
        int mod = irange(2,10);
        for(int y=rr.min.y;y<=rr.max.y;y++){
            for(int x=rr.min.x;x<=rr.max.x;x++) {
                Cell *c = get(x,y);
                bool to_put = (x%mod==0) && (y%mod==0);
                if( (x%mod==0) && (y==rr.min.y || y == rr.max.y) ) to_put = true;
                if( (y%mod==0) && (x==rr.min.x || x == rr.max.x) ) to_put = true;
                if(c && to_put ) {
                    c->bt = BT_BARRIERGEN;
                }
            }
        }
    }
    
    // Wall of a room
    if( range(0,100) < 20 ) {
        // surround with blocks
        edgeRectB( rr, BT_CRYSTAL );
        if( range(0,100) < 30 ) {
            edgeRectCrystalBeamer( rr, irange(1,4), 1, irange(0,100)<30 );
        }
    }
    if( range(0,100) < 10 ) {
        edgeRectCrystalBeamer( rr, irange(1,4), 1, irange(0,100) < 30 );
    }
    if( range(0,100) < 50 ) {
        // Put a gate on a direction that is nearer to core
        DIR d = reverseDir( node->entrance_hint);
        if( node->hasConnection(d) ) {
            buildRoomGate( rr.getCenterEdge(d), d );
        }
    }

    // Put walls first and then fortress
    if( range(0,100) < 50 ) {
        buildFortressByLevel( rr.center(), 2, 99 );
    } else {
        // no fortress
        if( range(0,100) < 30 ) {
            // center beamer
            int sz = irange(1,5), mgn = irange(1,3);
            Rect putrect( rr.center() - Pos2(sz,sz), rr.center() + Pos2(sz,sz) );
            edgeRectCrystalBeamer( putrect, 1, mgn, true );
            if( range(0,100)<50 && mgn >= 2 ) {
                // bonuses
                fillRectB( putrect.expand(-2,-2), BT_WEAK_CRYSTAL );
            }
        }
    }
    
    return rr;
}

Rect Field::buildCorridorRoom( Rect rect ) {
    Pos2 center = rect.center();
    Pos2 lb = center;
    Pos2 rt = center;
    Rect rr(lb,rt);
    fillRectSG( rr, ST_NONE, GT_DUNGEON );
    fillRectB( rr, BT_AIR );
    return rr;
}

Rect Field::buildFortressRoom( Rect rect ) {
    fillRectSG( rect, ST_NONE, GT_TELEPORTER_INACTIVE );
    fillRectB( rect, BT_AIR );
    fillRectFortressId( rect, 0 );
    return rect;
}
Rect Field::buildCoreRoom( Rect rect ) {
    fillRectSG( rect, ST_NONE, GT_TELEPORTER_INACTIVE );
    fillRectB( rect, BT_AIR );

    Pos2 center = rect.center();

    putCoveredCore(center);

    return rect;
}
void Field::putCoveredCore( Pos2 lb ) {
    Cell *c = get(lb);
    c->gt = GT_CORE;
    c->bt = BT_CORE_COVER;
    c = get(lb+Pos2(1,0));
    c->gt = GT_CORE;
    c->bt = BT_CORE_COVER;    
    c = get(lb+Pos2(0,1));
    c->gt = GT_CORE;
    c->bt = BT_CORE_COVER;    
    c = get(lb+Pos2(1,1));
    c->gt = GT_CORE;
    c->bt = BT_CORE_COVER;    
    for(int y= lb.y - 1; y <= lb.y+2; y++ ) {
        for(int x=lb.x-1; x <= lb.x+2; x++ ) {
            if( y >= lb.y && y < lb.y+2 && x >= lb.x && x < lb.x+2) continue;
            Cell *c = get(x,y);
            if(c) c->bt = BT_ENEMY_FRAME;
         }
    }
}

// Put a collidor through a pit
void Field::fillPitCollidor( Pos2 from, DIR dir ) {
    //    print("fillPitCollidor: %d,%d dir:%d", from.x, from.y, dir );
    Pos2 cur = from;
    for(int i=0;i<100;i++) {
        Cell *c = get(cur);
        if(c && c->gt == GT_PIT ) {
            c->gt = GT_DUNGEON;
            c->bt = BT_AIR;
        } else {
            break;
        }
        cur += dirToPos2(dir);
    }
}

// Put an entrance of a dungeon
Rect Field::buildEntranceRoom( Rect rect, DIR dir, MazeNode *node ) {
    Rect rr = rect;
    rr.min += Pos2(5,5);
    rr.max -= Pos2(5,5);

    fillRectSG( rr, ST_NONE, GT_DUNGEON );
    fillRectB( rr, BT_AIR );
    
    Pos2 corridor_from = rr.getCenterEdge( dir ) + dirToPos2(dir);
    fillPitCollidor( corridor_from, dir );
    // Wider collidor
    switch(dir) {
    case DIR_DOWN:  
    case DIR_UP:
        fillPitCollidor( corridor_from + Pos2(1,0),dir );
        break;
    case DIR_RIGHT:
    case DIR_LEFT:
        fillPitCollidor( corridor_from + Pos2(0,1),dir );        
        break;
    default:break;
    }
    
    // PUt a fortress
    buildFortressByLevel( rr.center(), 1,2 );

    // gate
    if( node->con_up && dir != DIR_UP ) buildRoomGate( rr.getCenterEdge(DIR_UP), DIR_UP );
    if( node->con_down && dir != DIR_DOWN ) buildRoomGate( rr.getCenterEdge(DIR_DOWN), DIR_DOWN );
    if( node->con_right && dir != DIR_RIGHT ) buildRoomGate( rr.getCenterEdge(DIR_RIGHT), DIR_RIGHT );    
    if( node->con_left && dir != DIR_LEFT ) buildRoomGate( rr.getCenterEdge(DIR_LEFT), DIR_LEFT );
    
    return rr;
}


// Put a collidor that have 2 corner
//
//       from
//       *
// a_len |         b_len
//       P--------------------Q
//                            | c_len
//                         to *

void Field::buildCorridorGS( Pos2 from, Pos2 to, int w, DIR dir, GROUNDTYPE gt, SURFACETYPE st, bool overwrite_enemy_building ) {
    Pos2 p_pos, q_pos;
    if( dir == DIR_DOWN || dir == DIR_UP ) {
        int avg_y = ( to.y + from.y ) / 2;
        p_pos = Pos2( from.x, avg_y );
        q_pos = Pos2( to.x, avg_y );
    } else {
        int avg_x = ( to.x + from.x ) / 2;
        p_pos = Pos2( avg_x, from.y );
        q_pos = Pos2( avg_x, to.y );        
    }
    orthoLineGSB( from, p_pos, w, gt, st, BT_AIR, overwrite_enemy_building, true );
    orthoLineGSB( p_pos, q_pos, w, gt, st, BT_AIR, overwrite_enemy_building, true );
    orthoLineGSB( q_pos, to, w, gt, st, BT_AIR, overwrite_enemy_building, true );

    if( range(0,100) < 50 ) orthoOutlineBSelectG( from, p_pos, w, GT_PIT, BT_CRYSTAL );
    if( range(0,100) < 50 ) orthoOutlineBSelectG( p_pos, q_pos, w, GT_PIT, BT_CRYSTAL );
    if( range(0,100) < 50 ) orthoOutlineBSelectG( q_pos, to, w, GT_PIT, BT_CRYSTAL );
}

// Put an outline around a ground type (straight)
void Field::orthoOutlineBSelectG( Pos2 from, Pos2 to, int w, GROUNDTYPE gt_toselect, BLOCKTYPE bt_toput ) {
    int dx,dy;
    if( to.x > from.x ) dx = 1; else if( to.x < from.x ) dx = -1; else dx = 0;
    if( to.y > from.y ) dy = 1; else if( to.y < from.y ) dy = -1; else dy = 0;
    int nloop = maxi( abs(to.x-from.x), abs(to.y-from.y) );
    Pos2 cur = from;
    for(int i=0;i<=nloop;i++){
        for(int x=0;x<w;x++) {
            for(int y=0;y<w;y++) {
                Cell *cells[8];
                get8(cur + Pos2(x,y),cells);
                for(int j=0;j<8;j++){
                    if( cells[j] && cells[j]->gt == gt_toselect && cells[j]->bt == BT_AIR ) {
                        cells[j]->bt = bt_toput;
                    }
                }
            }
        }
        cur += Pos2(dx,dy);
    }
}

void Field::orthoLineGSB( Pos2 from, Pos2 to, int w, GROUNDTYPE gt, SURFACETYPE st, BLOCKTYPE bt, bool overwrite_enemy_building, bool convert_crystal ) {
    int dx,dy;
    if( to.x > from.x ) dx = 1; else if( to.x < from.x ) dx = -1; else dx = 0;
    if( to.y > from.y ) dy = 1; else if( to.y < from.y ) dy = -1; else dy = 0;
    int nloop = maxi( abs(to.x-from.x), abs(to.y-from.y) );
    Pos2 cur = from;
    for(int i=0;i<=nloop;i++) {
        for(int x=0;x<w;x++) {
            for(int y=0;y<w;y++){
                Cell *c = get(cur + Pos2(x,y));
                if(!c)break;
                c->gt = gt;
                c->st = st;
                bool to_write_bt = true;
                if( overwrite_enemy_building == false && c->isEnemyBuilding() ) to_write_bt = false;
                if( to_write_bt ) {
                    if( convert_crystal ) {
                        if( c->bt == BT_CRYSTAL ) c->bt = BT_WEAK_CRYSTAL; else c->bt = bt;
                    } else {
                        c->bt = bt;
                    }
                }
            }
        }
        cur += Pos2(dx,dy);
    }
}

void Field::fillShifter( Rect r ) {
    int type = irange(0,100) % 3;

    if( type == 0 ) {
        // random type
        int mod = irange(4,20), cnt=0;
        for(int y=r.min.y; y <= r.max.y; y++ ) {
            for(int x=r.min.x; x <= r.max.x; x++) {
                if( (++cnt)%mod == 0 ) {
                    Cell *c = get(x,y);
                    if(!c)continue;
                    c->gt = GT_SHIFTER;
                    c->dir = randomDir();
                }
            }
        }
    } else if( type == 1 ) {
        // center line
        int sz = irange( r.width()/4,r.width()/2) ;
        edgeRectShifter( r.expand(-sz,-sz), irange(1,3), range(0,100) < 50 );
    } else if( type == 2 ) {
        // entrance guard
        edgeRectShifter( r.expand(-2,-1), r.width()/4-1, true );
    }
}

void Field::buildRoomGate( Pos2 at, DIR todir ) {
    Cell *a, *b;
    if( todir == DIR_UP || todir == DIR_DOWN ) {
        a = get( at + Pos2(-1,0) );
        b = get( at + Pos2(2,0) );
    } else {
        a = get( at + Pos2(0,-1) );
        b = get( at + Pos2(0,2) );
    }
    if(a)a->bt = BT_PAIRBARRIER;
    if(b)b->bt = BT_PAIRBARRIER;
}
void Field::edgeRectCrystalBeamer( Rect r, int mod, int mgn, bool reverse ) {
    int cnt=0;
    for(int y=r.min.y+mgn; y <= r.max.y-mgn; y++ ) {
        if((++cnt)%mod==0) {
            Cell *lc = get(r.min.x,y);
            if(lc) lc->tryPutCrystalBeamer( reverse?DIR_LEFT:DIR_RIGHT);
            Cell *rc = get(r.max.x,y);
            if(rc) rc->tryPutCrystalBeamer( reverse?DIR_RIGHT:DIR_LEFT);
        }
    }
    for(int x=r.min.x+mgn; x <= r.max.x-mgn; x++) {
        if((++cnt)%mod==0) {
            Cell *uc = get(x,r.max.y);
            if(uc) uc->tryPutCrystalBeamer( reverse?DIR_UP:DIR_DOWN);
            Cell *dc = get(x,r.min.y);
            if(dc) dc->tryPutCrystalBeamer( reverse?DIR_DOWN:DIR_UP);            
        }
    }
}
void Field::edgeRectShifter( Rect r, int mgn, bool reverse ) {
    for(int y=r.min.y+mgn; y <= r.max.y-mgn; y++ ) {
        Cell *lc = get(r.min.x,y);
        if(lc) lc->putShifter( reverse?DIR_RIGHT:DIR_LEFT);
        Cell *rc = get(r.max.x,y);
        if(rc) rc->putShifter( reverse?DIR_LEFT:DIR_RIGHT);
    }
    for(int x=r.min.x+mgn; x <= r.max.x-mgn; x++) {
        Cell *uc = get(x,r.max.y);
        if(uc) uc->putShifter( reverse?DIR_DOWN:DIR_UP );
        Cell *dc = get(x,r.min.y);
        if(dc) dc->putShifter( reverse?DIR_UP:DIR_DOWN);            
    }    
}

int Field::scatterBonEarthGround( Pos2 center, int dia, BLOCKTYPE bt_toput, int n ) {
    int putn = 0;
    for(int i=0;i<n;i++){
        Pos2 randp( irange(-dia,dia+1), irange(-dia,dia+1) );
        Pos2 p = center + randp;
        if( p.len(center) > dia ) continue;
        Cell *c = get(p);
        if(!c)continue;
        if( c->isEarthGround() && c->isWater() == false && (c->bt == BT_AIR||c->isEarthBlock()) ) {
            c->bt = bt_toput;
            putn++;
        }
    }
    return putn;
}

    
void Field::buildMegaResourceSite( Pos2 p, BLOCKTYPE bt, int dia, float rock_rate ) {
    Pos2 from = p.randomize(dia);
    Vec2 fromv( from.x*PPC, from.y*PPC );
    Vec2 vv = Vec2(0,0).randomize(1).normalize( range(50,200) );
    for(int i=0;i<dia;i++) {
        fromv += vv;
        Vec2 curv = fromv;
        Vec2 v = Vec2(0,0).randomize(1).normalize( range(10,50) );
        for(int j=0;j<dia;j++) {
            curv += v;
            int partdia = range(dia/2,(dia-j)/2);
            if( range(0,1) < rock_rate ) {
                BLOCKTYPE rockbt = BT_ROCK;
                if(range(0,100)<70) rockbt = BT_SOIL;
                scatterBonEarthGround( Pos2(curv.x/PPC,curv.y/PPC), partdia, rockbt, partdia*partdia/2);
            } else {
                scatterBonEarthGround( Pos2(curv.x/PPC,curv.y/PPC), partdia, bt, partdia*partdia/2);
            }
            
        }
    }
}

// 1 chunk per 1 frame is enough fast.
void Field::pollNetworkCache() {
    int poll_tosim_per_loop = 20;
    for(int i=0;i<poll_tosim_per_loop;i++) {
        int tosim_ind = load_poll_count % elementof(to_sim);
        load_poll_count++;
        ToSim *ts = & to_sim[tosim_ind];
        //        print("ts cnt:%d %d,%d ii:%d",ts->cnt, ts->chx, ts->chy , tosim_ind );
        
        if(ts->cnt == 0 ) continue;
        Pos2 lb( ts->chx * CHUNKSZ, ts->chy * CHUNKSZ );
        CHUNKLOADSTATE state = getChunkLoadState(lb);

        // 
        switch(state) {
        case CHUNKLOADSTATE_INIT:
            {
                int ls_x = lb.x / CHUNKSZ, ls_y = lb.y / CHUNKSZ;
                int ls_ind = ls_x  + ls_y * (width/CHUNKSZ);
                //                                print("to load! pjid:%d %d,%d", g_current_project_id, ls_x, ls_y);
                Pos2 lb( ls_x * CHUNKSZ, ls_y * CHUNKSZ );
                dbLoadFieldFileSend( g_current_project_id, lb, CHUNKSZ, CHUNKSZ );
                load_state[ls_ind] = CHUNKLOADSTATE_LOADING;
                break;
            }
        case CHUNKLOADSTATE_LOADING:
            break;
        case CHUNKLOADSTATE_LOADED:
            break;
        case CHUNKLOADSTATE_OUT_OF_FIELD:
            break;
        }
    }
}
int Field::calcChunkLoadState(Pos2 at) {
    int ls_x = at.x / CHUNKSZ, ls_y = at.y / CHUNKSZ;
    int ls_w = width / CHUNKSZ, ls_h = height / CHUNKSZ;
    int ls_ind = ls_x + ls_y * ls_w;
    if(ls_ind >= 0 && ls_ind < ls_w * ls_h ) {
        return ls_ind;
    } else {
        return -1;
    }
}
CHUNKLOADSTATE Field::getChunkLoadState( Pos2 at ) {
    int ls_ind = calcChunkLoadState(at);
    if( ls_ind < 0 ) {
        return CHUNKLOADSTATE_OUT_OF_FIELD;
    } else {
        return load_state[ls_ind];
    }
}
bool Field::isChunkLoaded( Vec2 at ) {
    Pos2 p = vec2ToPos2(at);
    CHUNKLOADSTATE lst = getChunkLoadState(p);
    return( lst == CHUNKLOADSTATE_LOADED );
}

void Field::setChunkLoadState( Pos2 lb, CHUNKLOADSTATE state ) {
    int ls_ind = calcChunkLoadState(lb);
    load_state[ls_ind] = state;
}
bool Field::setLockGot( int chx, int chy ) {
    for(int i=0;i<elementof(to_sim);i++) {
        if( to_sim[i].cnt > 0 && to_sim[i].chx == chx && to_sim[i].chy == chy ) {
            if( to_sim[i].lock_state == LOCKSTATE_SENT ) {
                to_sim[i].lock_state = LOCKSTATE_GOT;
                //            print("set to_sim lockstate:%d,%d",chx,chy);
            } else {
                print("Field::setLockGot(%d,%d): warn: invalid lock state:%d",chx,chy, to_sim[i].lock_state);
            }
            return true;
        }
    }
    return false;
}
void Field::applyFlagCands( ProjectInfo *pinfo ) {
    assert( elementof(flag_cands) == elementof(pinfo->flag_cands) );
    for(int i=0;i<elementof(flag_cands);i++) {
        flag_cands[i] = pinfo->flag_cands[i];
        //        print("applyFlagCands: %d: %d,%d [%d]", i, flag_cands[i].pos.x, flag_cands[i].pos.y, flag_cands[i].finished );
    }
}
void Field::copyFlagCands( ProjectInfo *pinfo ) {
    assert( elementof(flag_cands) == elementof(pinfo->flag_cands) );
    for(int i=0;i<elementof(flag_cands);i++) {
        pinfo->flag_cands[i] = flag_cands[i];
    }
}
// Assuming REACTOR is placed at position p
int Field::countReactorArm( Pos2 p ) {
    int n=0;
    Cell *uc = get(p + Pos2(0,1) );
    if(uc && uc->bt == BT_REACTOR_ARM && uc->dir == DIR_DOWN ) n++; // dir: face to reactor
    Cell *dc = get(p + Pos2(0,-1) );
    if(dc && dc->bt == BT_REACTOR_ARM && dc->dir == DIR_UP ) n++;
    Cell *lc = get(p + Pos2(-1,0) );
    if(lc && lc->bt == BT_REACTOR_ARM && lc->dir == DIR_RIGHT ) n++;
    Cell *rc = get(p + Pos2(1,0) );
    if(rc && rc->bt == BT_REACTOR_ARM && rc->dir == DIR_LEFT ) n++;
    return n;
}
bool Field::findNearestBlock( Pos2 from, BLOCKTYPE bt, int dia, Pos2 *out ) {
    for(int y=from.y-dia;y<=from.y+dia;y++) {
        for(int x=from.x-dia;x<=from.x+dia;x++) {
            Cell *c = get(x,y);
            if( c && c->bt == bt ) {
                *out = Pos2(x,y);
                return true;
            }
        }
    }
    return false;
}
PowerEquipmentNode *Field::findNearestExchange( Pos2 from ) {
    Pos2 exat;
    bool res = findNearestBlock( from, BT_EXCHANGE_ACTIVE, 2, &exat );
    if(!res) {
        res = findNearestBlock( from, BT_EXCHANGE_INACTIVE, 2, &exat );
        if(!res) return NULL;
    }
        
    Cell *c = get(exat);
    if(!c) return NULL;
    
    if( c->powergrid_id == 0 ) return NULL;
    
    PowerGrid *pg = g_ps->findGridById( c->powergrid_id );
    if(!pg) return NULL;

    PowerEquipmentNode *node = pg->findEquipment(exat);

    return node;
}

void Field::saveAt( Cell *c, int dia ) {
    dbPutFieldFileSendAt( this, g_current_project_id, getCellPos(c), dia);
}
void Field::saveAt( Pos2 p, int dia ) {
    Cell *c = get(p);
    if(c) saveAt( c, dia );
}

void Field::checkSaveAllUnsaved( double wait ) {
    for(int i=0;i<elementof(to_sim);i++) {
        if( to_sim[i].cnt > 0 && to_sim[i].last_important_change_at > to_sim[i].last_save_at ) {
            print("checkSaveAllUnsaved: saving %d,%d", to_sim[i].chx, to_sim[i].chy );
            saveAt( Pos2( to_sim[i].chx*CHUNKSZ, to_sim[i].chy*CHUNKSZ ) );
        }
    }
    double st = now();
    while( now() < st + wait ) {
        vce_heartbeat();
    }
}

void Field::setEnvironment( Rect r, BIOMETYPE bmt ) {
    for(int i=0;i<elementof(envs);i++) {
        if( envs[i].bmt == BMT_NONE ) {
            envs[i] = Environment( bmt, r );
            break;
        }
    }
}
Environment *Field::getEnvironment( Pos2 at ) {
    Environment *out=NULL;
    for( int i=0;i<elementof(envs);i++) {
        if( envs[i].bmt == BMT_NONE )break;
        if( envs[i].rect.includes(at) ) {
            out =  & envs[i];
        }
    }
    return out;
}

/////////////////////

void ResourceDeposit::dump( ResourceDepositDump *dump ) {
    assert( elementof(dump->items) == item_num );
    for(int i=0;i<item_num;i++) {
        dump->items[i].itt = items[i].itt;
        dump->items[i].num = items[i].num;
        dump->items[i].dur = items[i].dur;
    }
}
void ResourceDeposit::applyDump( ResourceDepositDump *dump ) {
    assert( elementof(dump->items) == item_num );
    for(int i=0;i<item_num;i++) {
        items[i].set( (ITEMTYPE)dump->items[i].itt, dump->items[i].num, dump->items[i].dur );
    }    
}

