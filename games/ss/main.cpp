#include "stdafx.h"

#include <stdio.h>
#include <assert.h>
#if defined(__APPLE__)
#include <strings.h>
#endif
#include <math.h>

#include "vce.h"
#include "moyai/client.h"
#include "dimension.h"
#include "conf.h"
#include "ids.h"
#include "item.h"
#include "util.h"
#include "net.h"
#include "field.h"
#include "mapview.h"
#include "char.h"
#include "enemy.h"
#include "hud.h"
#include "pc.h"
#include "pwgrid.h"
#include "atlas.h"
#include "effect.h"
#include "globals.h"

#if defined(WIN32)
	#include <dbghelp.h>
	#pragma comment (lib, "DbgHelp.lib")
#endif


///////////


LARGE_INTEGER g_perfFreq;

MoyaiClient *g_moyai_client;
Viewport *g_world_viewport;
Viewport *g_hud_viewport;

Layer *g_ground_layer;
Layer *g_electricity_layer;
Layer *g_fluid_layer;
Layer *g_block_layer;
Layer *g_roof_layer;
Layer *g_flyer_layer;
Layer *g_char_layer;
Layer *g_effect_layer;
Layer *g_durbar_layer;
Layer *g_cursor_layer;
Layer *g_info_layer;
Layer *g_belt_layer;
Layer *g_gauge_layer;
Layer *g_hud_layer;
Layer *g_msg_layer;
Layer *g_hints_layer;
Layer *g_debug_layer;
Layer *g_wm_layer;


TileDeck *g_base_deck;
TileDeck *g_ascii_deck;
TileDeck *g_effect_deck;
TileDeck *g_girev_deck;
Texture *g_title_tex;
Texture *g_logo_tex;
Prop2D *g_titlebg_prop;
Prop2D *g_titlelogo_prop;

Camera *g_world_camera;
Camera *g_hud_camera;

AnimCurve *g_digsmoke_curve;
AnimCurve *g_spark_curve;
AnimCurve *g_cellbubble_curve;

ColorReplacerShader *g_eye_col_replacer;

SoundSystem *g_sound_system;

Sound *g_step_sound;
Sound *g_bgm_desert_sound;
Sound *g_bgm_tundra_sound;
Sound *g_bgm_jungle_sound;
Sound *g_bgm_dungeon_sound;
Sound *g_bgm_volcano_sound;
Sound *g_bgm_title_sound;
Sound *g_dig_sound;
Sound *g_gamestart_sound;
Sound *g_spark_sound;
Sound *g_killcreature_sound;
Sound *g_killmachine_sound;
Sound *g_shoot_sound[PC_SHOOT_SOUND_CANDIDATE_NUM];
Sound *g_beamhitwall_sound;
Sound *g_pickup_sound;
Sound *g_blaster_explode_sound;
Sound *g_destroy_building_sound;
Sound *g_wormdamage_sound;
Sound *g_wormkill_sound;
Sound *g_drink_sound;
Sound *g_pcdamage_sound;
Sound *g_wormbite_sound;
Sound *g_craft_sound;
Sound *g_researchdone_sound;
Sound *g_sig_explosion_sound;
Sound *g_shoot_sig_sound;
Sound *g_hitspario_sound;
Sound *g_beamhithard_sound;
Sound *g_eye_repair_sound;
Sound *g_repair_done_sound;
Sound *g_wipe_fortress_sound;
Sound *g_sandwalk_sound;
Sound *g_steponbrick_sound;
Sound *g_shield_sound;
Sound *g_volcano_ready_sound;
Sound *g_volcano_explode_sound;
Sound *g_fbshoot_sound;
Sound *g_cutweed_sound;
Sound *g_beestart_sound;
Sound *g_switch_sound;
Sound *g_core_sound;
Sound *g_switch_off_sound;
Sound *g_fall_sound;
Sound *g_lowhp_sound;
Sound *g_cursormove_sound;
Sound *g_cant_sound;
Sound *g_warp_sound;
Sound *g_put_sound;
Sound *g_windowopen_sound;
Sound *g_getflag_sound;
Sound *g_all_clear_sound;
Sound *g_beep_sounds[4];
Sound *g_shootsnow_sound;
Sound *g_slimejump_sound;
Sound *g_shoottornado_sound;
Sound *g_birdfly_sound;
Sound *g_abandon_sound;

int g_last_render_cnt ;
float g_zoom_rate = 2.0f;
const float ZOOM_RATE_MAX = 2.0f, ZOOM_RATE_MIN = 0.6f;

Font *g_font;

TileDeck *g_durbar_deck;
TileDeck *g_enebar_deck;

MapView *g_mapview;

int g_total_frame_count, g_frame_count, g_last_frame_count;
double g_total_frame_accum_time = 0.0;
double g_input_accum_time = 0.0;

Gauge *g_elec_gauge;
Gauge *g_hp_gauge;
Gauge *g_armor_gauge;

GridCursor *g_gridcurs[4];  // 最大4個、右アナログを8方向に倒したと仮定して表示する。 真上から右回り

FlagArrow *g_flagarrow;

double g_now_time;

Belt *g_belt;
Suit *g_suit;

Inventory *g_invwin;
LabWindow *g_labwin;
WorldMap *g_wm;
ExchangeWindow *g_exwin;
PortalWindow *g_portalwin;
CharMakeWindow *g_charmakewin;
CharNameWindow *g_charnamewin;
ProjectTypeWindow *g_projtypewin;
ProjectListWindow *g_projlistwin;
ProjectInfoWindow *g_projinfowin;
SpecialMenuWindow *g_spmenuwin;
FollowWindow *g_followwin;
MessageWindow *g_msgwin;
TitleWindow *g_titlewin;
RankTypeWindow *g_ranktypewin;
RankingWindow *g_rankwin;
FriendWindow *g_friendwin;
DebugWindow *g_debugwin;
CreditWindow *g_creditwin;

MilestoneDisplay *g_milestone_display;

CharGridTextBox *g_stateline;
CharGridTextBox *g_nicktb;

CharGridTextBox *g_powergrid_state; 

Log *g_log;

bool g_game_paused = true;
bool g_private_project = false;

int g_ideal_frame_rate = 60; // 30 or 60

long long g_last_ping_rtt_usec;

////////////////////////////////////

// パッド
int g_joystick_axes = 0;
int g_joystick_buttons = 0;

// ゲーム内容
Field *g_fld;
LocalPC *g_pc;
PowerSystem *g_ps;
ResourceDeposit *g_depo;

Flag *g_flag;

bool g_enable_auto_charge = false;
bool g_enable_network = true;
bool g_enable_debug_menu = false;
bool g_enable_debug_joystick = false;
float g_pc_walk_speed_accel = 1;
bool g_enable_infinity_items = false;
bool g_skip_projectinfowin_worldmap = false;
bool g_enable_nearcast_log = false;
bool g_enable_autoplay = false;

double g_enter_key_down_at;

bool g_network_initialized = false;

RUNSTATE g_runstate = RS_INIT;
FieldSaver *g_fsaver = NULL;

int g_current_project_id = 0;
double g_powersystem_lock_obtained_at = 0;
bool g_program_finished = 0;

////////////////////////////////////
void copyToRight( char *out, size_t outsize, const char *in ) {
    int l = strlen(in);
    int margin_left = outsize - 1 - l;
    for(int i=0;i<margin_left;i++) {
        out[i] = ' ';
    }
    for(int i=margin_left;i<outsize-1;i++) {
        out[i] = in[i-margin_left];
    }
    out[outsize-1] = '\0';
}
void commaInteger( char *out, size_t outsz, unsigned long long i ) {
    int one  = i%1000LL; 
    int kilo = (i/1000LL)%1000;
    int mega = (i/1000000LL)%1000;
    int giga = (i/1000000000LL)%1000;
    int tera = (i/1000000000000LL)%1000;
    int peta = (i/1000000000000000LL)%1000;
    
    if(i<1000) {
        snprintf(out, outsz, "%03d", one );
    } else if( i < 1000000 ) {
        snprintf(out, outsz, "%d,%03d", kilo, one );
    } else if( i < 1000000000 ) {
        snprintf(out, outsz, "%d,%03d,%03d", mega, kilo, one );
    } else if( i < 1000000000000 ) {
        snprintf(out, outsz, "%d,%03d,%03d,%03d", giga, mega, kilo, one );
    } else if( i < 1000000000000000 ) {
        snprintf(out, outsz, "%d,%03d,%03d,%03d,%03d", tera, giga, mega, kilo, one );
    } else if( i < 1000000000000000000 ) {
        snprintf(out, outsz, "%d,%03d,%03d,%03d,%03d,%03d", peta, tera, giga, mega, kilo, one );
    } else {
        snprintf(out, outsz, "999,999,999,999,999,999" );
    }
}
void shorterInteger( char *out, size_t outsz, unsigned long long i ) {
    if(i<1000) {
        snprintf(out, outsz, "%lld",i);
    } else if( i < 1000000 ) {
        snprintf(out, outsz, "%lldK", i/1000LL );
    } else if( i < 1000000000 ) {
        snprintf(out, outsz, "%lldM", i/1000000LL );
    } else if( i < 1000000000000 ) {
        snprintf(out, outsz, "%lldG", i/1000000000LL );
    } else if( i < 1000000000000000 ) {
        snprintf(out, outsz, "%lldT", i/1000000000000LL );
    } else if( i < 1000000000000000000 ) {
        snprintf(out, outsz, "%lldP", i/1000000000000000LL );
    } else {
        snprintf(out, outsz, "999P" );
    }
}

void updateNickTB() {
    if( g_pc && g_pc->nickname[0]) {
        char numstr[32];
        commaInteger( numstr, sizeof(numstr), g_pc->score );
        Format fmt( "PLAYER:%s SP:%s", g_pc->nickname, numstr );
        char s[32];
        copyToRight(s, sizeof(s), fmt.buf );
        g_nicktb->setString(WHITE, s );
        g_nicktb->setColor( Color(1,1,1,0.5) );
    } else {
        g_nicktb->setString(WHITE, "");
    }
}

