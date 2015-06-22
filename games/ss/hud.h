#ifndef _HUD_H_
#define _HUD_H_


// One line Grid based text box, no font
class CharGridTextBox : public Prop2D {
public:
    Grid *bg;
    CharGrid *cg;
    CharGridTextBox( int w );
    void setString( Color c, const char *s );
    bool isEqual( const char *s, int l );
    bool isEmpty();
    void clear() { cg->clear(); }
    void fillBG( int ind );
    void setColor(Color c) { cg->fillColor(c); }
};

// One line input.
class CharGridTextInputBox : public CharGridTextBox {
public:
    
    char s[128]; // null terminated
    char cursor_char;
    bool show_cursor;
    
    CharGridTextInputBox( int w );
    void appendChar( char ch );
    void deleteTailChar();
    char *get() { return s; }
    virtual bool prop2DPoll(double dt);
    
};

// Multiple lines with scrolling.
class CharGridTextArea : public Prop2D  {
public:
    Grid *bg;
    CharGrid *cg;
    int to_write; // where to write? (from the top)
    CharGridTextArea( int w, int h );
    void clear();
    void writeLine( Color c, const char *s );
    void fillBG( int ind );
    void scrollUp();
};


class ItemPanel : public Prop2D {
public:
    Prop2D *item_prop;

    Prop2D *num_prop;
    CharGrid *num_grid;
    CharGrid *cap_grid;
    Prop2D *cap_prop;
    Prop2D *dur_prop;
    bool show_durbar;
    ItemPanel( bool show_durbar );
    void update( Item *itm );
    void setItemDirect( TileDeck *deck, int index, float scl );
    void setSelected(bool selected );
    void spikeEffect(){
        if(item_prop){
            item_prop->setScl(96,96);
            item_prop->seekScl(48,48,0.2);
        }
    }
    void hide() { toggle(false); }
    void show() { toggle(true); }
    void toggle( bool visibility );
    void setCaption( Color c, const char *s );
};



class Belt {
public:
    ItemPanel *items[PC_BELT_NUM];

    Belt();
    void update();
    void spikeEffect(int ind);
    static Vec2 getPosByIndex( int ind );
};

class Suit {
public:
    ItemPanel *items[PC_SUIT_NUM];

    Suit();
    void update();
    static Vec2 getPosByIndex( int ind );
};



class PropGroup {
public:
    Prop2D **props;
    int num;
    PropGroup( int num );
    ~PropGroup();
    int reg( Prop2D *p );
    Prop2D *find(int id);
    void clear();
    Prop2D *find( Vec2 lc );
    Prop2D *getNext( int cur_id, DIR dir );
    Prop2D *getNext( Prop2D *from, DIR dir );
    Prop2D *getNearest(Vec2 at, Prop2D *except );
    void getMinMax( Vec2 *min, Vec2 *max );
    bool isBottom( Prop2D *to_test );
    bool isTop( Prop2D *to_test );    
    Prop2D *getTop();
    Prop2D *getBottom();    
};


// Only blinking
class Blinker : public Prop2D {
public:
    double blink_interval;
    double last_blink_at;
    Blinker() : Prop2D(), blink_interval(1), last_blink_at(0) {
    }
    void startBlink(double intvl){ blink_interval = intvl; last_blink_at = accum_time; setColor(1,1,1,1); }
    void stopBlink(){
        blink_interval = 0;
    }
    void ensureVisible() {
        last_blink_at = accum_time;
        setColor(1,1,1,1);
    }
    virtual bool blinkerPoll(double dt){return true;}
    virtual bool prop2DPoll(double dt){
        if( blink_interval > 0 ){
            if( accum_time > last_blink_at + blink_interval ){
                if(color.r > 0 ) {
                    setColor(0,0,0,0);
                } else  {
                    setColor(1,1,1,1);
                }

                last_blink_at = accum_time;
            }
        } 
        if(!blinkerPoll(dt)){
            return false;
        }
        return true;
    }
};

