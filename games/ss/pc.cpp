
#include "moyai/client.h"

#include "dimension.h"
#include "ids.h"
#include "conf.h"
#include "item.h"
#include "net.h"
#include "char.h"
#include "hud.h"
#include "pc.h"
#include "util.h"
#include "field.h"
#include "atlas.h"
#include "enemy.h"
#include "effect.h"
#include "pwgrid.h"
#include "mapview.h"
#include "globals.h"



PC::PC(Vec2 lc) : Char(CAT_PC, lc, g_base_deck, g_char_layer ), ItemOwner(PC_ITEM_NUM, 1), ideal_v(0,0), dir(DIR_DOWN), body_size(8), shoot_v(0,0), selected_item_index(PC_SUIT_NUM), last_charge_at(0), enable_ai(false), warping(false), warp_dir(DIR_NONE), knockback_until(0), knockback_v(0,0), enable_hpbar(false), enable_enebar(false), equip_prop(NULL), died_at(0), total_milestone_cleared(0), shoot_sound_index(0), last_network_update_at(0), energy_chain_heat_count(0), score(0), recovery_count_left(PC_RECOVERY_COUNT_MAX), nktb(NULL), last_insert_history_at(0) {
    tex_epsilon=0;
    priority = PRIO_CHAR;

    hp = maxhp = PC_MAXHP;
    ene = maxene = 0; // updated afterward
    armor = maxarmor = 0;
    for(int i=0;i<elementof(history_loc);i++) history_loc[i] = lc;



    face_prop = new Prop2D();
    face_prop->setScl(24);
    face_prop->setDeck( g_base_deck );
    face_prop->setIndex( face_base_index = B_ATLAS_PC_FACE_FRONT);
    addChild( face_prop );

    body_prop = new Prop2D();
    body_prop->setScl(24);
    body_prop->setDeck( g_base_deck );
    body_prop->setIndex( body_base_index = B_ATLAS_PC_BODY_BASE );
    addChild( body_prop );
    
    hair_prop = new Prop2D();
    hair_prop->setScl(24);
    hair_prop->setDeck( g_base_deck );
    hair_prop->setIndex( hair_base_index = B_ATLAS_PC_HAIR_FRONT );
    addChild(hair_prop);

    equip_prop = new Prop2D();
    equip_prop->setScl(24);
    equip_prop->setDeck( g_base_deck );
    equip_prop->setIndex( B_ATLAS_PC_EQUIPMENT_BEAMGUN_BASE );
    addChild( equip_prop );

    beam_base_index = B_ATLAS_PC_BEAM;

    // no main sprite
    setIndex(-1);

    body_prop->priority = 1;
    face_prop->priority = 2;
    hair_prop->priority = 3;
    equip_prop->priority = 4; // Changes depending on direction of PC

    nickname[0] = '\0'; // Will be overwritten by PARTY protocol
    
}

