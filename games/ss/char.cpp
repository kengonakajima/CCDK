#include "moyai/client.h"

#include "conf.h"
#include "dimension.h"
#include "ids.h"
#include "item.h"
#include "util.h"
#include "net.h"
#include "field.h"
#include "char.h"
#include "effect.h"
#include "mapview.h"
#include "enemy.h"
#include "hud.h"
#include "atlas.h"
#include "pc.h"
#include "globals.h"

Char::Char( CATEGORY cat, Vec2 lc, TileDeck *dk, Layer *tgtlayer, int client_id, int internal_id ) : Prop2D(), category(cat), clean_at(0), client_id(client_id), internal_id(internal_id) {
    setScl(24,24);
    setDeck(dk);
    setLoc(lc);
    tgtlayer->insertProp(this);
}
Char::~Char() {
    if( isLocal() ) realtimeCharDeleteSend(this);
}

bool Char::prop2DPoll( double dt ) {
    if( charPoll(dt) == false ) return false;
    if( clean_at > 0 && accum_time > clean_at ) return false;
    if( loc.x < 0 ||  loc.x > g_fld->loc_max.x || loc.y < 0 || loc.y > g_fld->loc_max.y  ) return false;
    return true;
}
Vec2 Char::calcNextLoc( Vec2 from, Vec2 nextloc, float body_size, bool enemy, bool flying, bool swimming, BLOCKTYPE bt_can_enter ) {
    Vec2 out = nextloc;
    int hitbits;
    if( (hitbits=g_fld->getEnterableBits(nextloc, body_size, enemy, flying, swimming, bt_can_enter )) > 0 ) {
        
        out = from;
        // Try X and Y independently
        Vec2 nxloc( nextloc.x, from.y );
        bool nxok=false, nyok=false;
        if( g_fld->getEnterableBits(nxloc, body_size, enemy, flying, swimming, bt_can_enter ) == 0 ) {
            nxok = true;
        }
        Vec2 nyloc( from.x, nextloc.y );
        if( g_fld->getEnterableBits(nyloc, body_size, enemy, flying, swimming, bt_can_enter ) == 0 ) {
            nyok = true;
        }

        if( nxok ) out.x = nxloc.x; 
        if( nyok ) out.y = nyloc.y;

        // hit on something.
        onTouchWall( nextloc, hitbits, nxok, nyok );
        
        return out;
    } else {
        return nextloc;
    }
}

void Char::selectByCategory( Vec2 center, float dia, CATEGORY cat, Char *out[], int *outlen ) {
    Prop *hits[1024];
    int hitlen = elementof(hits);
    g_char_layer->selectCenterInside( center,dia, hits, &hitlen );
    int cnt=0;
    for(int i=0;i<hitlen;i++){
        Char*ch = (Char*) hits[i];
        if(ch->category == cat ){
            out[cnt++] = ch;
            if(cnt==*outlen)break;
        }
    }
    *outlen = cnt;
}

Char *Char::getNearestByCategory( Vec2 from, CATEGORY cat, Char *except ) {
    float minl = 9999999999;
    Char *out = NULL;
    Char *cur = (Char*) g_char_layer->prop_top;
    while(cur) {
        if( cur->category == cat && cur != except ) {
            float l = from.len(cur->loc);
            if( l < minl ) {
                minl = l;
                out = cur;
            }
        }
        cur = (Char*)cur->next;
    }
    return out;
}
Char *Char::getByNetworkId( int client_id, int internal_id ) {
    Char *cur = (Char*) g_char_layer->prop_top;
    while(cur) {
        if( cur->client_id == client_id && cur->internal_id == internal_id ) {
            return cur;
        }
        cur = (Char*) cur->next;
    }
    return NULL;
}


/////////////////
Beam::Beam( Vec2 lc, Vec2 at, BEAMTYPE beamtype, int base_index, int ene, int shooter_id, int client_id, int internal_id ) : Char( CAT_BEAM, lc, g_base_deck, g_char_layer, client_id, internal_id ), type(beamtype), ene(ene), shooter_id(shooter_id) {

    switch(beamtype) {
    case BEAMTYPE_NORMAL:
        v = lc.to(at).normalize( BEAM_NORMAL_VEL );
        clean_at = 1;
        setIndex( base_index );
        break;
    case BEAMTYPE_BLASTER:
        clean_at = 0.5;        
        v = lc.to(at).normalize( BEAM_BLASTER_VEL );

        break;
    }


    setRot(atan2( v.y, v.x ));
    setColor(WHITE);

    updateIndex();
    tex_epsilon = DEFAULT_TEX_EPS;

    hitsz = 3 + (float)(ene/16);

    if( isLocal() ) realtimeNewBeamSend(this);
}