// 360にのみ対応. キーボードを優先する.
void getPadState( Vec2 *ctl_move, Vec2 *ctl_shoot, float *ctl_zoom  ) {
    ctl_move->x = ctl_move->y = ctl_shoot->x = ctl_shoot->y = 0;
    
    if( g_joystick_axes == 5 ) { // xbox 360
        float pos[8]={0,0,0,0,0,0,0,0};
        assert(elementof(pos) >= g_joystick_axes);
        glfwGetJoystickPos( GLFW_JOYSTICK_1, pos, g_joystick_axes);
        
        ctl_move->x = pos[0]; // left stick
        ctl_move->y = pos[1]; 
        *ctl_zoom = pos[2]; // RT, LT
        ctl_shoot->x = pos[4]; // right stick
        ctl_shoot->y = pos[3] * -1;

        if( g_enable_debug_joystick ) {
            g_log->printf( WHITE, "JS: shoot:%f,%f", pos[3], pos[4] );
        }

        if( ctl_move->len() < 0.2 ) ctl_move->x = ctl_move->y = 0;
        if( ctl_shoot->len() < 0.2 ) ctl_shoot->x = ctl_shoot->y = 0;
    }
    
    if( glfwGetKey( 'W' ) ) ctl_move->y = 1;
    if( glfwGetKey( 'S' ) ) ctl_move->y = -1;
    if( glfwGetKey( 'A' ) ) ctl_move->x = -1;
    if( glfwGetKey( 'D' ) ) ctl_move->x = 1;
    if( glfwGetKey( 'V' ) ) *ctl_zoom = 1;
    if( glfwGetKey( 'B' ) ) *ctl_zoom = -1;

    if( glfwGetKey( GLFW_KEY_UP) ) ctl_shoot->y = 1;
    if( glfwGetKey( GLFW_KEY_DOWN) ) ctl_shoot->y = -1;
    if( glfwGetKey( GLFW_KEY_RIGHT) ) ctl_shoot->x = 1;
    if( glfwGetKey( GLFW_KEY_LEFT) ) ctl_shoot->x = -1;
}



void get4Locs( Vec2 center, float distance, Vec2 *out, bool adjust_to_grid ) {
    static Vec2 directions[4] = { Vec2(0,1), Vec2(1,0), Vec2(0,-1), Vec2(-1,0) };
    for(int i=0;i<4;i++){
        if( adjust_to_grid ) {
            out[i] = toCellCenter( center + directions[i] * distance );
        } else {
            out[i] = center + directions[i] * distance;
        }
    }
}

void setupGridCursor() {
    for(int i=0;i<elementof(g_gridcurs);i++) {
        g_gridcurs[i] = new GridCursor();
        g_gridcurs[i]->setVisible(false);
    }
}

void updateGridCursor( Vec2 ctl ) {
    static Pos2 last_center(-1,-1);

    Pos2 cur_center( g_pc->loc.x / PPC, g_pc->loc.y / PPC );

    float aim_thres = 0.1f, action_thres = 0.98f;
    bool aiming = ctl.len() > aim_thres && ctl.len() < action_thres;
    bool to_action = ctl.len() >= action_thres;
    //    print("aiming:%d action:%d",aiming, to_action);
    
    bool show=false;
    int cursor_num = 4;
    Vec2 locs[4];
    if( g_pc->isPuttableSelected() || g_pc->isDiggableSelected() ) {
        if( aiming || to_action ) {
            // ジョイスティックを使う場合
            cursor_num = 1;
            locs[0] = toCellCenter( g_pc->loc + ctl.normalize(BLOCK_PUT_DISTANCE));
            show = true;
        } else {
            get4Locs( g_pc->loc, BLOCK_PUT_DISTANCE, locs, true );
            show = true;            
        }
    } else if( g_pc->isItemSelected( ITT_BLASTER ) || g_pc->isItemSelected( ITT_HEAL_BLASTER )  ) {
        if( aiming || to_action )  {
            cursor_num = 1;
            locs[0] = g_pc->loc + ctl.normalize(BLASTER_DISTANCE);
            show = true;
        } else {
            get4Locs( g_pc->loc, BLASTER_DISTANCE, locs, false );
            show = true;
        }
    }
    for(int i=0;i<4;i++){
        g_gridcurs[i]->setVisible(false);
    }
    for(int i=0;i<cursor_num;i++) {
        GridCursor *g = g_gridcurs[i];            
        g->loc = locs[i];
        g->setVisible(show);
        if( g_pc->isItemSelected( ITT_POLE ) ) {
            g->setIndex( BT_POLE );
            g->setColor( Color(1,1,1,0.3) );
            Prop2D *p = g->ensureAttachment( g->loc+Vec2(0,POLE_STEM_Y_DIFF), g_base_deck, B_ATLAS_POLE_STEM );
            p->setColor( Color(1,1,1,0.3) );
            if( last_center != cur_center ) p->clearPrims();
            if(p->getPrimNum() == 0 ) {
                Pos2 cands[2];
                int n = 2;
                Pos2 fromp( g->loc.x/PPC, g->loc.y/PPC);
                g_fld->findPoleNeighbours( fromp, cands,&n);
                if(n > 0 ) {
                    if( n == 2 ) {
                        PowerGrid *g0 = g_ps->findGrid( cands[0] );
                        PowerGrid *g1 = g_ps->findGrid( cands[1] );
                        if(g0==g1) n = 1; // 同じpowergridに対して複数つなぐことはしない。(factorioの場合は多少はつなぐが)
                    }
                    for(int i=0;i<n;i++) {
                        Vec2 from = Vec2(0,POLE_STEM_Y_DIFF);
                        Vec2 to = from + Vec2( (cands[i].x-fromp.x) * PPC, (cands[i].y-fromp.y)*PPC);
                        Color col = Color(0.5,1,0.7,0.3);
                        Cell *c = g_fld->get( locs[i] );
                        if( c && c->isBrightColor() ) col = Color(0,0,0,0.4);
                        catenaryCurve( p, from, to, col, 2 );
                    }
                }
            }
        } else {
            g->clearAttachment();
            Cell *c = g_fld->get( locs[i] );
            if(c) {
                g->setBright( !c->isBrightColor() );
            }
        }
    }

    last_center = cur_center;
}

void updateToSim() {
    if( (g_total_frame_count % 5 ) == 0) {
        int x_mgn= 4;
		int y_mgn= 3;
        for(int dy=-y_mgn;dy<=y_mgn;dy++) {
            for(int dx=-x_mgn;dx<=x_mgn;dx++) {
                g_fld->setToSim(g_pc->loc + Vec2(dx*PPC*CHUNKSZ,dy*PPC*CHUNKSZ), 200 );    
            }
        }
    }
#if 0    
    for(int i=0;i<10;i++) {
        Vec2 at = g_pc->loc.randomize(200); 
        g_fld->setToSim(at, 100 );
    }
#endif    
}

// ゲーム外のウインドウ
bool isOutgameWindowVisible() {
    return g_charmakewin->visible || g_projtypewin->visible || g_projlistwin->visible || g_charnamewin->visible || g_projinfowin->visible || g_titlewin->visible || g_ranktypewin->visible || g_rankwin->visible || g_friendwin->visible || g_creditwin->visible;
}
// ゲーム中のウインドウ
bool isIngameWindowVisible(){
    return g_invwin->visible || g_labwin->visible || g_exwin->visible || g_wm->visible || g_portalwin->visible || g_spmenuwin->visible || g_followwin->visible || g_msgwin->visible || g_debugwin->visible;
}
// 何らかのウインドウ
bool isWindowVisible() {
    return isOutgameWindowVisible() || isIngameWindowVisible();
}
// ingameWindowをトグルできる条件か
bool isIngameWindowTogglable() {
    return isIngameWindowVisible() || isOutgameWindowVisible() == false;
}



// 一旦全部閉じてから指定されたものをだす
void toggleWindow( const char *name ) {

    if( isWindowVisible() ) {
        g_labwin->hide();
        g_wm->hide();
        g_exwin->hide();
        g_invwin->hide();
        g_portalwin->hide();
        g_charmakewin->hide();
        g_charnamewin->hide();
        g_projtypewin->hide();
        g_projlistwin->hide();
        g_projinfowin->hide();
        g_spmenuwin->hide();
        g_followwin->hide();        
        return;
    }
    
    if( strcmp(name,"inventory") == 0 ) {
        if( g_invwin->getVisible() ) {
            g_invwin->hide();
        } else {
            g_invwin->show();
        }        
    } else if( strcmp(name, "lab" ) == 0 ) {
        if( g_labwin->getVisible() ) {
            g_labwin->hide();
        } else {
            g_labwin->show();
        }
    } else if( strcmp(name, "map" ) == 0 ) {
        if( g_wm->getVisible() ) {
            g_wm->hide();
        } else {
            g_wm->show();
        }
    } else if( strcmp(name, "exchange" ) == 0 ) {
        if( g_exwin->getVisible() ) {
            g_exwin->hide();
        } else {
            g_exwin->show();
        }
    } else if( strcmp( name, "portal" ) == 0 ) {
        g_portalwin->toggle( ! g_portalwin->visible );
    } else if( strcmp( name, "charmake" ) == 0 ) {
        g_charmakewin->toggle( ! g_charmakewin->visible );
    } else if( strcmp( name, "charname" ) == 0 ) {
        g_charnamewin->toggle( !g_charnamewin->visible );
    } else if( strcmp( name, "projtype" ) == 0 ) {
        g_projtypewin->toggle( ! g_projtypewin->visible );
    } else if( strcmp( name, "projlist" ) == 0 ) {
        g_projlistwin->toggle( ! g_projlistwin->visible );
    } else if( strcmp( name, "projinfo" ) == 0 ) {
        g_projinfowin->toggle( ! g_projinfowin->visible );
    } else if( strcmp( name, "title" ) == 0 ) {
        g_titlewin->toggle( !g_titlewin->visible);
    } else if( strcmp( name, "ranktype" ) == 0 ) {
        g_ranktypewin->toggle( !g_ranktypewin->visible );
    } else if( strcmp( name, "credit" ) == 0 ) {
        g_creditwin->toggle( !g_creditwin->visible );
    } else if( strcmp( name, "ranking" ) == 0 ) {
        g_rankwin->toggle( !g_rankwin->visible );
    } else if( strcmp( name, "debug" ) == 0 ) {
    }
}