bool PC::charPoll( double dt ) {

    if( isDBNetworkActive() && g_fld->getChunkLoadState( vec2ToPos2(loc)) != CHUNKLOADSTATE_LOADED ) return true;    
    
    if( pcPoll(dt) == false ) return false;
    
    //
    DIR d = ideal_v.toDir();
    //    print("pc:%f %f ind:%d d:%d", ideal_v.x, ideal_v.y, index, d  );

    if( d != DIR_NONE ) {
        dir = d;
        if( (dir == DIR_RIGHT || dir == DIR_LEFT) && ideal_v.y < 0 ) dir = DIR_DOWN;
    }
    DIR shootd = shoot_v.toDir();
    if( shootd != DIR_NONE ) {
        dir = shootd;
    }


    hair_prop->setVisible(!warping);
    body_prop->setVisible(!warping);
    face_prop->setVisible(!warping);

        
    if( warping ) {
        setIndex( B_ATLAS_WARP_EFFECT_BASE + irange(0,2) );
        setXFlip( irange(0,2) );
        setYFlip( irange(0,2) );
        int dx,dy;
        dirToDXDY( warp_dir, &dx, &dy );
        Vec2 nextloc = loc + Vec2(dx,dy) * PPC;
        Cell *nc = g_fld->get(nextloc);
        if( nc && nc->bt == BT_CABLE ) {
            // OK go
            loc = nextloc;
        } else {
            // Can't go front
            if( nc && nc->bt == BT_PORTAL_ACTIVE ) {
                warping = false;
                loc = g_fld->findSafeLoc(nextloc);
                print("warp end at active portal" );
            } else {
                // See right and left
                bool can_go_right = false, can_go_left = false;
                dirToDXDY( rightDir(warp_dir), &dx, &dy );
                Vec2 rloc = loc + Vec2(dx,dy) * PPC;
                Cell *rc = g_fld->get(rloc);
                if( rc && rc->bt == BT_CABLE ) can_go_right = true;
                dirToDXDY( leftDir(warp_dir), &dx, &dy);
                Vec2 lloc = loc + Vec2(dx,dy) * PPC;
                Cell *lc = g_fld->get(lloc);
                if( lc && lc->bt == BT_CABLE ) can_go_left = true;

                if( can_go_right && can_go_left) {
                    if( range(0,100)<50) {
                        loc = rloc;
                        warp_dir = rightDir(warp_dir);
                    } else {
                        loc = lloc;
                        warp_dir = leftDir(warp_dir);
                    }
                } else if( can_go_left ) {
                    loc = lloc;
                    warp_dir = leftDir(warp_dir);
                } else if( can_go_right ) {
                    loc = rloc;
                    warp_dir = rightDir(warp_dir);
                } else {
                    warping = false;
                    print("warp end on cable end" );
                    loc = g_fld->findSafeLoc(nextloc);
                }
            }
        }
        ene -= WARP_ENE_CONSUME_PER_FRAME;
        if( ene < 0 ) {
            ene = 0;
            warping = false;
        }
        // Equipment is not visible during warping
        equip_prop->setVisible(false);
    } else if( died_at == 0 ) {
        setIndex(-1);
        bool walking = (d != DIR_NONE);
        body_prop->setXFlip( false );
        body_prop->setYFlip( false );
        face_prop->setXFlip( false );
        hair_prop->setXFlip( false );        
        int base_index = body_base_index;
        
        int anim_d = ( (int)(accum_time*12) % 2);
        switch(dir) {
        case DIR_UP:
            if(walking) {
                body_prop->setIndex(base_index + 6 + anim_d);
            } else {
                body_prop->setIndex(base_index+5);
            }
            hair_prop->setIndex( hair_base_index + 2 );
            face_prop->setIndex(-1);
            body_prop->setXFlip(false);
            equip_prop->priority = 0;            
            break;
        case DIR_DOWN:
            if(walking) {
                body_prop->setIndex(base_index+1+anim_d);
            } else {
                body_prop->setIndex(base_index+0);
            }
            hair_prop->setIndex( hair_base_index+0);
            face_prop->setIndex( face_base_index+0);
            body_prop->setXFlip(false);
            equip_prop->priority = 4;
            break;
        case DIR_RIGHT:
            if(walking) {
                body_prop->setIndex(base_index+3+anim_d);
            } else {
                body_prop->setIndex(base_index+3);
            }
            hair_prop->setIndex( hair_base_index+1);
            hair_prop->setXFlip(true);
            face_prop->setIndex( face_base_index+1);
            face_prop->setXFlip(true);            
            body_prop->setXFlip(true);
            equip_prop->priority = 4;            
            break;
        case DIR_LEFT:
            if(walking) {
                body_prop->setIndex(base_index+3+anim_d);
            } else {
                body_prop->setIndex(base_index+3);
            }
            hair_prop->setIndex( hair_base_index+1);
            hair_prop->setXFlip(false);
            face_prop->setIndex( face_base_index+1);
            face_prop->setXFlip(false);                        
            body_prop->setXFlip(false);
            equip_prop->priority = 0;            
            break;
        default:
            assert(false);
        }

        float gtspeedrate = 1;
        {
            Cell *uc = g_fld->get(loc + Vec2(0,body_size));
            Cell *dc = g_fld->get(loc + Vec2(0,-body_size));
            Cell *rc = g_fld->get(loc + Vec2(body_size,0));
            Cell *lc = g_fld->get(loc + Vec2(-body_size,0));
            float ur=1, dr=1,rr=1,lr=1;
            if(uc) ur = uc->getWalkSpeedRate();
            if(dc) dr = dc->getWalkSpeedRate();
            if(rc) rr = rc->getWalkSpeedRate();
            if(lc) lr = lc->getWalkSpeedRate();
            float udmax = (ur > dr) ? ur : dr;
            float lrmax = (rr > lr) ? rr : lr;
            gtspeedrate = (udmax > lrmax) ? udmax : lrmax;

            if( ideal_v.len() > 0 && updateInterval( INTVL_IND_PC_FOOTSMOKE, 0.25 ) ) {
                if( gtspeedrate < 1 ) {
                    Cell *c = g_fld->get(loc);
                    if( c->gt == GT_JUNGLE ) {
                        int n = irange(1,4);
                        for(int i=0;i<n;i++) createLeafEffect(loc + Vec2( range(-PPC/2,PPC/2),-PPC/2) );
                    }  else {
                        createSmokeEffect(loc + Vec2(0,-body_size));
                    }
                    soundPlayAt(g_sandwalk_sound,loc,1);
                } else if( (uc&&uc->gt == GT_BRICK_PANEL)||
                           (dc&&dc->gt == GT_BRICK_PANEL)||
                           (rc&&rc->gt == GT_BRICK_PANEL)||
                           (lc&&lc->gt == GT_BRICK_PANEL) ) {
                    soundPlayAt(g_steponbrick_sound,loc,1);
                }
            }
        }

        
        //
        float vel = PC_MAX_WALK_SPEED * gtspeedrate * g_pc_walk_speed_accel;
        Vec2 nextloc;
        if( accum_time < knockback_until ) {
            nextloc = loc + knockback_v * dt;
        } else {
            nextloc = loc + ideal_v * dt * vel;
        }
        Vec2 prevloc = loc;
        loc = calcNextLoc( loc, nextloc, body_size, false, NOT_FLYING, NOT_SWIMMING, BT_AIR );

        // To avoid stuck in blocks or water
        if( accum_time > last_insert_history_at + 1 ) {
            last_insert_history_at = accum_time;
            insertHistory(loc);
        }
        if( loc == prevloc ) {
            if( ideal_v.len() > 0 ) {
                // Try to touch wall (Assume ideal_v has length of 1)
                Vec2 finalepsloc = loc;
                for(int i=1;i<PPC/2;i++) {
                    Vec2 epsnextloc = loc + ideal_v.normalize(1*i);                    
                    Vec2 epsloc = calcNextLoc( loc, epsnextloc, body_size, false, NOT_FLYING, NOT_SWIMMING, BT_AIR );
                    if( epsloc != loc ) {
                        finalepsloc = epsloc;
                    }
                }
                if( finalepsloc != loc ) {
                    // OK could go!
                    loc = finalepsloc;                    
                } else {
                    // Try opposit direction
                    Vec2 revnextloc = loc + ideal_v * dt * vel * -1;
                    Vec2 revloc = calcNextLoc( loc, revnextloc, body_size, false, NOT_FLYING, NOT_SWIMMING, BT_AIR );
                    if( revloc == loc ) {
                        // Can't go to opposite direction. Stucked!

                        // Try center position of the nearest cell.
                        Vec2 centerloc = toCellCenter(loc);
                        Vec2 centerloctest = calcNextLoc( loc, centerloc, body_size, false, NOT_FLYING, NOT_SWIMMING, BT_AIR );
                        if( centerloctest != loc ) {
                            loc = centerloctest;
                        } else {
                            // Then still try 4 directions...
                            Vec2 cands[4] = {
                                Vec2(centerloc+Vec2(PPC,0)),
                                Vec2(centerloc+Vec2(0,PPC)),
                                Vec2(centerloc+Vec2(-PPC,0)),
                                Vec2(centerloc+Vec2(0,-PPC))
                            };
                            bool ok = false;
                            for(int i=0;i<4;i++) {
                                Vec2 totest = cands[i];
                                Vec2 testresult = calcNextLoc( loc, totest, body_size, false, NOT_FLYING, NOT_SWIMMING, BT_AIR );
                                if( testresult != loc ) {
                                    loc = testresult;
                                    ok = true;
                                    break;
                                }
                            }

                            if(!ok) {
                                print("detected bury..");
                                loc = getFromHistory();
                            }
                        }
                    }
                }
            }
        }
        

        // 
        if( accum_time > last_charge_at + PC_CHARGE_INTERVAL && isLocal() ) {
            last_charge_at = accum_time;

            // Try to recharge if possible
            for(int y=-2;y<=2;y++) {
                for(int x=-2;x<=2;x++) {
                    Pos2 p( (int)(loc.x/PPC) + x, (int)(loc.y/PPC) + y );
                    Cell *c = g_fld->get(p);
                    if(!c)continue;
            
                    switch(c->bt){
                    case BT_MOTHERSHIP:
                        if( ene < maxene ) {
                            charge(MOTHERSHIP_CHARGE_UNIT);
                        }
                        break;
                    case BT_REACTOR_ACTIVE:
                        if( c->content > 0 ) {
                            int room = maxene - ene;
                            if(room>0) {
                                int to_move = room;
                                if( c->content < to_move ) to_move = c->content;
                                charge(to_move);
                                c->content -= to_move;
                            }
                        }
                    default:
                        break;
                    }
                }
            }
        }
        {
            Cell *c = g_fld->get(loc);
            if( c && c->isTooHot() ) {
                onTooHot();
            }
            if( c && c->gt == GT_TELEPORTER_INACTIVE ) {
                c->gt = GT_TELEPORTER_ACTIVE;
                g_fld->notifyChanged(c);
            }
            if( c && c->powergrid_id != 0 ) {
                PowerGrid *pg = g_ps->findGridById(c->powergrid_id);
                if(pg) {
                    Pos2 p(loc.x/PPC,loc.y/PPC);
                    for(int y=-2;y<=2;y++) {
                        for(int x=-2;x<=2;x++) {
                            Pos2 polep = p + Pos2(x,y);
                            Cell *nodec = g_fld->get(polep);
                            if(nodec && nodec->bt == BT_POLE ) {
                                //                                print("pc: nodegid:%d cellgid:%d", nodec->powergrid_id, c->powergrid_id );
                                if( nodec->powergrid_id == c->powergrid_id ) {
                                    g_powergrid_state->setLoc(toCellCenter(polep) + Vec2(-24,24) );
                                    g_powergrid_state->setVisible(true);
                                    Format fmt( "ID:%d ENE: %d", pg->id, pg->ene );
                                    g_powergrid_state->setString(WHITE,fmt.buf);
                                } 
                            }
                        }
                    }
                }
            } else {
                g_powergrid_state->setVisible(false);
            }
        }

        // Draw equipment
        ItemConf *itc = items[selected_item_index].conf;
        if(itc && itc->is_beamgun ) {
            int eqind = 0;
            bool xfl = false;
            Vec2 eqv = calcEquipPosition(dir);
            
            switch(dir) {
            case DIR_DOWN:
                eqind = B_ATLAS_PC_EQUIPMENT_BEAMGUN_BASE;
                render_children_first = false;
                break;
            case DIR_UP:
                eqind = B_ATLAS_PC_EQUIPMENT_BEAMGUN_BASE+2;
                render_children_first = true;
                break;
            case DIR_RIGHT:
                eqind = B_ATLAS_PC_EQUIPMENT_BEAMGUN_BASE+1;
                render_children_first = false;
                break;
            case DIR_LEFT:
                eqind = B_ATLAS_PC_EQUIPMENT_BEAMGUN_BASE+1;
                xfl = true;
                render_children_first = true;
                break;
            
            default:break;
            }
            equip_prop->setXFlip( xfl );
            equip_prop->setIndex( eqind );        
            equip_prop->setVisible(true);
            equip_prop->setLoc( loc + eqv );
        } else {
            equip_prop->setVisible(false);
        }
        body_prop->setUVRot(false);
        face_prop->setUVRot(false);
        hair_prop->setUVRot(false);
        body_prop->setColor( Color(1,1,1,1));
        face_prop->setColor( Color(1,1,1,1));
        hair_prop->setColor( Color(1,1,1,1));
    } else if( accum_time > died_at && accum_time < died_at + 3 ) {
        body_prop->setIndex( body_base_index+3);
        face_prop->setIndex( face_base_index+1);
        hair_prop->setIndex( hair_base_index+1);
        body_prop->setUVRot(true);
        face_prop->setUVRot(true);
        hair_prop->setUVRot(true);
        equip_prop->setVisible(false);
        body_prop->setColor( Color(1,0.4,0.4,1) );
        face_prop->setColor( Color(1,0.4,0.4,1) );
        hair_prop->setColor( Color(1,0.4,0.4,1) );
    } else {
        respawn();
    }

    // Edge of the world
    float mgn = body_size * 1.3;
    if( loc.x < mgn ) loc.x = mgn;
    if( loc.x > g_fld->loc_max.x - mgn ) loc.x = g_fld->loc_max.x - mgn;
    if( loc.y < mgn ) loc.y = mgn;
    if( loc.y > g_fld->loc_max.y - mgn ) loc.y = g_fld->loc_max.y - mgn;

    // Auto HP recover
    if( hp <= maxhp - PC_MAXHP/2 ) {
        ITEMTYPE use_item = ITT_ERROR;
        if( countItem( ITT_APPLE ) >= 1 ) {
            use_item = ITT_APPLE;
        } else if( countItem( ITT_HP_POTION ) >= 1 ) {
            use_item = ITT_HP_POTION;
        }
        if( use_item != ITT_ERROR ) {
            ItemConf *itc = ItemConf::getItemConf(use_item);
            recoverHP( itc->hp_recover_amount );
            decItem( use_item, 1 );
        }
    }

    // bar
    if( enable_hpbar ) {
        DurBar *db = DurBar::ensure(this, hp, maxhp, g_durbar_deck );
        db->draw_offset.y = 8;
        db->setVisible( hp < maxhp  );
    }
    if( enable_enebar ) {
        DurBar *db = DurBar::ensure(this, ene, maxene, g_enebar_deck );
        db->draw_offset.y = 0;
        db->setVisible( ene < maxene );
    }

    if( client_id > 0 && last_network_update_at < accum_time - 2 ) {
        return false;
    }

    if( !g_enable_autoplay) {
        if( energy_chain_heat_count > 0 && updateInterval( INTVL_IND_PC_ENEHEAT, 1.0f ) ) {
            energy_chain_heat_count --;
            if( energy_chain_heat_count > ENE_CHAIN_FOLLOW_THRES ) {
                if( nickname[0] != '\0' && isWindowVisible() == false ) {
                    if( isFollowing( user_id ) == false ) { 
                        g_followwin->update( user_id, nickname, face_base_index, hair_base_index );
                        g_followwin->show();
                        energy_chain_heat_count = 0;
                        g_windowopen_sound->play();
                    }
                }
            }
        }
    }
    
    // Show player name
    if( this != g_pc && loc.len(g_pc->loc) < 6 * PPC && score>0) {
        if( !nktb ) {
            nktb = new CharGridTextBox(16);
            nktb->setScl(6,6);
            nktb->setString(WHITE,"");
            g_effect_layer->insertProp(nktb);
        }
        if( updateInterval( INTVL_IND_PC_SHOW_NAME, 2.0f ) ) { 
            char numstr[8];
            shorterInteger(numstr,sizeof(numstr),score);
            Format fmt("%s(%s)", nickname, numstr );
            nktb->setString(WHITE,fmt.buf);
        }
        nktb->setLoc(loc + Vec2(-12,16));
    } else {
        if( nktb ) {
            nktb->to_clean = true;
            nktb = NULL;
        }
    }

    // Adjust position
    body_prop->loc = loc;
    face_prop->loc = loc;
    hair_prop->loc = loc;


    
    return true;
}


