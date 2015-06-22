#ifndef _NET_H_
#define _NET_H_

#include "vce.h"
#include "ssproto_cli.h"
#include "picojson.h"

class ProjectState {
public:
    char owner_name[32];
    time_t started_at;    
    int num_player;
};


typedef enum {
    RS_INIT = 0,
    RS_MAKING_CHARACTER,
    RS_MADE_CHARACTER,
    RS_MAIN_MENU,
    RS_SELECTING_PROJECT,
    RS_SELECTED_PROJECT,    
    RS_IN_PROJECT,
    RS_LEAVING_PROJECT,
    RS_LEFT_PROJECT
} RUNSTATE;



//

typedef enum {
    FSS_INIT,
    FSS_TO_SEND,
    FSS_SENT,
} FIELDSAVESTATE;

class Field;

class FieldSaver {
public:
    Field *tgtf;
    int chw,chh;
    FIELDSAVESTATE *state;
    static const int MAX_CONCURRENT = 128;
    int num_concurrent;
    int savechunksz;
    char user_name[256];
    bool active;
    int project_id;
    FieldSaver( Field *f, int savechunksz );
    ~FieldSaver();
    void start( const char *username, int project_id );
    void poll( bool *finished );
    int countUnsent();
    int countTotalNum() { return chw * chh; }
};



bool initNetwork( const char *dbhost, int dbport, const char *rthost, int rtport );
void pollNetwork( double nt );
void pollDBPing(double nt);
void pollRealtimePing(double nt);
bool isRealtimeNetworkActive() ;
void finishRealtimeNetwork();
void finishDBNetwork();
bool isDBNetworkActive() ;
void makePCFieldName( char *out, size_t outsz) ;

extern char g_user_name[128];
extern char g_project_by_owner_ht_key[128];
extern char g_dbhost[128];
extern char g_rthost[128];

extern conn_t g_rtconn, g_dbconn;

extern int g_db_timeout_sec;

class PCDump;

bool dbSavePC();
bool dbLoadPC();

bool dbPing();
bool dbSave( const char *key, const char *field, const char *buf, size_t sz );
bool dbLoad( const char *key, const char *field, char *buf, size_t *sz );

bool dbSaveFileSync( const char *fn, const char *data, size_t sz );
bool dbLoadFileSync( const char *fn, char *out, size_t *outsz );

bool dbLoadJSON( const char *key, const char *field, picojson::value *v );
bool dbSaveJSON( const char *key, const char *field, picojson::value *v );
bool dbLoadIntArrayJSON( const char *key, const char *field, int *out, int *outlen );
bool dbSaveIntArrayJSON( const char *key, const char *field, const int *in, int inlen );
bool dbAppendIntArrayJSON( const char *key, const char *field, int val );

class FlagCandidate {
public:
    Pos2 pos;
    bool finished;
    FlagCandidate() : finished(false) {}
};

typedef struct {
public:
    int project_id;
    int owner_uid;
    char owner_username[32];
    char owner_nickname[32];
    //    int milestone_cleared;
    unsigned int created_at;
    FlagCandidate flag_cands[MILESTONE_MAX];
} ProjectInfo;

int countMilestoneCleared(ProjectInfo *pinfo );

bool dbSaveProjectInfo( ProjectInfo *pinfo );
bool dbLoadProjectInfo( int project_id, ProjectInfo *out );

int dbGetNewId( char *key );
int dbGetNewProjectId();
int dbGetNewUserId();


unsigned int dbGetTime(int *usec = NULL );

bool ensureUserID( const char *username );


class Field;
void dbSaveFieldSync( const char *user_name, int project_id ) ;
bool dbPutFieldFileSend( Field *f, int qid, int project_id, Pos2 lb, int w, int h ) ;
bool dbLoadFieldFileSend( int project_id, Pos2 lb, int w, int h  );
void dbPutFieldFileSendAt( Field *f, int project_id, Pos2 at, int dia );


bool dbSaveFieldRevealSync( int pjid );
bool dbLoadFieldRevealSync( int pjid );

bool dbSaveFieldEnvSync( int pjid );
bool dbLoadFieldEnvSync( int pjid );
    
int dbCountOnlinePlayerSync( int project_id );

bool dbPushStringToList( const char *key, const char *s, int trim, int *cnt );
bool dbGetStringFromList( const char *key, int s_ind, int e_ind, char **out, int *out_num );

