
#include "moyai/client.h"

#include "ids.h"
#include "dimension.h"
#include "util.h"
#include "atlas.h"
#include "conf.h"
#include "item.h"
#include "net.h"
#include "field.h"
#include "char.h"
#include "hud.h"
#include "mapview.h"
#include "effect.h"
#include "enemy.h"
#include "pc.h"
#include "globals.h"

////////////////


Enemy::Enemy( Vec2 lc, TileDeck *dk, bool flying, bool swimming, float fly_height, int client_id, int internal_id ) : Char( CAT_ENEMY, lc, dk, g_char_layer, client_id, internal_id ), v(0,0), hp(1), maxhp(1), fly_height(fly_height), flying(flying), swimming(swimming), shadow_prop(NULL), timeout(ENEMY_DEFAULT_TIMEOUT), beam_hits(true), machine(true), reward_itt(ITT_EMPTY), explosion_type(EXT_MACHINE), bt_can_enter(BT_AIR), body_size(8), hit_category(CAT_INVAL), push_pc(false), hyper_until(0), enable_hyper_effect(false), fly_on_kill_num(0), egg_on_kill_num(0), enable_durbar(true), enable_move(true), enemy_type(ET_INVAL), net_auto_sync_move_interval_sec(2), base_score(0) {
    tex_epsilon = 1.0/1024.0/2.0;
    setFragmentShader(g_eye_col_replacer);
    render_children_first = true; // Shadow is always under main sprite
}
Enemy::~Enemy() {
    if( enemy_type != ET_INVAL ) realtimeCharDeleteSend(this);
}
void Enemy::checkLocalSyncMoveSend() {
    if( isLocal() ) {
        realtimeEnemyMoveSend(this);
    }
}

void Enemy::ensureShadow() {
    if( !shadow_prop ) {
        shadow_prop = createShadow(loc);
        addChild(shadow_prop);
    }
}

bool Enemy::charPoll( double dt ) {
    if( enemyPoll(dt) == false ) {
        return false;
    }

    Vec2 nloc;
    if( enable_move ) {
        nloc = loc + v * dt;
    } else {
        nloc = loc;
    }

    // Enemy hits enemy. used only by Builder. Builder can't go on to other Builder.
    if( hit_category != CAT_INVAL ) {
        Char *nearest = getNearestByCategory( loc, hit_category, this );
        if(nearest ) {
            float l = nearest->loc.len(loc);
            if( l < body_size*2 ) {
                Vec2 relv = nearest->loc.to(loc).normalize(PPC*8);
                nloc += relv * dt;
            }
        }
    }

    // Enemy pushes PC (used only by Builder now)
    if( push_pc ) {
        Char *cur = (Char*) g_char_layer->prop_top;
        while(cur) {
            if( cur->category == CAT_PC ) {
                float l = cur->loc.len(loc);
                if( l < body_size ) {
                    PC *pc = (PC*)cur;
                    pc->onPushed( GROUNDER_PUSH_PC_DAMAGE, this);
                    onPushPC(pc);
                }
            }
            cur = (Char*) cur->next;
        }
    }
    
    Vec2 finalcand = calcNextLoc( loc, nloc, body_size, true, flying, swimming, bt_can_enter );
    if( finalcand == loc ) {
        // can't move..
    }
    loc = finalcand;
    
    //
    draw_offset.y = fly_height;
    if( flying && shadow_prop ) {
        shadow_prop->setLoc(loc);
    }
    
    // no flickering 
    if( timeout > 0 && accum_time > timeout ) {
        onTimeout();
        return false;
    }

    // Hyper effect
    if( accum_time < hyper_until && enable_hyper_effect ){
        if( updateInterval( INTVL_IND_HYPER, 0.5 ) && range(0,10) < 10 ) {
            Particle *p = createHyperSpark(loc.randomize(PPC/2) + Vec2(0,fly_height));
            if( enable_move) p->v = v;
        }
    }

    // Getting hyper 
    Cell *c = g_fld->get(loc);
    if( c && c->hyper_count > 0 ) {
        hyper_until = accum_time + HYPER_TIME_SEC;
    }

    // Auto HP recovery by hyper
    if( hyper_until > accum_time ) {
        if( updateInterval(INTVL_IND_HYPER,0.5) ) {
            hp ++;
            if( hp>maxhp )hp = maxhp;
        }
    }

    // regular syncing
    if( net_auto_sync_move_interval_sec > 0 ) {
        if( enemy_type != ET_INVAL && updateInterval( INTVL_IND_AUTOSYNC, net_auto_sync_move_interval_sec ) && isLocal() ) {
            realtimeEnemyMoveSend(this);
        }
    }
    
    return true;
}
void Enemy::notifyHitBeam( Beam *b, int dmg ) {
    if( dmg <= 0 ) return;
    onBeam(b,dmg);
    realtimeEnemyDamageSend(this,dmg); // Send damage packet even when this instance is not generated locally (assume no cheating)
    bool killed = applyDamage(dmg);
    if(killed) {
        // Destroyed!
        bool hyp = hyper_until > accum_time;
        int bonus_rate = hyp ? 2 : 1;
        g_pc->modScore( base_score * bonus_rate );
    }
}
// Returns true when destroyed
bool Enemy::applyDamage( int dmg ) {
    hp -= dmg ;
    if( enable_durbar ) {
        DurBar *db = DurBar::ensure(this, hp, maxhp, g_durbar_deck );
        db->draw_offset.y = fly_height;
    }
    if( hp<= 0 ) {
        switch(explosion_type) {
        case EXT_NONE: break;
        case EXT_CREATURE:
            createDigSmoke(loc + Vec2(0,fly_height), 1);
            soundPlayAt(g_killcreature_sound,loc,1);
            break;
        case EXT_MACHINE:
            createExplosion( loc + Vec2(0,fly_height), 2,3 );
            soundPlayAt(g_killmachine_sound,loc,1);
            if(hyper_until > accum_time ){
                Vec2 tgt;
                if( g_fld->findEnemyAttackTarget(loc,&tgt, MACHINE_SHOOT_DISTANCE ) ) {
                    Bullet::shootFanAt( BLT_SPARIO, loc, tgt, 1, M_PI/8.0 ); 
                }
            }
            break;
        case EXT_BOSS:
            createBossExplosion( loc + Vec2(0,fly_height) );
            soundPlayAt(g_destroy_building_sound,loc,1);
            break;
        }

        if( fly_on_kill_num > 0 && hyper_until > accum_time ) {
            for(int i=0;i<fly_on_kill_num;i++){
                new Fly( loc );
            }
        }
        if( egg_on_kill_num > 0 && hyper_until > accum_time ) {
            for(int i=0;i<egg_on_kill_num;i++){
                Egg *e = new Egg( loc );
                e->v = Vec2(0,0).randomize(PPC*3);
            }
        }

        to_clean = true;
        if(enable_durbar) DurBar::clear(this);
        switch( reward_itt ) {
        case ITT_ARTIFACT:
            new Debri( loc, B_ATLAS_ITEM_ARTIFACT );
            break;
        default:
            break;
        }
        if( machine && maxhp > 5 ) {
            if( range(0,100) < 10 ) {
                new Debri(loc, B_ATLAS_ITEM_ENERGY_PARTICLE );
            }
        }
        onKill();
        return true;
    } else {
        if(machine) {
            soundPlayAt(g_beamhithard_sound,loc,1);                    
        } else {
            soundPlayAt(g_wormdamage_sound,loc,1);            
        }
        return false;
    }
}
Enemy *Enemy::getNearestShootable( Vec2 from ) {
    float minl = 9999999999;
    Enemy *out = NULL;
    Char *cur = (Char*) g_char_layer->prop_top;
    while(cur) {
        if( cur->category == CAT_ENEMY ){
            Enemy *e = (Enemy*)cur;
            if( e->beam_hits ) {
                float l = from.len(e->loc);
                if( l < minl ) {
                    minl = l;
                    out = e;
                }
            }
        }
        cur = (Char*)cur->next;
    }
    return out;
}


/////////////////////////////