Vec2 PC::calcEquipPosition( DIR d ) {
    switch(d) {
    case DIR_DOWN: return Vec2(-5,-7);
    case DIR_UP: return Vec2(6,-4); 
    case DIR_RIGHT: return Vec2(6,-6);
    case DIR_LEFT: return Vec2(-6,-6);
    case DIR_NONE: return Vec2(0,0);
    default: assert(false);
    }
    return Vec2(0,0);
}

void PC::refreshStatus() {
    // recalc battery capacity
    calcEnhancement( &maxene, &armor, &maxarmor );
    if(ene>maxene) ene = maxene;
    onStatusUpdated();
}

bool PC::findAcceleratorInSuit() {
    for(int i=0;i<PC_SUIT_NUM;i++) {
        if( items[i].itt == ITT_ACCELERATOR && items[i].dur > 0 ) return true;
    }
    return false;
}
void PC::calcEnhancement( int *battery_capacity, int *cur_armor, int *max_armor ) {
    int bat = 0, arm = 0, marm = 0;
    for(int i=0;i<PC_SUIT_NUM;i++) {
        if( items[i].itt == ITT_BATTERY1) {
            bat += ENE_PER_BATTERY_LV1 * items[i].num;
        } else if( items[i].itt == ITT_BATTERY2) {
            bat += ENE_PER_BATTERY_LV2 * items[i].num;
        } else if( items[i].itt == ITT_SHIELD ) {
            arm += items[i].dur;
            marm += items[i].conf->dur;
        }
    }
    *battery_capacity = bat;
    *cur_armor = arm;
    *max_armor = marm;
}