void Beam::updateIndex() {
    switch(type) {
    case BEAMTYPE_NORMAL:
        break;
    case BEAMTYPE_BLASTER:
        setIndex( B_ATLAS_BLASTER_FRAGMENT_BASE + ((int)(accum_time*20)%2) );
        break;
    }
}



void Beam::createSparkEffect() {
    if( isLocal() ) {
        realtimeCreateBeamSparkEffectSend(this);
    }
    for(int i=0;i<5;i++){
        switch(type) {
        case BEAMTYPE_NORMAL:
            {
                Particle *e = new Particle( loc, range(0.5,1),range(0.5,1),0, 0.2,0, index+AU, g_base_deck, true );
                e->v = (v*0.7).randomize(200);
            }
            break;
        case BEAMTYPE_BLASTER:
            new Particle( loc, 1,2,0,  0.3,0.4, B_ATLAS_BLASTER_EXPLOSION, g_base_deck, true );            
            break;
        }
    }    
}


bool Beam::charPoll( double dt ) {
    // Shifter tile bends beam
    Cell *c = g_fld->get(loc);
    if( c->gt == GT_SHIFTER && c->st == ST_NONE && c->bt == BT_AIR ) {
        Vec2 dv = dirToVec2(c->dir) * PPC* SHIFTER_ACCEL;
        v += dv * dt;
        if( v.len() > BEAM_NORMAL_VEL ) {
            v = v.normalize(BEAM_NORMAL_VEL);
        }
        setRot( atan2(v.y,v.x));
        //        print("v:%f %f  dv:%f %f d:%d",v.x,v.y, dv.x, dv.y, c->dir );
    }
    
    loc += v * dt;

    // Stronger is bigger
    float s = PPC;
    if( ene >= 64 ) s *= 3; else if( ene >= 16 ) s *= 2; else if( ene >= 4 ) s *= 1.5;
    setScl(s);
    
    // Shoot on blocks
    Vec2 rt,lt,rb,lb;
    Cell *rtc = g_fld->get( rt = loc + Vec2(hitsz,hitsz));
    Cell *ltc = g_fld->get( lt = loc + Vec2(-hitsz,hitsz));
    Cell *rbc = g_fld->get( rb = loc + Vec2(hitsz,-hitsz));
    Cell *lbc = g_fld->get( lb = loc + Vec2(-hitsz,-hitsz));
    Cell *tgtc = NULL;
    Vec2 tgtat;
    Vec2 candat[4];
    Cell *cands[4];
    int candi=0;
    // 
    if(rtc&&rtc->isBeamHit() && rtc->isImmutableAgainstBeam()==false ) { cands[candi] = rtc; candat[candi] = rt; candi++; }
    if(rbc&&rbc->isBeamHit() && rbc->isImmutableAgainstBeam()==false ) { cands[candi] = rbc; candat[candi] = rb; candi++; }
    if(ltc&&ltc->isBeamHit() && ltc->isImmutableAgainstBeam()==false ) { cands[candi] = ltc; candat[candi] = lt; candi++; }
    if(lbc&&lbc->isBeamHit() && lbc->isImmutableAgainstBeam()==false ) { cands[candi] = lbc; candat[candi] = lb; candi++; }
    if( candi > 0 ) {
        int ind = irange(0,candi);
        tgtc = cands[ind];
        tgtat = candat[ind];
    }

    // Out of the world
    if(!rtc)return false;

    if( rtc && rtc->gt == GT_JUNGLE && range(0,100) < 1 ) {
        createLeafEffect(loc);
    }

    updateIndex();

    if( isRemote() ) return true;

    if(tgtc) {
        int consumed;
        BLOCKTYPE orig_bt = tgtc->bt;
        if( g_fld->damage(tgtat,ene,&consumed,this) ) {
            createSparkEffect();
            if( orig_bt == BT_CELL || orig_bt == BT_FLYGEN ) {
                soundPlayAt(g_wormdamage_sound,loc,1);
            } else if( orig_bt != BT_SNOW && orig_bt != BT_IVY && orig_bt != BT_TREE && orig_bt != BT_BOMBFLOWER ) {
                soundPlayAt(g_beamhithard_sound,loc,1);
            }
            if( orig_bt == BT_BARRIER && tgtc->hyper_count > 0 ) {
                Vec2 tgt;
                if( g_fld->findEnemyAttackTarget(loc,&tgt, MACHINE_SHOOT_DISTANCE ) ) {
                    int n = irange(1,4);
                    for(int i=0;i<n;i++) Bullet::shootAt( BLT_SPARIO, loc, tgt );
                }
            }
            ene -= consumed;
            if( ene <= 0 ) return false; else return true;
        }
    } else {
        // Immutable cells
        Cell *cells[4];
        g_fld->getCorner4( loc, 1, &cells[0], &cells[1], &cells[2], &cells[3] );
        for(int i=0;i<4;i++) {
            if(cells[i] && cells[i]->isImmutableAgainstBeam()) {
                soundPlayAt(g_beamhithard_sound,loc,1);
                createSparkEffect();
                return false;
            }
        }
    }

    if( type == BEAMTYPE_BLASTER ) {
        float s = PPC;
        g_fld->meltSnow(loc + Vec2(-s,-s) );
        g_fld->meltSnow(loc + Vec2(-s,s) );
        g_fld->meltSnow(loc + Vec2(s,-s) );
        g_fld->meltSnow(loc + Vec2(s,s) );        
    } else {
        if( range(0,100) < (float)(ene)/2.0 ) {
            g_fld->meltSnow(loc);
        }
    }
        

    // Shoot on enemies
    Char *cur = (Char*) g_char_layer->prop_top;
    while(cur) {
        if( cur->isEnemyCategory() ) {
            Enemy *e = (Enemy*) cur;
            if( e->hitWithFlyHeight(this,PPC/2) && e->beam_hits ) {
                int dmg = ene;
                if( dmg > e->hp ) dmg = e->hp;
                e->notifyHitBeam(this, dmg);
                createSparkEffect();
                //
                ene -= dmg;
                if(ene<=0) to_clean = true;
                g_fld->meltSnow(loc);
            }
        } else if( cur->category == CAT_PC ) {
            // recharging other player characters
            PC *pc = (PC*) cur;
            if( pc->hit(this,PPC/2)) {
                //                print("pcid:%d shooter:%d ene:%d/%d", pc->id, shooter_id, pc->ene, pc->maxene );
                if( pc->id != shooter_id && shooter_id == g_pc->id ) {
                    //                    print("PC:E:%d id:%d max:%d", pc->ene, pc->id, pc->maxene );
                    int charged = pc->charge(ene);
                    if(charged>0) {
                        pc->energy_chain_heat_count ++;
                        //                        print("sending E-chain e:%d(%d>%d) to: %d-%d  heat:%d",
                        //                              ene, charged, pc->ene, pc->client_id, pc->internal_id ,pc->energy_chain_heat_count );
                        realtimeEnergyChainSend(pc,charged);
                        return false;
                    } 
                }
            }
        }
        
        cur = (Char*) cur->next;
    }




    return true;
}

