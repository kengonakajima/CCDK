#ifndef _EFFECT_H_
#define _EFFECT_H_



class Particle : public Prop2D {
public:
    Vec2 v;
    float start_scl, max_scl, fin_scl;
    float attack_dur, sustain_dur;
    float base_scl;
    float gravity_effect;
    float friction;
    bool xflipping;
    bool flickering;
    int rand_inds[4];
    int rand_inds_used;

    float sway_x_vel, sway_x_width;
    float sway_xscl_vel;

    Vec2 goal;
    bool die_nearby_goal;
    
    Particle( Vec2 lc, float start_scl, float max_scl, float fin_scl, float attack_dur, float sustain_dur, int atlas_index, TileDeck *deck, float g );
    
    virtual bool prop2DPoll(double dt);
};


//Particle *createColorFragment( Vec2 loc, float vel, float dur, Color c );
//void createColorFragments( Vec2 loc, int n );
//void createWaterSplash( Vec2 loc, int n );
//Particle * createTwinkleEffect( Vec2 loc, Vec2 iniv, float scl );


void createBloodEffect( Vec2 loc , float rate, bool local=true );
void createSmokeEffect( Vec2 loc, bool local=true ); 
void createPositiveEffect( Vec2 loc, int n, bool local=true ); 
void createChargeEffect( Vec2 loc, int n, bool local=true ) ;
void createDigSmoke(Vec2 loc, float scl, bool local=true );
void createSpark( Vec2 loc, float scl, bool local=true );
void createShieldSpark( Vec2 loc, bool local=true );
void createSnowFlake( Vec2 loc ); // no need to sync
void createLeafEffect( Vec2 loc, bool local=true ) ;
void createCellBubbleBlue( Vec2 loc, bool local=true );
void createCellBubbleRed( Vec2 loc, bool local=true );
void createBuildingExplosion( Vec2 lc, bool local=true );
void createBossExplosion( Vec2 at, bool local=true );
void createExchangeLinkBall( Vec2 from, Vec2 to ); // no need to sync

Particle *createExplosion(Vec2 loc, float scl, float dur_rate, bool local=true ); 
Particle *createHyperSpark( Vec2 loc ); // no need to sync

void stopEffect();


#endif