bool PC::addItemByDebri( Debri *d ) {
    if( d->index == B_ATLAS_HP_PARTICLE ) {
        recoverHP(HEAL_BLASTER_HP_UNIT);
        return true;
    }
 
    ItemConf *itc = ItemConf::getByIndex( d->index );
    if(itc) {
        if( itc->itt == ITT_APPLE && hp < maxhp ) {
            recoverHP(APPLE_HP_UNIT);
            return true;
        }
        if( itc->itt == ITT_ENERGY_PARTICE ) {
            if( ene < maxene ) {
                charge( ENERGY_PARTICLE_UNIT );
                return true;
            } else {
                return false;
            }
        }
        if( incItem( itc->itt, 1, PC_SUIT_NUM, false ) ) {
            return true;
        }
    }
    return false;
}


void PC::addInitialItems() {
    g_pc->incItemIndex( 0, ITT_BATTERY1, 1 );
    //    g_pc->incItemIndex( 1, ITT_BATTERY2, 1 );    
    //   g_pc->incItemIndex( PC_ITEM_NUM - 3, ITT_BEAMGUN1, 1 );    
    g_pc->incItem( ITT_BEAMGUN1, 1, PC_SUIT_NUM, false );

    //    g_pc->incItem( ITT_BLASTER, 20, PC_SUIT_NUM, false );
    //    g_pc->incItem( ITT_HEAL_BLASTER, 20, PC_SUIT_NUM, false );    

    //    g_pc->incItem( ITT_IRON_PLATE, 50, PC_SUIT_NUM, false );    
    //    g_pc->incItem( ITT_RAREMETAL_CRYSTAL, 50, PC_SUIT_NUM, false );
    //    g_pc->incItem( ITT_MICROBE, 50, PC_SUIT_NUM, false );

    //    ResearchState::unlock( ITT_BRICK_PANEL );
    g_pc->incItem( ITT_DEBRI_SOIL, 30, PC_SUIT_NUM, false );        
    
    //g_pc->incItem( ITT_HP_POTION, 2, PC_SUIT_NUM, false );
    //    g_pc->incItem( ITT_APPLE, 5, PC_SUIT_NUM, false );

    
}

void PC::recoverHP( int amount ) {
    createPositiveEffect( loc, 5 );
    soundPlayAt( g_drink_sound, loc,1 );
    hp += amount;
    if(hp>maxhp) hp=maxhp;
    onStatusUpdated();
}

void PC::moveSelection( DIR dir ) {
    if(dir == DIR_DOWN ) { 
        selected_item_index ++;
        if( selected_item_index >= (PC_SUIT_NUM+PC_BELT_NUM) ) selected_item_index = PC_SUIT_NUM;        
    } else if(dir == DIR_UP) {
        selected_item_index --;
        if( selected_item_index < PC_SUIT_NUM ) selected_item_index = PC_SUIT_NUM+PC_BELT_NUM-1;
    }

    if(g_belt) {
        g_cursormove_sound->play();
        g_belt->update();
    }
}

bool PC::isDiggableSelected() {
    ItemConf *itc = items[ selected_item_index ].conf;
    if( itc && itc->itt == ITT_SHOVEL ) return true;
    return false;
}
bool PC::isPuttableSelected() {
    ItemConf *itc = items[ selected_item_index ].conf;
    if( itc && itc->puttable ) {
        return true;
    }
    return false;
}
bool PC::isItemSelected( ITEMTYPE itt ) {
    ItemConf *itc = items[ selected_item_index ].conf;
    return( itc && itc->itt == itt );
}


void PC::onAttacked( int dmg, Enemy *e ) {
    if(e) {
        knockback_until = accum_time + 0.2;
        knockback_v = e->loc.to(loc).normalize(PPC*4);
    }

    int hp_to_dec = 0;
    int dur_to_dec = 0;
    if( armor >= dmg ) {
        //        armor -= dmg;
        dur_to_dec = dmg;
        createShieldSpark(loc);
        soundPlayAt( g_shield_sound, loc,1);        
    } else {
        hp_to_dec = dmg - armor;
        dur_to_dec = dmg - armor;

        createBloodEffect( loc, dmg );
        soundPlayAt( g_pcdamage_sound, loc,1);        
    }
    wearSuitItem( ITT_SHIELD, dur_to_dec );
    
    hp -= hp_to_dec;
    if( hp <= 0 ) {
        hp = 0;
        if( died_at == 0 ) died_at = accum_time;
    }
    onStatusUpdated();
}