Worm::Worm( Vec2 lc, int client_id, int internal_id ) : Enemy( lc, g_base_deck, NOT_FLYING, NOT_SWIMMING, 0, client_id, internal_id ), shooter(false), shoot_at(2), last_hit_at(0), rest_until(0), turn_at(0) {
    machine = false;
    hp = maxhp = 2;
    egg_on_kill_num = irange(1,3);
    setIndex( B_ATLAS_WORM_NORMAL_BASE );
    setScl( range(PPC,PPC*1.5) );
    explosion_type = EXT_CREATURE;
    enable_hyper_effect = true;
    bt_can_enter = BT_CELL;
    enemy_type = ET_WORM;
    net_auto_sync_move_interval_sec = 1;
    base_score = 50;
    if(isLocal()) realtimeNewEnemySend( this );
}
void Worm::onBeam( Beam *b, int dmg ) {
    // knockback
    v = b->v;
    turn_at = accum_time + KNOCKBACK_DELAY;
    realtimeEnemyKnockbackSend(this);
}
        

bool Worm::enemyPoll( double dt ) {
    // Get stronger when walk on ENHANCER
    Cell *c = g_fld->get(loc);
    if(!c)return false;
    Cell *uc = g_fld->get(loc+Vec2(0,PPC));
    Cell *dc = g_fld->get(loc+Vec2(0,-PPC));
    Cell *lc = g_fld->get(loc+Vec2(PPC,0));
    Cell *rc = g_fld->get(loc+Vec2(-PPC,0));
    
    if( shooter == false &&
        (c->gt == GT_ENHANCER || (uc&&uc->gt == GT_ENHANCER) || (dc&&dc->gt == GT_ENHANCER) || (rc&&rc->gt == GT_ENHANCER) || (lc&&lc->gt == GT_ENHANCER)) ) {
        hp = maxhp = 10;
        setIndex( B_ATLAS_WORM_SHOOTER_BASE );
        egg_on_kill_num = irange(4,10);
        base_score = 100;
        shooter = true;
    }

    
    if( accum_time < rest_until ) {
        v *= 0;
    } else {
        if( c->gt == GT_GRASS ) {
            if( accum_time > rest_until + 1 ) {
                rest_until = accum_time + 0.5;
            }
        }
        if( accum_time > turn_at ) {
            Vec2 tgt;
            bool hastgt = false;
            if( g_fld->findEnemyAttackTarget(loc, &tgt, WORM_SHOOT_DISTANCE ) ) {
                v = loc.to(tgt).normalize(PPC*3);
                hastgt = true;
            } else {
                v = Vec2(0,0).randomize(1).normalize(PPC*2);
            }

            turn_at = accum_time + ( hastgt ? 1 : 3 );
        }
    }

    // Shoot
    if( accum_time > shoot_at ) {
        if( shooter && range(0,100) < 20 ) {
            if(isLocal()) new Bullet( BLT_POISONBALL, loc, g_pc->loc );
            v *= 0;
            shoot_at = accum_time + range(1,3);
        } 
    }
    updateDefaultRightFacedRot(v);

    // Direct damage on touching PC
    if( hit(g_pc,PPC/4) && last_hit_at < accum_time - 0.5 ) {
        last_hit_at = accum_time;
        turn_at = accum_time + KNOCKBACK_DELAY;
        int dmg = WORM_DAMAGE;
        if( shooter ) dmg *= 2;
        g_pc->onAttacked(dmg, this);
        v = loc.to(g_pc->loc).normalize( -PPC * 10 );
        soundPlayAt( g_wormbite_sound, loc,1 );
        realtimeEnemyKnockbackSend(this);
    }
    
    
    return true;
}


//////////////////////////

Egg::Egg(Vec2 lc, int client_id, int internal_id ) : Enemy( lc, g_base_deck, NOT_FLYING, NOT_SWIMMING, 0, client_id, internal_id ), hatch_at(7) {
    setIndex( B_ATLAS_WORM_EGG );
    hp = maxhp = 1;
    setScl(PPC);
    explosion_type = EXT_CREATURE;
    v = Vec2(0,0);
    timeout = 999; // Must hatch before timeout
    setRot( irange(0,M_PI*2));
    enemy_type = ET_EGG;
    net_auto_sync_move_interval_sec = 2;
    base_score = 100;
    if(isLocal()) realtimeNewEnemySend(this);
}

bool Egg::enemyPoll(double dt) {
    v *= 0.95;
    if( accum_time > hatch_at ) {
        if(isLocal()) new Worm( loc );
        return false;
    }
    return true;
}

/////////////////////////
Fly::Fly( Vec2 lc, int client_id, int internal_id ) : Enemy( lc, g_base_deck, FLYING, NOT_SWIMMING, DEFAULT_FLY_HEIGHT, client_id, internal_id ), turn_at(1) {
    hp = maxhp = 1;
    setScl(PPC*1.5);
    explosion_type = EXT_CREATURE;
    v = Vec2(0,0);
    timeout = ENEMY_DEFAULT_TIMEOUT / 2;
    enable_hyper_effect = true;
    ensureShadow();
    v = Vec2(0,0).randomize(PPC*2);
    enemy_type = ET_FLY;
    net_auto_sync_move_interval_sec = 1;
    base_score = 20;
    if(isLocal()) realtimeNewEnemySend( this);
}



bool Fly::enemyPoll(double dt) {
    setIndex( B_ATLAS_FLY_BASE + ((int)(accum_time * 10) % 2) );
    if(accum_time > turn_at ) {
        turn_at = accum_time + range(1,2);
        Vec2 tgt;
        if( g_fld->findEnemyAttackTarget(loc,&tgt,WORM_SHOOT_DISTANCE)) {
            target = tgt;
        } else {
            target = loc.randomize(PPC*10);
        }
        checkLocalSyncMoveSend();
    }
    //
    float vel = (hyper_until > accum_time) ? PPC*20 : PPC * 5;
    
    Vec2 vv = loc.to(target).normalize(vel);
    v += vv * dt;
    v -= v * dt * 0.5;

    if( loc.len(g_pc->loc) < PPC/2 ) {
        to_clean = true;
        g_pc->onPushed(FLY_DAMAGE,this);        
    }
    return true;
}




/////////////////////