class Gauge : public Prop2D {
public:
    char name[16];
    Grid *base_grid;
    Blinker *bar_prop;
    Prop2D *cap_prop;
    CharGrid *cap_grid;
    Color color;
    Vec2 loc;
    int grid_len;
    float high_thres_rate;
    Color high_color;
    float low_thres_rate;
    

    Gauge( const char *nm, Vec2 lc, Color c, int glen );
    void update( int cur_num, int max_num );
    void setThresColorHigh( float rate, Color c );
    void setLowThres(float rate);
    void toggle(bool visibility);
    
};


// One icon and a number 
class PictLabel : public Prop2D {
public:
    Prop2D *num_prop;
    CharGrid *num_grid;
    Vec2 loc;
    PictLabel( Vec2 lc, int pict_index );
    void setNumber( int n);
    void setString( const char *s );
    void spikeEffect(float scl){
        setScl(scl*2,scl*2);
        seekScl(scl,scl,0.2);
    }
    void toggle( bool flg ) {
        Prop2D::setVisible(flg);
        num_prop->setVisible(flg);
    }
    void setNumColor( Color c ) {
        num_grid->fillColor(c);
    }
    
};




class GridCursor : public Prop2D {
public:
    Prop2D *attachment;
    GridCursor();
    Prop2D *ensureAttachment( Vec2 at, TileDeck *dk, int index );
    void clearAttachment();
    void setBright(bool bright);
    bool prop2DPoll(double dt);
};

void setupGridWindowFrame( Grid *g, int w, int h, int base_frame_index, float alpha );

class Window : public Prop2D {
public:
    Grid *bg;
    Window( int w, int h, int base_frame_index, float alpha = 1 );

    virtual bool prop2DPoll(double dt);
    virtual bool windowPoll(double dt){ return true; };
    virtual void toggle( bool visibility ) {}
    void hide() { toggle(false); }
    void show() { toggle(true); }
    void moveCursor( DIR dir ) {}
    void selectAtCursor() {}
    Vec2 getTitleLoc() { return loc + Vec2(30,bg->height*24-36); }
    Vec2 getHeaderLoc() { return getTitleLoc() + Vec2(40,-80); }
};


// OUTPUT       INPUT
// [output item]     E 50 [ING0]x5   [ING1]x3  [ING2]x2   [MAKE]
typedef enum {
    CRAFTOPTMODE_CRAFTABLE = 0,
    CRAFTOPTMODE_LOCKED = 1,
    CRAFTOPTMODE_ONRESEARCH = 2,
    CRAFTOPTMODE_RESEARCHDONE = 3,
    CRAFTOPTMODE_EMPTY = 4, 
} CRAFTOPTMODE;

class CraftOption : public ItemPanel {
public:
    PictLabel *ene;
    PictLabel *ingrs[RESEARCH_INGREDIENT_NUM];
    ItemConf *itc;
    CRAFTOPTMODE mode;
    
    CraftOption( ItemConf *itc, Vec2 lc, CRAFTOPTMODE mode, Layer *l );
    void toggle(bool visibility);
    void update( ItemConf *itc, CRAFTOPTMODE mode );
    void updateColor( CRAFTOPTMODE mode );
    void updateResearchIngr();
    void setIngredientNum( int i, int cur, int n );
    void clearIngrs();
};

class Inventory : public Window {
public:
    int cursor_at; // -1: no cursor shown
    int craft_cursor_at; // -1: nothing selected
    int selected_at; // -1: nothing selected
    ItemPanel *items[PC_ITEM_NUM];
    PropGroup *group, *craftgroup;
    CharGridTextBox *craft_caption;
    static const int CRAFT_PER_PAGE = 10;
    CraftOption *crafts[CRAFT_PER_PAGE];
    CharGridTextBox *itemname_caption;
    int craftpage;
    
    Inventory();
    virtual void toggle(bool vis);
    void update();    
    virtual bool prop2DPoll( double dt );

