
#include "moyai/client.h"

#include "effect.h"
#include "atlas.h"
#include "conf.h"
#include "dimension.h"
#include "net.h"
#include "globals.h"


Particle::Particle( Vec2 lc, float start_scl, float max_scl, float fin_scl, float attack_dur, float sustain_dur, int atlas_index, TileDeck *deck , float g ) : Prop2D(), start_scl(start_scl), max_scl(max_scl), fin_scl(fin_scl), attack_dur(attack_dur), sustain_dur(sustain_dur), gravity_effect(g), friction(0), xflipping(false), flickering(false), rand_inds_used(0), sway_x_vel(0), sway_x_width(0), sway_xscl_vel(0), die_nearby_goal(false) {
    setLoc(lc);
    setDeck(deck);
    setIndex(atlas_index);

    setScl( start_scl, start_scl );
    base_scl = 24;
    g_effect_layer->insertProp(this);
}

bool Particle::prop2DPoll(double dt){
    //        print("expl: scl:%f %d %f", scl.x, id , accumTime );
    float rate = 1;
    if( accum_time < attack_dur ){
        rate = start_scl + ( accum_time / attack_dur ) * ( max_scl - start_scl );
    } else if( accum_time < attack_dur + sustain_dur ){
        float t = accum_time - attack_dur;
        rate = max_scl + ( t / sustain_dur ) * ( fin_scl - max_scl );
    }
    scl.y = scl.x = rate * base_scl;
    if( accum_time > ( attack_dur + sustain_dur ) ){
        return false;
    }
    v.y = v.y - gravity_effect * dt;

    if( friction != 0 ){
        v = v.friction(friction * dt);
    }
    loc += v * dt;
    if( xflipping ) setXFlip( irange(0,2) );
    if( flickering ) setVisible( irange(0,2) );
    if( rand_inds_used > 0 ) {
        setIndex( rand_inds[ irange(0,rand_inds_used) ] );
    }
    if( sway_x_vel > 0 ) {
        float d = cos(accum_time * sway_x_vel) * sway_x_width * dt;
        loc.x += d;
    }
    if( sway_xscl_vel > 0 ) {
        Vec2 s = scl;
        setScl( absolute( s.x * cos(accum_time * sway_xscl_vel) ), s.y );
    }
    if( die_nearby_goal && loc.len(goal) < 8 ) return false;
    return true;
}

void createDigSmoke(Vec2 loc, float scl, bool local ){
    Particle *e = new Particle(loc,scl,scl,scl, 0.2,0.2, B_ATLAS_DIGSMOKE_BASE, g_base_deck, 0 );
    e->setAnim( g_digsmoke_curve );
    e->setColor( Color(1,1,1, range(0.5,1) ) );
    if(local) realtimeEffectSend( EFT_DIGSMOKE, loc, scl,0 );
}
static void createCellBubble( Vec2 loc, bool blue ) {
    Particle *e = new Particle(loc,1,1,1, 0.5,1, B_ATLAS_CELL_BUBBLE_BASE, g_base_deck, 0 );
    e->setAnim( g_cellbubble_curve );
    if(blue) e->setColor( Color(0.3,0.4,1, range(0.5,1) ) ); else e->setColor( Color(1,1,1, range(0.5,1) ) );
    e->v = Vec2(0, range(12,36));    
}
void createCellBubbleBlue( Vec2 loc, bool local ) {
    createCellBubble( loc, true );
    if(local) realtimeEffectSend( EFT_CELL_BUBBLE, loc, 1 );
}
void createCellBubbleRed( Vec2 loc, bool local ) {
    createCellBubble( loc, false );
    if(local) realtimeEffectSend( EFT_CELL_BUBBLE, loc, 0 );    
}

void createSpark( Vec2 loc, float scl, bool local ) {
    Particle *e = new Particle(loc,scl,scl,scl, 0.2,0.2, B_ATLAS_SPARK_BASE, g_base_deck, 0 );
    e->setAnim( g_spark_curve );
    e->setColor( WHITE );
    if(local) realtimeEffectSend( EFT_SPARK, loc, scl );
}
void createShieldSpark( Vec2 loc, bool local ) {
    float scl = range(1,2);
    Particle *e = new Particle(loc,scl,scl,scl, 0.2,0.2, B_ATLAS_SHIELD_WORKING, g_base_deck, 0 );
    e->setColor( WHITE );
    e->flickering = true;
    e->seekRot( M_PI * 10, 0.2 );
    if(local) realtimeEffectSend( EFT_SHIELD_SPARK, loc );
}

Particle *createHyperSpark( Vec2 loc ) {
    Particle *e = new Particle( loc, 1,1,1, 0.2, 0.2, B_ATLAS_HYPERSPARK_BASE, g_base_deck, 0 );
    e->xflipping = true;
    e->flickering = true;
    e->rand_inds[0] = B_ATLAS_HYPERSPARK_BASE;
    e->rand_inds[1] = B_ATLAS_HYPERSPARK_BASE+1;
    e->rand_inds[2] = B_ATLAS_HYPERSPARK_BASE+2;
    e->rand_inds_used = 3;
    return e;
}
#if 0
Particle *createColorFragment( Vec2 loc, float vel, float dur, Color c ){
    Particle *e = new Particle(loc,  0.3,0.5,0, dur, 0.1, B_ATLAS_SLASH, g_base_deck, 0 );
    e->v = Vec2( range(-1,1), range(-1,1) ).normalize(vel);
    e->setColor(c);
    e->xflipping = true;
    return e;
}

void createColorFragments( Vec2 loc, int n ){
    for(int i=0;i<n;i++){
        createColorFragment( loc, range(100,200), range(0.1,0.3), Color(range(0.7,1),range(0.7,1),range(0.7,1),0.7) );
    }
}
#endif