Bullet::Bullet( BULLETTYPE blt, Vec2 at, Vec2 to, int client_id, int internal_id ) : Enemy( at, g_base_deck, FLYING, NOT_SWIMMING, 0, client_id, internal_id ), hit_pc_damage(1), hit_wall_damage(1), bullet_type(blt), round_vel(0), round_dia(0), hit_beam_size(0), friction(0), rot_speed(0) {
    clean_at = 20;
    setIndex(0);
    float vel = PPC;
    timeout = 10;
    net_auto_sync_move_interval_sec = 0; // no regular interval sync, because going straight with constant velocity.
    
    bool to_rot = false;
    bool to_quantize = true;
    float hyper1_5x=1, hyper2x = 1, hyper3x = 1, hyper4x = 1;

    Cell *c = g_fld->get(at);
    if( c && c->hyper_count > 0 ) {
        hyper1_5x = 1.5;
        hyper2x = 2;
        hyper3x = 3;        
        hyper4x = 4;
        hyper_until = timeout;
    }

    switch(blt) {
    case BLT_POISONBALL:
        vel = PPC * range(3,6) * hyper2x;
        timeout /= hyper2x;
        setIndex( B_ATLAS_POISONBALL_BASE );
        setScl( range( PPC, PPC*2));
        hit_pc_damage = POISONBALL_DAMAGE;
        hit_wall_damage = 0;
        clean_at = 3;
        beam_hits = false;
        break;
    case BLT_SPARIO:
        vel = PPC * 4 * hyper2x;
        setIndex( (hyper2x>1) ? B_ATLAS_HYPER_SPARIO : B_ATLAS_SPARIO );
        setScl( PPC*1.5, PPC*1.5 );
        hit_pc_damage = SPARIO_DAMAGE * hyper2x;
        hit_wall_damage = 0;
        beam_hits = false;
        explosion_type = EXT_MACHINE;
        timeout /= hyper2x;
        break;
    case BLT_SIG:
        category = CAT_ENEMY; // This bullet can be destroyed by beam
        to_rot = true;
        hp = 1;
        vel = PPC * 8 * hyper2x;
        timeout /= hyper2x;
        setIndex( B_ATLAS_SIG );
        setScl( PPC*1.5, PPC*1.5 );
        hit_pc_damage = SPARIO_DAMAGE * hyper2x;
        hit_wall_damage = ( hyper2x > 1 ) ? 4 : 1;
        hit_beam_size = PPC/3;
        soundPlayAt( g_shoot_sig_sound, at,1 );
        explosion_type = EXT_MACHINE;
        enable_hyper_effect = true;
        base_score = 30;
        break;
    case BLT_BELO:
        category = CAT_ENEMY; // This bullet can be destroyed by beam
        hp = 5 * hyper4x;
        vel = PPC * hyper2x;
        timeout /= hyper2x;
        setIndex( B_ATLAS_BELO_BASE );
        setScl( PPC*2, PPC*2 );
        hit_pc_damage = 20 * hyper2x;
        hit_wall_damage = ( hyper2x > 1 ) ? 8 : 2;
        round_vel = 5 * hyper4x;
        round_dia = PPC*5 * hyper4x;
        hit_beam_size = PPC/2;
        explosion_type = EXT_MACHINE;
        base_score = 100;
        break;

    case BLT_FIREBALL: // undestroyable
    case BLT_FIRE:
    case BLT_ROTFIRE:
        if( blt == BLT_FIREBALL) {
            vel = PPC * range(3,25); 
            timeout = range(7,10);
            setIndex( B_ATLAS_STRAIGHT_FIREBALL );
            setScl( PPC* range(1,2));            
            to_rot = true;
            friction = 0.98;                        
        } else if( blt == BLT_FIRE ){
            vel = PPC; 
            timeout = range(3,5);
            setIndex( B_ATLAS_FIRE );
            to_rot = false;
            setScl( PPC );
            friction = 0.98;            
        } else if( blt == BLT_ROTFIRE ) {
            vel = PPC*4 * hyper2x; 
            setIndex( B_ATLAS_ROUND_FIRE );
            to_rot = false;
            rot_speed = 20;
            setScl( PPC );            
        }
        
        hit_wall_damage = 0;
        hit_pc_damage = FIREBALL_DAMAGE;
        beam_hits = false;        
        to_quantize = false;
        break;
    case BLT_SNOWBALL:
        vel = PPC * 6 * hyper2x;
        timeout = 10 / hyper2x;
        setIndex( B_ATLAS_SNOWBALL );
        setScl(PPC);
        to_rot = true;
        hit_pc_damage = 10;
        hp = 1;
        explosion_type = EXT_CREATURE;
        beam_hits = true;
        base_score = 20;
        break;
    case BLT_TORNADO:
        vel = PPC * 4 * hyper2x;
        timeout = 15/hyper2x;
        setIndex(0); // Animated in polling func
        setScl(PPC);
        enable_hyper_effect = true;
        hit_pc_damage = 20 * hyper2x;
        beam_hits = false;
        hit_wall_damage = 2 * hyper2x;
        bt_can_enter = BT_SNOW; // Goes over the snow
        break;
    case BLT_SHRIMP_DEBRI:
        setIndex( B_ATLAS_SHRIMP_DEBRI );
        vel = PPC * 10;
        timeout = 1;
        setScl(PPC);
        hit_pc_damage = 20;
        beam_hits = true;
        hp = 2;
        hit_wall_damage = 1;
        rot_speed = 40;
        explosion_type = EXT_CREATURE;
        base_score = 10;
        break;
    case BLT_BOUNCER:
        setIndex( B_ATLAS_BOUNCER );
        vel = PPC * 6 * hyper1_5x;
        timeout = 15/hyper2x;
        setScl(PPC);
        hit_pc_damage = 20;
        beam_hits = true;
        hp = 3 * hyper3x;
        hit_wall_damage = 0;
        explosion_type = EXT_MACHINE;
        body_size = 4;
        base_score = 50;
        break;
    case BLT_CRYSTAL_BEAM:
        setIndex( B_ATLAS_STRAIGHT_BEAM );
        vel = PPC * 10 * hyper1_5x;
        timeout = 8/hyper1_5x;
        setScl(PPC);
        hit_pc_damage = 40;
        beam_hits = false;
        hp = 0;
        hit_wall_damage = 10;
        body_size = 2;
        to_rot = true;
        break;
    default:
        assertmsg( false, "invalid bullettype:%d", blt);
    }
    
    v = at.to(to).normalize(vel);

    if( to_quantize ) v = quantizeAngle(v, 32 );

    if( to_rot ) updateDefaultRightFacedRot(v);
    
    if( hit_wall_damage == 0 && c && c->isPlayerBuilding() == false && c->isEnterable(true,true,false,BT_AIR) == false ) {
        to_clean = true;
        // TODO: Now erasing bullet just after appearing. But ideally this shold't appear. This causes unnecessary network traffic.
    } else {
        if( isLocal() ) realtimeNewBulletSend(this);
    }
}
Bullet::~Bullet() {
    if(isLocal()) realtimeCharDeleteSend(this);    
}
bool Bullet::enemyPoll( double dt ) {

    if( friction > 0 ) {
        v *= friction;
    }
        
    if( round_vel > 0 ) {
        loc += Vec2( cos( accum_time*round_vel), sin(accum_time*round_vel) ) * round_dia * dt;
    }

    if( rot_speed > 0 ) {
        setRot( accum_time * rot_speed );
    }

    if( hit_pc_damage > 0 ) { 
        if( loc.len(g_pc->loc) < PPC/3 ) {
            g_pc->onAttacked(hit_pc_damage, this);
            createDigSmoke(loc,1);
            return false;
        }
    }

    if( bullet_type == BLT_BELO ) {
        setIndex( B_ATLAS_BELO_BASE + (int)( accum_time * 3 ) % 2 );
    }
    if( (bullet_type == BLT_FIREBALL || (bullet_type == BLT_FIRE ) ) && v.len() < PPC ) {
        setIndex( B_ATLAS_FIRE );
        setXFlip( ((int)(accum_time*32) % 2) );
        setRot(0);
    }
    if( bullet_type == BLT_FIRE || bullet_type == BLT_FIREBALL || bullet_type == BLT_ROTFIRE ) {
        g_fld->meltSnow(loc);
    }
    if( bullet_type == BLT_TORNADO ) {
        float hyrate = hyper_until > accum_time ? 3 : 1;
        setIndex( B_ATLAS_TORNADO_BASE + ( (int)(accum_time*15) % 4) );
        float maxvel = PPC*4*hyrate;
        Vec2 dv = loc.to(g_pc->loc).normalize(maxvel);
        v += dv * dt;
        if( v.len() > maxvel) v = v.normalize(maxvel);
    }
    if( bullet_type == BLT_POISONBALL ) {
        float s = 1 + sin( accum_time * 20 ) * 0.4;
        setScl(s*PPC*2);
    }
    
    return true;
}


void Bullet::shootAt( BULLETTYPE blt, Vec2 from, Vec2 at ) {
    new Bullet(blt,from,at);
}

void Bullet::shootFanAt( BULLETTYPE blt, Vec2 from, Vec2 at, int side_n, float pitch ) {
    Bullet *center = new Bullet(blt, from,at);
    float rad = atan2( center->v.y, center->v.x );
    
    for(int i=0;i<side_n;i++) {
        Vec2 sidev( cos(rad + pitch*(i+1)), sin(rad + pitch*(i+1)) );
        Bullet *b = new Bullet(blt,from, from + sidev.normalize( center->v.len() ) );

        sidev = Vec2( cos(rad - pitch*(i+1)), sin(rad - pitch*(i+1)) );
        b = new Bullet(blt, from, from + sidev.normalize( center->v.len() ) );
    }
}