///////////////////////
Debri::Debri( Vec2 lc, Vec2 iniv, int ind, int client_id, int internal_id ) : Char( CAT_DEBRI, lc, g_base_deck, g_char_layer, client_id, internal_id ), v(iniv), falling_to_pit(false), rotate_speed(0) {
    init(ind);
    if( isLocal() ) realtimeNewDebriSend(this);    
}

Debri::Debri( Vec2 lc, int ind, int client_id, int internal_id ) : Char( CAT_DEBRI, lc, g_base_deck, g_char_layer, client_id, internal_id ), falling_to_pit(false), rotate_speed(0) {
    init(ind);
    float vel = 5;
    v = Vec2( range(-vel*PPC,vel*PPC), range(-vel*PPC,vel*PPC) );
    if( isLocal() ) realtimeNewDebriSend(this);        
}
void Debri::init( int ind ) {
    tex_epsilon = DEFAULT_TEX_EPS;
    setIndex(ind);
    if( index < 13*16 ) setScl(PPC*0.6); else setScl(PPC);
    falling_to_pit = false;
    if( ind == B_ATLAS_ITEM_DARK_MATTER_PARTICLE ) {
        setFragmentShader( g_eye_col_replacer );
    }
}

bool Debri::charPoll( double dt ) {
    if( falling_to_pit ) {
        if( scl.x < 0.1 ) return false; else return true;
    }

    if( index == B_ATLAS_ITEM_ENERGY_PARTICLE ) {
        setScl(PPC*range(0.1,1));
    }
    if( index == B_ATLAS_ITEM_HYPER_PARTICLE ) {
        if( updateInterval(0, 0.25f) && range(0,100)< 20 ) {
            Particle *p = createHyperSpark(loc);
            p->v = Vec2(0,0).randomize(100);
        }
    }
    
    Vec2 nextloc = loc + v * dt;

    Cell *c = g_fld->get(nextloc);
    
    if( c && c->isWall(false) ) {
        // Just reversing. First X and then Y
        bool xhit=false, yhit=false;
        Vec2 nxloc = loc + Vec2( v.x * dt, 0 );
        Cell *nxc = g_fld->get(nxloc);
        
        if( (!nxc) || nxc->isWall(false)) {
            v.x *= -1;
            xhit = true;
        }
        Vec2 nyloc = loc + Vec2( 0, v.y * dt );
        Cell *nyc = g_fld->get(nyloc);
        if( (!nyc) || nyc->isWall(false) ) {
            v.y *= -1;
            yhit = true;
        }
        if( xhit && yhit ) v *= 0;
    } else if(c && c->gt == GT_PIT ) {
        Cell *cells[4];
        g_fld->getCorner4(loc, PPC/3, &cells[0], &cells[1], &cells[2], &cells[3] );
        bool tofall = true;
        for(int i=0;i<4;i++) {
            if( cells[i] && cells[i]->gt != GT_PIT ) tofall = false;
        }
        if( tofall ) {
            soundPlayAt( g_fall_sound, loc,1 );
            falling_to_pit = true;
            seekScl( 0,0, 0.5 );
        }
    }
    loc = nextloc;
    v *= 0.98;
    if( v.len() < 5 ) v *=0;

    // Timeout flickering
    if( accum_time > DEBRI_WARN_ERASE_SEC ) {
        setVisible(  (int)(accum_time * 20) % 2 );
        if( accum_time > DEBRI_ERASE_SEC ) {
            return false;
        } 
    }

    // Field class implements item gathering by PCs
    if( g_fld->checkGatherDebri(this) ) {
        realtimeCharDeleteSend(this);
        return false;
    }

    rot += rotate_speed;
    
    return true;
}