    virtual void moveCursor( DIR dir );
    virtual void selectAtCursor();
    void spikeEffect( int ind );
};

class BlinkCursor : public Blinker {
public:
    BlinkCursor( DIR dir = DIR_UP );
};


class LabWindow : public Window {
public:
    int cursor_at;
    static const int RESEARCH_PER_PAGE = 8;
    CraftOption *opts[RESEARCH_PER_PAGE];
    PropGroup *group;
    CharGridTextBox *title_tb;
    CharGridTextBox *itemname_caption;
    CharGridTextBox *state_caption;
    BlinkCursor *cursor;
    int cursor_id_at; // ID of prop2d. not an index.
    PictLabel *item_left; // 残り数
    int page;
    double research_lock_obtained_at;
    
    LabWindow();
    virtual void toggle(bool vis);
    void toggleContent(bool vis);
    void update();

    virtual void moveCursor(DIR dir);
    virtual void selectAtCursor();
    void poll();

    void updateGroup();
    void updateCursor();

    void onLockObtained();
    void onLockReleased();

    void updateCaption();
    void cancel() { hide(); }
};

class FacePanel : public Prop2D {
public:
    Prop2D *face_prop;
    Prop2D *hair_prop;
    FacePanel();
    void update( int faceind, int hairind );
    void setScl(float s);
    bool prop2DPoll( double dt );
};

typedef enum {
    WMIND_LOCALPC = 1,
    WMIND_FLAG = 2,
    WMIND_PC = 3,
    WMIND_MOTHERSHIP = 4,
} WMINDTYPE;

class WorldMapIndicator : public Prop2D {
public:
    WMINDTYPE type;
    FacePanel *face;    
    int client_id;
    char nickname[PC_NICKNAME_MAXLEN+1];
    CharGridTextBox *name_tb;
    double last_packet_at;
    Pos2 pos_hint;
    WorldMapIndicator( WMINDTYPE t );
    virtual bool prop2DPoll( double dt );
    void setVisible(bool vis);
    virtual void onDelete() {
        if(face) face->to_clean = true;
        if(name_tb) name_tb->to_clean = true;
    }
};

class WorldMap : public Prop2D {
public:
    Image *img;
    Texture *tex;
    WorldMapIndicator *localpc, *flag, *mothership;
    WorldMapIndicator *remotepcs[128];
    CharGridTextBox *playtime_tb;
    
    WorldMap();
    virtual void toggle(bool vis);
    void show() { toggle(true); };
    void hide() { toggle(false); };
    void update();
    void ensureRemotePC( int client_id, const char *nickname, int face_ind, int hair_ind, Vec2 loc );
    void updatePlayTime();
    virtual bool prop2DPoll( double dt );
};


class ExchangeWindow : public Window {
public:
    ItemPanel *pcitems[PC_ITEM_NUM];
    ItemPanel *storeditems[DEPOSIT_ITEM_NUM];
    int cursor_id_at; // ID, not an index. Because this have 2 arrays.
    PropGroup *group;
    CharGridTextBox *deposit_caption;
    CharGridTextBox *withdraw_caption;

    CharGridTextBox *title_tb;
    CharGridTextBox *state_caption; // Show "loading"
    double depo_lock_obtained_at;
    
    ExchangeWindow();
    virtual void toggle( bool vis );
    void toggleContent(bool vis);
    void update();
    void cancel();
    void poll();
    void updateCaption( int cur_ene, int req_ene );
    void onLockObtained();
    void onLockReleased();
    
    virtual void moveCursor(DIR dir);
    virtual void selectAtCursor();    
    
};

class PortalWindow : public Window {
public:
    ItemPanel *directions[4];
    int cursor_at;
    CharGridTextBox *title_tb;

    PortalWindow();
    virtual void toggle( bool vis );
    void update();
    void updateCaption( int cur_ene, int req_ene );
    void poll();
    void cancel() { hide(); }
    virtual void moveCursor( DIR dir );
    virtual void selectAtCursor();
};