Beam *Bullet::checkHitBeam() {
    Char *cur = (Char*) g_char_layer->prop_top;
    while(cur) {
        if( cur->category == CAT_BEAM ) {
            if( cur->hit(this,hit_beam_size) ) {
                return (Beam*)cur;                
            }
        }
        cur = (Char*) cur->next;
    }
    return NULL;
}
void Bullet::onTouchWall( Vec2 nextloc, int hitbits, bool nxok, bool nyok ) {
    Vec2 rt_at, rb_at, lt_at, lb_at;
    Cell *rtc = (hitbits & Field::WALKABLE_BIT_HIT_RT) ? g_fld->get( rt_at = nextloc+Vec2(body_size,body_size)) : NULL;
    Cell *ltc = (hitbits & Field::WALKABLE_BIT_HIT_LT) ? g_fld->get( lt_at = nextloc+Vec2(-body_size,body_size)) : NULL;
    Cell *rbc = (hitbits & Field::WALKABLE_BIT_HIT_RB) ? g_fld->get( rb_at = nextloc+Vec2(body_size,-body_size)) : NULL;
    Cell *lbc = (hitbits & Field::WALKABLE_BIT_HIT_LB) ? g_fld->get( lb_at = nextloc+Vec2(-body_size,-body_size)) : NULL;
    Cell *tgtc = NULL;
    
    
    // Find the nearest cell
    float minl = 999999,l;
    Vec2 at;
    if( (l=loc.len(rt_at)) < minl ) { tgtc = rtc; minl = l; at = rt_at; }
    if( (l=loc.len(lt_at)) < minl ) { tgtc = ltc; minl = l; at = lt_at; }
    if( (l=loc.len(rb_at)) < minl ) { tgtc = rbc; minl = l; at = rb_at; }
    if( (l=loc.len(lb_at)) < minl ) { tgtc = lbc; at = lb_at; }

    if( tgtc == NULL ) return;
    

    if( bullet_type == BLT_SNOWBALL ) {
        g_fld->tryPutBlockNearestOnAir(loc, BT_SNOW );
        to_clean = true;
    } else if( bullet_type == BLT_BOUNCER && tgtc->isPlayerBuilding() == false ) { // bounding bullet
        bool xrev = (!nxok), yrev = (!nyok);
        if( hitbits & Field::WALKABLE_BIT_HIT_RT ) {
            if( hitbits & Field::WALKABLE_BIT_HIT_LT ) { // Up
                yrev = true;
            } else if( hitbits & Field::WALKABLE_BIT_HIT_RB ){ // Right
                xrev = true;
            } else { // Right Up
            }
        } else if( hitbits & Field::WALKABLE_BIT_HIT_LT ) {
            if( hitbits & Field::WALKABLE_BIT_HIT_LB ) { // Left
                xrev = true;
            } else { // Left Up
            }
        } else if( hitbits & Field::WALKABLE_BIT_HIT_LB ) {
            if( hitbits & Field::WALKABLE_BIT_HIT_RB ) { // Down
                yrev = true;
            } else { // Left Down
            }
        } else if( hitbits & Field::WALKABLE_BIT_HIT_RB ) { // Right Down
        }
        if(xrev) v.x *= -1;
        if(yrev) v.y *= -1;

        if(nxok==false && nyok == false) to_clean = true;
    } else {
        int damage = hit_wall_damage;
        if( tgtc->isPlayerBuilding() ) {
            damage += 1;
        }
        if( tgtc->bt == BT_SNOW && (bullet_type== BLT_FIREBALL || bullet_type == BLT_ROTFIRE || bullet_type == BLT_FIRE ) ) {
            damage += 1;
        }

        if( damage > 0 ) {
            g_fld->damage( at, damage, NULL, this );
        }        
        createExplosion( loc, 1, 1 );
        if( hit_wall_damage > 0 )    soundPlayAt( g_hitspario_sound, loc,1);
        to_clean = true;
    }
    
}

//////////////


Takwashi::Takwashi( Vec2 lc, int client_id, int internal_id ) : Enemy( lc, g_base_deck, FLYING, NOT_SWIMMING, DEFAULT_FLY_HEIGHT, client_id, internal_id ), shoot_start(0), shoot_end(0), last_shoot_at(0), to_shoot(false) {
    setIndex( B_ATLAS_TAKWASHI );
    v = Vec2(0,0);
    hp = maxhp = 25;
    explosion_type = EXT_MACHINE;
    setFragmentShader(g_eye_col_replacer);
    enable_hyper_effect = true;
    ensureShadow();
    enemy_type = ET_TAKWASHI;
    net_auto_sync_move_interval_sec = 1;
    base_score = 800;
    if(isLocal()) realtimeNewEnemySend(this);
}
bool Takwashi::enemyPoll(double dt) {
    if( shoot_start == 0 ) {
        shoot_start = accum_time + 2;
        shoot_end = accum_time + 4;
    } else {
        if( accum_time > shoot_end ) {
            shoot_start = shoot_end = 0;
        }
    }
    if( accum_time > shoot_start && accum_time < shoot_end ) {
        if( v.len() > 0 ) {
            Vec2 tgt;
            if( (to_shoot = g_fld->findEnemyAttackTarget(loc, &tgt, TAKWASHI_SHOOT_DISTANCE)) ) {            
                shoot_tgt = tgt;
                updateDefaultRightFacedRot(loc.to(tgt));    
            } 
        }
        float intvl = 0.1;
        if( hyper_until > accum_time ) intvl /= 2;
        if( accum_time > last_shoot_at + intvl) {
            last_shoot_at = accum_time;
            if( to_shoot && isLocal() ) Bullet::shootAt( BLT_SIG, loc + Vec2(0,fly_height), shoot_tgt );
        }
        v = Vec2(0,0);
    } else {
        if( v.len() == 0 ) {
            Vec2 tgt;
            if( g_fld->findEnemyAttackTarget( loc, &tgt, TAKWASHI_SHOOT_DISTANCE ) ) {
                float vel = PPC;
                if( hyper_until > accum_time ) vel *= 3;
                v = loc.to(tgt).normalize(vel);
                updateDefaultRightFacedRot(v);    
            }
        }
        
    }
    
    return true;
}



///////////////////////
Girev::Girev( Vec2 lc, int client_id, int internal_id ) :Enemy( lc, g_girev_deck, FLYING, NOT_SWIMMING, DEFAULT_FLY_HEIGHT, client_id, internal_id ), shoot_at(0), turn_at(0), repairer_at(3) {
    setIndex(0);
    v = Vec2(0,0);
    hp = maxhp = 500;
    setScl(64,64);
    reward_itt = ITT_ARTIFACT;
    explosion_type = EXT_BOSS;
    enable_hyper_effect = true;
    timeout = ENEMY_DEFAULT_TIMEOUT * 100;
    ensureShadow();
    enemy_type = ET_GIREV;
    net_auto_sync_move_interval_sec = 1;
    base_score = 4000;
    if(isLocal()) realtimeNewEnemySend(this);
}