/////////////////////////////
Prop2D *createShadow( Vec2 lc ) {
    Prop2D *p = new Prop2D();
    p->setDeck(g_base_deck);
    p->setIndex( B_ATLAS_FLYER_SHADOW );
    p->setScl(PIXEL_PER_CELL,PIXEL_PER_CELL);
    p->setColor( 1,1,1,0.5);
    p->setLoc(lc);
    return p;
}



// div_num : Divide a circle(360deg) by this number
Vec2 quantizeAngle( Vec2 v, int div_num ) {
    float l = v.len();
    float r = atan2(v.y,v.x);
    float unit = M_PI / (float)(div_num/2);    
    float positive_r = M_PI + r + (unit/2.0); // from -PI to PI. (0 ~ 2*M_PI)
    int ir = (int)(positive_r / unit) % div_num;
    float quantized_r = ir * unit - M_PI;

    return Vec2( cos(quantized_r), sin(quantized_r) ) * l;
}



Flyer::Flyer( CATEGORY cat, Vec2 lc, float h, TileDeck *dk, int ind, int client_id, int internal_id ) : Char(cat,lc,dk, g_flyer_layer, client_id, internal_id ), h(h), v_h(0), gravity(0) {
    setIndex(ind);
    shadow_prop = createShadow(lc);
    addChild(shadow_prop);

    last_loc = loc;
        
}