Particle *createExplosion(Vec2 loc, float scl, float dur_rate, bool local ){
    //    if(loc.len(g_pc->loc)>EFFECT_VIEW_RANGE)return NULL;
    Particle *e = new Particle(loc,1*scl,2*scl,0, 0.05 * dur_rate,0.1*dur_rate, B_ATLAS_BLASTER_EXPLOSION, g_base_deck, 0 );
    if(local) realtimeEffectSend( EFT_EXPLOSION, loc, scl, dur_rate );
    return e;
}
void createBuildingExplosion( Vec2 lc, bool local ) {
    createExplosion(lc, 2,2, local );
}


void createBloodEffect( Vec2 loc, float rate, bool local ){
    for(int i=0;i<5;i++){
        float minscl = range( 0.1,0.2);
        float maxscl = range( 0.1,0.3);
        Particle *e = new Particle( loc, minscl,maxscl,0, 0.2,0.2, B_ATLAS_WHITE_FILL, g_base_deck, 300 + 200*rate );
        e->v = Vec2( range(-30*rate,30*rate), range(50 +rate*20,120 +rate*30) );
        e->setColor( 1,0,0,range(0.5,1) );
    }
    if(local) realtimeEffectSend( EFT_BLOOD, loc, rate, 0 );
}
void createPositiveEffect( Vec2 loc, int n, bool local ) {
    for(int i=0;i<n;i++) {
        Particle *e = new Particle( loc.randomize(20), 0.3, 0.5, 0, 0.5, 0, B_ATLAS_WHITE_CROSS, g_base_deck, 0 );
        e->setColor( 0x0598ed );
        e->v.y = 100;
        e->seekColor( Color(0.7,0.7,1,0), 0.5 );
    }
    if(local) realtimeEffectSend( EFT_POSITIVE, loc, n );
}

void createChargeEffect( Vec2 loc, int n, bool local ) {
    for(int i=0;i<n;i++) {
        Particle *e = new Particle( loc.randomize(20), 0.3, 0.5, 0, 0.5, 0, B_ATLAS_ELECTRO, g_base_deck, 0 );
        //        e->setColor( 0x0de874 );
        e->v.y = 100;
        e->seekColor( Color(1,1,1,0), 0.5 );
    }
    if(local) realtimeEffectSend( EFT_CHARGE, loc, n );
}


void createSmokeEffect( Vec2 loc, bool local ){
    Particle *e = new Particle( loc, 0.6,1,1, 0.5,0.2, B_ATLAS_SMOKE, g_base_deck, false );
    e->v = Vec2( range(-30,30), range(10,30) );
    e->setColor( 1,1,1, range(0.2,0.5));
    e->seekColor( Color(1,1,1,0), 1 );
    if(local) realtimeEffectSend( EFT_SMOKE,loc );
}


void createSnowFlake( Vec2 loc ) {
    int ind = B_ATLAS_SNOW_BASE + irange(0,3);
    float s = range(1,2);
    Particle *e = new Particle( loc, s,s,s, 1,2,  ind, g_base_deck, false );
    e->v.y = - 24 * range(1,1.5);
    e->setColor( Color(1,1,1,0) );
    e->seekColor( Color(1,1,1,1), 1 );
    e->sway_x_width = range(10,20);
    e->sway_x_vel = range(0.2,2);
    e->sway_xscl_vel = range(1,5);    
}

void createLeafEffect( Vec2 loc, bool local ) {
    int rel = irange(0,2);
    float s = range(0.5,1);
    Particle *e = new Particle( loc, s,s,0, 0.4,0.1, B_ATLAS_LEAF_BASE + rel, g_base_deck, true );
    e->v = Vec2( range(-100,100), range(150,200));
    e->gravity_effect = 800;
    e->seekRot(M_PI*4,2);
    if(local) realtimeEffectSend( EFT_LEAF, loc );
}

void createBossExplosion( Vec2 at, bool local ) {
    for(int i=0;i<15;i++) {
        Particle *p = createExplosion( at, range(1,3),range(3,7), false ); // Skipping network
        p->v = Vec2(0,0).randomize(1).normalize( range(24*3,24*10) );
    }
    if(local) realtimeEffectSend( EFT_BOSS_EXPLOSION, at );
}

void createExchangeLinkBall( Vec2 from, Vec2 to ) {
    Particle *p = new Particle( from, 0.5,0.5,0.5, 0,20, B_ATLAS_EXCHANGE_LINK_BALL_BASE, g_base_deck, false );
    p->rand_inds[0] = B_ATLAS_EXCHANGE_LINK_BALL_BASE;
    p->rand_inds[1] = B_ATLAS_EXCHANGE_LINK_BALL_BASE+1;
    p->rand_inds_used = 2;
    p->v = from.to(to).normalize( 10*24);
    p->setColor( Color(1,1,1,0.5) );
    p->die_nearby_goal = true;
    p->goal = to;
}

void stopEffect() {
    pauseSituationBGM(true);
    glfwSleep( HIT_STOP_TIME );
    pauseSituationBGM(false);    
    
}
#if 0
Particle * createTwinkleEffect( Vec2 loc, Vec2 iniv, float scl ){
    if(loc.len(g_pc->loc)>EFFECT_VIEW_RANGE)return NULL;    
    Particle *e = new Particle( loc, 1*scl,2*scl,0, 0.03,0.25, ATLAS_TWINKLE, g_base_deck, false );
    e->setColor( range(0.5,1), range(0.5,1), 1, 0.9 );
    e->v = iniv;
    //    e->seekRot(M_PI*2,1);
    return e;
}
#endif