void onDirectionKey( DIR d ) {
    if( g_msgwin->visible ) {
        g_msgwin->moveCursor(d);
    } else if( g_invwin->visible ) {
        g_invwin->moveCursor(d);
    } else if( g_labwin->visible ) {
        g_labwin->moveCursor(d);
    } else if( g_exwin->visible ) {
        g_exwin->moveCursor(d);
    } else if( g_portalwin->visible ) {
        g_portalwin->moveCursor(d);
    } else if( g_charmakewin->visible ) {
        g_charmakewin->moveCursor(d);
    } else if( g_projtypewin->visible ) {
        g_projtypewin->moveCursor(d);
    } else if( g_projlistwin->visible ) {
        g_projlistwin->moveCursor(d);
    } else if( g_charnamewin->visible ) {
        g_charnamewin->moveCursor(d);
    } else if( g_projinfowin->visible ) {
        g_projinfowin->moveCursor(d);
    } else if( g_spmenuwin->visible ) {
        g_spmenuwin->moveCursor(d);
    } else if( g_followwin->visible ) {
        g_followwin->moveCursor(d);
    } else if( g_titlewin->visible ) {
        g_titlewin->moveCursor(d);
    } else if( g_ranktypewin->visible ) {
        g_ranktypewin->moveCursor(d);        
    } else if( g_rankwin->visible ) {
        g_rankwin->moveCursor(d);
    } else if( g_friendwin->visible ) {
        g_friendwin->moveCursor(d);
    } else if( g_debugwin->visible ) {
        g_debugwin->moveCursor(d);
    }
}

void onEnterKey() {
    if( g_msgwin->visible ) {
        g_msgwin->selectAtCursor();
    } else if( g_invwin->visible ) {
        g_invwin->selectAtCursor();
    } else if( g_labwin->visible ) {
        g_labwin->selectAtCursor();
    } else if( g_exwin->visible ) {
        g_exwin->selectAtCursor();
    } else if( g_portalwin->visible ) {
        g_portalwin->selectAtCursor();
    } else if( g_charmakewin->visible ) {
        g_charmakewin->selectAtCursor();
    } else if( g_projtypewin->visible ) {
        g_projtypewin->selectAtCursor();
    } else if( g_projlistwin->visible ) {
        g_projlistwin->selectAtCursor();
    } else if( g_charnamewin->visible ) {
        g_charnamewin->selectAtCursor();
    } else if( g_projinfowin->visible ) {
        g_projinfowin->selectAtCursor();
    } else if( g_spmenuwin->visible ) {
        g_spmenuwin->selectAtCursor();
    } else if( g_followwin->visible ) {
        g_followwin->selectAtCursor();
    } else if( g_titlewin->visible ) {
        g_titlewin->selectAtCursor();
    } else if( g_ranktypewin->visible ) {
        g_ranktypewin->selectAtCursor();
    } else if( g_creditwin->visible ) {
        g_creditwin->selectAtCursor();
    } else if( g_rankwin->visible ) {
        g_rankwin->selectAtCursor();
    } else if( g_friendwin->visible ) {
        g_friendwin->selectAtCursor();
    } else if( g_debugwin->visible ) {
        g_debugwin->selectAtCursor();
    }
}
void onEscapeKey() {
    if( g_msgwin->visible ) { g_msgwin->cancel(); return; }
    if( g_wm->visible ) { g_wm->hide(); return; }
    if( g_titlewin->visible ) { g_titlewin->cancel(); return; }    
    if( g_ranktypewin->visible ) { g_ranktypewin->cancel(); return; }
    if( g_creditwin->visible ) { g_creditwin->cancel(); return; }
    if( g_rankwin->visible ) { g_rankwin->cancel(); return; }
    if( g_friendwin->visible ) { g_friendwin->cancel(); return; }
    if( g_projtypewin->visible ) { g_projtypewin->cancel(); return; }
    if( g_projlistwin->visible ) { g_projlistwin->cancel(); return; }
    if( g_projinfowin->visible ) { g_projinfowin->cancel(); return; }
    if( g_exwin->visible ) { g_exwin->cancel(); return; }
    if( g_labwin->visible ) { g_labwin->cancel(); return; }
    if( g_followwin->visible ) { g_followwin->cancel(); return; }
    if( g_invwin->visible ) { g_invwin->hide(); return; }
    if( g_debugwin->visible ) { g_debugwin->hide(); return; }
    if( g_charmakewin->visible ) return; 
    if( g_spmenuwin->visible ) { g_spmenuwin->cancel(); return; }
    if( g_portalwin->visible ) { g_portalwin->cancel(); return; }
    if( g_runstate == RS_IN_PROJECT ) g_spmenuwin->show();
}

void saveExitProgram(int code ) {
    if( isRealtimeNetworkActive() ) {
        finishRealtimeNetwork();
    }
    if( isDBNetworkActive() && g_pc->nickname[0] ) {
        dbSavePC();
        finishDBNetwork();
    }
    exit(code);
}

int windowCloseCallback(void) {
    g_program_finished = true;
    return 0;
}

void GLFWCALL keyCallback( int key, int action ) {
    
    // Enterだけオート連射
    if(key ==GLFW_KEY_ENTER ) {
        if(action) {
            g_enter_key_down_at = now();
        } else {
            g_enter_key_down_at = 0;
        }
    }

    //
    
    if( action == 0 ) return;

    //    print("key: k:%d act:%d", key, action );


    switch(key) {
    case 'K':
        if( g_enable_debug_menu ) {
            g_pc->incItem( ITT_IRON_PLATE, DEFAULT_STACK_MAX, PC_SUIT_NUM, false );
            g_pc->incItem( ITT_RAREMETAL_CRYSTAL, DEFAULT_STACK_MAX, PC_SUIT_NUM, false );
        }
        break;
    case 'J':
        if( g_enable_debug_menu ) {
            for(int i=0;i<10;i++){
                g_pc->incItem( ITT_MICROBE, DEFAULT_STACK_MAX, PC_SUIT_NUM, false );
                g_pc->incItem( ITT_DEBRI_HARDROCK, DEFAULT_STACK_MAX, PC_SUIT_NUM, false );
                g_pc->incItem( ITT_DEBRI_ROCK, DEFAULT_STACK_MAX, PC_SUIT_NUM, false );
                g_pc->incItem( ITT_DEBRI_SOIL, DEFAULT_STACK_MAX, PC_SUIT_NUM, false );
            }
        }
        break;
    case 'Y':
        if( g_enable_debug_menu ) {
            glfwSleep(0.3);
        }
        break;
    case 'H':
        if( g_enable_debug_menu ) g_pc->clearItems();        
        break;
    case 'P':
        if( g_enable_debug_menu ) g_pc->loc.y = 1500 * PPC;
        break;
    case 'I':
        if( g_enable_debug_menu ) g_pc->loc.y = 24 * PPC;
        break;
    case 'L':
        if( g_enable_debug_menu ) {
            g_pc->incItem( ITT_ARTIFACT, DEFAULT_STACK_MAX, PC_SUIT_NUM, false );
            g_pc->incItem( ITT_DARK_MATTER_PARTICLE, DEFAULT_STACK_MAX, PC_SUIT_NUM, false );
            g_pc->incItem( ITT_HYPER_PARTICLE, DEFAULT_STACK_MAX, PC_SUIT_NUM, false );
        }
        break;
    case 'U':
        if(g_enable_debug_menu){
            Cell *c = g_fld->get( g_pc->loc + Vec2(PPC*3,0) );
            if(c) {
                c->bt = BT_GRAY_GOO;
                g_fld->notifyChanged(c);
            }
        }        
        break;
    case 'T':
        if( g_enable_debug_menu ) {
            for(int i=0;i<10;i++) new Shrimp( g_pc->loc + Vec2(100,100).randomize(50) );
        }
        break;
    case 'M':
        if( isIngameWindowTogglable() ) toggleWindow("map");
        break;
    case 'E':
        if( isIngameWindowTogglable() ) toggleWindow("inventory");
        break;
    case '0':
        if( g_enable_debug_menu ) {
            g_program_finished = true;
            if( g_current_project_id > 0 )  g_fld->checkSaveAllUnsaved(0.2);
            realtimeEventSend( EVT_LOGOUT, g_pc->nickname, 0, 0 );
            
#ifdef __APPLE__
            realtimeDebugCommandBroadcast( "exit_program" );
#endif
            networkLoop(0.2); // 確実に残りを送信する
        }        
        break;
    case GLFW_KEY_TAB:
        g_pc->moveSelection( DIR_DOWN );
        break;

    case GLFW_KEY_RIGHT:
        //    case 'D':
        onDirectionKey( DIR_RIGHT );
        break;
    case GLFW_KEY_LEFT:
        //    case 'A':
        onDirectionKey( DIR_LEFT );
        break;
    case GLFW_KEY_UP:
        //    case 'W':
        onDirectionKey( DIR_UP );
        break;
    case GLFW_KEY_DOWN:
        //    case 'S':
        onDirectionKey( DIR_DOWN );
        break;
    case GLFW_KEY_ENTER:
        onEnterKey();
        break;
    case GLFW_KEY_ESC: // パッドだとキャンセルボタンか
        onEscapeKey();
        break;
    default:
        break;
    }
}


// 0行が真っ赤(0....0)、だんだん緑になって、31行が全部緑(1....1).
// uvなので、左下が(0,0)
// 0000
// 0001
// 0011
// 0111
// 1111
// 横が31だったら、縦は32個まで必要.
void setupDurBarImage( TileDeck *target, Color col_for_damage, Color col_for_health ) {
    Image *img = new Image();
    img->setSize( 32, 64 );
    for(int y=0;y<64;y++) {
        for(int x=0;x<32;x++){
            Color c = col_for_health;
            if( x > y )  c = col_for_damage;
            img->setPixel(x,y,c);
        }
    }
    Texture *t = new Texture();
    t->setImage( img );
    target->setTexture(t);
    target->setSize( 1,64, 32, 1 );
    //target->setLinearMagFilter();
}


////////////////
// 常にローカルキャラを起点にして距離計算
void soundPlayAt( Sound *snd, Vec2 at, float distance_rate, bool to_distribute ) {
    float l = g_pc->loc.to(at).len() * distance_rate;
    float vol=0;
    if( l < 12 * PIXEL_PER_CELL ) {
        vol=1;
    } else if( l > 60 * PIXEL_PER_CELL ){
        vol=0;
    } else {
        vol = 1.0 - ( l - 12 * PIXEL_PER_CELL ) / ( 60 * PIXEL_PER_CELL - 12 * PIXEL_PER_CELL );
    }
    if(vol>0) snd->play(vol);
    if( to_distribute ) realtimePlaySoundSend(snd,at);
}

////////////////

// インゲームから出るときは、特別なキャラ以外全部消す
void clearNormalCharacters() {
    Char *cur = (Char*) g_char_layer->prop_top;
    while(cur) {
        if( cur != g_pc && cur != g_flag ) cur->to_clean = true;
        cur = (Char*) cur->next;
    }

}

////////////////
Sound *g_current_bgm = NULL;
void stopSituationBGM() {
    if(g_current_bgm) {
        g_current_bgm->stop();
        g_current_bgm = NULL;
    }
}
void pauseSituationBGM( bool to_pause ) {
    if( g_current_bgm) g_current_bgm->pause(to_pause);
}