void PC::suicide() {
    hp = 0;
    died_at = accum_time;
    onStatusUpdated();
}
void PC::respawn() {
    Pos2 p = g_fld->getRespawnPoint();
    loc = Vec2( p.x*PPC,p.y*PPC);
    clearHistory(loc);
    hp = maxhp;
    died_at = 0;
    hudKilledMessage(nickname, true);
}

// Returns how much energy is charged
int PC::charge( int e ) {
    int room = maxene - ene;
    if( e > 0 && room > 0 ) {
        int prevene = ene;
        createChargeEffect(loc,1 + e / 100 );
        ene += e;
        if(isLocal()) wearBattery(); // Remote characters are not wearing batteries.
        if( ene > maxene ) ene = maxene;
        soundPlayAt( g_drink_sound, loc,1 );
        onStatusUpdated();
        return ene-prevene;
    } else {
        return 0;
    }
}


bool PC::hasEnough( ITEMTYPE itt, int num ) {
    ItemConf *cf = ItemConf::getItemConf(itt);
    assert(cf);
    int tot=0;
    for(int i=0;i<item_num;i++) {
        if( items[i].itt == itt && items[i].num > 0) {
            tot += items[i].num;
            if( tot >= num ) return true;
        }
    }
    return false;
}





// Returns true if possible.
// possible_num : How many items can be crafted using current inventory items.
bool PC::isCraftable( ItemConf *prodconf, int *possible_num ) {
    *possible_num = 0;

    if( ResearchState::itemIsLocked(prodconf->itt) ) return false;
    if( prodconf->required_ene > ene ) return false;

    // バッテリを必要とする場合、使い果たさないようにしないとハマる
    

    // 持ち物がいっぱいでつくれないときでも、あと何個作れるかは計算する
    bool done = false;
    int pn=1;
    for(;;pn++){
        for(int i=0;i<elementof(prodconf->ingredients);i++ ){
            Ingredient *ing = & prodconf->ingredients[i];
            if( ing->itt > ITT_ERROR && hasEnough( ing->itt, ing->n * pn ) == false ) {
                done = true;
                pn --;  
                break;
            }
        }
        if(done)break;
    }
    *possible_num = pn;

    if( pn == 0 ) {
        return false;
    } else if( pn > 0 ) { 
        if( incItem(prodconf->itt, 1, PC_SUIT_NUM, true ) == false ) {
            return false;
        }
    }
    return true;
}


bool PC::craft( ItemConf *prodconf) {
    assert(prodconf);
    int possible_num;
    if( isCraftable(prodconf, &possible_num) == false ) return false;
    for(int i=0;i<elementof(prodconf->ingredients);i++ ){
        Ingredient *ing = & prodconf->ingredients[i];
        if( ing->itt > ITT_ERROR ) {
            bool ret = decItem( ing->itt, ing->n );
            assert(ret);
        }
    }
    ene -= prodconf->required_ene;
    onStatusUpdated();
    soundPlayAt( g_craft_sound, loc,1 );
    return incItem( prodconf->itt, 1, PC_SUIT_NUM, false );
}

void PC::startWarp( Pos2 portalp, DIR to_dir ) {
    warping = true;
    int dx,dy;
    dirToDXDY(to_dir, &dx, &dy );
    Pos2 from = portalp + Pos2(dx,dy);
    setLoc( toCellCenter( Vec2(from.x*PPC, from.y*PPC ) ) );
    warp_dir = to_dir;

}
PC *PC::getNearestPC( Vec2 from ) {
    Char *ch = Char::getNearestByCategory( from, CAT_PC, NULL );
    return (PC*)ch;
}

void PC::wearSuitItem( ITEMTYPE itt, int amount ) {
    for(int i=0;i<PC_SUIT_NUM;i++) {
        if( items[i].itt == itt ) {
            if( items[i].dur > amount ) {
                items[i].dur -= amount;
            } else {
                amount -= items[i].dur;
                items[i].dur = 0;
                items[i].setEmpty();
            }
        }
    }
    refreshStatus();
}
void PC::wearBattery() {
    for(int i=0;i<PC_SUIT_NUM;i++) {
        if( items[i].itt == ITT_BATTERY1 || items[i].itt == ITT_BATTERY2 ) {
            items[i].dur --;

            if(items[i].dur == 0 ) {
                items[i].setEmpty();
            }
        }
    }
    refreshStatus();    
}


void PC::clearHistory(Vec2 at) {
    for(int i=0;i<elementof(history_loc);i++){
        history_loc[i] = at + Vec2(0,0).randomize(10*PPC);
    }        
}
void PC::insertHistory( Vec2 lc ) {
    if( lc == history_loc[0] ) return;
    for(int i=elementof(history_loc)-1;i>=1;i--){
        history_loc[i] = history_loc[i-1];
    }
    history_loc[0] = lc;
}

Vec2 PC::getFromHistory() {
    for(int i=0;i<elementof(history_loc);i++) {
        Vec2 at = toCellCenter( history_loc[i]);
        Cell *c = g_fld->get(at);
        if(c && c->isEnterable(false,false,false,BT_AIR) ) {
            return at;
        }
    }
    g_log->printf( WHITE, "STUCK.. BACK TO MOTHERSHIP!" );
    return toCellCenter( g_fld->getRespawnPoint() );
}

float PC::getShootIntervalSec() {
    if( findAcceleratorInSuit() ) {
        return PC_SHOOT_INTERVAL_ACCELERATED_SEC;
    } else {
        return PC_SHOOT_INTERVAL_NORMAL_SEC;
    }
}

