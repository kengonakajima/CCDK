#ifndef _GLOBALS_H_
#define _GLOBALS_H_


class Field;
class MapView;
class PC;
class LocalPC;
class Gauge;
class Belt;
class Suit;
class Inventory;
class PowerSystem;
class WorldMap;
class ExchangeWindow;
class Flag;
class MilestoneDisplay;
class FlagArrow;
class FieldSaver;

extern Layer *g_char_layer;
extern Layer *g_ground_layer;
extern Layer *g_electricity_layer;
extern Layer *g_fluid_layer;
extern Layer *g_block_layer;
extern Layer *g_roof_layer;
extern Layer *g_flyer_layer;
extern Layer *g_durbar_layer;
extern Layer *g_cursor_layer;
extern Layer *g_effect_layer;
extern Layer *g_info_layer;
extern Layer *g_belt_layer;
extern Layer *g_gauge_layer;
extern Layer *g_hud_layer;
extern Layer *g_msg_layer;
extern Layer *g_hints_layer;
extern Layer *g_wm_layer;

extern TileDeck *g_base_deck;
extern TileDeck *g_durbar_deck;
extern TileDeck *g_enebar_deck;
extern TileDeck *g_ascii_deck;
extern TileDeck *g_girev_deck;

extern Field *g_fld;

extern FlagArrow *g_flagarrow;

extern MapView *g_mapview;
extern LocalPC *g_pc;
extern Flag *g_flag;
extern MilestoneDisplay *g_milestone_display;

extern Sound *g_step_sound;
extern Sound *g_bgm_sound;
extern Sound *g_dig_sound;
extern Sound *g_gamestart_sound;
extern Sound *g_spark_sound;
extern Sound *g_killcreature_sound;
extern Sound *g_killmachine_sound;
extern Sound *g_beamhitwall_sound;
extern Sound *g_shoot_sound[PC_SHOOT_SOUND_CANDIDATE_NUM];
extern Sound *g_pickup_sound;
extern Sound *g_blaster_explode_sound;
extern Sound *g_destroy_building_sound;
extern Sound *g_wormdamage_sound;
extern Sound *g_drink_sound;
extern Sound *g_pcdamage_sound;
extern Sound *g_wormbite_sound;
extern Sound *g_craft_sound;
extern Sound *g_researchdone_sound;
extern Sound *g_sig_explosion_sound;
extern Sound *g_shoot_sig_sound;
extern Sound *g_hitspario_sound;
extern Sound *g_beamhithard_sound;
extern Sound *g_eye_repair_sound;
extern Sound *g_repair_done_sound;
extern Sound *g_wipe_fortress_sound;
extern Sound *g_sandwalk_sound;
extern Sound *g_steponbrick_sound;
extern Sound *g_shield_sound;
extern Sound *g_volcano_ready_sound;
extern Sound *g_volcano_explode_sound;
extern Sound *g_fbshoot_sound;
extern Sound *g_cutweed_sound;
extern Sound *g_beestart_sound;
extern Sound *g_switch_sound;
extern Sound *g_core_sound;
extern Sound *g_switch_off_sound;
extern Sound *g_fall_sound;
extern Sound *g_lowhp_sound;
extern Sound *g_cursormove_sound;
extern Sound *g_cant_sound;
extern Sound *g_warp_sound;
extern Sound *g_put_sound;
extern Sound *g_windowopen_sound;
extern Sound *g_getflag_sound;
extern Sound *g_wormkill_sound;
extern Sound *g_all_clear_sound;
extern Sound *g_beep_sounds[4];
extern Sound *g_shootsnow_sound;
extern Sound *g_slimejump_sound;
extern Sound *g_shoottornado_sound;
extern Sound *g_birdfly_sound;
extern Sound *g_abandon_sound;

extern AnimCurve *g_digsmoke_curve;
extern AnimCurve *g_spark_curve;
extern AnimCurve *g_cellbubble_curve;

extern ColorReplacerShader *g_eye_col_replacer;

extern Font *g_font;
extern Camera *g_world_camera;
extern Camera *g_hud_camera;
extern float g_zoom_rate;

extern PowerSystem *g_ps;

extern Gauge *g_hp_gauge;
extern Gauge *g_elec_gauge;
extern Gauge *g_armor_gauge;

extern Belt *g_belt;
extern Suit *g_suit;

extern double g_now_time;

extern Inventory *g_invwin;
extern ExchangeWindow *g_exwin;
class LabWindow;
extern LabWindow *g_labwin;

extern RUNSTATE g_runstate;

#define WHITE Color(1,1,1,1)
#define RED Color(1,0,0,1)

extern WorldMap *g_wm;

bool isWindowVisible();
void toggleWindow( const char *name );

extern FieldSaver *g_fsaver;

extern int g_user_id;

class CharNameWindow;
extern CharNameWindow *g_charnamewin;
class ProjectInfoWindow;
extern ProjectInfoWindow *g_projinfowin;
class ProjectListWindow;
extern ProjectListWindow *g_projlistwin;
class ProjectTypeWindow;
extern ProjectTypeWindow *g_projtypewin;
class CreditWindow;
extern CreditWindow *g_creditwin;

extern int g_current_project_id;

extern SoundSystem *g_sound_system;

void soundPlayAt( Sound *snd, Vec2 at, float distance_rate, bool to_distribute = true );
extern void exit_program(int code);
extern bool g_program_finished;

class Log;
extern Log *g_log;

void pollRenderMoyai();

class CharGridTextBox;
extern CharGridTextBox *g_powergrid_state;

class ResourceDeposit;
extern ResourceDeposit *g_depo;

void clearNormalCharacters();

class FollowWindow;
extern FollowWindow *g_followwin;

extern bool g_enable_network;

void stopSituationBGM() ;
void pauseSituationBGM( bool to_pause ) ;

extern bool g_game_paused;

class MessageWindow;
extern MessageWindow *g_msgwin;

class TitleWindow;
extern TitleWindow *g_titlewin;

class RankTypeWindow;
extern RankTypeWindow *g_ranktypewin;

class RankingWindow;
extern RankingWindow *g_rankwin;

extern bool g_enable_debug_menu;
extern bool g_enable_autoplay;
extern bool g_enable_infinity_items;

class DebugWindow;
extern DebugWindow *g_debugwin;

extern float g_pc_walk_speed_accel;
extern bool g_enable_auto_charge;

class FriendWindow;
extern FriendWindow *g_friendwin;

#ifdef NDEBUG
#undef assert
#define assert(x) if(!(x)){ ::print("ASSERT! STOPPING PROGRAM EVEN IN RELASE BUILD."); int *p=0; *p=123; }
#endif

void updateNickTB();
void shorterInteger( char *out, size_t outsz, unsigned long long i ) ;

extern int g_ideal_frame_rate;

extern long long g_last_ping_rtt_usec;
extern bool g_skip_projectinfowin_worldmap;
extern bool g_enable_nearcast_log;

void setTitleScreenVisible( bool imgvis, bool logovis );

#endif