void updateSituationBGM() {
    
    static double last_bgm_check_at = 0;
    static double bgm_fadeout_at = 0;

    if( isOutgameWindowVisible() ) {
        if( g_bgm_title_sound->isPlaying() == false ) g_bgm_title_sound->play();
        stopSituationBGM();
        last_bgm_check_at = g_now_time;
        return;
    } else {
        if( g_bgm_title_sound->isPlaying() ) g_bgm_title_sound->stop();
    }
    
    if( g_wipe_fortress_sound->isPlaying() ) {
        stopSituationBGM();
        last_bgm_check_at = g_now_time;
        return;
    }

    const float intvl = 3;
    const float fade_dur = 2;
    
    if( last_bgm_check_at < g_now_time - intvl ) {
        last_bgm_check_at = g_now_time;
        Environment *env = g_fld->getEnvironment( vec2ToPos2(g_pc->loc) );
        if(env) {
            Sound *tgt = NULL;
            switch(env->bmt) {
            case BMT_NONE: break;
            case BMT_DESERT: tgt = g_bgm_desert_sound; break;
            case BMT_VOLCANO: tgt = g_bgm_volcano_sound; break;
            case BMT_TUNDRA:
            case BMT_MOUNTAIN:
                tgt = g_bgm_tundra_sound; break;
            case BMT_JUNGLE: tgt = g_bgm_jungle_sound; break;
            case BMT_LAKE: tgt = g_bgm_desert_sound; break;
            case BMT_DUNGEON: tgt = g_bgm_dungeon_sound; break;
            }
            if(tgt != g_current_bgm ) {
                if(g_current_bgm ) {
                    if( bgm_fadeout_at == 0 ) {
                        bgm_fadeout_at = g_now_time + fade_dur;
                    } else {
                        if( g_now_time > bgm_fadeout_at ) {
                            bgm_fadeout_at = 0;
                            g_current_bgm->stop();
                            g_current_bgm = NULL;
                        }
                    }
                }
                if(tgt) {
                    if( g_current_bgm == NULL ) {
                        print("start bgm bmt:%d", env->bmt );
                        tgt->play();
                        g_current_bgm = tgt;
                    }
                }
            }
        } else {
            print("check bgm.. bmt not found here");
        }
    }
    
    if( bgm_fadeout_at > 0 ) {
        double time_left = bgm_fadeout_at - g_now_time;
        double rate = time_left / fade_dur;
        if( rate>0) {
            if( g_current_bgm ) g_current_bgm->setVolume(rate);
        } else {
            stopSituationBGM();
            bgm_fadeout_at = 0;
        }
    }
}

void setTitleScreenVisible( bool imgvis, bool logovis ) ;

// 方向キーの連打をシミュレートする
void checkPadRepeat( Vec2 v ) {
    bool stick_on = ( v.len() > JOYSTICK_CURSOR_THRES );
    static double last_stick_on = 0;
    static double last_repeat_on = 0;
    static DIR last_stick_dir = DIR_NONE;

    if( stick_on ) {
        DIR d = v.toDir();
        if( d != last_stick_dir ) {
            last_stick_on = g_now_time;
            last_stick_dir = d;
        } else if( last_stick_on == 0 ) {
            last_stick_on = g_now_time;
            last_stick_dir = d;
        } else if( last_stick_on > 0 ){
            if( g_now_time > last_stick_on + JOYSTICK_REPEAT_WAIT ) {
                if( g_now_time > last_repeat_on + JOYSTICK_REPEAT_INTERVAL ) {
                    last_repeat_on = g_now_time;
                    onDirectionKey(d);
                }
            }
        }
    } else {
        last_stick_on = 0;
        last_stick_dir = DIR_NONE;
    }
    
    
}