void PC::dump( PCDump *out ) {
    out->shoot_sound_index = shoot_sound_index;
    out->hair_base_index = hair_base_index;
    out->face_base_index = face_base_index;
    out->body_base_index = body_base_index;
    out->beam_base_index = beam_base_index;
    out->total_milestone_cleared = total_milestone_cleared;
    for(int i=0;i<PC_ITEM_NUM;i++) {
        if( items[i].isEmpty() ) {
            out->items[i].itt = 0;
            out->items[i].num = 0;
            out->items[i].dur = 0;
        } else {
            out->items[i].itt = items[i].itt;
            out->items[i].num = items[i].num;
            out->items[i].dur = items[i].dur;
        }
    }
    out->hp = hp;
    out->ene = ene;
    assert( elementof(nickname) == elementof(out->nickname) );
    for(int i=0;i<elementof(nickname);i++ ) out->nickname[i] = nickname[i];
    out->score = score;
    out->recovery_count_left = recovery_count_left;
    out->last_recovery_at_sec = last_recovery_at_sec;
    print("pc::dump: ssi:%d hbi:%d fbi:%d bbi:%d beambi:%d totms:%d nick:'%s'",
          shoot_sound_index, hair_base_index, face_base_index, body_base_index, beam_base_index, total_milestone_cleared, nickname );    
}
void PC::applyPCDump( PCDump *in ) {
    shoot_sound_index = in->shoot_sound_index;
    hair_base_index = in->hair_base_index;
    face_base_index = in->face_base_index;
    body_base_index = in->body_base_index;
    beam_base_index = in->beam_base_index;
    total_milestone_cleared = in->total_milestone_cleared;

    for(int i=0;i<PC_ITEM_NUM;i++) {
        if( in->items[i].itt == 0 ) {
            items[i].setEmpty();
        }else {
            //            print("applyPCDump: item[%d] t:%d n:%d", i, in->items[i].itt, in->items[i].num );
            items[i].set( (ITEMTYPE) in->items[i].itt, in->items[i].num, in->items[i].dur );
        }
    }

    hp = in->hp;
    ene = in->ene;
    score = in->score;
    recovery_count_left = in->recovery_count_left;
    last_recovery_at_sec = in->last_recovery_at_sec;
    for(int i=0;i<elementof(nickname);i++ ) nickname[i] = in->nickname[i];
    onStatusUpdated();
    onItemUpdated(-1);
    print("applyPCDump: ssi:%d hbi:%d fbi:%d bbi:%d beambi:%d totms:%d nick:'%s' score:%lld",
          shoot_sound_index, hair_base_index, face_base_index, body_base_index, beam_base_index, total_milestone_cleared, nickname, score );
}

void PC::setNickname( const char *s ) {
    snprintf( nickname, sizeof(nickname), "%s", s );
}

void PC::onDelete() {
    if(nktb) {nktb->to_clean = true; nktb = NULL; }
    DurBar::clear(this);
    
}

// Check if items are swappable index i0 and i1.
// You can only put equipables in Suit.
bool PC::isSwappable( int i0, int i1 ) {
    if( i0 < PC_SUIT_NUM ) {
        print("i0 to suit");
        if( items[i1].isEmpty() ) return true;
        return items[i1].conf->is_passive_mod;
    } else if( i1 < PC_SUIT_NUM ) {
        print("i1 to suit");        
        if( items[i0].isEmpty() ) return true;
        return items[i0].conf->is_passive_mod;
    } else {
        return true;
    }
}

void PC::abandonItem( int ind ) {
    assert(ind >=0 && ind < item_num );
    if( items[ind].isEmpty() == false ) {
        items[ind].setEmpty();
    }
    onItemUpdated(-1);
}
int PC::countBeamguns() {
    return countItem(ITT_BEAMGUN1) + countItem(ITT_BEAMGUN2) + countItem(ITT_BEAMGUN3) + countItem(ITT_BEAMGUN4);
}

//////////////////////////////

LocalPC::LocalPC( Vec2 lc ) : PC(lc), can_put_block_as_wall(true), can_put_block_to_remove_water(true) {
    leaving_tb = new CharGridTextBox(20);
    leaving_tb->setScl(16);
    leaving_tb->setLoc(-100,30);
    leaving_tb->setString(WHITE, "NOT SET");
    leaving_tb->setVisible(false);
    g_hud_layer->insertProp(leaving_tb);
}

void LocalPC::onItemUpdated( int ind ) {
    refreshStatus();        
    
    if(g_belt) {
        g_belt->update();
        if( ind >= PC_BELT_NUM && ind < PC_BELT_NUM+PC_SUIT_NUM ) g_belt->spikeEffect(ind-PC_SUIT_NUM);
    }
    if(g_suit) {
        g_suit->update();
    }
    if(g_invwin) {
        g_invwin->update();
        if( ind >= 0 ) {
            g_invwin->spikeEffect(ind);
        }
    }
    if(g_exwin ) {
        g_exwin->update();
    }
}
void LocalPC::onStatusUpdated() {
    hudUpdatePCStatus();
    hudUpdateItems();
}
void LocalPC::onTouchWall( Vec2 nextloc, int hitbits, bool nxok, bool nyok ) {
    //    print("onTouchWall loc:%f,%f nextloc:%f,%f",loc.x/PPC,loc.y/PPC, nextloc.x/PPC, nextloc.y/PPC );
    float hitsz = PPC;
    Cell *rtc = g_fld->get( loc + Vec2(hitsz,hitsz) );
    Cell *ltc = g_fld->get( loc + Vec2(-hitsz,hitsz) );
    Cell *rbc = g_fld->get( loc + Vec2(hitsz,-hitsz) );
    Cell *lbc = g_fld->get( loc + Vec2(-hitsz,-hitsz) );
    Cell *cc = g_fld->get( loc + ideal_v * PPC );

    Cell *tgtc = NULL;
    if( cc && cc->isGUIEnabler() ) {
        tgtc = cc;
    }
    if( (hitbits & Field::WALKABLE_BIT_HIT_RT) && rtc && rtc->isGUIEnabler() ) {
        tgtc = rtc;
    }
    if( (hitbits & Field::WALKABLE_BIT_HIT_LT) && ltc && ltc->isGUIEnabler() ) {
        tgtc = ltc;
    }
    if( (hitbits & Field::WALKABLE_BIT_HIT_RB) && rbc && rbc->isGUIEnabler() ) {
        tgtc = rbc;
    }
    if( (hitbits & Field::WALKABLE_BIT_HIT_LB) && lbc && lbc->isGUIEnabler() ) {
        tgtc = lbc;
    }
    
    if( tgtc && isWindowVisible() == false && last_touch_wall_at != loc && knockback_until < accum_time ) {
        last_touch_wall_at = loc;
        switch( tgtc->bt ) {
        case BT_MOTHERSHIP:
            // Give battery1 and beamgun1 for free for avoiding complete stuck
            {
                bool show_lab = true;
                if( last_recovery_at_sec < ((unsigned int)now()-24*60*60) &&
                    recovery_count_left < PC_RECOVERY_COUNT_MAX ) recovery_count_left++;
                bool show_warn = false;
                if( countItem(ITT_BATTERY1)==0) {
                    if( recovery_count_left > 0 ) { 
                        hudShowConfirmRecoveryMessage( ITT_BATTERY1 );
                    } else {
                        show_warn = true;
                    }
                    show_lab = false;
                } else if( countBeamguns()==0) {
                    if( recovery_count_left > 0 ) { 
                        hudShowConfirmRecoveryMessage( ITT_BEAMGUN1 );
                    } else {
                        show_warn = true;
                    }
                    show_lab = false;
                }
                if( show_lab ) {
                    toggleWindow( "lab" );
                } else if( show_warn ) {
                    hudShowWarningMessage( "NO RECOVERY IN NEXT 24 HOURS" );
                }
            }
            
            break;
        case BT_EXCHANGE_ACTIVE:
        case BT_EXCHANGE_INACTIVE:            
            toggleWindow( "exchange" );
            break;
        case BT_PORTAL_ACTIVE:
        case BT_PORTAL_INACTIVE:
            toggleWindow( "portal" );
            break;
        default:
            break;
        }
    }

}