bool Girev::enemyPoll(double dt) {
    if( isLocal() ) { 
        if( accum_time > turn_at) {
            float intvl = 2;
            if( hyper_until > accum_time ) intvl = 0.8;
            turn_at = accum_time + intvl;
            Vec2 tgt;
            if( g_fld->findEnemyAttackTarget( loc, &tgt, MACHINE_SHOOT_DISTANCE) ) {
                float vel = PPC;
                if( hyper_until > accum_time ) vel *= 2;
                v = loc.to(tgt).normalize(vel);
                if(isLocal()) Bullet::shootFanAt( BLT_SPARIO, loc + Vec2(0,fly_height), tgt, 8, M_PI/8.0 );
            }
            checkLocalSyncMoveSend();
        }
        if( accum_time > repairer_at ) {
            float intvl = 2;
            if( hyper_until > accum_time ) intvl /= 4;
            repairer_at = accum_time + intvl;
            new Repairer(loc);
        }
    }        
    return true;
}
///////////////////////
Repairer::Repairer( Vec2 lc, int client_id, int internal_id ) : Enemy(lc, g_base_deck, FLYING, NOT_SWIMMING, DEFAULT_FLY_HEIGHT, client_id, internal_id ), turn_at(0), spark_at(0) {
    setIndex( B_ATLAS_ZOSHI_BASE );
    v = Vec2(0,0);
    hp = maxhp = 20;
    setScl(24*1.5);
    timeout = ENEMY_DEFAULT_TIMEOUT * 2;
    explosion_type = EXT_MACHINE;
    enable_hyper_effect = true;
    ensureShadow();
    enemy_type = ET_REPAIRER;
    net_auto_sync_move_interval_sec = 1;
    base_score = 1000;
    if(isLocal()) realtimeNewEnemySend(this);
}
bool Repairer::enemyPoll(double dt) {
    setIndex( B_ATLAS_ZOSHI_BASE + (((int)(accum_time*30) % 4)));

    if( accum_time > turn_at ) {
        float intvl = 3;
        float vel = PPC * 2;
        if( hyper_until > accum_time ) {
            intvl /= 2;
            vel *= 2;
        }
        turn_at = accum_time + intvl;
        if( isLocal() ) {
            v = Vec2(0,0).randomize(1).normalize(vel);
            checkLocalSyncMoveSend();
        }
    }

    // Where to repair?
    int sight = 15;
    Pos2 from( loc.x/PPC, loc.y/PPC);
    DIR dir_to_go = DIR_NONE;
    Cell *to_fix = NULL;
    for(int i=1;i<sight;i++) {
        Cell *c = g_fld->get( from + Pos2(i,0) );
        if(c && c->isToRepair() ) { dir_to_go = DIR_RIGHT; to_fix = c; break; }
        c = g_fld->get( from + Pos2(-i,0) );
        if(c && c->isToRepair() ) { dir_to_go = DIR_LEFT; to_fix = c; break; }
        c = g_fld->get( from + Pos2(0,i) );
        if(c && c->isToRepair() ) { dir_to_go = DIR_UP; to_fix = c; break; }
        c = g_fld->get( from + Pos2(0,-i) );
        if(c && c->isToRepair() ) { dir_to_go = DIR_DOWN; to_fix = c; break; }
    }
    if( dir_to_go != DIR_NONE ) {
        int dx,dy;
        dirToDXDY(dir_to_go,&dx,&dy);
        v = Vec2(dx,dy) * PPC*2;

        if( accum_time > spark_at ) {
            spark_at = accum_time + 0.5;
            int x,y;
            g_fld->getCellXY(to_fix, &x,&y);
            if( from.len(Pos2(x,y)) < 2 ) {
                Vec2 spark_at = toCellCenter(x,y);
                createSpark(spark_at, range(0.7,1.5));
                to_fix->content ++;
                soundPlayAt(g_eye_repair_sound,loc,1);
                if( to_fix->content > 10 ) {
                    if( to_fix->bt == BT_ENEMY_EYE_DESTROYED ) {
                        BLOCKTYPE cands[] = { BT_MONO_EYE, BT_COMBI_EYE, BT_PANEL_EYE, BT_SHIELD_EYE, BT_ROUND_EYE };
                        to_fix->bt = choose( cands );
                        to_fix->content = 0;
                        g_fld->notifyChanged(to_fix);
                        assert( to_fix->fortress_id > 0 );
                        //                        Fortress *ft = g_fld->searchFortress( to_fix->fortress_id );
                        //                        assert(ft);
                        //                        ft->setEyeRepaired( Pos2(x,y), to_fix->bt );
                    } else if( to_fix->st == ST_GROUNDWORK && to_fix->bt == BT_AIR ) {
                        Pos2 at(x,y);
                        if( g_fld->checkBarrierGen(at, DIR_UP, BARRIER_INTERVAL) >0 ||
                            g_fld->checkBarrierGen(at, DIR_DOWN, BARRIER_INTERVAL) >0 ||
                            g_fld->checkBarrierGen(at, DIR_RIGHT, BARRIER_INTERVAL) >0 ||
                            g_fld->checkBarrierGen(at, DIR_LEFT, BARRIER_INTERVAL) >0 ) {
                            to_fix->st = ST_NONE;
                            to_fix->bt = BT_BARRIERGEN;
                            g_fld->notifyChanged(to_fix);
                        } else {
                            to_fix->st = ST_NONE;
                            g_fld->buildRandomGrounder(at);
                        }
                            
                    }
                    soundPlayAt( g_repair_done_sound,loc,1);
                }
            }
        }
    }
    
    return true;    
}

////////////////////
Chaser::Chaser( Vec2 lc, int client_id, int internal_id ) :Enemy(lc, g_base_deck, FLYING, NOT_SWIMMING, 0.1, client_id, internal_id ), reset_target_at(0), shoot_at(2), accel_at(1) {
    hp = maxhp = 1;
    setScl(24*1.5);
    explosion_type = EXT_MACHINE;
    v = Vec2(0,0).randomize(1).normalize( range(3,5)*PPC );
    thres = range(8,16) * PPC;
    enable_hyper_effect = true;
    ensureShadow();
    enemy_type = ET_CHASER;
    net_auto_sync_move_interval_sec = 0.2;  // Quicker, because of frequent turning
    base_score = 400;
    if(isLocal()) realtimeNewEnemySend(this);
}

bool Chaser::enemyPoll(double dt) {
    setIndex( B_ATLAS_TOROID_BASE + (( (int)(accum_time*12) % 7 )));
    if( fly_height < DEFAULT_FLY_HEIGHT ) {
        fly_height += PPC * dt;
    }

    if( isLocal() ) {
        if( accum_time > reset_target_at ) {
            reset_target_at = accum_time + 2;
            Vec2 tgt;
            if( g_fld->findEnemyAttackTarget( loc, &tgt, MACHINE_SHOOT_DISTANCE ) ) {
                target = tgt;
            }
        }
        // Rest at the beginning and then start to keep modest distance to target. 
        if( accum_time > accel_at ) {
            float l = loc.len(target);
            float vel = PPC*8;
            Vec2 dv = loc.to(target).normalize(vel);
            if( l < thres ) {
                v += -dv * dt;
            } else {
                v += dv * dt;
            }
            if( v.len() > vel) v = v.normalize(vel);
        }
    
        //
        if( accum_time > shoot_at ) {
            float intvl = range(1,3);
            if( hyper_until > accum_time ) intvl /= 8;
            shoot_at = accum_time + intvl;
            if(isLocal()) Bullet::shootAt( BLT_SPARIO, loc + Vec2(0,fly_height), target );
        }
    }
    return true;
}
//////////////////////////
Builder::Builder( Vec2 lc, int client_id, int internal_id ) : Enemy( lc, g_base_deck, NOT_FLYING, NOT_SWIMMING, 0, client_id, internal_id ), turn_at(0), building(false), build_dir(DIR_NONE), build_cnt(0), build_after_at(8) {
    category = CAT_BUILDER;
    hit_category = CAT_BUILDER;
    body_size = 10;
    hp = maxhp = 10;
    setScl(24*1.5); // because pixel art is 16x16
    explosion_type = EXT_MACHINE;
    v = Vec2(0,0);
    setIndex( B_ATLAS_BUILDER_BASE );
    timeout = ENEMY_DEFAULT_TIMEOUT * 3;
    push_pc = true;
    enable_hyper_effect = true;
    enemy_type = ET_BUILDER;
    net_auto_sync_move_interval_sec = 1;
    base_score = 1500;
    if( isLocal() ) realtimeNewEnemySend(this);
}

bool Builder::enemyPoll(double dt) {

    if( isLocal() ) { 
        if( accum_time > turn_at ) {
            turn_at = accum_time + 4;
            building = ( range(0,100) < 20 );
            if( accum_time < build_after_at ) building = false;
            if(building) {
                v *= 0;
            } else {
                float vel = PPC * 2;
                if( hyper_until > accum_time ) vel *= 3;
                v = randomDirToVec2().normalize( vel );
            }
            checkLocalSyncMoveSend();
        }
        if( building ) {
            // Go to the center of a cell
            Vec2 cellcenter = toCellCenter( loc);
            if( loc.len(cellcenter) > 1 ) {
                v = loc.to(cellcenter).normalize(PPC);
            } else {
                if( v.len() > 0 ) {
                    turn_at = accum_time + 2;
                    v *= 0;
                    build_cnt = 0;
                }

                if( build_dir == DIR_NONE ) build_dir = randomDir();
				
                if( updateInterval( INTVL_IND_BUILDER_CHECKBUILD, 0.2 )  ) {
                    Vec2 build_at = toCellCenter( loc + dirToVec2(build_dir) * PPC );
                    createSpark( build_at, range(0.7,1.5) );
                    soundPlayAt(g_eye_repair_sound,loc,1);
                    build_cnt ++;
                
                    if( build_cnt > 5 ) {
                        g_fld->buildRandomGrounder(Pos2(build_at.x/PPC,build_at.y/PPC));
                        build_cnt = 0;
                        turn_at = accum_time;
                    }
                }
                // 
            }
        }
    }
    
    return true;
}
void Builder::onTouchWall( Vec2 nextloc, int hitbits, bool nxok, bool nyok ) {
    v = randomDirToVec2().normalize( PPC*2 );    
}