////////////////
double g_last_now_time = 0;
void updateGame(void) {
	g_last_now_time = g_now_time;
    g_now_time = now();
    double elt_sec = g_now_time - g_last_now_time;

    g_last_now_time = g_now_time;

    double step_time = 1.0f / (double)g_ideal_frame_rate;
    Prop::frame_step_time = step_time;
    if( elt_sec > step_time*2 ) elt_sec = step_time*2;

    g_frame_count ++;
    
	g_total_frame_accum_time += elt_sec;
	while (g_total_frame_accum_time >= step_time)
	{
		++g_total_frame_count;
		g_total_frame_accum_time -= step_time;
	}
    
    switch( g_runstate ) {
    case RS_INIT:
        setTitleScreenVisible(true,false);        
        if( isDBNetworkActive() ) {
            bool ret;

            ret = ensureUserID( g_user_name );
            if(!ret) {
                // useridを取得できないようだと、これ以降何もできないので終了
                print("ensureUserID failed");
                exit(1);
            }

            ret = dbLoadPC();

            if( g_enable_infinity_items ) {
                g_pc->clearItems();
            }
            
            updateNickTB();
            if(!ret) {
                toggleWindow("charmake");
                g_runstate = RS_MAKING_CHARACTER;
            } else {
                g_runstate = RS_MADE_CHARACTER;
            }
        } else {
            g_pc->addInitialItems();
            g_fld->generateSync();
            setTitleScreenVisible(false,false);
            g_runstate = RS_IN_PROJECT;
        }
        break;
    case RS_MAKING_CHARACTER:
        if( g_charmakewin->visible == false && g_charnamewin->visible == false ) toggleWindow("charmake");
        break;
    case RS_MADE_CHARACTER:
        updateNickTB();
        toggleWindow("title");        
        g_runstate = RS_MAIN_MENU;
        break;
    case RS_MAIN_MENU:
        setTitleScreenVisible( true, true );
        break;
    case RS_SELECTING_PROJECT:
        setTitleScreenVisible( true, false );
        if( g_projlistwin->visible) g_projlistwin->poll();
        if( g_projinfowin->visible) g_projinfowin->poll();
        break;
    case RS_SELECTED_PROJECT:
        {
            g_runstate = RS_IN_PROJECT; // ゲーム開始
            g_gamestart_sound->play();
            
            dbUpdatePresenceSend();

            g_private_project = !(dbCheckProjectIsShared( g_current_project_id, g_user_id ) || dbCheckProjectIsPublished( g_current_project_id ));
            
            // 電源システムを全部ロードする
            reloadPowerSystem();
            dbLoadResearchState( g_current_project_id );
            
            // ゲーム始めるときに旗のリストをロードして反映する
            ProjectInfo pinfo;
            bool res = dbLoadProjectInfo( g_current_project_id, &pinfo );
            assert(res); // ここまできてるのにファイルが無いのはおかしい。
            g_fld->applyFlagCands( &pinfo );
            dbLoadFieldRevealSync( g_current_project_id );
            dbLoadFieldEnvSync( g_current_project_id );
            g_flag->warpToNextLocation();
            //            g_log->printf(WHITE, "hudUpdateMilestone countMilestoneCleared:%d ", countMilestoneCleared(&pinfo));
            hudUpdateMilestone( countMilestoneCleared(&pinfo) );

            g_pc->loc = toCellCenter( g_fld->getRespawnPoint() );
            g_pc->clearHistory( g_pc->loc );
            g_pc->hp = g_pc->maxhp;
            g_pc->onStatusUpdated();
            g_log->printf( WHITE, "LOGGED IN TO PROJECT ID:%d", g_current_project_id );

            dbEnsureImage( g_current_project_id ); // 長い時間projectinfoを表示してることがあるので。。

            realtimeEventSend( EVT_LOGIN, g_pc->nickname, 0, 0 );
            updateToSim();

            setTitleScreenVisible(false,false);
        }
        break;
    case RS_IN_PROJECT:
        if( g_private_project && isIngameWindowVisible() ) {
            g_game_paused = true;
        } else {
            g_game_paused = false;
        }
        if( g_total_frame_count % 50 == 0 ) { 
            realtimeUpdateNearcastPositionSend( g_pc->loc.x / PPC, g_pc->loc.y / PPC );
            pollPowerSystemLock();
        }
        if( g_flag->all_cleared == false && g_total_frame_count % (INCREMENT_PLAYTIME_INTERVAL_SEC*60) == 0 ) {
            dbIncrementPlaytimeSend( g_current_project_id, INCREMENT_PLAYTIME_INTERVAL_SEC);
        }

        // debugging
        if( g_enable_auto_charge && g_pc->ene < g_pc->maxene/2 ) g_pc->charge(1000);
        if( g_enable_infinity_items ) {
            if( g_pc->countItem( ITT_SHIELD ) == 0 ) g_pc->incItem( ITT_SHIELD, 1,0, false );
            if( g_pc->countItem( ITT_ACCELERATOR ) == 0 ) g_pc->incItem( ITT_ACCELERATOR, 1,0, false );
            if( g_pc->countItem( ITT_BATTERY1 ) == 0 ) g_pc->incItem( ITT_BATTERY1, 1,0, false );
            if( g_pc->items[PC_SUIT_NUM].itt != ITT_BEAMGUN4 ) {
                g_pc->incItem( ITT_BEAMGUN4, 1, PC_SUIT_NUM, false );
            }
            if(g_pc->items[PC_SUIT_NUM+1].itt != ITT_BRICK_PANEL ) {
                g_pc->items[PC_SUIT_NUM+1].setEmpty();
                g_pc->incItem( ITT_BRICK_PANEL, 50, PC_SUIT_NUM+1, false );                
            }
            if( g_enable_autoplay ) {
                if( g_pc->items[PC_SUIT_NUM+1].itt == ITT_BRICK_PANEL ) {
                    int orig_si = g_pc->selected_item_index;
                    g_pc->selected_item_index = PC_SUIT_NUM+1;
                    g_pc->tryAction(Vec2(range(-1,1),range(-1,1)));
                    g_pc->selected_item_index = orig_si;
                }
            }
        }

        
        break;        
    case RS_LEAVING_PROJECT:
        break;
    case RS_LEFT_PROJECT:
        g_game_paused = true;
        g_current_project_id = 0;        
        toggleWindow("title");
        g_runstate = RS_MAIN_MENU;
        setTitleScreenVisible(true,true);
        break;        
    }
    

    // 
    Vec2 ctl_move, ctl_shoot; // 一定以上傾けると、その方角に撃つ。
    float ctl_zoom=0;

    bool is_iconified = glfwGetWindowParam( GLFW_ICONIFIED );
    if( is_iconified == false ) getPadState( &ctl_move, &ctl_shoot, &ctl_zoom );

    if( g_enable_autoplay ) {
        static double last_turn_at = 0;
        static Vec2 auto_ctl_move, auto_ctl_shoot;
        if( g_pc->accum_time > last_turn_at + 0.2 ) {
            Vec2 to_ms = (g_flag->loc - g_pc->loc).normalize(0.4);
            Vec2 direction = ( Vec2(range(-1,1), range(-1,1) ) + to_ms ).normalize(1);
            auto_ctl_move = direction;
            auto_ctl_shoot = direction.normalize(1);
            last_turn_at = g_pc->accum_time;
        }

        ctl_move = auto_ctl_move;
        ctl_shoot = auto_ctl_shoot;

        if( g_labwin->getVisible() ) onEscapeKey();
    }
    
    
	g_input_accum_time += elt_sec;
	while (g_input_accum_time >= step_time)
	{
		if( ctl_zoom > 0.5 ) {
			g_zoom_rate *= 1.05; 
		} else if( ctl_zoom < -0.5 ) {
			g_zoom_rate *= 0.95;
		}
		g_input_accum_time -= step_time;
	}
    
    static Vec2 last_ctl_move;
    if( isWindowVisible() ) {
        checkPadRepeat( ctl_move );
        g_pc->ideal_v = Vec2(0,0);
        float thres = JOYSTICK_CURSOR_THRES;
        if( last_ctl_move.len() < thres && ctl_move.len() >= thres ) {
            DIR dir = vec2ToDir(ctl_move);
            onDirectionKey(dir);
        }
    } else {
        g_pc->ideal_v = ctl_move;

        bool to_auto_repeat = true;
        if( g_pc->isItemSelected( ITT_BLASTER ) || g_pc->isItemSelected( ITT_HEAL_BLASTER ) ) to_auto_repeat = false; // 連打するとマズいアイテム
        
        static double last_shoot_at=0;
        if( ctl_shoot.len() > 0.98 ) {
            //            print("last_shoot_at: %f in:%f",last_shoot_at, g_pc->getShootIntervalSec() );
            if( last_shoot_at == 0 || (to_auto_repeat && g_now_time > (last_shoot_at + g_pc->getShootIntervalSec() ) ) ) {
                if( g_pc->tryAction( ctl_shoot ) ) {
                    last_shoot_at = g_now_time;
                }
            }
            g_pc->shoot_v = ctl_shoot;
        } else {
            g_pc->shoot_v = Vec2(0,0);
            last_shoot_at = 0;
            g_pc->resetPutBlockFlags();
        }
    }
    last_ctl_move = ctl_move;


    updateGridCursor( ctl_shoot );
    
    // Enterキー連打
    if( g_enter_key_down_at > 0 ) {
        double elt = g_now_time - g_enter_key_down_at;
        if( elt < 0.3 ) {
            // 待ち時間
        } else {
            if( (g_frame_count%2)==0){
                onEnterKey();
            }
            
        }
    }

    pollNetwork(g_last_now_time);

    g_log->poll();
    if( g_exwin->visible ) g_exwin->poll();
    if( g_portalwin->visible ) g_portalwin->poll();
    if( g_labwin->visible ) g_labwin->poll();
    if( g_msgwin->visible ) g_msgwin->poll();
    
    //
    if( g_game_paused ) {
        g_char_layer->skip_poll = true;
    } else {
        g_char_layer->skip_poll = false;
    }

    int cnt = g_moyai_client->poll(elt_sec);

    static double last_print_at=0;
    if( g_now_time > last_print_at + 1 ) {
        last_print_at = g_now_time;
        int to_sim_n, lock_n;
        to_sim_n = g_fld->countToSim( &lock_n );
        extern int g_last_join_channel_id;
        extern int g_last_sendbuf_len, g_last_recvbuf_len;
        extern int g_realtime_client_id;
        char tmp[200];
        int world_lock_sec;
        int pgid=0, ftid=0;
        Cell *c = g_fld->get( g_pc->loc );
        if(c) {
            pgid = c->powergrid_id;
            ftid = c->fortress_id;
        }
        if( g_powersystem_lock_obtained_at == 0 ) world_lock_sec = 0; else world_lock_sec = (int)( now() - g_powersystem_lock_obtained_at );

        int rt_rb = 0, rt_wb = 0;
        static int last_rt_rb = 0, last_rt_wb = 0;
        int db_rb = 0, db_wb = 0;
        static int last_db_wb = 0, last_db_rb = 0;
        conn_stat_t rt_cst, db_cst;
        if( isRealtimeNetworkActive() && isDBNetworkActive() ) {
            vce_conn_get_stat( g_rtconn, &rt_cst );
            vce_conn_get_stat( g_dbconn, &db_cst );
        }
        rt_rb = rt_cst.recv_byte - last_rt_rb;
        rt_wb = rt_cst.send_byte - last_rt_wb;
        db_rb = db_cst.recv_byte - last_db_rb;
        db_wb = db_cst.send_byte - last_db_wb;        
        last_rt_rb = rt_cst.recv_byte;
        last_rt_wb = rt_cst.send_byte;
        last_db_rb = db_cst.recv_byte;
        last_db_wb = db_cst.send_byte;
        
        
        snprintf( tmp, sizeof(tmp),
                  "FPS:%d st:%d ping:%dms prp:%d(%d) sim:%d pf:%d chg:%d tosim:%d(lk:%d) wl:%d z:%.1f pc(%.1f,%.1f) ch:%d cid:%d SR:%d/%d GID:%d ft:%d rt:%d/%d db:%d/%d rb:%d",
                  g_frame_count,
                  g_runstate,
                  (int)(g_last_ping_rtt_usec / 1000),
                  cnt, g_last_render_cnt ,
                  g_fld->sim_clock_count,
                  g_fld->simchunk_per_poll,
                  g_fld->last_chunk_change_cnt,
                  to_sim_n, lock_n,
                  world_lock_sec,
                  g_zoom_rate,
                  g_pc->loc.x/PPC, g_pc->loc.y/PPC,
                  g_last_join_channel_id,
                  g_realtime_client_id,
                  g_last_sendbuf_len,
                  g_last_recvbuf_len,
                  pgid,
                  ftid,
                  rt_rb/1024,rt_wb/1024, db_rb/1024,db_wb/1024,
                  g_last_recvbuf_len/1024
                );
        if( rt_rb > 1024*1250 ) {
            g_log->printf(WHITE, "TOO MANY PACKETS? : %d Kbytes/sec", rt_rb/1024);
        }
        //        fprintf( stderr, "%s\n",tmp );
        g_stateline->setString( Color(1,1,1,0.5), tmp );
        g_stateline->setVisible( g_enable_debug_menu );
        g_last_frame_count = g_frame_count;
        g_frame_count = 0;
    }

    g_world_camera->loc.x = g_pc->loc.x;
    g_world_camera->loc.y = g_pc->loc.y;

    if( g_runstate == RS_IN_PROJECT ) {
        if(  g_game_paused == false ) {
            hudUpdatePCStatus();
            if( g_ps ) g_ps->poll(elt_sec);
        }
        // pauseしていてもやるよ
        g_fld->sim( g_game_paused );
        updateToSim();
        if( isDBNetworkActive() ) g_fld->pollNetworkCache();
    }

                                                                                                                                       
    static int last_wheel = 0;                                                                                                          
    int wheel = glfwGetMouseWheel();                                                                                                    
    int dwheel = wheel - last_wheel;                                                                                                    
    last_wheel = wheel;


    float zoomspeed_div;
#ifdef __APPLE__
    zoomspeed_div = 100.0f;
#else
    zoomspeed_div = 10.0f;
#endif
    g_zoom_rate += ((float)dwheel/zoomspeed_div);
    if( g_zoom_rate > ZOOM_RATE_MAX ) {                                                                                               
        g_zoom_rate = ZOOM_RATE_MAX;
    } else if( g_zoom_rate < ZOOM_RATE_MIN ) {                                                                                        
        g_zoom_rate = ZOOM_RATE_MIN;
    }      

    if( g_joystick_buttons > 0 && is_iconified == false ) {
        unsigned char btn[12];
        glfwGetJoystickButtons( GLFW_JOYSTICK_1, btn, 12 );
        //        print( "jsbtn: %d %d %d %d  %d %d %d %d  %d %d %d %d",btn[0], btn[1], btn[2], btn[3],btn[4], btn[5], btn[6], btn[7],btn[8], btn[9], btn[10], btn[11] );

        static int last_js_rb=0, last_js_lb=0, last_js_y=0, last_js_a=0,last_js_b=0, last_js_back=0,last_js_start=0;
        // 360を基本とする
        int js_a = btn[0]; 
        int js_b = btn[1];
        //        int js_x = btn[2];
        int js_y = btn[3];
        int js_lb = btn[4];
        int js_rb = btn[5];
        int js_back = btn[6];
        int js_start = btn[7];
        

        if( g_runstate == RS_IN_PROJECT ) {
            if( last_js_lb == 0 && js_lb ) {
                g_pc->moveSelection( DIR_UP );
            }
            if( last_js_rb == 0 && js_rb ) {
                g_pc->moveSelection( DIR_DOWN );
            }
            if( last_js_y == 0 && js_y ) {
                toggleWindow("inventory");
            }
        }

        if( last_js_a == 0 && js_a ) {
            onEnterKey();
            g_enter_key_down_at = now();
        } else if( last_js_a && js_a == 0 ) {
            g_enter_key_down_at = 0;
        }
        if( last_js_b == 0 && js_b && isWindowVisible() ) onEscapeKey();
        if( last_js_back == 0 && js_back ) onEscapeKey();
        if( last_js_start == 0 && js_start ) {
            if( isOutgameWindowVisible() ) {
                onEscapeKey(); // show special menu
            } else if( isIngameWindowVisible() ) {
                onEscapeKey();
            } else {
                toggleWindow("map");
            }
        }

        last_js_a = js_a;
        last_js_b = js_b;
        last_js_y = js_y;
        last_js_lb = js_lb;
        last_js_rb = js_rb;
        last_js_back = js_back;
        last_js_start = js_start;
    }
    

        
    if( g_mapview ) {
        g_mapview->poll( Vec2(g_world_camera->loc.x, g_world_camera->loc.y), g_zoom_rate );
        pollExchangeEffect();
    }

    if( g_flag ) {
        if( g_flag->all_cleared ) {
            g_flagarrow->setVisible(false);
        } else {
            g_flagarrow->setVisible(true);            
            g_flagarrow->updateLoc( g_pc->loc, g_flag->loc );
        }
    }
    
    g_world_viewport->setScale2D(SCRW/g_zoom_rate,SCRH/g_zoom_rate);

    // 色
    {
        const unsigned int eye_colors[] = { 0xff0000,  0xff00ba, 0xae00ff, 0x3c00ff, 0x0084ff, 0x00fff0,
                                            0x00ff72, 0x00ff00, 0xa8ff00, 0xfff600, 0xff9600, 0xff3000 };
        unsigned int ei = (int)(g_pc->accum_time * 4) % elementof(eye_colors);
        g_eye_col_replacer->setColor( Color(0,1,1,1), Color( eye_colors[ei]), 0.02 );
    }

    // BGM
    updateSituationBGM();
    
	{
		PerformanceCounter perf("Frame render");
		g_last_render_cnt = g_moyai_client->render();
	}
}