bool Flyer::charPoll( double dt ) {
    shadow_prop->setLoc(loc);
    
    // Moving in altitute axis
    h += v_h * dt;

    if( gravity != 0 ){
        v_h += gravity * dt;
    }

    if( h < 0 ) {
        land();
        return false;
    }
    // Where to draw
    draw_offset.y = h;

    loc += v * dt;
    
    if( !flyerPoll(dt)){
        return false;
    }
    
    // Landing (ex. blaster hit)
    if( gotToGoal() ){
        land();
        return false;
    }
        
    
    last_loc = loc;
    return true;
}

///////////////////////

Blaster::Blaster( Vec2 lc, Vec2 aim, bool heal, int shooter_id, int client_id, int internal_id ) : Flyer( CAT_BLASTER, lc, 0, g_base_deck, heal ? B_ATLAS_ITEM_HEAL_BLASTER : B_ATLAS_ITEM_BLASTER, client_id, internal_id ), shooter_id(shooter_id) {
    to_heal = heal;
    goal = aim;
    h = PIXEL_PER_CELL/2;
    float l = lc.to(aim).len();
    if( l >0 && l < PIXEL_PER_CELL*2 ){
        v_h = 80;
    } else if( l < PIXEL_PER_CELL*4) {
        v_h = 150;
    } else if( l < PIXEL_PER_CELL*6 ){
        v_h = 200;        
    } else {
        v_h = 250;        
    }
    gravity = -800;
    v = lc.to(aim).normalize(200);
    
    seekRot( M_PI*16, 2 );
    type = FLT_BLASTER;
    if(isLocal()) realtimeNewFlyerSend(this);        
}
bool Blaster::flyerPoll( double dt ){
    return true;
}
void Blaster::land(){
    to_clean = true;
    soundPlayAt(g_blaster_explode_sound,loc,1);

    if( to_heal ) {
        createExplosion(loc, 1, 2 );
    } else {
        g_fld->burn(loc,this);    
    }
    
    float r = 0;
    for(int i=0;i<8;i++){
        Vec2 tov = Vec2::angle(r).normalize(range(100,150) );
        r += M_PI/4.0;
        if( to_heal ) {
            Debri *d = new Debri( loc, B_ATLAS_HP_PARTICLE );
            d->rotate_speed = range(7,10);
            d->setColor( Color(1,1,1,0.7));
            d->setScl( range(0.7,1) *PPC );
        } else {
            new Beam( loc, loc+tov, BEAMTYPE_BLASTER, -1, 20, shooter_id );
        }
    }
}


////////////////////////////
Flag::Flag() : Char( CAT_FLAG, Vec2(0,0), g_base_deck, g_char_layer ), all_cleared(false) {
    warpToNextLocation();
}

bool Flag::charPoll( double dt) {
    setIndex( B_ATLAS_FLAG_BASE + (int)(accum_time*4)%2 ) ;
    if(loc.len(g_pc->loc) < PPC && all_cleared == false ){
        stopEffect();
        soundPlayAt(g_getflag_sound,loc,1);
        Pos2 flag_pos(loc.x/PPC,loc.y/PPC);
        g_fld->clearFlag(flag_pos);
        
        warpToNextLocation();

        int cnt = dbSaveFlagCands();
        //        g_log->printf(WHITE, "hudUpdateMilestone dbSaveFlagCands:%d ", cnt );
        hudUpdateMilestone(cnt);
        realtimeEventSend( EVT_MILESTONE, g_pc->nickname, cnt );
        realtimeEventSend( EVT_CLEAR_FLAG, "clear_flag", flag_pos.x, flag_pos.y );
        hudMilestoneMessage( g_pc->nickname, cnt, true );
        dbSaveMilestoneProgressLog( g_current_project_id );
        if( cnt == MILESTONE_MAX ) hudCongratulateProjectIsOver();
    }
    return true;
}
void Flag::warpToNextLocation() {
    Pos2 rp = g_fld->getRespawnPoint();
    FlagCandidate *fc = g_fld->findNearestFlagCand(rp);
    if(fc) {
        loc = toCellCenter( Vec2( fc->pos.x * PPC, fc->pos.y * PPC ) );
        print("flag pos: %f %f", loc.x, loc.y );
    } else {
        all_cleared = true;
        setVisible(false);
    }
}