void LocalPC::onPushed( int dmg, Enemy *e ) {
    onAttacked(dmg,e);
}
void LocalPC::onTooHot() {
    if( updateInterval( INTVL_IND_PC_TOOHOT, 0.15f ) ) {
        onAttacked( LAVA_DAMAGE, NULL );
    }
}



bool LocalPC::pcPoll( double dt ) {
    ChunkStat *cs = g_fld->getChunkStat( Pos2( loc.x/PPC, loc.y/PPC) );
    if(cs && cs->isTundra() ) {
        int maxn = 2 / g_zoom_rate;
        int n = irange(1,maxn*2);
        for(int i=0;i<n;i++) {
            Vec2 diav( range(-SCRW,SCRW), range(-SCRH,SCRH) );
            diav /= g_zoom_rate;
        
            createSnowFlake( loc +diav );
        }
    }

    if( ( hp < maxhp * 0.3 ) && updateInterval( INTVL_IND_PC_ALARM_LOWHP, 1.0f ) && died_at == 0) {
        soundPlayAt(g_lowhp_sound,loc,1);
    }

    if( isRealtimeNetworkActive() ) {
        realtimeSyncPCSend(this);
    }

    if( updateInterval( INTVL_IND_PC_CHARGE_PWGRID, 0.2f ) ) { 
        Cell *c = g_fld->get(loc);
        if( c && c->powergrid_id>0) {
            PowerGrid *g = g_ps->findGridById(c->powergrid_id);
            //                        print("pgid:%d", c->powergrid_id );
            if(g) {
                int to_get = maxene - ene;
                if( g->ene < to_get ) to_get = g->ene;
                g->use(to_get);
                charge(to_get);
            } 
        }
    }
    if( warping && updateInterval( INTVL_IND_PC_WARP_SOUND, 1.0f ) ) {
        soundPlayAt(g_warp_sound,loc,1);
    }
    return true;
}
bool LocalPC::tryAction( Vec2 direction ) {
    assert( selected_item_index >= PC_SUIT_NUM && selected_item_index <= PC_SUIT_NUM+3 );

    if( isDBNetworkActive() && g_fld->getChunkLoadState( vec2ToPos2(loc)) != CHUNKLOADSTATE_LOADED ) return false;
    
    if( knockback_until > accum_time ) return false;
    if( died_at > 0 ) return false;
    if( items[selected_item_index].isEmpty() ) return false;
    ItemConf *itc = items[selected_item_index].conf;
    if(!itc) return false;

    Vec2 d = direction;
    if( d.x < -0.5 ) d.x = -1; else if( d.x > 0.5 ) d.x = 1; else d.x = 0;
    if( d.y < -0.5 ) d.y = -1; else if( d.y > 0.5 ) d.y = 1; else d.y = 0;
            
    Vec2 lc = toCellCenter(loc + d * BLOCK_PUT_DISTANCE);
    Cell *c = g_fld->get(lc);

    
    if(itc->puttable ) {
        if( lc.len( toCellCenter(g_fld->getRespawnPoint())) < 4*PPC) {
            g_log->printf( WHITE, "CANNOT PUT ANYTHING NEARBY RESPAWN POINT" );
            return false;
        }
        if(c && c->isBlockPuttable() ) {
            bool put_block = false, removed_water = false;
            switch(itc->itt) {
            case ITT_DEBRI_SOIL:
            case ITT_DEBRI_ROCK:
            case ITT_DEBRI_HARDROCK:
                if( c->st == ST_NONE && c->gt == GT_ROCK && itc->itt == ITT_DEBRI_SOIL ) {
                    // To fix burned ground
                    c->gt = GT_SOIL;
                    last_put_debri_at = toCellCenter(lc);
                } else if( c->st == ST_NONE ) {
                    // Put as a block
                    if( can_put_block_as_wall && toCellCenter(lc) != last_put_debri_at ) {
                        if(itc->itt == ITT_DEBRI_SOIL ) c->bt = BT_SOIL;
                        if(itc->itt == ITT_DEBRI_ROCK ) c->bt = BT_ROCK;
                        if(itc->itt == ITT_DEBRI_HARDROCK ) c->bt = BT_HARDROCK;
                        put_block = true;
                        last_put_debri_at = toCellCenter(lc);
                    } else {
                        return false;
                    }
                } else if( c->isWater() ) {
                    // Fill up a water.
                    if( can_put_block_to_remove_water && toCellCenter(lc) != last_put_debri_at ) {
                        c->st = ST_NONE;                    
                        if( itc->itt == ITT_DEBRI_SOIL ) {
                            c->gt = GT_SOIL;
                        } else if( itc->itt == ITT_DEBRI_ROCK ) {
                            c->gt = GT_ROCK;
                        } else if( itc->itt == ITT_DEBRI_HARDROCK ) {
                            c->gt = GT_ROCK;
                        }
                        last_put_debri_at = toCellCenter(lc);                        
                        removed_water = true;
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }

                if( put_block ) {
                    can_put_block_to_remove_water = false;
                } else if( removed_water ) {
                    can_put_block_as_wall = false;
                } 

                break;
            case ITT_REACTOR:
                {
                    bool success = false;
                    if( c->st == ST_NONE ) {
                        Pos2 p(lc.x/PPC,lc.y/PPC);
                        if( g_fld->checkReactorPuttable(p) ) {
                            c->bt = BT_REACTOR_INACTIVE;
                            g_fld->saveAt(c, 2 );
                            success = true;
                        }
                    } 
                    if(success==false) {
                        g_cant_sound->play();
                        g_log->printf( WHITE, "CAN'T PUT REACTOR HERE" );
                        return false;
                    }
                }
                break;
            case ITT_POLE:
                if( c->st == ST_NONE ) {
                    Pos2 p(lc.x/PPC,lc.y/PPC);

                    // ENE can be reset on reload. So, overwrite ENE only when having 1 candidate.
                    Pos2 cand;
                    int ncand=1;
                    int orig_ene = 0;
                    g_fld->findPoleNeighbours( p, &cand, &ncand );
                    print("findPoleNeighbours: ncand:%d",ncand);
                    if(ncand==1) { //
                        PowerGrid *g = g_ps->findGrid( cand );
                        if(g) {
                            print("found cand! ene:%d",g->ene);
                            orig_ene = g->ene;
                        }
                    }
                    print("orig_ene:%d",orig_ene);
        
                    reloadPowerSystem();
                    int pgid = g_ps->addNode(p);
                    if( pgid <= 0 ) {
                        // Full?
                        g_log->printf( WHITE, "CAN'T PUT POLE ANY MORE" );
                        return false;
                    }
                    PowerGrid *ng = g_ps->findGridById(pgid);
                    if(ng && orig_ene > ng->ene ) {
                        ng->ene = orig_ene;
                        print("overwriting ene");
                    }
                    
                    c->bt = BT_POLE;
                    g_fld->setPowerGrid(p,pgid);
                    savePowerSystem();
                    g_fld->saveAt(c, 3 );
                    realtimePowerNodeSend( pgid, p );
                } else {
                    return false;
                }
                break;
            case ITT_EXCHANGE:
                if( c->st == ST_NONE ) {
                    g_fld->setExchange(c);
                } else {
                    return false;
                }
                break;
            case ITT_PORTAL:
                if( c->st == ST_NONE ) {
                    g_fld->setPortal(c);
                } else {
                    return false;
                }
                break;
            case ITT_CABLE:
                if( c->st == ST_NONE ) {
                    c->bt = BT_CABLE;
                    // Need to modify around cells quickly
                    g_fld->notifyChangedAround( g_fld->getCellPos(c),2);
                } else {
                    return false;
                }
                break;
            case ITT_DEBRI_IRONORE:
                if( c->st == ST_NONE ) {
                    c->bt = BT_IRONORE;
                } else {
                    return false;
                }
                break;
            case ITT_DEBRI_RAREMETALORE:
                if( c->st == ST_NONE ) {
                    c->bt = BT_RAREMETALORE;
                } else {
                    return false;
                }
                break;
            case ITT_TURRET:
                if( c->st == ST_NONE ) {
                    c->bt = BT_TURRET_INACTIVE;
                } else {
                    return false;
                }
                break;
            case ITT_FENCE:
                if( c->st == ST_NONE ) {
                    c->bt = BT_FENCE;
                } else {
                    return false;
                }
                break;
            case ITT_TREE_SEED:
                if( c->bt == BT_AIR && c->st == ST_NONE ) { 
                    c->bt = BT_TREE;
                }
                break;
            case ITT_WEED_SEED:
                if( c->st == ST_NONE ) {
                    c->st = ST_WEED;
                }
                break;
            case ITT_BRICK_PANEL:
                if( c->gt == GT_DEEP ) {
                    g_log->printf(WHITE, "WATER IS TOO DEEP TO PUT A BRICK PANEL" );
                    return false;
                }
                if( c->isWater() && c->gt != GT_DEEP ) {
                    c->st = ST_NONE;
                }
                if( c->st == ST_NONE && c->gt != GT_BRICK_PANEL ) {
                    c->gt = GT_BRICK_PANEL;
                } else {
                    return false;
                }
                break;
                
            default:
                assertmsg(false, "not implemented? itemid:%d", itc->itt);
            }
            soundPlayAt( g_put_sound, loc,1 );
            c->untouched = false;
            g_fld->notifyChanged5(c, true ); 
            g_fld->saveAt(c); // soon save
            decItemIndex( selected_item_index , 1);
            //            items[selected_item_index].dump();
            if( items[selected_item_index].num == 0 ) {
                // automatically swap if having same items in inventory (for convenience)
                int ind = findItemType( itc->itt );
                if( ind >= 0 ) swapItem( ind, selected_item_index );
            }
        }
        return true;
    } else {
        switch( itc->itt ) {
        case ITT_BEAMGUN1:
        case ITT_BEAMGUN2:
        case ITT_BEAMGUN3:
        case ITT_BEAMGUN4:
            {
                //                print("B ene:%d/%d", ene,maxene);
                int beam_ene = getBeamgunEne(itc);                                
                if( ene < beam_ene ) return false;
                ene -= beam_ene;
                soundPlayAt(g_shoot_sound[shoot_sound_index],loc,1);
                Vec2 handv;
                switch(direction.toDir() ) {
                case DIR_DOWN:
                    handv.x = -4;
                    handv.y = -8;
                    break;
                case DIR_UP:
                    handv.x = 4;
                    handv.y = 8;
                    break;
                case DIR_LEFT:
                    handv.x = -8;
                    handv.y = -6;
                    break;
                case DIR_RIGHT:
                    handv.x = 8;
                    handv.y = -6;
                    break;
                default:
                    break;
                }
                new Beam( loc + handv, loc + direction + handv, BEAMTYPE_NORMAL, beam_base_index, beam_ene, this->id );

                items[selected_item_index].dur --;
                if(items[selected_item_index].dur <= 0 ) {
                    decItemIndex( selected_item_index, 1 );
                }
                hudUpdateItems();
                if( findAcceleratorInSuit() ) {
                    wearSuitItem( ITT_ACCELERATOR, 1 );
                }
                if( g_invwin->visible ) g_invwin->update();
                return true;                
            }
            
        case ITT_BLASTER:
            new Blaster( loc, loc + direction * BLASTER_DISTANCE, false, id );
            decItemIndex( selected_item_index, 1 );
            return true;
        case ITT_HEAL_BLASTER:
            new Blaster( loc, loc + direction * BLASTER_DISTANCE, true, id );
            decItemIndex( selected_item_index, 1 );
            return true;
        case ITT_SHOVEL:
            if( c && c->bt == BT_AIR && c->canDig() ) {
                int digged = c->dig();
                if( digged != Cell::DIGGED_NONE ) {
                    soundPlayAt( g_cutweed_sound, loc, 1 );
                    if( digged == Cell::DIGGED_PLANT ) {
                        for(int i=0;i<10;i++ ) createLeafEffect( loc + direction * BLOCK_PUT_DISTANCE );
                    } else if( digged == Cell::DIGGED_EARTH ) {
                        for(int i=0;i<5;i++) createSmokeEffect( loc + direction * BLOCK_PUT_DISTANCE );
                    }
                    
                    items[selected_item_index].dur --;
                    if(items[selected_item_index].dur <= 0 ) {
                        decItemIndex( selected_item_index, 1);
                    }
                    hudUpdateItems();
                    g_fld->notifyChanged(c);
                }
            }
            break;
        default:
            break;
        }
    }
    return false;
}

void LocalPC::modScore( int add ) {
    score += add;
    updateNickTB();
}