//////////////////////

HyperPutter::HyperPutter( Vec2 lc, int client_id, int internal_id ) : Enemy( lc, g_base_deck, FLYING, NOT_SWIMMING, 0, client_id, internal_id ) {
    setIndex( B_ATLAS_HYPERPUTTER_BASE );
    hp = maxhp = 1;
    setScl(PPC);
    explosion_type = EXT_MACHINE;
    v = Vec2(0,0);
    curdir = randomDir(); // Use random only in the beginning and then go deterministic
    goalpos = Pos2( lc.x/PPC, lc.y/PPC ) + dirToPos2(curdir);
    ensureShadow();
    enemy_type = ET_HYPERPUTTER;
    net_auto_sync_move_interval_sec = 1;
    base_score = 1000;
    if(isLocal())realtimeNewEnemySend(this);
}

bool HyperPutter::enemyPoll(double dt) {
    if( fly_height < DEFAULT_FLY_HEIGHT ) {
        fly_height += PPC * dt;
    }
    if( isLocal() ) {
        Pos2 p(loc.x/PPC,loc.y/PPC);
        if( v.len() == 0 ) {
            Pos2 dp = (goalpos - p);
            v = Vec2(dp.x,dp.y) * PPC*4;
            checkLocalSyncMoveSend();
        } else {
            Vec2 goalat = toCellCenter( goalpos );
            if( goalat.len(loc) < 3 ) {
                v *= 0;
                Cell *c = g_fld->get(p);
                if(!c)return false;
                if( c->bt == BT_AIR && c->st == ST_NONE && c->isBlockPuttable() ) {
                    c->st = ST_HYPERLINE;
                    g_fld->notifyChanged(c);
                    createSpark( loc, 1 );
                    return false;
                }
                int rr = c->randomness % 100;
                if( rr > 90 ) {
                    if( (rr % 2) == 0 ) {
                        curdir = rightDir(curdir);
                    } else {
                        curdir = leftDir(curdir);
                    }
                }
                goalpos = p + dirToPos2(curdir);
            }
        }
    }
    
    return true;
}

////////////////////////

Voider::Voider( Vec2 lc, int client_id, int internal_id ) : Enemy( lc, g_base_deck, FLYING, NOT_SWIMMING, DEFAULT_FLY_HEIGHT, client_id, internal_id ), erase_at(7) {
    setIndex( B_ATLAS_VOIDER );
    hp = maxhp = 50;
    setScl(PPC*1.5);
    explosion_type = EXT_MACHINE;
    v = Vec2(0,0).randomize(PPC*2);
    ensureShadow();
    enable_durbar = false;
    enemy_type = ET_VOIDER;
    net_auto_sync_move_interval_sec = 2;
    base_score = 300;
    if(isLocal()) realtimeNewEnemySend(this);
}

bool Voider::enemyPoll(double dt) {
    v *= 0.99;
    if( hp < maxhp ) {
        erase_at = 4;
        hp = maxhp;
        v = Vec2(0,0).randomize(PPC*2);
        new Voider(loc);
    }
    if( accum_time > erase_at/2 ) {
        float r = (erase_at - accum_time) / (erase_at/2);
        setScl(r*PPC*1.5);
        if( r < 0.2 ) return false;
    }
    return true;
}

/////////////////////////
VolcanicBomb::VolcanicBomb( Vec2 lc, int client_id, int internal_id ) : Flyer( CAT_VOLCANIC_BOMB, lc, 0, g_base_deck, B_ATLAS_VOLCANIC_BOMB, client_id, internal_id ) {
    v = Vec2(0,0).randomize( PPC*irange(2,4));
    v_h = PPC * range(7,15);
    gravity = - PPC*5 + range(-PPC,PPC);
    type = FLT_VOLCANICBOMB;
    if(isLocal()) realtimeNewFlyerSend(this);
}
bool VolcanicBomb::flyerPoll( double dt ) {
    Vec2 avg_v = v + Vec2(0,v_h);
    setRot( atan2(avg_v.y, avg_v.x) );
    return true;
}
void VolcanicBomb::land() {
    soundPlayAt(g_blaster_explode_sound,loc,1);
    createExplosion(loc, 1, 2 );

    Pos2 p(loc.x/PPC,loc.y/PPC);
    g_fld->scatterLava(p);
    for(int i=0;i<5;i++) {
        createSmokeEffect( loc.randomize(PPC) );
    }
}
///////////////////////

FireGenerator::FireGenerator( Vec2 lc, int client_id, int internal_id ) : Enemy( lc, g_base_deck, NOT_FLYING, NOT_SWIMMING, 0, client_id, internal_id ) {
    setIndex(-1);
    clean_at = 2;
    enemy_type = ET_FIREGENERATOR;
    if(isLocal()) realtimeNewEnemySend(this);
}
bool FireGenerator::enemyPoll( double dt ) {
    if(isLocal()) {
        if( updateInterval( INTVL_IND_FIREGENERATOR_SHOOT, 0.1 ) ) {
            if( range(0,100) < 10 ) soundPlayAt( g_fbshoot_sound, loc,1 );    
            shoot();
        }
    }
    return true;
}
void FireGenerator::shoot() {
    Cell *c = g_fld->get(loc);
    if( c && c->dir != DIR_NONE ) {
        Vec2 v = Vec2::fromDir( c->dir ).normalize(PPC);
        if(isLocal()) Bullet::shootAt( BLT_FIREBALL, loc + v, loc + v * 2 + Vec2(0,0).randomize(3) );
    }
}
/////////////////////
FireCopter::FireCopter( Vec2 lc, int client_id, int internal_id ) : Enemy( lc, g_base_deck, FLYING, NOT_SWIMMING, PPC, client_id, internal_id ) {
    vel = PPC * range(1,2);
    setIndex( B_ATLAS_FIRECOPTER );
    hp = maxhp = 16;
    machine = false;
    setScl(PPC);
    explosion_type = EXT_MACHINE;
    enable_hyper_effect = true;
    ensureShadow();
    gen_child_at = 0;
    turn_at = 0;
    enemy_type = ET_FIRECOPTER;
    base_score = 900;
    
    // Fixed number of circulating fire
    for(int i=0;i<elementof(childids);i++) childids[i] = 0;

    net_auto_sync_move_interval_sec = 1;
    if(isLocal()) realtimeNewEnemySend(this);
    
}

FireCopter::~FireCopter() {
    for(int i=0;i<elementof(childids);i++) {
        Bullet *b = (Bullet*) g_char_layer->findPropById( childids[i] );
        if(b) b->to_clean = true;
    }
}
bool FireCopter::enemyPoll(double dt) {
    float hyperrate = (hyper_until > accum_time) ? 2 : 1;
    
    if( accum_time > turn_at ) {
        turn_at = accum_time + 2;
        Vec2 tgt;
        if( g_fld->findEnemyAttackTarget(loc,&tgt, WORM_SHOOT_DISTANCE ) ) {
            v = loc.to(tgt).normalize(vel * hyperrate );
        }
        checkLocalSyncMoveSend();
    }
        

    // Child fireballs.  Simply sync position by accum_time. 
    for(int i=0;i<elementof(childids);i++) {
        Bullet *b = (Bullet*) g_char_layer->findPropById( childids[i] );
        if(!b) {
            float distance = loc.len(g_pc->loc);
            if( accum_time > gen_child_at && distance < 40*PPC ) {
                gen_child_at = accum_time + 0.3 / hyperrate;
                // The body is destroyed!
                b = new Bullet( BLT_ROTFIRE, loc,loc, DUMMY_CLIENT_ID );
                childids[i] = b->id;
            }
        }

        if(b) {
            float rad = (M_PI*2/(float)elementof(childids));
            Vec2 rv( cos(accum_time+rad*i), sin(accum_time+rad*i) );
        
            float distance = b->accum_time * PPC;
            if( distance > PPC * 3 ) distance = PPC*3;        
            b->loc = loc + rv * distance + Vec2(0,fly_height);
        }
    }
    return true;
}
// This is called on kill locally. Not called when killed by remote.
void FireCopter::onKill() {
    for(int i=0;i<elementof(childids);i++) {
        Bullet *b = (Bullet*) g_char_layer->findPropById( childids[i] );  
        if(b) {
            Vec2 vv = loc.to(b->loc).normalize(PPC*8);
            b->to_clean = true;
            realtimeCharDeleteSend(b);
            new Bullet( BLT_ROTFIRE, b->loc, b->loc + vv );
            childids[i] = 0;
        }
    }
}
/////////////////////////
// Jump by fly_vy
SnowSlime::SnowSlime( Vec2 lc, int client_id, int internal_id ) : Enemy( lc, g_base_deck, FLYING, NOT_SWIMMING, 0, client_id, internal_id ) {
    setIndex( B_ATLAS_SNOWSLIME );
    vel = PPC*4;
    hp = maxhp = 12;
    machine = false;
    setScl(PPC);
    explosion_type = EXT_CREATURE;
    enable_hyper_effect = true;
    turn_at = 0;
    fly_vy = 0;
    ensureShadow();
    shoot_at = 2;
    enemy_type = ET_SNOWSLIME;
    base_score = 700;
    if(isLocal()) realtimeNewEnemySend(this);
}