// マルチプレイのデバッグでウインドウの位置を変更するのが面倒なので。
#ifdef __APPLE__
void tryDebugToggleWindowPos() {
    char buf[100];
    size_t sz = sizeof(buf);
    bool to_shift = false;
    if( readFile("/tmp/sswinpos",buf,&sz) ) to_shift = true;
    if( to_shift ) {
        glfwSetWindowPos( 200,0 );
        system("rm /tmp/sswinpos");
    } else {
        glfwSetWindowPos( 600,0 );
        writeFile( "/tmp/sswinpos", "hello", 5 );
    }
}
#endif

void setupTitleBG() {
    g_titlebg_prop = new Prop2D();
    g_titlebg_prop->setTexture(g_title_tex);
    g_titlebg_prop->setScl(SCRW,SCRH);
    g_titlebg_prop->setLoc(0,0);
    g_hud_layer->insertProp(g_titlebg_prop);
    g_titlelogo_prop = new Prop2D();
    g_titlelogo_prop->setTexture(g_logo_tex);
    g_titlelogo_prop->setScl(400*2,178*2);
    g_titlelogo_prop->setLoc(0,150);
    g_hud_layer->insertProp(g_titlelogo_prop);
}
void setTitleScreenVisible( bool imgvis, bool logovis ) {
    g_titlebg_prop->setVisible(imgvis);
    g_titlelogo_prop->setVisible(logovis);
}        

////////////////