class CharMakeWindow : public Window {
public:
    int cursor_id_at;
    CharGridTextBox *title_tb;
    Vec2 charloc;
    Prop2D *hair_prop;
    Prop2D *face_prop;
    Prop2D *body_prop;
    Prop2D *equip_prop;
    Prop2D *beam_prop;
    
    Prop2D *hair_cands[8];
    Prop2D *face_cands[8];
    Prop2D *body_cands[8];
    Prop2D *beam_cands[8];
    Prop2D *sound_cands[PC_SHOOT_SOUND_CANDIDATE_NUM];

    CharGridTextBox *hair_caption;
    CharGridTextBox *face_caption;
    CharGridTextBox *body_caption;
    CharGridTextBox *beam_caption;
    CharGridTextBox *sound_caption;
    CharGridTextBox *next_button;

    PropGroup *group;
    BlinkCursor *cursor;

    int selected_shoot_sound_index;
    
    CharMakeWindow();
    virtual void toggle(bool vis);
    void update();
    virtual void moveCursor( DIR dir );
    virtual void selectAtCursor();
    virtual bool windowPoll(double dt);
};

/////////////
class ProjectTypeWindow : public Window {
public:
    CharGridTextBox *title_tb;

    PictLabel *mine;
    PictLabel *shared;
    PictLabel *pub;
    CharGridTextBox *back_tb;

    int cursor_id_at;
    PropGroup *group;
    BlinkCursor *cursor;
    
    ProjectTypeWindow();
    virtual void toggle(bool vis);
    void update();

    virtual void moveCursor( DIR dir );
    virtual void selectAtCursor();
    void cancel();
};

typedef enum {
    PJT_MINE = 1,
    PJT_SHARED = 2,
    PJT_PUBLIC = 3,
} PROJECTTYPE;

typedef enum {
    PJLT_INVAL = 0,
    PJLT_PRIVATE = 1,
    PJLT_SHARED = 2,
    PJLT_PUBLIC = 3,    
} PROJECTLISTTYPE;

class ProjectListWindow : public Window {
public:
    CharGridTextBox *title_tb;
    CharGridTextBox *header;
    static const int LINE_NUM = 12;
    CharGridTextBox *lines[LINE_NUM];

    BlinkCursor *cursor;
    int cursor_at;
    int project_num;
    int project_id_cache[LINE_NUM];
    
    PROJECTLISTTYPE list_type;
    
    ProjectListWindow();
    virtual void toggle(bool vis);
    void update();
    void updateProjectList();    
    void clear();
    void poll();
    void cancel();
    virtual void moveCursor( DIR dir );
    virtual void selectAtCursor();

};


class ProjectInfoWindow : public Window {
public:
    CharGridTextBox *title_tb;
    CharGridTextBox *go_tb;
    CharGridTextBox *back_tb;
    CharGridTextBox *share_tb;
    CharGridTextBox *publish_tb;
    CharGridTextBox *delete_tb;

    
    BlinkCursor *cursor;
    int cursor_at_id;
    PropGroup *group;
    int project_id;
    Prop2D *map_prop;
    Texture *map_tex;
    Image *map_img;
    CharGridTextArea *info_ta;

    WorldMapIndicator *indicators[512];
    Window *previous_window;
    double last_poll_maxcon_at;
    
    ProjectInfoWindow();
    virtual void toggle(bool vis );
    
    virtual void moveCursor( DIR dir );    
    virtual void selectAtCursor();
    void cancel();
    void update();
    void updateCursor();    
    void setProjectId( int pjid) { project_id = pjid; }
    void updateParty( int userid, const char *nickname, Vec2 wloc, int hair_ind, int face_ind );
    void updateGoButton();
    void poll();
    void clearFlagIndicators();
    void insertFlagIndicator( Pos2 p );
    void showPreviousWindow();
};