bool SnowSlime::enemyPoll( double dt ) {
    float hyrate = (accum_time< hyper_until) ? 2 : 1;
    if( accum_time > turn_at ) {
        soundPlayAt( g_slimejump_sound, loc, 3 );
        turn_at = accum_time + 2;
        flying = true;
        fly_vy = PPC*8*hyrate;
        v = Vec2(0,0).randomize(1).normalize(vel*hyrate);
        checkLocalSyncMoveSend();
    }

    if( isLocal() ) {
        if( accum_time > shoot_at ) {
            shoot_at = accum_time + 1.5 / hyrate;
            if( g_fld->countBlock( loc, 2, BT_SNOW ) == 0 ) {
                Vec2 tgt;
                if( g_fld->findEnemyAttackTarget(loc,&tgt,WORM_SHOOT_DISTANCE) ) {
                    if(isLocal()) Bullet::shootAt( BLT_SNOWBALL, loc + Vec2(0,fly_height), tgt );
                    soundPlayAt( g_shootsnow_sound, loc, 1 );
                }
            }        
        }
    }

    // Jumping
    fly_vy -= 400 * dt;
    fly_height += fly_vy * dt;
    if( fly_height < 0 ) {
        fly_height = 0;
        v *= 0;
        flying = false;
    }

    // Just disappear when buried in snow
    Cell *c = g_fld->get(loc);
    if(c&&c->bt == BT_SNOW ) return false;
    

    return true;
}
///////////////////////
Bird::Bird( Vec2 lc, int client_id, int internal_id ) : Enemy( lc, g_base_deck, FLYING, NOT_SWIMMING, PPC, client_id, internal_id ), turn_at(0) {
    hp = maxhp = 8;
    setScl(PPC);
    machine = false;
    ensureShadow();
    enable_hyper_effect = true;
    explosion_type = EXT_CREATURE;
    bt_can_enter = BT_SNOW; // Special ability to go over snow
    target = loc;
    enemy_type = ET_BIRD;
    base_score = 1200;
    if(isLocal()) realtimeNewEnemySend(this);
}

bool Bird::enemyPoll(double dt) {
    setIndex( B_ATLAS_BIRD_BASE + ((int)( accum_time * 10 ) % 2  ) );
    if( accum_time > turn_at ) {
        turn_at = accum_time + 2;
        Vec2 tgt;
        if( g_fld->findEnemyAttackTarget( loc, &tgt, WORM_SHOOT_DISTANCE ) ) {
            Vec2 rel = Vec2(0,0).randomize(1).normalize(PPC*4); 
            target = tgt + rel;
            checkLocalSyncMoveSend();
            if( isLocal() ) {
                Bullet::shootAt( BLT_TORNADO, loc, target );
                soundPlayAt( g_shoottornado_sound, loc, 1 );
            }
        }
    }
    float hyrate = (accum_time < hyper_until ) ? 2 : 1;
    float maxvel = 12*PPC*hyrate;
    Vec2 dv = loc.to(target).normalize(maxvel/2);
    v += dv * dt;
    if(v.len() > maxvel ) v = v.normalize(maxvel);

    if( updateInterval( INTVL_IND_BIRD_SOUND, 0.25) ) {
        soundPlayAt( g_birdfly_sound, loc, 3, false );
     }
    return true;
}

/////////////////////////
Shrimp::Shrimp( Vec2 lc, int client_id, int internal_id ) : Enemy( lc, g_base_deck, NOT_FLYING, SWIMMING, 0, client_id, internal_id ), turn_at(0), start_at(2) {
    hp = maxhp = 5;
    setScl(PPC);
    machine = false;
    enable_hyper_effect = true;
    explosion_type = EXT_CREATURE;
    setIndex( B_ATLAS_SHRIMP );

    setColor( Color(1,1,1,0.3) );

    v = Vec2(0,0).randomize(1);
    enemy_type = ET_SHRIMP;
    base_score = 300;
    if(isLocal()) realtimeNewEnemySend(this);
    
}

bool Shrimp::enemyPoll( double dt ) {
    float hyrate = accum_time < hyper_until ? 2 : 1;

    bool prev_enable_move = enable_move;
    enable_move = (int)(accum_time*5 * hyrate) % 2;

    if( enable_move == false && prev_enable_move == true ) {
        if( loc.len(target) < PPC*4*hyrate*hyrate ) {
            if( isLocal() ) Bullet::shootAt( BLT_SHRIMP_DEBRI, loc, target);
        }

    }
    
    setRot( atan2( v.y, v.x ) );
                                
    if( accum_time < start_at ) {
        return true;
    }
    
    if( accum_time > turn_at ) {
        turn_at = accum_time + 1/hyrate;
        Vec2 tgt;
        if( g_fld->findEnemyAttackTarget(loc, &tgt, SHRIMP_SEEK_DISTANCE ) ) {
            float vel = 6 * PPC * hyrate * range(0.5,1);
            v = loc.to(tgt).normalize(vel);
            target = tgt;
            checkLocalSyncMoveSend();
        }
    }

    // Looks dark in the water
    bool out_of_water = false;
    Cell *cells[4];
    g_fld->getRectCorner(loc, 8, cells );
    for(int i=0;i<4;i++) {
        if( cells[i] && cells[i]->isWater()== false) {
            out_of_water = true;
            break;
        }
    }
    setColor(1,1,1, out_of_water ? 1.0 : 0.4 );
    beam_hits = out_of_water;
    if( out_of_water && timeout > accum_time+20 ) timeout = accum_time+20;
    
    return true;
}
void Shrimp::onKill() {
    if( accum_time < hyper_until ) blast(true);
}
void Shrimp::onTimeout() {
    if( accum_time < hyper_until ) blast(isLocal());
}
void Shrimp::blast( bool is_local ) {
    Vec2 tgt;
    if( g_fld->findEnemyAttackTarget(loc, &tgt, SHRIMP_SEEK_DISTANCE ) ) {
        if( is_local ) Bullet::shootFanAt( BLT_SHRIMP_DEBRI, loc, tgt, 2, M_PI/8.0 ); 
    }
}
    
/////////////////////

BombFlowerSeed::BombFlowerSeed( Vec2 lc, int client_id, int internal_id ) : Flyer( CAT_BOMBFLOWER_SEED, lc, 0, g_base_deck, B_ATLAS_BOMBFLOWER_SEED, client_id, internal_id ) {
    v = Vec2(0,0).randomize(1).normalize( PPC * irange(3,5) );
    v_h = PPC * range(3,4);
    gravity = - PPC*5;
    type = FLT_BOMBFLOWERSEED;
    if(isLocal()) realtimeNewFlyerSend(this);
}
bool BombFlowerSeed::flyerPoll( double dt ) {
    if( h< 0 ) {
        land();
    }
    return true;
}
void BombFlowerSeed::land() {
    to_clean = true;
    Cell *c = g_fld->get(loc);
    if(c && c->isBlockPuttable() ) {
        c->bt = BT_BOMBFLOWER;
        c->content = 0;
        g_fld->notifyChanged(c);
        soundPlayAt( g_cutweed_sound, loc,1 );
        for(int i=0;i<3;i++) createLeafEffect(loc);
    }
}