bool dbAppendStringArray( const char *key, const char *field, const char *s, int trim);
bool dbGetStringArray( const char *key, const char *field, char **out, int *outn );

typedef struct {
public:
    time_t at;
    char caption[32];    
} ProjectEvent;

bool dbAppendProjectStatus( int pjid, const char *msg );
bool dbLoadProjectStatus( int pjid, ProjectEvent *ev, int *evnum  );

bool dbDeleteProject( int project_id );

class PowerSystemDump;

bool dbLoadPowerSystem( int pjid, PowerSystemDump *out );
bool dbSavePowerSystem( int pjid, PowerSystemDump *psd );

bool dbSaveResearchState( int pjid );
bool dbLoadResearchState( int pjid );

//////////////////////

void broadcast( int type_id, const char *data, int data_len );


bool realtimeUnlockGridSend( int chx, int chy );
bool realtimeLockGridSend( int chx, int chy );
bool realtimeLockKeepGridSend( int chx, int chy );
void realtimeJoinChannelSend( int channel_id ) ;
int realtimeGetChannelMemberCountSync( int channel_id, int *maxnum ) ;

class Cell;
void realtimeSyncCellSend( Cell *c, char chgtype, bool send_far = false );
void realtimeSyncChunkSnapshotSend( int chx, int chy );

class PC;
void realtimeSyncPCSend( PC *pc );

void realtimeUpdateNearcastPositionSend( int x, int y );
class Beam;
void realtimeNewBeamSend( Beam *b );
void realtimeCreateBeamSparkEffectSend( Beam *b );
class Char;
void realtimeCharDeleteSend( Char *ch );
class Sound;
void realtimePlaySoundSend( Sound *snd, Vec2 at);

typedef enum {
    EFT_DIGSMOKE = 1,
    EFT_BLOOD,
    EFT_SMOKE,
    EFT_POSITIVE,
    EFT_CHARGE,
    EFT_SPARK,
    EFT_SHIELD_SPARK,
    EFT_LEAF,
    EFT_CELL_BUBBLE,
    EFT_EXPLOSION,
    EFT_BOSS_EXPLOSION,
} EFFECTTYPE;

void realtimeEffectSend( EFFECTTYPE et, Vec2 at, float opt0=0, float opt1=0 );
class Debri;
void realtimeNewDebriSend( Debri *d ) ;
class Bullet;
void realtimeNewBulletSend( Bullet *b );

typedef enum {
    ET_INVAL = 0,
    ET_FLY = 1,
    ET_WORM,
    ET_EGG,
    ET_TAKWASHI,
    ET_REPAIRER,
    ET_CHASER,
    ET_BUILDER,
    ET_GIREV,
    ET_HYPERPUTTER,
    ET_VOIDER,
    ET_FIREGENERATOR,
    ET_FIRECOPTER,
    ET_SNOWSLIME,
    ET_BIRD,
    ET_SHRIMP,
    ET_BEE,
    ET_DEFENDER,
    ET_TELEPORTER,
    ET_COREBALL
} ENEMYTYPE;
class Enemy;
void realtimeNewEnemySend( Enemy *e );
void realtimeEnemyMoveSend( Enemy *e );
void realtimeEnemyDamageSend( Enemy *e, int damage );
void realtimeEnemyKnockbackSend( Enemy *e );

void realtimeDebugCommandBroadcast( const char *cmd );

#define DUMMY_CLIENT_ID (-1) // Use this value to treat a prop is not generated in local but not syncing 

typedef enum {
    FLT_BLASTER = 1,
    FLT_BOMBFLOWERSEED,
    FLT_VOLCANICBOMB,
} FLYERTYPE;
class Flyer;
void realtimeNewFlyerSend( Flyer *f ) ;

typedef enum {
    EVT_MILESTONE = 1,
    EVT_FORTRESS_DESTROYED = 2,
    EVT_LOGIN = 3,
    EVT_LOGOUT = 4,
    EVT_CLEAR_FLAG = 5,
    EVT_RESEARCH_DONE = 6,    
} EVENTTYPE;
void realtimeEventSend( EVENTTYPE t, const char *msg="", int opt0=0, int opt1=0 );

typedef enum {
    LOCK_POWERSYSTEM = 1,
    LOCK_RESOURCEDEPO = 2,
    LOCK_RESEARCH = 3,
} LOCKCATEGORY;