int moaiMain( int argc, char **argv ){
    print("program start. argc:%d", argc );
    for(int i=0;i<argc;i++) {
        print("argv[%d]:'%s'", i, argv[i] );
        if( strncmp( argv[i], "--username=", strlen("--username=") ) == 0 ) {
            char *p = argv[i] + strlen("--username=");
            strncpy( g_user_name, p, sizeof(g_user_name) );            
        } else if( strncmp( argv[i], "--dbhost=", strlen("--dbhost=") ) == 0 ) {
            char *p = argv[i] + strlen("--dbhost=");
            strncpy( g_dbhost, p, sizeof(g_dbhost) );
        } else if( strncmp( argv[i], "--rthost=", strlen("--rthost=") ) == 0 ) {
            char *p = argv[i] + strlen("--rthost=");
            strncpy( g_rthost, p, sizeof(g_rthost) );                        
        } else if( strncmp( argv[i], "--debug", strlen("--debug") ) == 0 ) {
            g_enable_debug_menu = true;
        } else if( strncmp( argv[i], "--autoplay", strlen("--autoplay") ) == 0 ) {
            g_enable_autoplay = true;
            g_enable_auto_charge = true;
            g_enable_infinity_items = true;
            g_pc_walk_speed_accel = 3;
            g_enable_debug_menu = true;
        } else if( strncmp( argv[i], "--autocharge", strlen("--autocharge") ) == 0 ) {
            g_enable_auto_charge = true;
        } else if( strncmp( argv[i], "--speedwalk", strlen("--speedwalk") ) == 0 ) {
            g_pc_walk_speed_accel = 2;
        } else if( strncmp( argv[i], "--infitems", strlen("--infitems") ) == 0 ) {
            g_enable_infinity_items = true;
        } else if( strncmp( argv[i], "--jsdebug", strlen("--jsdebug") ) == 0 ) {
            g_enable_debug_joystick = true;
        } else if( strncmp( argv[i], "--nclog", strlen("--nclog") ) == 0 ) {
            g_enable_nearcast_log = true;
        } else if( strncmp( argv[i], "--skip-wm", strlen("--skip-wm") ) == 0 ) {
            g_skip_projectinfowin_worldmap = true;
        } else if ( strncmp( argv[i], "--offline", strlen("--offline") ) == 0 ) {
			g_enable_network = false;
		} else if( strncmp( argv[i], "--fps=", strlen("--fps=") ) == 0 ) {
            char *p = argv[i] + strlen("--fps=");            
            g_ideal_frame_rate = atoi( p);
        } else if( strcmp( argv[i], "--long-db-timeout" ) == 0 ) {
            g_db_timeout_sec = 40;
        } else if( strncmp( argv[i], "--host=", strlen("--host=") ) == 0 ) {
            char *p = argv[i] + strlen("--host=");
            if( strcmp(p, "2" ) == 0 ) {
                strncpy( g_rthost, "192.168.11.2", sizeof(g_rthost) );
                strncpy( g_dbhost, "192.168.11.2", sizeof(g_dbhost) );                                        
            } else if( strcmp(p, "3" ) == 0 ) {
                strncpy( g_rthost, "192.168.11.3", sizeof(g_rthost) );
                strncpy( g_dbhost, "192.168.11.3", sizeof(g_dbhost) );
            } else if( strcmp(p, "4" ) == 0 ) {
                strncpy( g_rthost, "192.168.11.4", sizeof(g_rthost) );
                strncpy( g_dbhost, "192.168.11.4", sizeof(g_dbhost) );
            } else if( strcmp(p, "6" ) == 0 ) {
                strncpy( g_rthost, "192.168.11.6", sizeof(g_rthost) );
                strncpy( g_dbhost, "192.168.11.6", sizeof(g_dbhost) );
            } else if( strcmp(p, "7" ) == 0 ) {
                strncpy( g_rthost, "192.168.11.7", sizeof(g_rthost) );
                strncpy( g_dbhost, "192.168.11.7", sizeof(g_dbhost) );                                                
            } else if( strcmp(p, "lin" ) == 0 ) {
                strncpy( g_rthost, "106.187.55.104", sizeof(g_rthost) );
                strncpy( g_dbhost, "106.187.55.104", sizeof(g_dbhost) );
            } else if( strcmp(p, "local" ) == 0 ) {
                strncpy( g_rthost, "127.0.0.1", sizeof(g_rthost) );
                strncpy( g_dbhost, "127.0.0.1", sizeof(g_dbhost) );
            } else {
                assertmsg( false, "invalid --host specified : '%s'",p);
            }
        }
    }

    print("username set '%s' dbhost:%s rthost:%s ", g_user_name, g_dbhost, g_rthost );

#ifdef WIN32
	FreeConsole();
#endif    
    // glfw
    if( !glfwInit() ) {
        print("can't init glfw");
        return 1;
    }

    if( !glfwOpenWindow( SCRW, SCRH, 0,0,0,0, 0,0, GLFW_WINDOW ) ) {
        print("can't open glfw window");
        glfwTerminate();
        return 1;
    }

    glfwSetWindowTitle( "SpaceSweeper" );
    glfwEnable( GLFW_STICKY_KEYS );
    //glfwSwapInterval( WAIT_FOR_VSYNC ); // vsyncを待つが、環境によっては機能しないので、fpsが高いようなら vsyncが使えないフラグを立てて使う
    glfwSwapInterval( 0); // for shinra

    
#ifdef WIN32
	glewInit();
#endif
    glClearColor(0,0,0,1);

    glfwSetKeyCallback( keyCallback );
    glfwSetWindowCloseCallback( windowCloseCallback );

    g_joystick_axes = glfwGetJoystickParam( GLFW_JOYSTICK_1, GLFW_AXES );
    g_joystick_buttons = glfwGetJoystickParam( GLFW_JOYSTICK_1, GLFW_BUTTONS );
    print("glfwGetJoystickParam: axes: %d buttons: %d", g_joystick_axes, g_joystick_buttons );
    
    // sounds
    print("loading sounds");
    g_sound_system = new SoundSystem();
    g_step_sound = g_sound_system->newSound( "sounds/landed.wav" ); // 
    g_dig_sound = g_sound_system->newSound( "sounds/digsoil.wav" );    // 
    g_bgm_desert_sound = g_sound_system->newSound( "sounds/BGM_Desert.wav", 0.7, false );
    g_bgm_desert_sound->setLoop(true);
    g_bgm_tundra_sound = g_sound_system->newSound( "sounds/BGM_Tundra.wav", 0.7, false );
    g_bgm_tundra_sound->setLoop(true);
    g_bgm_jungle_sound = g_sound_system->newSound( "sounds/BGM_LakeSwamp.wav", 0.7, false );
    g_bgm_jungle_sound->setLoop(true);
    g_bgm_dungeon_sound = g_sound_system->newSound( "sounds/BGM_Dungeons.wav", 0.7, false );
    g_bgm_dungeon_sound->setLoop(true);
    g_bgm_volcano_sound = g_sound_system->newSound( "sounds/BGM_Volcano.wav", 0.7, false );
    g_bgm_volcano_sound->setLoop(true);
    g_bgm_title_sound = g_sound_system->newSound( "sounds/BGM_TitleJingle.wav", 0.7, false );
    g_bgm_title_sound->setLoop(true);    
 
    g_gamestart_sound = g_sound_system->newSound( "sounds/gamestart.wav" ); // 
    g_spark_sound = g_sound_system->newSound( "sounds/spark.wav" ); // PC shoot on a wall that never be damaged.
    g_killcreature_sound = g_sound_system->newSound( "sounds/killenemy.wav", 0.3, false ); // PC shoot and kill enemy creatures.
    g_killmachine_sound = g_sound_system->newSound( "sounds/machine_explo.wav", 0.3, false ); // PC shoot and destroy enemy machines.
    g_shoot_sound[0] = g_sound_system->newSound( "sounds/shoot.wav", 0.5, false );          // PC shooting sound option 0.
    g_shoot_sound[1] = g_sound_system->newSound( "sounds/shoot2.wav", 0.8, false );   // PC shooting sound option 1.
    g_shoot_sound[2] = g_sound_system->newSound( "sounds/shootrough.wav", 0.7, false );     // PC shooting sound option 2.
    g_shoot_sound[3] = g_sound_system->newSound( "sounds/shootring.wav", 0.3, false );      // PC shooting sound option 3.
    g_shoot_sound[4] = g_sound_system->newSound( "sounds/shoot5.wav", 0.5, false );       // PC shooting sound option 4.
    g_shoot_sound[5] = g_sound_system->newSound( "sounds/shoot6.wav", 0.5, false  );    // PC shooting sound option 5.
    g_beamhitwall_sound = g_sound_system->newSound( "sounds/beamhitwall.wav" ); // PC beam hit and make damage on a wall.
    g_pickup_sound = g_sound_system->newSound( "sounds/pickup.wav" ); // PC picks an item up.
    g_blaster_explode_sound = g_sound_system->newSound( "sounds/zap.wav" ); // PC throw and blow blaster bomb.
    g_destroy_building_sound = g_sound_system->newSound( "sounds/destroybuilding.wav" ); // PC shoot and destroy enemy building.
    g_wormdamage_sound = g_sound_system->newSound( "sounds/wormhit.wav", 0.1, false ); // PC shoot and damage enemy worm.
    g_wormkill_sound = g_sound_system->newSound( "sounds/wormkill.wav", 1, false ); // PC kills enemy worm.    
    g_drink_sound = g_sound_system->newSound( "sounds/drink.wav" ); // Drink or eat something to increase PC hitpoint.
    g_pcdamage_sound = g_sound_system->newSound( "sounds/hurt.wav"); // PC get damaged. (expect Male/Female versions)
    g_wormbite_sound = g_sound_system->newSound( "sounds/wormbite.wav" ); // An enemy worm bites a PC.
    g_craft_sound = g_sound_system->newSound( "sounds/craft.wav" ); // Player successfully crafted an item from other items.
    g_researchdone_sound = g_sound_system->newSound( "sounds/researchdone.wav" ); // Get a flag and achieve a milestone.
    g_sig_explosion_sound = g_sound_system->newSound( "sounds/sig_explosion.wav" ); // Fast and small missile "SIG" hit and destroy a wall.
    g_shoot_sig_sound = g_sound_system->newSound( "sounds/shoot_sig.wav" ); // Enemy shoots a fast and small missile SIG.
    g_hitspario_sound = g_sound_system->newSound( "sounds/enemyspariohit.wav", 0.05, false  ); // Enemy beam hits on hard things like a wall.
    g_beamhithard_sound = g_sound_system->newSound( "sounds/beamhithard.wav", 0.2, false ); // PC beam hits on hard things like a wall.
    g_eye_repair_sound = g_sound_system->newSound( "sounds/eye_repair.wav", 0.1, false ); // An enemy "Builder" repairs enemy fortress eye.
    g_repair_done_sound = g_sound_system->newSound( "sounds/repair_done.wav" ); // An enemy "Builder" finishes building enemy building.
    g_wipe_fortress_sound = g_sound_system->newSound( "sounds/yulab_maou.wav" ); // PC destroyed a fortress. 
    g_sandwalk_sound = g_sound_system->newSound( "sounds/sandwalk.wav", 0.3,false ); // PC walk slow on sand
    g_steponbrick_sound = g_sound_system->newSound( "sounds/steponbrick.wav", 0.3,false ); // PC walk faster on brick panel. (2 times/sec)
    g_shield_sound = g_sound_system->newSound( "sounds/shield.wav" ); // PC blocks enemy attack with energy shield.
    g_volcano_ready_sound = g_sound_system->newSound( "sounds/volcano_ready.wav" ); // Volcano is ready to erupt. (2 times/sec)
    g_volcano_explode_sound = g_sound_system->newSound( "sounds/volcano_explode.wav" ); //  Volcano erupts.
    g_fbshoot_sound = g_sound_system->newSound( "sounds/fbshoot.wav" ); // Enemy shoot a fire-ball.
    g_cutweed_sound = g_sound_system->newSound( "sounds/cutweed.wav" ); // Cutting a weed on the ground.
    g_beestart_sound = g_sound_system->newSound( "sounds/beestart.wav" ); // Activating harmful nest of bees
    g_switch_sound = g_sound_system->newSound( "sounds/switch.wav" ); // Turn on or off the switches in enemy fortress.
    g_core_sound = g_sound_system->newSound( "sounds/core.wav" ); // Enemy core(boss) is active (0.5times)
    g_switch_off_sound = g_sound_system->newSound( "sounds/switchoff.wav", 0.3, false );   // Turn off enemy laser turrets.
    g_fall_sound = g_sound_system->newSound( "sounds/fall.wav" ); // Items fall into a hole on the ground.
    g_lowhp_sound = g_sound_system->newSound( "sounds/lowhp.wav", 0.5, false ); // Alarm buzzer on PC low health (1 time/sec)
    g_cursormove_sound = g_sound_system->newSound( "sounds/cursormove.wav", 0.5, false ); // short clicky sound when GUI cursor moves
    g_cant_sound = g_sound_system->newSound( "sounds/cant.wav", 1, false );  // cannot do something (GUI).
    g_warp_sound = g_sound_system->newSound( "sounds/warp.wav", 1, false );  // during warping.
    g_put_sound = g_sound_system->newSound( "sounds/put.wav", 1, false );  // put blocks.
    g_windowopen_sound = g_sound_system->newSound( "sounds/windowopen.wav", 1, false );  // window opens.
    g_getflag_sound = g_sound_system->newSound( "sounds/getflag.wav", 1, false );  // get flag.
    g_all_clear_sound = g_sound_system->newSound( "sounds/congratulation.wav", 1, false );  // game all cleared
    g_beep_sounds[0] = g_sound_system->newSound( "sounds/beep1.wav", 1, false );  // 
    g_beep_sounds[1] = g_sound_system->newSound( "sounds/beep2.wav", 1, false );  // 
    g_beep_sounds[2] = g_sound_system->newSound( "sounds/beep3.wav", 1, false );  // 
    g_beep_sounds[3] = g_sound_system->newSound( "sounds/beep4.wav", 1, false );  // 
    g_shootsnow_sound = g_sound_system->newSound( "sounds/shootsnow.wav", 1, false ); // slime shoot snowball
    g_slimejump_sound = g_sound_system->newSound( "sounds/slimejump.wav", 1, false ); // slime jumps
    g_shoottornado_sound = g_sound_system->newSound( "sounds/shoottornado.wav", 1, false ); // bird shoot tornado
    g_birdfly_sound = g_sound_system->newSound( "sounds/birdfly.wav", 1, false ); // bird flies
    g_abandon_sound = g_sound_system->newSound( "sounds/abandon.wav", 1, false ); // abandon item
    
    // moyai
    g_moyai_client = new MoyaiClient();

    g_world_viewport = new Viewport();
    g_world_viewport->setSize(SCRW,SCRH);
    g_world_viewport->setScale2D(SCRW,SCRH);

    g_hud_viewport = new Viewport();
    g_hud_viewport->setSize(SCRW,SCRH);
    g_hud_viewport->setScale2D(SCRW,SCRH);
    
    g_ground_layer = new Layer();
    g_electricity_layer = new Layer();    
    g_fluid_layer = new Layer();
    g_block_layer = new Layer();
    g_roof_layer = new Layer();
    g_flyer_layer = new Layer();
    g_char_layer = new Layer();
    g_effect_layer = new Layer();
    g_durbar_layer = new Layer();
    g_cursor_layer = new Layer();
    g_info_layer = new Layer();
    g_belt_layer = new Layer();
    g_gauge_layer = new Layer();
    g_hud_layer = new Layer();
    g_msg_layer = new Layer();
    g_hints_layer = new Layer();
    g_wm_layer = new Layer();
    g_debug_layer = new Layer();

    g_ground_layer->setViewport(g_world_viewport);
    g_electricity_layer->setViewport(g_world_viewport);    
    g_fluid_layer->setViewport(g_world_viewport);
    g_block_layer->setViewport(g_world_viewport);
    g_roof_layer->setViewport(g_world_viewport);
    g_flyer_layer->setViewport(g_world_viewport);
    g_char_layer->setViewport(g_world_viewport);
    g_effect_layer->setViewport(g_world_viewport);
    g_durbar_layer->setViewport(g_world_viewport);
    g_cursor_layer->setViewport(g_world_viewport);
    g_info_layer->setViewport(g_world_viewport);
    g_belt_layer->setViewport(g_hud_viewport);
    g_gauge_layer->setViewport(g_hud_viewport);
    g_hud_layer->setViewport(g_hud_viewport);
    g_msg_layer->setViewport(g_hud_viewport);
    g_hints_layer->setViewport(g_hud_viewport);
    g_wm_layer->setViewport(g_hud_viewport);
    g_debug_layer->setViewport(g_hud_viewport);

    // 描画順を決定
    g_moyai_client->insertLayer(g_ground_layer);
    g_moyai_client->insertLayer(g_electricity_layer);    
    g_moyai_client->insertLayer(g_fluid_layer);
    g_moyai_client->insertLayer(g_block_layer);
    g_moyai_client->insertLayer(g_char_layer);    
    g_moyai_client->insertLayer(g_roof_layer);
    g_moyai_client->insertLayer(g_flyer_layer);
    g_moyai_client->insertLayer(g_effect_layer);    
    g_moyai_client->insertLayer(g_durbar_layer);
    g_moyai_client->insertLayer(g_cursor_layer);
    g_moyai_client->insertLayer(g_info_layer);
    g_moyai_client->insertLayer(g_gauge_layer);
    g_moyai_client->insertLayer(g_hints_layer);    
    g_moyai_client->insertLayer(g_belt_layer);
    g_moyai_client->insertLayer(g_hud_layer);
    g_moyai_client->insertLayer(g_msg_layer);
    g_moyai_client->insertLayer(g_wm_layer);
    g_moyai_client->insertLayer(g_debug_layer);

    
    g_world_camera = new Camera();
    g_world_camera->setLoc(0,0);
    g_hud_camera = new Camera();
    g_hud_camera->setLoc(0,0);    

    g_ground_layer->setCamera(g_world_camera);
    g_electricity_layer->setCamera(g_world_camera);
    g_fluid_layer->setCamera(g_world_camera);
    g_block_layer->setCamera(g_world_camera);
    g_roof_layer->setCamera(g_world_camera);
    g_flyer_layer->setCamera(g_world_camera);
    g_char_layer->setCamera( g_world_camera);
    g_effect_layer->setCamera( g_world_camera);
    g_durbar_layer->setCamera( g_world_camera);        
    g_cursor_layer->setCamera( g_world_camera);
    g_info_layer->setCamera( g_world_camera);
    g_belt_layer->setCamera( g_hud_camera);
    g_gauge_layer->setCamera( g_hud_camera);
    g_hud_layer->setCamera( g_hud_camera);
    g_msg_layer->setCamera( g_hud_camera);
    g_hints_layer->setCamera( g_hud_camera );
    g_wm_layer->setCamera( g_hud_camera );
    g_debug_layer->setCamera( g_hud_camera );

    // tex
    print("loading texture");
    Texture *t;
    t  = new Texture();
    t->load("./images/ssbase1024.png");
    t->setLinearMinFilter(); // Don't set magfilter (default uses NEAREST)
    g_base_deck = new TileDeck();
    g_base_deck->setTexture(t);
    g_base_deck->setSize(32,32, 24,24 );

    
    t = new Texture();
    t->load( "./images/asciibase256.png" );
    g_ascii_deck = new TileDeck();
    g_ascii_deck->setTexture(t);
    g_ascii_deck->setSize( 32,32, 8,8 );

    t = new Texture();
    t->load( "./images/girev64.png" );
    g_girev_deck = new TileDeck();
    g_girev_deck->setTexture(t);
    g_girev_deck->setSize(1,1,64,64);

    g_title_tex = new Texture();
    g_title_tex->load( "./images/planet.png" );
    g_logo_tex = new Texture();
    g_logo_tex->load( "./images/sslogo400.png" );
   

    // anim
#define ANIMSET3(tgt,dur,loop,a1,a2,a3) { int inds[] = {a1,a2,a3}; tgt = new AnimCurve(dur,loop,inds,elementof(inds)); }
#define ANIMSET4(tgt,dur,loop,a1,a2,a3,a4) { int inds[] = {a1,a2,a3,a4}; tgt = new AnimCurve(dur,loop,inds,elementof(inds)); }

    
    //    g_digsmoke_curve = new AnimCurve( 0.1, false, (int[]){ B_ATLAS_DIGSMOKE_BASE, B_ATLAS_DIGSMOKE_BASE+1, B_ATLAS_DIGSMOKE_BASE+2},3 );
    //    g_spark_curve = new AnimCurve( 0.05, true, (int[]){ B_ATLAS_SPARK_BASE,B_ATLAS_SPARK_BASE+1,B_ATLAS_SPARK_BASE+2,B_ATLAS_SPARK_BASE+3},4);
    ANIMSET3( g_digsmoke_curve, 0.1, false, B_ATLAS_DIGSMOKE_BASE, B_ATLAS_DIGSMOKE_BASE+1, B_ATLAS_DIGSMOKE_BASE+2 );
    ANIMSET4( g_spark_curve, 0.05, true, B_ATLAS_SPARK_BASE,B_ATLAS_SPARK_BASE+1,B_ATLAS_SPARK_BASE+2,B_ATLAS_SPARK_BASE+3 );
    ANIMSET3( g_cellbubble_curve, 0.2, true, B_ATLAS_CELL_BUBBLE_BASE, B_ATLAS_CELL_BUBBLE_BASE+1, B_ATLAS_CELL_BUBBLE_BASE+2 );


    // アイの色
    g_eye_col_replacer = new ColorReplacerShader();
    if(!g_eye_col_replacer->init()) {
        print("can't initialize shader");
        exit(0);
    }
    
    // generated tex
    g_durbar_deck = new TileDeck();
    setupDurBarImage( g_durbar_deck, Color(1,0,0,1), Color(1,1,0,1) );
    g_enebar_deck = new TileDeck();
    setupDurBarImage( g_enebar_deck, Color(0,0.2,0.1,1), Color(0,1,0,1) );


    //////////////////////

    g_hp_gauge = new Gauge( "HP", Vec2(-SCRW/2+32,-SCRH/2+32), Color(1,0,0,1), 8 );
    g_hp_gauge->update( 50, 100 );
    g_elec_gauge = new Gauge( "E", Vec2(-SCRW/2+32+350, -SCRH/2+32), Color(0,0.7,0,1), 12 );
    g_elec_gauge->update( 100, 1000 );
    g_armor_gauge = new Gauge( "SHIELD", Vec2(-SCRW/2+32,-SCRH/2+32+48), Color(1,0,1,1), 8 );
    g_armor_gauge->update( 100,1000 );

    //////////////////////

    setupItemConf();

    g_log = new Log( Vec2( -SCRW/2+110,-SCRH/2+120), 12 );

    g_belt = new Belt();
    g_suit = new Suit();
    g_flagarrow = new FlagArrow();
    
    g_fld = new Field( FIELD_W, FIELD_H );
    g_fld->clear();
    //    g_fld->generate();
    g_fsaver = new FieldSaver(g_fld, CHUNKSZ );
    

    g_mapview = new MapView( g_fld->width, g_fld->height );

    
    Vec2 pc_at = pos2ToVec2(g_fld->getRespawnPoint());
    g_pc = new LocalPC( pc_at );


#if 0
    PC *npc = new PC( pc_at + Vec2(-10,-20) );
    npc->incItemIndex( PC_ITEM_NUM - 1, ITT_BATTERY, 1 );    
    npc->incItem( ITT_BEAMGUN1, 1 );
    npc->enable_ai = true;

    print("PCid:%d NPCid:%d", g_pc->id, npc->id );

#endif

    g_flag = new Flag();
    
    g_ps = new PowerSystem();

    g_depo = new ResourceDeposit();
        
    ////////////////

    setupGridCursor();

    setupTitleBG();
    
    g_invwin = new Inventory();
    g_invwin->hide();

    g_labwin = new LabWindow();
    g_labwin->hide();

    g_wm = new WorldMap();
    g_wm->hide();

    g_exwin = new ExchangeWindow();
    g_exwin->hide();

    g_portalwin = new PortalWindow();
    g_portalwin->hide();

    g_charmakewin = new CharMakeWindow();
    g_charmakewin->hide();

    g_charnamewin = new CharNameWindow();
    g_charnamewin->hide();

    g_projtypewin = new ProjectTypeWindow();
    g_projtypewin->hide();

    g_projlistwin = new ProjectListWindow();
    g_projlistwin->hide();    

    g_projinfowin = new ProjectInfoWindow();
    g_projinfowin->hide();

    g_spmenuwin = new SpecialMenuWindow();
    g_spmenuwin->hide();

    g_followwin = new FollowWindow();
    g_followwin->hide();

    g_titlewin = new TitleWindow();
    g_titlewin->hide();

    g_creditwin = new CreditWindow();
    g_creditwin->hide();
    
    g_ranktypewin = new RankTypeWindow();
    g_ranktypewin->hide();

    g_rankwin = new RankingWindow();
    g_rankwin->hide();

    g_friendwin = new FriendWindow();
    g_friendwin->hide();

    g_debugwin = new DebugWindow();
    g_debugwin->hide();

    g_msgwin = new MessageWindow();
    g_msgwin->hide();


    g_milestone_display = new MilestoneDisplay();
    g_milestone_display->updateMilestone( 0 );

    g_stateline = new CharGridTextBox( 200 );
    g_stateline->setScl(8);
    g_stateline->setLoc( Vec2( -SCRW/2+8, SCRH/2-36 ) );
    g_debug_layer->insertProp( g_stateline );

    g_nicktb = new CharGridTextBox(32);
    g_nicktb->setScl(16);
    g_nicktb->setLoc( Vec2(SCRW/2-500, SCRH/2-20));
    g_nicktb->setString(WHITE,"");
    g_hud_layer->insertProp(g_nicktb);

    g_powergrid_state = new CharGridTextBox( 20 );
    g_powergrid_state->setScl(8);
    g_powergrid_state->setString(WHITE, "(not set)");
    g_powergrid_state->setVisible(false);
    g_effect_layer->insertProp(g_powergrid_state);

    setupPartyIndicator();

    
    //////////////////
#ifdef WIN32    
	QueryPerformanceFrequency(&g_perfFreq);
#endif

    if( g_enable_network ) {
        g_network_initialized = initNetwork( g_dbhost, 22222, g_rthost, 22223 );
        if(!g_network_initialized) {
            networkFatalError( "CAN'T CONNECT TO SERVER" );
        }
    } else {
        g_titlebg_prop->setVisible(false);
    }
    
    static double last_time = 0;    
    while(g_program_finished==false){
        updateGame();
        double now_time = now();
        double dt = now_time-last_time;
        double ideal_step_time = 1.0f / (double)g_ideal_frame_rate;
        double lasttimes[30];
        lasttimes[g_total_frame_count % elementof(lasttimes) ] = dt;
        double avg,tot=0;
        for(int i=0;i<elementof(lasttimes);i++) tot+=lasttimes[i];
        avg = tot / elementof(lasttimes);

        static double to_sleep = 0;        
        if( avg > ideal_step_time ) {
            // Slower than ideal
            to_sleep -= 0.0001;
        } else {
            to_sleep += 0.0001;
        }
        //        print("hoge:%f avg:%f idl:%f tot:%f", to_sleep, avg, ideal_step_time, tot );
        last_time = now();
        if( to_sleep > 0 ) glfwSleep(to_sleep);
        
     }
    saveExitProgram(0);

    print("program finished");    

    return 0;
}