class SoftwareKeyboardWindow : public Window {
public:
    Prop2D *keys[100];
    
    BlinkCursor *cursor;
    int cursor_at_id;
    PropGroup *group;

    CharGridTextInputBox *input_tb;
    CharGridTextBox *ok_tb;
    CharGridTextBox *del_tb;

    int input_min_len;
    
    void update();
    static const char chars[26+26+10+1];

    SoftwareKeyboardWindow();
    void setupSoftwareKeyboard( int minl, int maxl );
};

class CharNameWindow : public SoftwareKeyboardWindow {
public:
    CharGridTextBox *title_tb;
    CharNameWindow();
    virtual void toggle(bool vis);
    void moveCursor( DIR d );
    void selectAtCursor();
};

/////////////

class SpecialMenuWindow : public Window {
public:
    CharGridTextBox *options[10];
    BlinkCursor *cursor;
    int cursor_at;
    SpecialMenuWindow();
    void moveCursor( DIR d );
    void selectAtCursor();
    void setupMenu();
    void toggle(bool vis);
    void cancel() { hide(); }
    void update();
};


/////////////

class FollowWindow : public Window {
public:
    CharGridTextBox *title_tb;
    FacePanel *face;

    CharGridTextBox *ok_tb;
    CharGridTextBox *later_tb;
    BlinkCursor *cursor;
    int cursor_at;
    char nickname[PC_NICKNAME_MAXLEN+1];
    int user_id;
    int face_index,hair_index;
    FollowWindow();
    void toggle( bool vis);
    void cancel() { hide(); }
    void selectAtCursor();
    void moveCursor( DIR d );
    void update( int uid, const char *nick, int face, int hair );
    void updateCursor();
    
};
/////////////

class MessageWindow : public Window {
public:
    CharGridTextArea *msgarea;
    CharGridTextBox *ok_tb;      
    CharGridTextBox *cancel_tb;
    BlinkCursor *cursor;
    double show_at;
    void (*close_callback)(bool positive);
    int cursor_at;
    bool with_cancel;

    MessageWindow();
    void toggle( bool vis);
    void clear();
    void writeLine( Color c, const char *s );
    void selectAtCursor();
    void poll();
    void setCloseCallback( void (*cb)(bool positive) ) { close_callback = cb; };
    void moveCursor( DIR d);    
    void setWithCancel( bool wc ) { with_cancel = wc; };
    static const int DELAY_TIME = 3;
    void toggleDelay( bool to_delay );
    void updateCursor();
    void cancel();
};


///////////////

class MenuWindow : public Window {
public:
    CharGridTextBox *title_tb;
    CharGridTextBox *options[20];
    int cursor_at;
    BlinkCursor *cursor;
    
    MenuWindow( const char *titlestr, float alpha );
    void setMenuEntry( int ind, Color col, const char *s );
    void toggle( bool vis );
    void selectAtCursor();
    void moveCursor( DIR d );
    void updateCursor();
    virtual void onEntrySelected(int ind) {};
    void setOptionsLoc( Vec2 lefttop );
};

///////////

class TitleWindow : public MenuWindow {
public:
    TitleWindow();
    virtual void onEntrySelected(int ind);    
    void cancel();
};

///////////

class RankTypeWindow : public MenuWindow {
public:
    RankTypeWindow();
    virtual void onEntrySelected(int ind);        
    void cancel();
};

///////////
class DebugWindow : public MenuWindow {
public:
    DebugWindow();
    virtual void onEntrySelected(int ind);
    void cancel();
};


////////////
typedef enum {
    RANKTYPE_QUICKEST_ALL_CLEAR,
    RANKTYPE_MILESTONE_PROGRES_DAY,
} RANKTYPE;
#define MSG_RANKTYPE_QUICKEST_ALL_CLEAR "QUICKEST ALL-CLEAR"
#define MSG_RANKTYPE_MOST_MILESTONE_PROGRESS "MOST MILESTONE PROGRESS IN 24 HOURS"