void realtimeLockProjectSend( int pjid, LOCKCATEGORY lc );
void realtimeUnlockProjectSend( int pjid, LOCKCATEGORY lc );
void realtimeLockKeepProjectSend( int pjid, LOCKCATEGORY lc );

void pollPowerSystemLock();

void realtimePowerNodeSend( int powergrid_id, Pos2 at );

void reloadPowerSystem();
void savePowerSystem();
void realtimePowerGridSend( int powergrid_id, int ene_diff );

bool dbSaveResourceDeposit( int pjid );
bool dbLoadResourceDeposit( int pjid );

void realtimeEnergyChainSend( PC *tgt, int ene );
void realtimePartySend();

void realtimeCleanAllSend() ;
void networkLoop( double dur ) ;

void realtimeRevealSend( int x, int y );



class Contact {
public:
    int user_id;
    char nickname[PC_NICKNAME_MAXLEN+1];
    int face_index;
    int hair_index;
};

bool dbSaveUserFile( int user_id, const char *prefix, const char *ptr, size_t sz );
bool dbLoadUserFile( int user_id, const char *prefix, char *ptr, size_t sz );

bool dbLoadFollowings();
bool dbSaveFollowings();
bool dbLoadFollowers();
bool dbSaveFollowers();

bool followContact( int user_id, const char * nickname, int face_index, int hair_index );
bool addFollowerContact( int following_uid, const char * nickname, int face_index, int hair_index );
void realtimeFollowSend( int following_uid, int followed_uid, const char *nick_follow, const char *nick_followed, int f, int h );
bool isFollowing(int uid);
bool isAFollower( int uid ) ;
bool isAFriend( int uid );
void unfriendUser( int uid );
int countFriends() ;
void dbShareProjectSend( int pjid ) ;
void dbPublishProjectSend( int pjid ) ;

void dbSearchPublishedProjects( int *pjids, int *num ) ;
void dbSearchSharedProjects( int *pjids, int *num ) ;

void dbUpdatePresenceSend();
void dbDeletePresenceSend() ;

void dbEnsureImage( int imgid );
void dbUpdateChunkImageSend( int imgid, int chx, int chy );

void dbLoadPNGImageSync( int imgid, Image *outimg ) ;
void dbLoadRawImageSync( int imgid, Image *outimg ) ;
void dbLoadRawImageSync( int imgid, int x, int y, int w, int h, unsigned char *out, size_t outsz ) ;
void dbUpdateImageAllRevealedSend( int imgid ) ;
void dbIncrementPlaytimeSend( int pjid, int sec ) ;
int dbLoadPlaytime( int pjid ) ;

int dbSaveFlagCands() ;

void dbSaveAllClearLog( int pjid, int accum_time_sec );

typedef struct {
    int project_id;
    int play_sec;
} AllClearLogEntry;

void dbLoadAllClearLogs( AllClearLogEntry *out, size_t *sz );

void dbSaveMilestoneProgressLog( int pjid );
typedef struct {
    int project_id;
    long long progress_at;
} MilestoneProgressLogEntry;
void dbLoadMilestoneProgressLogs( MilestoneProgressLogEntry *out, size_t *sz );

typedef struct {
    int project_id;
    int progress;
} MilestoneProgressRankEntry;
void dbLoadMilestoneProgressRanking( MilestoneProgressRankEntry *out, size_t *sz );

bool dbCheckProjectIsJoinable( int pjid, int userid ) ;

void networkFatalError( const char *msg ) ;

bool dbCheckProjectIsShared( int project_id, int owner_uid ) ;
bool dbCheckProjectIsPublished( int project_id );
void dbUnshareProjectSend( int project_id );
void dbUnpublishProjectSend( int project_id );

int getAllFriendContacts( Contact *contacts, int outmax );

class UnfriendLog {
public:
    int uids[SSPROTO_SHARE_MAX];
    UnfriendLog() {
        memset(uids,0,sizeof(uids));
    }
    void add(int uid) {
        for(int i=0;i<elementof(uids);i++) {
            if(uids[i]==0) {
                uids[i]=uid;
                break;
            }
        }
    }
};
void dbAppendSaveUnfriendLog(int unfriended_uid ) ;
void dbExecuteUnfriendLog() ;

#endif