#ifdef WIN32
void GenerateDump(EXCEPTION_POINTERS* pExceptionPointers, char * path, int length)
{
	// Executable name.
	GetModuleFileNameA(NULL, path, length);

	size_t nameLen = strlen(path);
	if (nameLen >= 4 &&  _stricmp (path+nameLen-4, ".exe") == 0)
	{
		path[nameLen-4] = 0;
	}

	// Add time stamp and process id.
	char timeStamp[64];
	SYSTEMTIME stLocalTime;
	GetLocalTime( &stLocalTime );
	sprintf_s (timeStamp, "_%04d%02d%02d%02d%02d%02d_%ld", stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay, stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond, GetCurrentProcessId());
	strcat_s (path, length, timeStamp);
	strcat_s (path, length, ".mdmp");

	HANDLE hDumpFile = CreateFileA(path, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_WRITE|FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

	if (hDumpFile != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION ExpParam;
		ExpParam.ThreadId = GetCurrentThreadId();
		ExpParam.ExceptionPointers = pExceptionPointers;
		ExpParam.ClientPointers = TRUE;

		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpWithDataSegs, &ExpParam, NULL, NULL);
		CloseHandle (hDumpFile);
	}
}

long WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS* p_exceptions)
{
	char path[MAX_PATH];
	GenerateDump(p_exceptions, path, MAX_PATH);

	// Throw any and all exceptions to the ground.
	return EXCEPTION_EXECUTE_HANDLER;
}

//int _tmain( int argc, _TCHAR* argv[] ) {
//int main( int argc, char **argv ) 
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{
	SetUnhandledExceptionFilter(UnhandledExceptionHandler);

	int result = moaiMain(__argc, __argv);
	return result;
}
#endif // #ifdef WIN32

#if defined(__APPLE__)
int main( int argc, char **argv ) {
	return moaiMain(argc,argv);
}
#endif