class RankingWindow : public Window {
public:
    CharGridTextBox *title_tb;
    CharGridTextBox *header;
    CharGridTextBox *lines[16];
    BlinkCursor *cursor;
    int cursor_at;
    int project_id_cache[16];
    
    RankingWindow();
    void cancel();
    void toggle( bool vis );
    void updateByType( RANKTYPE r );
    void updateCursor();
    void moveCursor( DIR d);
    void selectAtCursor();
};

class FriendWindow : public Window {
public:
    CharGridTextBox *header;
    CharGridTextBox *names[SSPROTO_SHARE_MAX]; // show all friends in a screen
    FacePanel *faces[SSPROTO_SHARE_MAX];
    CharGridTextBox *back_tb;
    int cursor_at;
    BlinkCursor *cursor;
    Contact friends[SSPROTO_SHARE_MAX];//cache
    
    FriendWindow();
    void cancel();
    void toggle( bool vis );
    void update();
    void updateCursor();
    void moveCursor(DIR d);
    void selectAtCursor();
    
};
class CreditWindow : public Window {
public:
    CharGridTextBox *lines[16];
    int cur_page;
    CreditWindow();
    void cancel();
    void toggle(bool vis);
    void selectAtCursor();
    void update();
    void progressPage();
    void clear();
    void show() { toggle(true); }
    void hide() { toggle(false); }
};

/////////////

class FlagArrow : public Prop2D {
public:
    CharGridTextBox *lv_caption;
    
    FlagArrow();
    void updateLoc(Vec2 from, Vec2 to );
    void updateLevel(int v);
};

///////////////
class MilestoneDisplay : public CharGridTextBox {
public:
    MilestoneDisplay();
    void updateMilestone( int lv );
};

///////////////

class LogText : public CharGridTextBox {
public:
    double last_updated_at;
    LogText() : CharGridTextBox(100), last_updated_at(0) {}
};


class Log {
 public:
    LogText **tbs;
    int line_len;
    int line_num;
    Vec2 loc;
    char last_message[1024];
    Log( Vec2 lc, int max_ln );
    void printf( Color c, const char *fmt, ... );
    void printf( const char *fmt, ... );
    void print( Color c, char *s);
    void poll();
    void delayPrint( float at, char *s );
};


///////////////
class PartyIndicator : public Prop2D {
public:
    Vec2 world_loc;
    int client_id;
    double last_packet_at;
    FacePanel *face;
    CharGridTextBox *distance_tb;
    
    PartyIndicator();
    virtual bool prop2DPoll( double dt );
    
    void setVisible( bool vis );
};
bool isInScreen( Vec2 tov );
Vec2 hudCalcScreenEdge( Vec2 tov, float leftmargin = 80, float rightmargin = 80, float topmargin = 30, float bottommargin = 100 );

void setupPartyIndicator();
void ensurePartyIndicator( int client_id, Vec2 wloc, int face_ind, int hair_ind ) ;

///////////////

void hudUpdatePCStatus();
void hudUpdateItems();
void hudUpdateMilestone( int cleared );
void hudMilestoneMessage( const char *nick, int cnt, bool write_db );
void hudFortressDestroyMessage( const char *nick, int size, bool write_db );
void hudCoreDestroyMessage( const char *nick, bool write_db );
void hudKilledMessage( const char *nick, bool write_db );
    
void hudCongratulateProjectIsOver();
void hudShowErrorMessage( const char *caption, const char *msg, void (*cbfunc)( bool positive) );
void hudShowWarningMessage( const char *msg );
void hudShowConfirmAbandonMessage( int ind_to_abandon );
void hudShowConfirmRecoveryMessage( ITEMTYPE itt ) ;

#define WINDOW_ALPHA 0.6
#define HUD_OUT_OF_SCREEN Vec2(-9999,-9999)

#define MSG_OCCUPIED_BY_USER "OCCUPIED BY USER!"


#endif