////////////////////////

Bee::Bee( Vec2 lc, int client_id, int internal_id ) : Enemy( lc, g_base_deck, FLYING, NOT_SWIMMING, PPC, client_id, internal_id ), turn_at(0) {
    hp = maxhp = 1;
    setScl(PPC);
    machine = false;
    ensureShadow();
    enable_hyper_effect = true;
    explosion_type = EXT_CREATURE;
    target = loc;
    fly_height = 0.01;
    timeout = 10;
    enemy_type = ET_BEE;
    base_score = 50;
    if(isLocal()) realtimeNewEnemySend(this);
}

bool Bee::enemyPoll( double dt ) {
    setIndex( B_ATLAS_BEE_BASE + ((int)(accum_time*20) % 2 ));

    if( fly_height < PPC ) fly_height += PPC * dt;
    float hyrate = accum_time < hyper_until ? 2 : 1;

    if( isLocal() ) {
        if( accum_time > turn_at ) {
            turn_at = accum_time + 2 / hyrate;
            Vec2 tgt;
            if( g_fld->findEnemyAttackTarget(loc,&tgt,WORM_SHOOT_DISTANCE) ) {
                target = tgt.randomize(PPC*(2+(id%4)));
            }
            checkLocalSyncMoveSend();
        }
    }
    int mod = client_id ? internal_id : id;
    float maxvel = PPC*(2+(mod%4))*hyrate;
    v += loc.to(target).normalize(maxvel*dt);
    if( v.len() > maxvel ) v = v.normalize(maxvel);

    if( (loc+Vec2(0,fly_height)).len(g_pc->loc) < PPC/2 ) {
        int dmg = 1 * hyrate;
        g_pc->onAttacked(dmg,this);
        return false;
    }
    return true;
}
///////////////////////////
Defender::Defender(Vec2 lc, int client_id, int internal_id ) : Enemy(lc, g_base_deck, FLYING, NOT_SWIMMING, PPC, client_id, internal_id ), turn_at(0) {
    hp = maxhp = 200;
    setScl(PPC*1.5);
    machine = true;
    ensureShadow();
    enable_hyper_effect = true;
    explosion_type = EXT_MACHINE;
    enemy_type = ET_DEFENDER;
    base_score = 2500;
    if(isLocal())realtimeNewEnemySend(this);
}

bool Defender::enemyPoll( double dt ) {
    setIndex( B_ATLAS_DEFENDER_BASE + ((int)(accum_time * 15)%2) );
    
    float hyrate = ( accum_time < hyper_until ) ? 2 : 1;

    if( isLocal() ) {
        if( accum_time > turn_at ) {
            turn_at = accum_time + 2 / hyrate;
            Vec2 tgt;
            if( g_fld->findEnemyAttackTarget(loc,&tgt,MACHINE_SHOOT_DISTANCE) ) {
                v = loc.to(tgt).normalize( PPC * hyrate * range(1,1.5));
            }
            checkLocalSyncMoveSend();
        }
        if( updateInterval( INTVL_IND_DEFENDER_SHOOT, 0.5 / hyrate ) ) {
            Vec2 tgt;
            if( g_fld->findEnemyAttackTarget(loc,&tgt,MACHINE_SHOOT_DISTANCE) ) {
                if(isLocal())Bullet::shootAt( BLT_BOUNCER, loc + Vec2(0,fly_height), tgt );
            }
        }
    }
    return true;
}
///////////////////////////////
Teleporter::Teleporter( Vec2 lc, int client_id, int internal_id ) : Enemy( lc, g_base_deck, FLYING, NOT_SWIMMING, 0.1, client_id, internal_id ) {
    hp = maxhp = 1;
    setScl(PPC);
    setIndex( B_ATLAS_TELEPORTER_BASE);
    machine = true;
    ensureShadow();
    enable_hyper_effect = true;
    explosion_type = EXT_MACHINE;

    start_at = TELEPORTER_FADE_SEC;
    fade_at = start_at + 2;

    enable_move = false;
    Vec2 tgt;
    if( g_fld->findEnemyAttackTarget(lc,&tgt,MACHINE_SHOOT_DISTANCE) ) {
        v = lc.to(tgt).normalize( PPC * 6 );
    }
    shot = false;

    enemy_type = ET_TELEPORTER;
    base_score = 200;
    if(isLocal())realtimeNewEnemySend(this);
}
bool Teleporter::enemyPoll(double dt) {
    if( fly_height < PPC/2 ) fly_height += PPC * dt;

    float hyrate = (accum_time < hyper_until) ? 2 : 1;
    if( hyrate > 1 && v.len() < PPC * 10 ) v = v.normalize(PPC*10);
    if( hyrate > 1 && fade_at > start_at + 1 ) fade_at = start_at + 1; 
    
    if( accum_time < start_at ) {
        float rate = (TELEPORTER_FADE_SEC - (start_at - accum_time)) / TELEPORTER_FADE_SEC;
        setIndex((int)(4 * rate) + B_ATLAS_TELEPORTER_BASE );
        enable_move = false;
    } else if( accum_time > fade_at + TELEPORTER_FADE_SEC ) {
        return false;
    } else if( accum_time > fade_at ) {
        float rate = (TELEPORTER_FADE_SEC - (accum_time - fade_at) ) / TELEPORTER_FADE_SEC;
        setIndex((int)(4 * rate) + B_ATLAS_TELEPORTER_BASE );
        enable_move = false;
        if( fly_height > 0 ) fly_height -= PPC * dt;
        if( shot == false ) {
            shot = true;
            Vec2 tgt;
            if( g_fld->findEnemyAttackTarget(loc,&tgt,MACHINE_SHOOT_DISTANCE) ) {
                if(hyrate > 1) {
                    if(isLocal()) Bullet::shootFanAt( BLT_SPARIO, loc+Vec2(0,fly_height), tgt, 2, M_PI/8.0 );
                } else {
                    if(isLocal()) Bullet::shootAt( BLT_SPARIO, loc+Vec2(0,fly_height), tgt + Vec2(0,0).randomize(PPC) );
                }
                
            }
        }
    } else {
        enable_move = true;
    }
    
    
    return true;
}
void Teleporter::onTouchWall( Vec2 nextloc, int hitbits, bool nxok, bool nyok ) {
    fade_at = accum_time;
}
//////////////////////
CoreBall::CoreBall( Vec2 lc, int client_id, int internal_id ) : Enemy( lc, g_base_deck, FLYING, NOT_SWIMMING, 0.1, client_id, internal_id ), turn_at(0) {
    setIndex( B_ATLAS_COREBALL );
    setScl(48);
    hp = maxhp = 200;
    explosion_type = EXT_MACHINE;
    ensureShadow();
    shoot_start_at = 1.5;
    enemy_type = ET_COREBALL;
    base_score = 100000;
    if( isLocal() ) realtimeNewEnemySend(this);
}

bool CoreBall::enemyPoll( double dt ) {
    float hyrate = (accum_time < hyper_until) ? 2 : 1;
    if( isLocal() ) {
        if( accum_time > turn_at ) {
            target = loc + Vec2(0,0).randomize(1).normalize( PPC*10 );
            v = loc.to(target).normalize( PPC * range(2,4) );
            turn_at = accum_time + range(2,5);
            checkLocalSyncMoveSend();
        }
        bool shoot_mode = (int)(accum_time/3) % 2;
        if(accum_time > shoot_start_at && shoot_mode) {
            if( updateInterval( INTVL_IND_CORE_SHOOT, 0.05 / hyrate ) ) {
                Vec2 tgt;
                if( g_fld->findEnemyAttackTarget( loc,&tgt,MACHINE_SHOOT_DISTANCE) ) {
                    switch( irange(0,3) ) {
                    case 0:
                        Bullet::shootFanAt( BLT_SPARIO, loc, tgt, 1, M_PI/8.0 );
                        break;
                    case 1:
                        Bullet::shootFanAt( BLT_SIG, loc, tgt, 1, M_PI/8.0 );
                        break;
                    case 2:
                        Bullet::shootFanAt( BLT_BOUNCER, loc, tgt, 1, M_PI/8.0 );
                        break;
                    }
                }
            }
        }
    }
    return true;
}
