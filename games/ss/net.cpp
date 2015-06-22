#include "vce.h"
#include "moyai/cumino.h"
#include "moyai/client.h"
#include "ssproto_cli.h"
#include "dimension.h"
#include "effect.h"
#include "conf.h"
#include "net.h"
#include "ids.h"
#include "conf.h"
#include "item.h"
#include "hud.h"
#include "char.h"
#include "pc.h"
#include "util.h"
#include "field.h"
#include "mapview.h"
#include "enemy.h"
#include "pwgrid.h"
#include "globals.h"

#include "util.h"
#include "field.h"
#include "picojson.h"

tcpcontext_t g_dbtcp, g_rttcp;
conn_t g_rtconn;   // Connection to realtime backend
conn_t g_dbconn;   // Connection to database backend

char g_dbhost[128] = "localhost";
char g_rthost[128] = "localhost";

char g_user_name[128] = "ringo"; // SDS or Shinra system gives
int g_user_id = 0; // User ID in space sweeper, independent with Shinra's ID. Stored in Redis
char g_project_by_owner_ht_key[] = "pj_by_owner_h"; // [uid]
char g_project_info_ht_key[] = "pj_info_h"; // [project_id]
char g_user_id_by_user_name_ht_key[] = "user_id_h"; // Search user ID from Shinra's user ID.
char g_project_id_generator_key[] = "project_id_gen"; // used in dbGetNewId
char g_user_id_generator_key[] = "user_id_gen";  // used in dbGetNewId
char g_project_status_ht_key[] = "project_status_h"; // [project_id]

char g_all_clear_ranking_z_key[] = "all_clear_z"; // Ranking by consumed time to beat a project value:project id
char g_ms_progress_z_key[] = "ms_progress_z"; // Ranking by project id(!), value:time. Be careful project id is stored in score field.

//
bool g_wait_for_net_result = false;
int g_net_result_code = 0;
int g_kvs_list_size = 0;
int g_net_result_value = 0;
int g_net_result_value_option = 0;

char g_kvs_results_work[SSPROTO_KVS_ARRAYLEN_MAX][SSPROTO_KVS_ELEMENT_MAX];
char *g_kvs_results[SSPROTO_KVS_ARRAYLEN_MAX];
size_t g_kvs_results_size[SSPROTO_KVS_ARRAYLEN_MAX];
size_t g_kvs_result_num = 0;
char g_db_file_got_data[ SSPROTO_FILE_SIZE_MAX ];
size_t g_db_file_got_size;

int g_kvs_result_value_type = 0;

char *g_db_get_file_store_buffer = NULL;
size_t g_db_get_file_size = 0;


void makeFieldFileFormat( char *out, size_t outsz, int pjid, int x, int y, int w, int h );
void parseFieldFileName( const char *filename, int *project_id, int *x, int *y, int *w, int *h );

int g_last_join_channel_id;
int g_last_sendbuf_len, g_last_recvbuf_len;

int g_realtime_client_id;

int g_db_timeout_sec = 10;


/////////
void net_fatal_callback( bool positive ) {
    g_program_finished = true;
}

void networkFatalError( const char *msg ) {
    hudShowErrorMessage( "FATAL ERROR!", msg, net_fatal_callback );
}

int conn_closewatcher( conn_t c, CLOSE_REASON reason ) {
    bool fatal = false;
    print("closewatcher. reason:%d\n", reason );    
    switch(reason) {
    case CLOSE_REASON_REMOTE:  // network error
    case CLOSE_REASON_TIMEOUT:
        fatal = true;
        break;
    case CLOSE_REASON_APPLICATION: // OK
        break;
    case CLOSE_REASON_UNKNOWN: // program error
    case CLOSE_REASON_DECODER:
    case CLOSE_REASON_ENCODER:
    case CLOSE_REASON_PARSER:
    case CLOSE_REASON_INTERNAL:
    default:
        fatal = true;
        break;        
    }
    g_realtime_client_id = 0;

    if(fatal) {
        networkFatalError( "FATAL NETWORK ERROR" );
    }
    return 0;
}
int ssproto_cli_sender( conn_t c, char *data, int len ) {
    //    print("ssproto_cli_sender: len:%d",len);
    int r = vce_protocol_unparser_bin32( c, data, len );
    if( r < 0 ) {
        if( vce_conn_is_valid(c) )  print("ssproto_cli_sender: unparser_bin32 failed! ret:%d",r);
    }
    return r;
}
int ssproto_cli_recv_error_callback( conn_t c, int e ) {
    print( "ssproto_cli_recv_error_callback: gen error: %d", e );
    return -1;
}


////////


char *debug_to_str( const char *s, int slen ) {
    static char tmp[1024];
    int l = MIN( slen, sizeof(tmp)-1);
    memcpy( tmp, s, l );
    tmp[l] = '\0';
    return tmp;
}

//////


bool initNetwork( const char *dbhost, int dbport, const char *rthost, int rtport ) {
    vce_initialize();
    vce_set_verbose_mode(1);
    
    g_dbtcp = vce_tcpcontext_create( 0, // client
                                     NULL, 0, // addr, port
                                     1, // maxcon
                                     1024*1024*16, 1024*1024*16, // rblen, wblen bigger, because of passing field data
                                     60, // timeout
                                     0, // nonblock_connect
                                     1, // nodelay
                                     0 // statebuf_size
                                     );
    if(!g_dbtcp) {
        print( "tcpcontext_create for db: %s", STRERR );
        return false;
    }
    vce_tcpcontext_set_conn_call_parser_per_heartbeat( g_dbtcp, 1000 );
    vce_tcpcontext_set_conn_parser( g_dbtcp, vce_protocol_parser_bin32, ssproto_cli_pcallback );
    vce_tcpcontext_set_conn_closewatcher( g_dbtcp, conn_closewatcher );
    vce_set_heartbeat_wait_flag(0);
    
    g_rttcp = vce_tcpcontext_create( 0, // client
                                     NULL, 0,
                                     1, // maxcon
                                     1024*1024, 1024*1024,
                                     60,
                                     0,
                                     1, // nodelay
                                     0 );
    if( !g_rttcp ) {
        print("tcpcontext_create for rt: %s", STRERR );
        return false;
    }
    vce_tcpcontext_set_conn_call_parser_per_heartbeat( g_rttcp, 1000 );
    vce_tcpcontext_set_conn_parser( g_rttcp, vce_protocol_parser_bin32, ssproto_cli_pcallback );
    vce_tcpcontext_set_conn_closewatcher( g_rttcp, conn_closewatcher );
    vce_set_heartbeat_wait_flag(0);                                     

    g_rtconn = vce_tcpcontext_connect( g_rttcp, rthost, rtport );
    if( !vce_conn_is_valid( g_rtconn ) ) {
        print( "vce_tcpcontext_connect failed realtime connection\n" );
        return false;
    }
    ssproto_conn_serial_send( g_rtconn );
    
    g_dbconn = vce_tcpcontext_connect( g_dbtcp, dbhost, dbport );
    if( !vce_conn_is_valid( g_dbconn ) ) {
        print( "vce_tcpcontext_connect failed db connection\n" );
        return false;
    }
    
    print("network initialized" );

    bool dbtest_ok = true;
    if( isDBNetworkActive() ) {
        if( dbPing() == false ) dbtest_ok = false;
        char testdata[] = "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
        char testkey[] = "testkey";
        char testfield[] = "testfield";
        if( dbSave( testkey, testfield, testdata, strlen(testdata) ) == false ) dbtest_ok = false;
        char loaddata[SSPROTO_KVS_ELEMENT_MAX];
        size_t sz = sizeof(loaddata);
        if( dbLoad( testkey, testfield, loaddata, &sz ) == false ) {
            dbtest_ok = false;
        } else {
            // validate data 
            assertmsg( sz == strlen(testdata), "bad size:%d %d", sz, strlen(testdata) );
            assertmsg( memcmp( testdata, loaddata, sz ) == 0, "data mismatch" );
        }
#ifdef __APPLE__
        // Try to save bigger file
        char bigdata[800000];
        for(int i=0;i<sizeof(bigdata);i++) bigdata[i] = i%0xff;
        double st = now();
        bool res = dbSaveFileSync( "ss_test_bigdata_file", bigdata, sizeof(bigdata) );
        assert(res);
        double et = now();
        print("dbSaveFileSync: %.3fsec for %d bytes", (et-st), sizeof(bigdata));
        char bigdata_loaded[ sizeof(bigdata) ];
        size_t loadsz = sizeof(bigdata);
        st = now();
        res = dbLoadFileSync( "ss_test_bigdata_file", bigdata_loaded, &loadsz );
        assert(res);
        et = now();
        assert(loadsz == sizeof(bigdata));
        for(int i=0;i<sizeof(bigdata);i++) {
            assert( bigdata_loaded[i] == bigdata[i] );
        }
        print("dbLoadFile: %.3fsec for %d bytes", (et-st), sizeof(bigdata));
#endif
    }
    return dbtest_ok;
}

    

int g_netpoll_cnt = 0;

bool isRealtimeNetworkActive() {
    return vce_conn_is_valid( g_rtconn ) &&  vce_conn_writable( g_rtconn ) > 100 ;
}
bool isDBNetworkActive() {
    return vce_conn_is_valid( g_dbconn ) && vce_conn_writable( g_dbconn ) > 100 ;
}
// Never reconnect after closing connection.
void finishRealtimeNetwork() {
    vce_conn_close( g_rtconn );
}
void finishDBNetwork() {
    vce_conn_close( g_dbconn );
}
void pollRealtimePing( double nt ) {
    static double last_rt_ping_at = 0;
    if( isRealtimeNetworkActive() ) {    
        if( last_rt_ping_at < nt - 1 ) {
            last_rt_ping_at = nt;
            ssproto_ping_send( g_rtconn, now_usec(), 0 );
            char *r,*s;
            vce_conn_get_buffer( g_rtconn, &r, &g_last_recvbuf_len, &s, &g_last_sendbuf_len );
        }
    }
}
void pollDBPing(double nt) {
    static double last_db_ping_at = 0;    
    if(isDBNetworkActive() ) {
        if( last_db_ping_at < nt - 2 ) {
            last_db_ping_at = nt;
            ssproto_ping_send( g_dbconn, now_usec(), 0 );
        }
    }
}

void pollRealtimeParty( double nt) {
    static double last_party_at = 0;
    
    if( isRealtimeNetworkActive() ) {        
        if( g_game_paused == false && last_party_at < nt - 3 ) {
            last_party_at = nt;
            realtimePartySend();
            dbUpdatePresenceSend();
        }        
    }
    
}
void pollNetwork(double nt) {

    pollRealtimePing(nt);
    pollRealtimeParty(nt);
    pollDBPing(nt);
    
    g_netpoll_cnt ++;    
    vce_heartbeat();
    vce_heartbeat();    // To get reply from database server within a frame. VCE performs parsing and sending in separate polling.
}

//////////////
bool ensureUserID( const char *username ) {
    char data[32];
    size_t sz = sizeof(data);
    bool res = dbLoad( g_user_id_by_user_name_ht_key, username, data, &sz );
    if(res) {
        // existing user
        g_user_id = atoilen( data, sz );
        print("ensureUserID loaded uid:%d", g_user_id );        
    } else {
        // new user
        g_user_id = dbGetNewUserId();
        snprintf(data,sizeof(data),"%d", g_user_id );
        res = dbSave( g_user_id_by_user_name_ht_key, username, data, strlen(data) );
        if(!res) {
            // database isn't available any more?
            return false;
        }
        print("ensureUserID got new uid:%d", g_user_id );
    }
    return true;
}


bool dbSavePC() {
    assert( g_user_id > 0 );
    PCDump pcd;
    g_pc->dump(&pcd);
    bool res = dbSaveUserFile( g_user_id, "pc", (const char*)&pcd, sizeof(pcd) );
    res &= dbSaveFollowers();
    res &= dbSaveFollowings();
    return res;
}
bool dbLoadPC() {
    PCDump pcd;
    bool res = dbLoadUserFile( g_user_id, "pc", (char*) &pcd, sizeof(pcd) );
    if(res) {
        g_pc->applyPCDump(&pcd);
        dbLoadFollowers();
        dbLoadFollowings();
        dbExecuteUnfriendLog();
    }
    return res;
}

// Game gets into fatal error state when timed out
void waitForReply( bool use_db = true ) {
    static int call_depth = 0;
    assertmsg( call_depth == 0, "waitForReply: can't call waitForReply in network callback func" );
    call_depth ++;
    double st = now();
    g_wait_for_net_result = 1;
    g_net_result_value = -123;
    g_net_result_value_option = -12345;    
    g_net_result_code = -1;
    while(g_wait_for_net_result) {
        double nt = now();
        pollDBPing(nt);
        pollRealtimePing(nt);
        for(int i=0;i<4;i++) {
            if(use_db) vce_heartbeat_on_single_conn(g_dbconn); else vce_heartbeat_on_single_conn(g_rtconn);
        }
        double elt = nt - st;
        if( elt > g_db_timeout_sec ) {
            // timeout
            print("waitForReply timed out!" );
            networkFatalError( "NETWORK TIMED OUT!" );
            finishDBNetwork();
            finishRealtimeNetwork();
            call_depth --;
            return;
        }
    }
    double et = now();
    double dt = et-st;
    
    if( dt > 0.1 ) {
        print("waitForReply: slow query: %dms", (int) (dt * 1000) );
    }
    call_depth--;
}
// Return neg value when error
int dbGetNewProjectId() {
    return dbGetNewId( g_project_id_generator_key );
}
int dbGetNewUserId() {
    return dbGetNewId( g_user_id_generator_key );
}

int dbGetNewId( char *key ) {
    ssproto_kvs_command_str_send( g_dbconn, 0, Format( "INCR %s", key ).buf );
    waitForReply();
    if( g_kvs_result_num == 1 ) {
        int id = atoilen( g_kvs_results[0], g_kvs_results_size[0] );
        print("dbGetNewGlobalId: got: %d", id );
        return id;
    }  else {
        print("dbGetNewGlobalId: no result" );
        return -1;
    }
}
// returns epoc time
unsigned int dbGetTime(int *usec) {
    char cmd[32] = "TIME";
    ssproto_kvs_command_str_send( g_dbconn, 0, cmd );
    waitForReply();
    if( g_kvs_result_num == 2 ) {
        // Redis returns 1) "1394502783"  2) "12662" 
        unsigned int out = strtoullen( g_kvs_results[0], g_kvs_results_size[0] );
        if(usec) *usec = strtoullen( g_kvs_results[1], g_kvs_results_size[1] );
        return out;
    } else {
        print("dbGetTime: no result from db" );
        *usec = 0;
        return 0;
    }
}


bool dbSaveProjectInfo( ProjectInfo *pinfo ) {
    return dbSave( g_project_info_ht_key, Format( "%d", pinfo->project_id ).buf, (const char*) pinfo, sizeof(*pinfo) ) ;
}
bool dbLoadProjectInfo( int project_id, ProjectInfo *out ) {
    size_t sz = sizeof(*out);    
    bool res = dbLoad( g_project_info_ht_key, Format( "%d",project_id ).buf, (char*) out, &sz );
    if(!res) return false;
    if( sz != sizeof(*out) ) {
        // Size differs, old data! Ignore such a old projects.
        print("dbLoadProjectInfo: got size:%d expect size:%d", sz, sizeof(*out) );
        return false;
    }
    //    for(int i=0;i<elementof(out->flag_cands);i++) print(" FC:%d,%d", out->flag_cands[i].pos.x, out->flag_cands[i].pos.y );
    
    return true;
}
// Returns number of cleared flags
int dbSaveFlagCands() {
    ProjectInfo pinfo;
    bool res = dbLoadProjectInfo( g_current_project_id, &pinfo );
    assert(res);
    g_fld->copyFlagCands( &pinfo );
    res = dbSaveProjectInfo( &pinfo );
    assertmsg(res, "can't save project info");
    int cnt = countMilestoneCleared(&pinfo);
    return cnt;
}

bool dbSavePowerSystem( int pjid, PowerSystemDump *psd ) {
    Format f( "powersystem_%d", pjid );
    return dbSaveFileSync( f.buf, (const char*)psd, sizeof(*psd) );
}
bool dbLoadPowerSystem( int pjid, PowerSystemDump *out ) {
    Format f( "powersystem_%d", pjid );    
    size_t sz = sizeof(*out);
    bool res = dbLoadFileSync( f.buf, (char*)out, &sz );
    if(res==false) {
        print("dbLoadPowerSystem: cant load file '%s'",f.buf);
        return false;
    }
    if( sz != sizeof(*out) ) {
        print("dbLoadPowerSystem: struct size changed? expect:%d saved:%d", sizeof(*out), sz );
        return false;
    }
    return true;
}
bool dbSaveResourceDeposit( int pjid ) {
    ResourceDepositDump dump;
    g_depo->dump( &dump );
    Format f( "deposit_%d", pjid );
    return dbSaveFileSync( f.buf, (const char*)&dump, sizeof(dump) );
}
bool dbLoadResourceDeposit( int pjid ) {
    Format f( "deposit_%d", pjid );    
    ResourceDepositDump dump;
    size_t sz = sizeof(dump);
    bool res = dbLoadFileSync( f.buf, (char*)&dump, &sz);
    if(!res) return false;
    if(sz != sizeof(dump) ) {
        print("dbLoadResourceDeposit: size differ: saved:%d expect:%d", sz, sizeof(dump) );
        return false;
    }
    g_depo->applyDump( &dump );
    return true;
}

// Research state doesn't have pointers, just can memcpy.
bool dbSaveResearchState( int pjid ) {
    Format f( "research_%d", pjid );
    size_t sz = sizeof(g_research_state);
    return dbSaveFileSync( f.buf, (const char*)g_research_state,sz );
}

bool dbLoadResearchState( int pjid ) {
    Format f( "research_%d", pjid );
    size_t sz = sizeof(g_research_state);
    bool res = dbLoadFileSync( f.buf, (char*)g_research_state, &sz );
    if(!res) resetResearchStateLocked();
    print("dbLoadResearchState: path:%s ret:%d", f.buf, res );
    return res;
}

/////////////

int ssproto_pong_recv( conn_t c, long long t_usec, int cmd ) {
    long long nt = now_usec();
    long long dt = (nt-t_usec);
    if(dt>200*1000) {
        Format fmt( "PING: SLOW RTT:%lld ms", dt/1000);
        print( fmt.buf );
        if( g_runstate == RS_IN_PROJECT ) {
            Color col = WHITE;
            if( dt > 1000*1000 ) col = RED;
            g_log->printf( col, fmt.buf );
        }
    }
    g_last_ping_rtt_usec = dt;
    return 0;
}
void net_ver_warn_callback( bool positive ) {
    g_titlewin->show();
}

int ssproto_version_notify_recv( conn_t c, unsigned int majv, unsigned int minv ) {
    unsigned int clminv, clmajv;
    clmajv = ssproto_cli_get_version( &clminv );
    if( majv != clmajv ) {
        Format fmt( "VERSION SV:%d CL:%d", majv, clmajv );
        networkFatalError(fmt.buf);
    } else {
        if( minv != clminv ) {
            Format fmt("SUBVER CL:%d SV:%d", clminv, minv );
            hudShowErrorMessage( "VERSION WARNING!", fmt.buf, net_ver_warn_callback );
        }
    }
    return 0;
}


typedef enum {
    QID_FIELDSAVER = 100,
    QID_FIELDLOADER = 101,
    QID_FIELDIMAGESAVE = 110,
    QID_PNGIMAGELOAD = 120,
    QID_RAWIMAGELOAD = 121,    
    QID_PUSHSTRINGTOLIST = 130,
    QID_GETSTRINGFROMLIST = 140,
    QID_APPENDSTRINGARRAY = 150,
    QID_GETSTRINGARRAY = 160,
    QID_SAVE_FILE_SYNC = 170,
    QID_LOAD_FILE_SYNC = 180,
    QID_LOAD_ALL_CLEAR_LOGS = 190,
    QID_LOAD_MS_PROGRESS_LOGS = 200,    
} QUERYID;



int ssproto_put_file_result_recv( conn_t _c, int query_id, int result, const char *filename ) {
    //          print("ssproto_put_file_result_recv: qid:%d res:%d filename:'%s'", query_id, result, filename );
    if( query_id == QID_FIELDSAVER ) {
        assert(g_fsaver);
        g_fsaver->num_concurrent --;
    } else if( query_id == QID_FIELDIMAGESAVE ) {
        //        print("ssproto_put_file_result_recv: query_id is QID_FIELDIMAGE, result:%d",result );
        g_wait_for_net_result = false;
        g_net_result_code = result;
    }  else if( query_id == QID_SAVE_FILE_SYNC ) {
        g_wait_for_net_result = false;
        g_net_result_code = result;
    }
    
    return 0;
}
int ssproto_get_file_result_recv( conn_t _c, int query_id, int result, const char *filename, const char *data, int data_len ) {
    //    print("ssproto_get_file_result_recv: qid:%d result:%d filename:'%s' datalen:%d", query_id, result, filename, data_len  );
    if( query_id == QID_FIELDLOADER ) {
        // Async loader
        static char decompressed[1024*1024];
        int decomp_len = memDecompressSnappy( decompressed, sizeof(decompressed), (char*)data, data_len );
        if( decomp_len < 0 ) {
            print("memDecompress error:%d", decomp_len );
            return 0;
        }

        int pjid, lb_x,lb_y,w,h;
        parseFieldFileName( filename, &pjid, &lb_x, &lb_y, &w, &h );

        if( w != CHUNKSZ || h != CHUNKSZ ) {
            print( "invalid save format: w,h:%d,%d",w,h );
            return 0;
        }

        for(int y=0;y<h;y++) {
            for(int x=0;x<w;x++) {
                int ind = x + y * w;
                char *from_ptr = decompressed + ind * sizeof(Cell);
                Cell *toc = g_fld->get(lb_x+x,lb_y+y);
                assertmsg(toc, "lb:%d,%d x,y:%d,%d",lb_x,lb_y,x,y);
                memcpy( toc, from_ptr, sizeof(Cell));                                
            }
        }
        int chx = lb_x / CHUNKSZ, chy = lb_y / CHUNKSZ;
        
        g_fld->notifyChunkChanged( chx, chy );
        g_fld->updateChunkStat( chx, chy );
        g_fld->setChunkLoadState( Pos2(lb_x, lb_y), CHUNKLOADSTATE_LOADED );
    } else if( query_id == QID_LOAD_FILE_SYNC ) {
        g_wait_for_net_result = false;
        g_net_result_code = result;
        g_db_file_got_size = data_len;
        print("ssproto_get_file_result_recv: datalen:%d",data_len);
        assert( data_len <= sizeof( g_db_file_got_data) );
        memcpy( g_db_file_got_data, data, data_len );
    }
       
    return 0;
}
int ssproto_check_file_result_recv( conn_t _c, int query_id, int result, const char *filename ) { return 0; }
int ssproto_notify_recv( conn_t _c, int sender_id, int type_id, const char *data, int data_len ) { return 0; }
int ssproto_generate_id_32_result_recv( conn_t _c, int query_id, int generated_id_start, int num ) {
    return 0;
}


int ssproto_kvs_command_str_result_recv( conn_t _c, int query_id, int retcode, int valtype, const char * const *result, int result_len ) {
    assert(g_wait_for_net_result);
    g_wait_for_net_result = false;
    g_net_result_code = retcode;
    
    g_kvs_result_value_type = valtype;
    for(int i=0;i<result_len;i++) {
        g_kvs_results[i] = g_kvs_results_work[i];
        g_kvs_results_size[i] = strlen(result[i]);
        memcpy(g_kvs_results_work[i], result[i], g_kvs_results_size[i]);
        //        print("KVS result[%d] size:%d str:'%s'", i, g_kvs_results_size[i], result[i] );
    }
    g_kvs_result_num = result_len;
    
    //    print("ssproto_kvs_command_str_result_recv: result num:%d valtype:%d retcode:%d", result_len, valtype, retcode );
    //    if( valtype == SSPROTO_KVS_VALUE_STRING) print("GOT KVS string value: '%s'", result[0] );
    
    return 0;

}

int ssproto_kvs_save_bin_result_recv( conn_t _c, int query_id, int retcode, int has_data, const char *key, const char *field ) {
    assert(g_wait_for_net_result);
    g_wait_for_net_result = false;
    g_net_result_code = retcode;

    //    print("ssproto_kvs_save_bin_result_recv: qid:%d retcode:%d has_data:%d key:'%s' field:'%s'",
    //          query_id, retcode, has_data, key, field );
    return 0;
}
int ssproto_kvs_load_bin_result_recv( conn_t _c, int query_id, int retcode, int has_data, const char *key, 
                                      const char *field, const char *data, int data_len ) {
    assert(g_wait_for_net_result);
    g_wait_for_net_result = false;
    g_net_result_code = retcode;
    
    g_kvs_result_num = has_data ? 1 : 0;
    g_kvs_results_size[0] = data_len;
    memcpy( g_kvs_results_work[0], data, data_len );
    g_kvs_results[0] = g_kvs_results_work[0];
    
    //    print("ssproto_kvs_load_bin_result_recv: qid:%d retcode:%d has_data:%d key:'%s' field:'%s' datalen:%d '",
    //          query_id, retcode, has_data, key, field, data_len );
    return 0;
}

///////



    
bool dbPing() {
    char pingstr[] = "PING";
    ssproto_kvs_command_str_send( g_dbconn, 0, pingstr);
    waitForReply();

    if( g_kvs_result_num == 1 && strcmp( g_kvs_results[0], "PONG" ) == 0 ) {
        return true;
    } else {
        print("dbPing: result num:%d resut[0]:'%s'", g_kvs_result_num, g_kvs_results[0] );
        return false;
    }
}

// Save file in database backend.
// It is not atmic. Note that it may fail.
bool dbSaveFileSync( const char *fn, const char *data, size_t sz ) {
    if( sz > SSPROTO_FILE_SIZE_ABS_MAX ) {
        print("dbSaveFileSync: data too big for '%s' sz:%u max:%d", fn, sz, SSPROTO_FILE_SIZE_ABS_MAX );
        return false;
    }
    int part_n = sz / SSPROTO_FILE_SIZE_MAX + (( sz % SSPROTO_FILE_SIZE_MAX > 0 ) ? 1 : 0);
    for(int i=0;i<part_n;i++) {
        Format fmt( "%s_part_%d", fn, i );
        int ofs = i * SSPROTO_FILE_SIZE_MAX;
        size_t partsz;
        if( i == part_n-1 ) {
            if( sz % SSPROTO_FILE_SIZE_MAX == 0 ) partsz = SSPROTO_FILE_SIZE_MAX; else partsz = sz % SSPROTO_FILE_SIZE_MAX;
        } else {
            partsz = SSPROTO_FILE_SIZE_MAX;
        }
        //        print("sending file part: '%s' size:%d total:%d", fmt.buf, partsz, sz );            
        int res = ssproto_put_file_send( g_dbconn, QID_SAVE_FILE_SYNC, fmt.buf, data + ofs, partsz );
        if(res<0) return false;
        g_wait_for_net_result = true;
        waitForReply();
        if( g_net_result_code != SSPROTO_OK ) {
            return false;
        }
    }
    return true;
}

bool dbLoadFileSync( const char *fn, char *out, size_t *outsz ) {
    if( *outsz > SSPROTO_FILE_SIZE_ABS_MAX ) {
        print("dbLoadFileSync: expecting data too big for '%s' sz:%d max:%d", fn, *outsz, SSPROTO_FILE_SIZE_ABS_MAX );
        return false;
    }
    size_t outmax = *outsz;
    int part_n = outmax / SSPROTO_FILE_SIZE_MAX + (( outmax % SSPROTO_FILE_SIZE_MAX > 0 ) ? 1 : 0);
    for(int i=0;i<part_n;i++) {
        Format fmt( "%s_part_%d", fn, i );
        int res = ssproto_get_file_send( g_dbconn, QID_LOAD_FILE_SYNC, fmt.buf );
        if(res<0) return false;
        g_wait_for_net_result = true;
        waitForReply();
        if( g_net_result_code != SSPROTO_OK ) {
            return false;
        }
        int ofs = i * SSPROTO_FILE_SIZE_MAX;
        size_t partsz;
        if( i == part_n-1 ) {
            if( outmax % SSPROTO_FILE_SIZE_MAX == 0 ) partsz = SSPROTO_FILE_SIZE_MAX; else partsz = outmax % SSPROTO_FILE_SIZE_MAX;
        } else {
            partsz = SSPROTO_FILE_SIZE_MAX;
        }
        size_t copy_size = MIN( g_db_file_got_size, partsz );
        //        print("loaded part '%s' size:%d ofs:%d", fmt.buf, copy_size, ofs );
        memcpy( out+ ofs, g_db_file_got_data, copy_size );
    }
    return true;
}


// Save binary data in KVS (not a static file). Synchronized.
bool dbSave( const char *key, const char *field, const char *buf, size_t sz ) {
    if( ! isDBNetworkActive() ) return false;
    ssproto_kvs_save_bin_send( g_dbconn, 0, key, field, buf, sz );
    waitForReply();
    //    print("dbSave: retcode:%d resultnum:%d", g_net_result_code , g_kvs_result_num );
    if( g_net_result_code == SSPROTO_OK ) {
        return true;
    } else {
        print("dbSave: error: %d", g_net_result_code );
        return false;
    }
}
// Load binary data from KVS, pair with dbSave().
bool dbLoad( const char *key, const char *field, char *buf, size_t *sz ) {
    if( ! isDBNetworkActive() ) return false;    
    ssproto_kvs_load_bin_send( g_dbconn, 0, key, field );
    waitForReply();
    //    print("dbLoad: retcode:%d resultnum:%d size[0]:%d", g_net_result_code, g_kvs_result_num, g_kvs_results_size[0] );
    // Can only be used with dbSave(), so only the first element is returned. Arrays elements will be ignored.
    if( g_net_result_code == SSPROTO_OK ) {
        if( g_kvs_result_num == 0 ) {
            return false;
        }
        size_t copylen = MIN( *sz, g_kvs_results_size[0] );
        memcpy( buf, g_kvs_results[0], copylen );
        *sz = copylen;
        return true;
    } else {
        *sz = 0;
        return false;        
    }
}


// Returns true if any form of JSON is loaded.
bool dbLoadJSON( const char *key, const char *field, picojson::value *v ) {
    static char buf[SSPROTO_KVS_BIN_MAX];
    size_t sz = sizeof(buf);
    bool ret = dbLoad(key,field,buf,&sz);
    if(!ret) return false;

    std::string err;
    char *startptr = buf, *endptr = buf+sz;
    picojson::parse( *v, startptr, endptr, &err );
    if(!err.empty()) {
        print("dbLoadJSON: picojson error: %s on key:'%s' field:'%s' json len:%d", err.c_str(), key,field, sz );
        return false;
    }
    return true;
}
bool dbSaveJSON( const char *key, const char *field, picojson::value *v ) {
    std::string serialized = v->serialize();
    bool res = dbSave( key, field, (char*) serialized.c_str(), serialized.size() );
    return res;
}
// Save int array in JSON in KVS.
bool dbSaveIntArrayJSON( const char *key, const char *field, const int *in, int inlen ) {
    picojson::array ary;
    for(int i=0;i<inlen;i++) {
        ary.push_back( picojson::value((double)in[i]) );
    }
    picojson::value v(ary);
    return dbSaveJSON( key, field, &v );
}
bool dbLoadIntArrayJSON( const char *key, const char *field, int *out, int *outlen ) {
    int outmax = *outlen;
    *outlen = 0;
    picojson::value v;
    bool res = dbLoadJSON( key, field, &v );
    if( res == false ) return false;
    picojson::array intary = v.get<picojson::array>();
    for(int i=0;i<intary.size() && i <  outmax; i++ ) {
        out[i] = (int) intary[i].get<double>();
        *outlen = i+1;
    }
    return true;
}
bool dbAppendIntArrayJSON( char *key, char *field, int val ) {
    picojson::value v;
    bool res = dbLoadJSON( key, field, &v );
    if(!res)return false;
    
    picojson::array intary = v.get<picojson::array>();
    intary.push_back( picojson::value((double)val));
    picojson::value newv = picojson::value(intary);
    return dbSaveJSON( key, field, &newv);
}


/////////////

FieldSaver::FieldSaver( Field *f, int savechunksz ) : tgtf(f), num_concurrent(0), savechunksz(savechunksz), active(false) {
    user_name[0] = '\0';
    project_id = 0;
    
    chw = f->width / savechunksz;
    chh = f->height / savechunksz;
    
    size_t sz = sizeof(FIELDSAVESTATE) * chw * chh;
    state = (FIELDSAVESTATE*) MALLOC(sz);
    for(int i=0;i<chw*chh;i++) state[i] = FSS_INIT;
}
void FieldSaver::start( const char *un, int pjid ) {
    project_id = pjid;
    snprintf( user_name, sizeof(user_name), "%s", un );
    for(int i=0;i<chw*chh;i++) state[i] = FSS_TO_SEND;
    num_concurrent = 0;
    active = true;
}
FieldSaver::~FieldSaver() {
    FREE(state);
}

int FieldSaver::countUnsent() {
    int cnt=0;
    for(int y=0;y<chh;y++) {
        for(int x=0;x<chw;x++) {
            FIELDSAVESTATE st = state[x+y*chw];
            if(st==FSS_TO_SEND)cnt++;
        }
    }
    return cnt;
}
void FieldSaver::poll( bool *finished ) {
    *finished = false;
    
    // TODO: Scanning array could be very slow.
    if( num_concurrent >= MAX_CONCURRENT ) return;
    if( user_name[0] == '\0' || project_id == 0 ) return;
    if( active == false ) return;

    int sent_cnt=0;
    *finished = true;
    for(int y=0;y<chh;y++) {
        for(int x=0;x<chw;x++) {
            FIELDSAVESTATE st = state[x+y*chw];
            switch(st){
            case FSS_INIT:
                break;
            case FSS_TO_SEND:
                {
                    *finished = false;
                    dbPutFieldFileSend( tgtf, QID_FIELDSAVER, project_id, Pos2(x*savechunksz,y*savechunksz), savechunksz, savechunksz );
                    num_concurrent ++;
                    //                    print("dbputfieldfilesend called. num_concurrent:%d xy:%d,%d",num_concurrent,x,y);
                    state[x+y*chw] = FSS_SENT;
                    if( num_concurrent >= MAX_CONCURRENT ) {
                        *finished = false;
                        return;
                    }
                }
                
                
                break;
            case FSS_SENT:
                sent_cnt++;
                break;
            default:
                assert(false);
            }
        }
    }
    //    print("sent_cnt:%d num_concurrent:%d",sent_cnt, num_concurrent);
    
    if( num_concurrent > 0 ) *finished = false;
    if( sent_cnt < chw*chh ) *finished = false;

}


void makeFieldFileFormat( char *out, size_t outsz, int pjid, int x, int y, int w, int h ) {
    snprintf( out, outsz,"field_data_%d_%d_%d_%d_%d", pjid, x,y,w,h);
}
void parseFieldFileName( const char *filename, int *project_id, int *x, int *y, int *w, int *h ) {
    strtok( (char*)filename, "_" ); // field
    strtok( NULL, "_" ); // data
    char *tk_pjid = strtok( NULL, "_" );
    char *tk_x = strtok( NULL, "_" );
    char *tk_y = strtok( NULL, "_" );
    char *tk_w = strtok( NULL, "_" );
    char *tk_h = strtok( NULL, "_" );

    assert( tk_h ); // To find application bug earlier
    *project_id = atoi( tk_pjid );
    *x = atoi(tk_x);
    *y = atoi(tk_y);
    *w = atoi(tk_w);
    *h = atoi(tk_h);
}

// Pack field rect to a buffer and send it by put_file().
void dbPutFieldFileSendAt( Field *f, int project_id, Pos2 at, int dia ) {
    int chx_0 = (at.x-dia)/CHUNKSZ, chx_1 = (at.x+dia)/CHUNKSZ;
    int chy_0 = (at.y-dia)/CHUNKSZ, chy_1 = (at.y+dia)/CHUNKSZ;
    for(int chx=chx_0; chx<=chx_1; chx++ ) {
        for(int chy=chy_0; chy<=chy_1; chy++) {
            Pos2 lb( chx * CHUNKSZ, chy * CHUNKSZ );
            // Can only send chunks that is aloready loaded, otherwise saves corrupt field data
            CHUNKLOADSTATE ls = g_fld->getChunkLoadState(lb);
            if(ls == CHUNKLOADSTATE_LOADED) dbPutFieldFileSend( f, 0 /* don't need reply */, project_id, lb, CHUNKSZ, CHUNKSZ );            
        }
    }
}

bool dbPutFieldFileSend( Field *f, int qid, int project_id, Pos2 lb, int w, int h ) {
    assert( lb.x % CHUNKSZ == 0 );
    assert( lb.y % CHUNKSZ == 0 );
    
    char fn[256];
    makeFieldFileFormat(fn, sizeof(fn), project_id, lb.x, lb.y, w,h );
    
    static char buf[1024*1024];
    size_t sz = sizeof(Cell) * w * h;
    assert(sz <= sizeof(buf));
    for(int y=0;y<h;y++) {
        for(int x=0;x<w;x++) {
            int ind = x + y * w;
            Cell *fromc = f->get(lb.x+x,lb.y+y);
            char *to_ptr = buf + ind * sizeof(Cell);
            memcpy( to_ptr, (char*)fromc, sizeof(Cell));
        }
    }
    static char compressed[1024*1024];
    size_t origsz = sizeof(Cell)*w*h;
    int compret = memCompressSnappy( compressed, sizeof(compressed), buf, origsz );
    //    print("Compress: orig:%d after:%d at:%d,%d %d,%d", origsz, compret, lb.x,lb.y,w,h );
    if( compret < 0 ) {
        assertmsg( false, "compress fail: origsz:%d", origsz);
    }
    
    int r = ssproto_put_file_send( g_dbconn, qid,  fn, compressed, compret );
    if(r<0) {
        print("ssproto_put_file_send failed! r:%d",r);
        return false;
    }
#if 0
    char *rb, *wb;
    int rblen, wblen;
    vce_conn_get_buffer( g_dbconn, &rb, &rblen, &wb, &wblen );
    print("XXXXXXXXXXXX: %d\n", wblen);
#endif    
    return true;
}
bool dbLoadFieldFileSend( int project_id, Pos2 lb, int w, int h  ) {
    assert( lb.x % CHUNKSZ == 0 );
    assert( lb.y % CHUNKSZ == 0 );

    char fn[256];
    makeFieldFileFormat(fn, sizeof(fn), project_id, lb.x, lb.y, w,h );

    int r = ssproto_get_file_send( g_dbconn, QID_FIELDLOADER, fn );
    //    print("ssproto_get_file_send: %d bytes", r );
    return r > 0;
}




// Syncronized saving
void dbSaveFieldSync( const char *user_name, int project_id ) {
    print("dbSaveFieldSync start");
    double st = now();
    g_fsaver->start( user_name, project_id );
    while(true) {
        vce_heartbeat_on_single_conn(g_dbconn);
        vce_heartbeat_on_single_conn(g_dbconn);
        bool finished;
        g_fsaver->poll(&finished);
        if(finished) break;
    }
    double et = now();
    print("dbSaveFieldSync end. time:%.3f",(et-st));    
}


bool dbSaveFieldRevealSync( int pjid ) {
    size_t sz = g_fld->getRevealBufferSize();
    Format f( "reveal_%d", pjid );
    return dbSaveFileSync( f.buf, (const char*) g_fld->revealed, sz );    
}
bool dbLoadFieldRevealSync( int pjid ) {
    size_t sz = g_fld->getRevealBufferSize();
    Format f( "reveal_%d", pjid );
    bool res = dbLoadFileSync( f.buf, (char*) g_fld->revealed, &sz );
    if(!res) return false;
    if( sz != g_fld->getRevealBufferSize() ) {
        return false;
    } else {
        return true;
    }
}

bool dbSaveFieldEnvSync( int pjid ) {
    Format f( "envs_%d", pjid );
    return dbSaveFileSync( f.buf, (const char*) g_fld->envs, sizeof(g_fld->envs) );
}
bool dbLoadFieldEnvSync( int pjid ) {
    size_t sz = sizeof( g_fld->envs);
    Format f( "envs_%d", pjid );
    bool res = dbLoadFileSync( f.buf, (char*) g_fld->envs, &sz );
    if(!res) return false;
    if( sz != sizeof( g_fld->envs ) ) {
        return false;
    } else {
        return true;
    }
}


//////////////



int ssproto_list_presence_result_recv( conn_t _c, int project_id, const int *user_ids, int user_ids_len ) {
    //    print("ssproto_list_presence_result_recv pj:%d uidn:%d",project_id, user_ids_len  );
    for(int i=0;i<user_ids_len;i++) {
        print("  id[%d] : %d", i, user_ids[i] );
    }
    return 0;
}

int g_count_presence_result;

int ssproto_count_presence_result_recv( conn_t _c, int project_id, int user_num ) {
    //    print("ssproto_count_presence_result_recv: pj:%d n:%d", project_id, user_num );
    if( g_wait_for_net_result ) {
        g_wait_for_net_result = false;
        g_count_presence_result = user_num;
    }
    return 0;
}

int dbCountOnlinePlayerSync( int project_id ) {
    g_wait_for_net_result = true;
    if( isDBNetworkActive() ) {
        ssproto_count_presence_send( g_dbconn, project_id );
        waitForReply();
        return g_count_presence_result;
    }
    return 0;
}
void dbUpdatePresenceSend() {
    if( g_current_project_id > 0 && g_user_id > 0 ) { 
        ssproto_update_presence_send( g_dbconn, g_current_project_id, g_user_id );
    }
}
void dbDeletePresenceSend() {
    if( g_current_project_id > 0 && g_user_id > 0 ) {
        ssproto_delete_presence_send( g_dbconn, g_current_project_id, g_user_id );
    }
}

// int *cnt: Number of elements after push
bool dbPushStringToList( const char *key, const char *s, int trim, int *cnt ) {
    if(!isDBNetworkActive()) return false;
    g_wait_for_net_result = true;    
    g_kvs_list_size = -1;
    ssproto_kvs_push_to_list_send( g_dbconn, QID_PUSHSTRINGTOLIST, key, s, trim );
    waitForReply();
    if(cnt) *cnt = g_kvs_list_size;
    return true;
}
int ssproto_kvs_push_to_list_result_recv( conn_t _c, int query_id, int retcode, const char *key, int cnt ) {
    if( query_id == QID_PUSHSTRINGTOLIST ) {
        //        print("ssproto_kvs_push_to_list_result_recv: k:'%s' cnt:%d",key,cnt);
        g_wait_for_net_result = false;
        g_kvs_list_size = cnt;
        g_net_result_code = retcode;
    }
    return 0;
}

bool dbGetStringFromList( const char *key, int s_ind, int e_ind, char **out, int *out_num ) {
    if(!isDBNetworkActive()) return false;
    g_wait_for_net_result = true;
    g_kvs_list_size = -1;
    //    print("ssproto_kvs_get_list_range_send! %d-%d", s_ind, e_ind );
    ssproto_kvs_get_list_range_send( g_dbconn, QID_GETSTRINGFROMLIST, key, s_ind, e_ind );
    waitForReply();
    if( g_net_result_code == SSPROTO_OK ) {
        //        print("dbGetStringFromList: list num:%d outn:%d", g_kvs_list_size, *out_num );
        int maxnum = *out_num;
        for(int i=0;i<g_kvs_list_size && i < maxnum;i++) {
            out[i] = g_kvs_results[i];
            *out_num = i+1;
        }
        return true;        
    } else {
        return false;
    }
}

int ssproto_kvs_get_list_range_result_recv( conn_t _c, int query_id, int retcode, int start_ind, int end_ind, const char *key, const char * const *result, int result_len ) {
    if( query_id == QID_GETSTRINGFROMLIST ) {
        g_wait_for_net_result = false;
        g_kvs_list_size = result_len;
        g_net_result_code = retcode;
        
        for(int i=0;i<result_len;i++) {
            //            print("result: %d: '%s'", i, result[i] );
            strncpy( g_kvs_results_work[i], result[i], sizeof(g_kvs_results_work[i]) );
            g_kvs_results[i] = g_kvs_results_work[i];
        }
    }
    return 0;
}

/////////

bool dbAppendStringArray( const char *key, const char *field, const char *s, int trim) {
    g_wait_for_net_result = true;
    ssproto_kvs_append_string_array_send( g_dbconn, QID_APPENDSTRINGARRAY, key, field, s, trim );
    waitForReply();
    if( g_net_result_code == SSPROTO_OK ) {
        return true;
    } else {
        return false;
    }
}

int ssproto_kvs_append_string_array_result_recv( conn_t _c, int query_id, int retcode, const char *key, const char *field ) {
    //    print("ssproto_kvs_append_string_array_result_recv: '%s':'%s' ret:%d", key,field,retcode);
    if( query_id == QID_APPENDSTRINGARRAY ) {
        g_wait_for_net_result = false;
        g_net_result_code = retcode;
    }
    return 0;
}

// outn: pass max length of array and get actual length of loaded array
bool dbGetStringArray( const char *key, const char *field, char **out, int *outn ) {
    g_wait_for_net_result = true;
    ssproto_kvs_get_string_array_send( g_dbconn, QID_GETSTRINGARRAY, key, field );
    waitForReply();
    if( g_net_result_code == SSPROTO_OK ) {
        int maxnum = *outn;
        int n = MIN(maxnum, g_kvs_list_size);
        for(int i=0;i<n;i++) {
            //            print("dbGetStringArray: %d: '%s'", i,  g_kvs_results[i] );
            out[i] = g_kvs_results[i];
        }
        *outn = n;
        return true;
    } else {
        return false;
    }
}

int ssproto_kvs_get_string_array_result_recv( conn_t _c, int query_id, int retcode, const char *key, const char *field, const char * const *result, int result_len ) {
    if( query_id == QID_GETSTRINGARRAY ) {
        g_wait_for_net_result = false;
        g_net_result_code = retcode;
        g_kvs_list_size = result_len;
        for(int i=0;i<result_len;i++) {
            //            print("result: %d: '%s'", i, result[i] );            
            strncpy( g_kvs_results_work[i], result[i], sizeof(g_kvs_results_work[i]));
            g_kvs_results[i] = g_kvs_results_work[i];
        }
    }
    return 0;
}

// Save with timestamp
bool dbAppendProjectStatus( int pjid, const char *msg ) {
    bool res = dbAppendStringArray( g_project_status_ht_key,
                                    Format( "%d", pjid).buf,
                                    Format( "%u %s", dbGetTime(), msg ).buf,
                                    PROJECT_STATUS_LEN );
    return res;
}


bool dbLoadProjectStatus( int pjid, ProjectEvent *ev, int *evnum  ) {
    char *ary[100];
    int n = elementof(ary);
    bool res = dbGetStringArray( g_project_status_ht_key,
                                 Format( "%d", pjid ).buf,
                                 ary, &n );
    if(!res) return false;
    int copy_n = MIN(n,*evnum);
    *evnum = copy_n;
    for(int i=0;i<copy_n;i++){
        char *tk0 = strtok(ary[i], " " ); // Number at the beginning
        char *tk1 = ary[i] + strlen(tk0)+1; // Number at the latter part
        ev[i].at = strtoul( tk0, NULL,10);
        snprintf( ev[i].caption, sizeof(ev[i].caption), "%s", tk1 );
        print("dbLoadProjectStatus: ary[%d]:'%s' t:%u cap:'%s'", i, ary[i], (unsigned int )ev[i].at, tk1 );
    }
    return true;
}

bool dbDeleteProject( int project_id ) {
    if( !isDBNetworkActive())return false;
    
    int projids[32];
    int projids_len = elementof(projids);
    bool res = dbLoadIntArrayJSON( g_project_by_owner_ht_key, g_user_name, projids, &projids_len );
    if(!res) return false;

    int modified[32];
    int outi=0;
    for(int i=0;i<projids_len;i++) {
        if( projids[i] == project_id ) {
            print("dbDeleteProject: found project_id:%d", project_id );
            continue;
        }
        modified[outi] = projids[i];
        outi++;
    }
    res = dbSaveIntArrayJSON( g_project_by_owner_ht_key, g_user_name, modified, outi );
    if(!res) {
        print("dbDeleteProject: can't save");
        return false;
    }
    dbUnshareProjectSend( project_id );
    dbUnpublishProjectSend( project_id );
    return true;
}

void dbUnshareProjectSend( int project_id ) {
    print("dbUnshareProjectSend: pjid:%d",project_id );
    ssproto_unshare_project_send( g_dbconn, project_id );
}
void dbUnpublishProjectSend( int project_id ) {
    print("dbUnpublishProjectSend: pjid:%d",project_id );    
    ssproto_unpublish_project_send( g_dbconn, project_id );
}
    
    



/////////////////
// realtime game sync packet types
typedef enum {
    PACKETTYPE_CELL = 1,
    PACKETTYPE_CHUNK_SNAPSHOT,
    PACKETTYPE_PC,
    PACKETTYPE_PC_TRY_ACTION,
    PACKETTYPE_NEW_BEAM, // 5
    PACKETTYPE_CREATE_BEAM_SPARK_EFFECT,
    PACKETTYPE_CHAR_DELETE,
    PACKETTYPE_PLAY_SOUND,
    PACKETTYPE_EFFECT,
    PACKETTYPE_NEW_DEBRI, // 10
    PACKETTYPE_NEW_BULLET,
    PACKETTYPE_NEW_ENEMY,
    PACKETTYPE_ENEMY_MOVE, // 13
    PACKETTYPE_ENEMY_DAMAGE,
    PACKETTYPE_NEW_FLYER,
    PACKETTYPE_EVENT,
    PACKETTYPE_DEBUG_COMMAND,
    PACKETTYPE_POWERNODE,
    PACKETTYPE_POWERGRID,
    PACKETTYPE_ENERGY_CHAIN,
    PACKETTYPE_PARTY,
    PACKETTYPE_REVEAL,
    PACKETTYPE_FOLLOW,
    PACKETTYPE_CELL_WITH_POS,
    PACKETTYPE_ENEMY_KNOCKBACK,
} PACKETTYPE;


////////
void nearcastLog(char ch) {
    if( g_enable_nearcast_log ) prt("%c",ch);
}
void realtimeJoinChannelSend( int pid ) {
    ssproto_join_channel_send( g_rtconn, pid );
}

// send_far sends to distant players. channelcast(everyone in a project) when destruction.
void realtimeSyncCellSend( Cell *c, char chgtype, bool send_far ) {
    if(!c)return;
    int x,y;
    g_fld->getCellXY( c, &x, &y );
    nearcastLog('C');
    int dia = SYNC_CELL_DIA;
    if( send_far ) dia = DESTROY_CELL_DIA; // ALL
    ssproto_nearcast_send( g_rtconn, x,y, dia, PACKETTYPE_CELL, (const char*) c, sizeof(*c) );
}

void realtimeUpdateNearcastPositionSend( int x, int y ) {
    ssproto_update_nearcast_position_send( g_rtconn, x,y );
}
void realtimeSyncChunkSnapshotSend( int chx, int chy ) {
    char buf[sizeof(Cell)*CHUNKSZ*CHUNKSZ];
    int ind=0;
    for(int y=0;y<CHUNKSZ;y++) {
        for(int x=0;x<CHUNKSZ;x++) {
            Cell *fromc = g_fld->get(chx*CHUNKSZ+x, chy*CHUNKSZ+y);
            char *ptr = buf + sizeof(Cell) * ind;
            memcpy( ptr, (char*) fromc, sizeof(Cell) );
            ind++;
        }
    }
    char compbuf[ sizeof(buf)*2 ];
    int compressed_len = memCompressSnappy( compbuf, sizeof(compbuf)*2, buf, sizeof(buf) );
    assert( compressed_len > 0 );
    nearcastLog('S');
    //    size_t sz = sizeof(Cell) * CHUNKSZ * CHUNKSZ;
    ssproto_nearcast_send( g_rtconn, chx*CHUNKSZ, chy*CHUNKSZ, SYNC_CELL_DIA, PACKETTYPE_CHUNK_SNAPSHOT, (const char*)compbuf, compressed_len );
}

class CharSyncPacket {
public:
    Vec2 loc;
    int client_id; // Derived from conn_t serial number in realtime backend.
    int internal_id; // Prop2D's id. Other game servers may have same numbers for different objects.
    CharSyncPacket() : loc(0,0), client_id(0), internal_id(0) {}
    CharSyncPacket(Char *ch) : loc(ch->loc) {
        if( ch->client_id > 0 ) {
            client_id = ch->client_id;
            internal_id = ch->internal_id;
        } else {
            client_id = g_realtime_client_id;
            internal_id = ch->id;
        }
    }
};


class PCSyncPacket : public CharSyncPacket {
public:
    int shoot_sound_index;
    int hair_base_index, face_base_index, body_base_index, beam_base_index;
    DIR dir;
    ITEMTYPE equip_itt;
    int ene,maxene,hp,maxhp;
    int died;
    int warping;
    PCSyncPacket(PC *pc) : CharSyncPacket(pc),
                           shoot_sound_index(pc->shoot_sound_index),
                           hair_base_index(pc->hair_base_index),
                           face_base_index(pc->face_base_index),
                           body_base_index(pc->body_base_index),
                           beam_base_index(pc->beam_base_index),
                           dir(pc->dir), ene(pc->ene), maxene(pc->maxene), hp(pc->hp), maxhp(pc->maxhp),
                           warping(pc->warping) {
        ItemConf *itc = pc->items[pc->selected_item_index].conf;
        if(itc) equip_itt = itc->itt; else equip_itt = ITT_EMPTY;
        died = pc->died_at > 0;
    }
};

#define NEARCAST_SEND( ch, dia, pkttype, pkt,logchar )                         \
    nearcastLog(logchar), ssproto_nearcast_send( g_rtconn, ch->loc.x/PPC, ch->loc.y/PPC, dia, pkttype, (const char*)&pkt, sizeof(pkt) );

void realtimeSyncPCSend( PC *pc ) {
    PCSyncPacket pkt(pc);
    NEARCAST_SEND( pc, SYNC_CHAR_DIA, PACKETTYPE_PC, pkt, '@' );
}

class PartyPacket : public PCSyncPacket {
public:
    char nickname[PC_NICKNAME_MAXLEN+1];
    long long score;
    int user_id;
    PartyPacket( PC *pc, int user_id ) : PCSyncPacket(pc), score(pc->score), user_id(user_id) {
        strncpy( nickname, pc->nickname, sizeof(nickname));
    }
};

void realtimePartySend() {
    PartyPacket pkt(g_pc,g_user_id);
    pkt.client_id = g_realtime_client_id;
    assert(g_user_id>0);
    ssproto_channelcast_send( g_rtconn, g_current_project_id, PACKETTYPE_PARTY, (const char*) &pkt, sizeof(pkt) );
}


class NewBeamPacket : public CharSyncPacket {
public:
    Vec2 v;
    int base_index;
    int ene;
    BEAMTYPE type;
    NewBeamPacket(Beam *b) : CharSyncPacket(b), v(b->v), base_index(b->index), ene(b->ene), type(b->type) {}
};

void realtimeNewBeamSend( Beam *b ) {
    NewBeamPacket pkt(b);
    NEARCAST_SEND( b, SYNC_CHAR_DIA, PACKETTYPE_NEW_BEAM, pkt, 'b' );
} 

void realtimeCreateBeamSparkEffectSend( Beam *b ) {
    CharSyncPacket pkt(b);
    NEARCAST_SEND( b, SYNC_CHAR_DIA, PACKETTYPE_CREATE_BEAM_SPARK_EFFECT, pkt, 'k' );
}

void realtimeCharDeleteSend( Char *ch ) {
    CharSyncPacket pkt(ch);
    ssproto_channelcast_send( g_rtconn, g_current_project_id, PACKETTYPE_CHAR_DELETE, (const char*)&pkt, sizeof(pkt) );
    //   NEARCAST_SEND( ch, DELETE_CHAR_DIA, PACKETTYPE_CHAR_DELETE, pkt, 'd' );        
}
class EnemyMovePacket : public CharSyncPacket {
public:
    ENEMYTYPE type;
    Vec2 v;
    Vec2 target;
    EnemyMovePacket(Enemy*e) : CharSyncPacket(e), type(e->enemy_type), v(e->v), target(e->target) {}
};
void realtimeEnemyMoveSend( Enemy *e ) {
    EnemyMovePacket pkt(e);
    NEARCAST_SEND( e, SYNC_CHAR_DIA, PACKETTYPE_ENEMY_MOVE, pkt, 'm' );
}
class EnemyKnockbackPacket : public CharSyncPacket {
public:
    ENEMYTYPE type;
    Vec2 v;
    EnemyKnockbackPacket(Enemy *e) : CharSyncPacket(e), type(e->enemy_type), v(e->v) {}
};
void realtimeEnemyKnockbackSend( Enemy *e ) {
    EnemyKnockbackPacket pkt(e);
    NEARCAST_SEND( e, SYNC_CHAR_DIA, PACKETTYPE_ENEMY_KNOCKBACK, pkt, 'k' );
}



class PlaySoundPacket {
public:
    Vec2 at;
    int sound_id;
    PlaySoundPacket( Vec2 at, int sndid ) : at(at), sound_id(sndid) {}
};

void realtimePlaySoundSend( Sound *snd, Vec2 at) {
    PlaySoundPacket pkt(at,snd->id);
    nearcastLog('p');
    ssproto_nearcast_send( g_rtconn, at.x/PPC, at.y/PPC, SYNC_CHAR_DIA, PACKETTYPE_PLAY_SOUND, (const char*) &pkt, sizeof(pkt) );
}

class EffectPacket {
public:
    EFFECTTYPE type;
    Vec2 at;
    float opts[2];
    EffectPacket( EFFECTTYPE t, Vec2 at, float opt0, float opt1 ) : type(t), at(at) {
        opts[0] = opt0;
        opts[1] = opt1;
    }
};
void realtimeEffectSend( EFFECTTYPE et, Vec2 at, float opt0, float opt1 ) {
    EffectPacket pkt( et, at, opt0, opt1 );
    nearcastLog('e');
    ssproto_nearcast_send( g_rtconn, at.x/PPC, at.y/PPC, SYNC_EFFECT_DIA, PACKETTYPE_EFFECT, (const char *)&pkt, sizeof(pkt) );
}

class NewDebriPacket : public CharSyncPacket {
public:
    Vec2 iniv;
    int index;
    NewDebriPacket( Debri *d ) : CharSyncPacket(d), iniv(d->v), index(d->index) {}
};

void realtimeNewDebriSend( Debri *d ) {
    NewDebriPacket pkt(d);
    NEARCAST_SEND( d, SYNC_CHAR_DIA, PACKETTYPE_NEW_DEBRI, pkt, 'r' );
}
class NewBulletPacket : public CharSyncPacket {
public:
    BULLETTYPE type;
    Vec2 v;
    NewBulletPacket( Bullet *b ) : CharSyncPacket(b), type(b->bullet_type), v(b->v) {}
};

void realtimeNewBulletSend( Bullet *b ) {
    assert( b->bullet_type != 0 );
    NewBulletPacket pkt(b);
    NEARCAST_SEND( b, SYNC_CHAR_DIA, PACKETTYPE_NEW_BULLET, pkt, 'B' );
}

class NewEnemyPacket : public CharSyncPacket {
public:
    ENEMYTYPE type;
    Vec2 iniv;
    NewEnemyPacket(Enemy *e ) : CharSyncPacket(e), type(e->enemy_type), iniv(e->v) {}
};

void realtimeNewEnemySend( Enemy *e ) {
    NewEnemyPacket pkt(e);
    NEARCAST_SEND( e, SYNC_CELL_DIA, PACKETTYPE_NEW_ENEMY, pkt, 'E' );
}

class EnemyDamagePacket : public CharSyncPacket {
public:
    int damage;
    EnemyDamagePacket(Enemy *e,int damage) : CharSyncPacket(e), damage(damage) {}
};
void realtimeEnemyDamageSend( Enemy *e, int damage ) {
    EnemyDamagePacket pkt(e,damage);
    NEARCAST_SEND( e, SYNC_CHAR_DIA, PACKETTYPE_ENEMY_DAMAGE, pkt, 'D' );
}

class NewFlyerPacket : public CharSyncPacket {
public:
    FLYERTYPE type;
    Vec2 v;
    float h;
    float v_h;
    float gravity;
    NewFlyerPacket( Flyer *f ) : CharSyncPacket(f), type(f->type), v(f->v), h(f->h), v_h(f->v_h), gravity(f->gravity) {}
};
void realtimeNewFlyerSend( Flyer *f ) {
    NewFlyerPacket pkt(f);
    NEARCAST_SEND( f, SYNC_CHAR_DIA, PACKETTYPE_NEW_FLYER, pkt, 'f' );
}


class EnergyChainPacket : public CharSyncPacket {
public:
    int ene_to_charge;
    EnergyChainPacket( PC *tgt, int ene ) : CharSyncPacket(tgt), ene_to_charge(ene) {}
};
void realtimeEnergyChainSend( PC *tgt, int ene ) {
    EnergyChainPacket pkt( tgt, ene );
    NEARCAST_SEND( tgt, SYNC_CHAR_DIA, PACKETTYPE_ENERGY_CHAIN, pkt, 'h' );
}

class FollowPacket {
public:
    int following_user_id; // Now this game server is trying to follow this user.
    int followed_user_id; // Now this user is about to be followed. Receiver checks if this ID is equal to itself and add modify its list.
    char nickname_following[PC_NICKNAME_MAXLEN+1];
    char nickname_followed[PC_NICKNAME_MAXLEN+1];
    int face_ind, hair_ind;
    FollowPacket( int following, int followed, const char *nick_follow, const char *nick_followed, int face, int hair ) : following_user_id(following), followed_user_id(followed), face_ind(face), hair_ind(hair) {
        strncpy( nickname_following, nick_follow, sizeof(nickname_following) );
        strncpy( nickname_followed, nick_followed, sizeof(nickname_followed) );        
    }
};

void realtimeFollowSend( int following_uid, int followed_uid, const char *nick_follow, const char *nick_followed, int f, int h ) {
    FollowPacket pkt( following_uid, followed_uid, nick_follow, nick_followed, f, h );
    ssproto_channelcast_send( g_rtconn, g_current_project_id, PACKETTYPE_FOLLOW, (const char*)&pkt, sizeof(pkt) );
}

/////////////////

class EventPacket {
public:
    EVENTTYPE type;
    char message[64];
    int opts[2];
    
    EventPacket( EVENTTYPE t, const char *msg, int opt0, int opt1 ) : type(t) {
        snprintf( message, sizeof(message), "%s", msg);
        opts[0] = opt0;
        opts[1] = opt1;
    }
};

void realtimeEventSend( EVENTTYPE t, const char *msg, int opt0, int opt1 ) {
    EventPacket pkt( t, msg, opt0, opt1 );
    ssproto_channelcast_send( g_rtconn, g_current_project_id, PACKETTYPE_EVENT, (const char*)&pkt, sizeof(pkt));
}

class PowerGridPacket {
public:
    int powergrid_id;
    int ene_diff;
    PowerGridPacket( int powergrid_id, int ene_diff ) : powergrid_id(powergrid_id), ene_diff(ene_diff) {}
};
void realtimePowerGridSend( int powergrid_id, int ene_diff ) {
    PowerGridPacket pkt( powergrid_id, ene_diff );
    ssproto_channelcast_send( g_rtconn, g_current_project_id, PACKETTYPE_POWERGRID, (const char*)&pkt, sizeof(pkt));
}

class RevealPacket {
public:
    int x,y;
    RevealPacket(int x, int y ) : x(x), y(y) {}
};
void realtimeRevealSend( int x, int y ) {
    RevealPacket pkt(x,y);
    ssproto_channelcast_send( g_rtconn, g_current_project_id, PACKETTYPE_REVEAL, (const char*)&pkt, sizeof(pkt));
}


///////////


void broadcast( int type_id, const char *data, int data_len ) {
    if( isRealtimeNetworkActive() ) {
        int res = ssproto_broadcast_send( g_rtconn, type_id, data, data_len );
        print("broadcast: res:%d",res);
    }
}

class DebugCommandPacket {
public:
    char command[128];
};

void realtimeDebugCommandBroadcast( const char *cmd ) {
    DebugCommandPacket pkt;
    snprintf(pkt.command, sizeof(pkt.command), "%s", cmd );
    broadcast( PACKETTYPE_DEBUG_COMMAND, (const char*)&pkt,sizeof(pkt));
}


int ssproto_broadcast_notify_recv( conn_t _c, int cli_serial, int type_id, const char *data, int data_len ) {
    print("ssproto_broadcast_notify_recv: t:%d data_len:%d", type_id, data_len );
    switch(type_id) {
    case PACKETTYPE_DEBUG_COMMAND:
        {
            assert( data_len == sizeof(DebugCommandPacket));
            DebugCommandPacket *pkt = (DebugCommandPacket*)data;
            if( strcmp(pkt->command, "exit_program" ) == 0 ) {
                g_program_finished = true;
            }
        }
        break;
    default:
        assertmsg(false, "invalid broadcast packet type:%d", type_id );
        break;
    }
    return 0;
}

// Send when putting a POLE. Receivers will put a pole but don't save field.
class PowerNodePacket {
public:
    int powergrid_id;
    Pos2 at;
    PowerNodePacket( int powergrid_id, Pos2 at) : powergrid_id(powergrid_id), at(at) {}
};

void realtimePowerNodeSend( int powergrid_id, Pos2 at ) {
    PowerNodePacket pkt( powergrid_id, at );
    ssproto_channelcast_send( g_rtconn, g_current_project_id, PACKETTYPE_POWERNODE, (const char*)&pkt, sizeof(pkt) );
}

int ssproto_channelcast_notify_recv( conn_t _c, int channel_id, int sender_cli_id, int type_id, const char *data, int data_len ) {
    if( type_id == PACKETTYPE_EVENT ) {
        if( g_game_paused ) return 0;        
        assert( data_len == sizeof(EventPacket) );
        EventPacket *pkt = (EventPacket*) data;
        switch(pkt->type) {
        case EVT_MILESTONE:
            hudMilestoneMessage( pkt->message, pkt->opts[0], false );
            //            g_log->printf(WHITE, "hudUpdateMilestone EVT_MILESTONE:%d ", pkt->opts[0] );
            hudUpdateMilestone( pkt->opts[0] );
            soundPlayAt(g_researchdone_sound,g_pc->loc,1);            
            break;
        case EVT_FORTRESS_DESTROYED:
            hudFortressDestroyMessage( pkt->message, pkt->opts[0], false );
            soundPlayAt( g_wipe_fortress_sound, g_pc->loc,1 );
            break;
        case EVT_LOGIN:
            g_log->printf(WHITE, "%s JOINED THIS PROJECT.", pkt->message );
            break;
        case EVT_LOGOUT:
            g_log->printf(WHITE, "%s LEFT THIS PROJECT.", pkt->message );
            break;
        case EVT_CLEAR_FLAG:
            {
                Pos2 flag_pos( pkt->opts[0], pkt->opts[1] );
                g_log->printf(WHITE, "EVT_CLEAR_FLAG: %d,%d", flag_pos.x, flag_pos.y );
                g_fld->clearFlag(flag_pos);
                g_flag->warpToNextLocation();
            }
            break;
        case EVT_RESEARCH_DONE:
            {
                ItemConf *itc = ItemConf::getItemConf( (ITEMTYPE) pkt->opts[0] );
                g_log->printf(WHITE, "RESEARCH ON %s DONE BY %s", itc->name, pkt->message );
                if( g_labwin->visible ) g_labwin->update();
            }
            break;
        }
    } else if( type_id == PACKETTYPE_POWERNODE ) {
        if( g_game_paused ) return 0;
        assert( data_len == sizeof(PowerNodePacket) );
        PowerNodePacket *pkt = (PowerNodePacket*) data;
        print("ssproto_channelcast_notify_recv: PowerNode packet! at:%d,%d gid:%d", pkt->at.x, pkt->at.y, pkt->powergrid_id );
        PowerGrid *pg = g_ps->findGridById( pkt->powergrid_id );
        if(pg) {
            PowerNode *pn = pg->findNode( pkt->at );
            if(pn) {
                // Put at the same position at the same time? Ignore.
                print("PowerNodePacket: PowerNode found! ignoreing.");
            } else {
                print("PowerNodePacket: PowerNode not found! adding." );
                g_ps->addNode( pkt->at );
            }
        } else {
            // No powergrid, it means it's the first power grid.
            g_ps->addNode( pkt->at );
        }
    } else if( type_id == PACKETTYPE_POWERGRID ) {
        if( g_game_paused ) return 0;        
        assert( data_len == sizeof(PowerGridPacket) );
        PowerGridPacket *pkt = (PowerGridPacket*) data;
        PowerGrid *pg = g_ps->findGridById( pkt->powergrid_id );
        if(pg) {
            pg->modEne( pkt->ene_diff, false );
        }
    } else if( type_id == PACKETTYPE_PARTY ) {
        assert( data_len == sizeof(PartyPacket) );
        PartyPacket *pkt = (PartyPacket*) data;
        //        print("party packet received. cid:%d nick:%s uid:%d score:%lld", pkt->client_id, pkt->nickname, pkt->user_id, pkt->score );
        ensurePartyIndicator( pkt->client_id, pkt->loc, pkt->face_base_index, pkt->hair_base_index );
        g_wm->ensureRemotePC( pkt->client_id, pkt->nickname, pkt->face_base_index, pkt->hair_base_index, pkt->loc );
        PC *pc = (PC*) Char::getByNetworkId( pkt->client_id, pkt->internal_id );
        if(pc) {
            pc->user_id = pkt->user_id;
            pc->score = pkt->score;
            strncpy( pc->nickname, pkt->nickname, sizeof(pc->nickname));
        }
        if( g_projinfowin->visible ) {
            print("infowin recv party packet. uid:%d nick:%s loc:%.1f,%.1f", pkt->user_id, pkt->nickname, pkt->loc.x, pkt->loc.y );
            g_projinfowin->updateParty( pkt->user_id,
                                        pkt->nickname,
                                        pkt->loc,
                                        pkt->hair_base_index, pkt->face_base_index
                                        );
        }
    } else if( type_id == PACKETTYPE_REVEAL ) {
        if( g_game_paused ) return 0;        
        assert( data_len == sizeof(RevealPacket) );
        RevealPacket *pkt = (RevealPacket*) data;
        g_fld->setChunkRevealed(pkt->x,pkt->y,true);
    } else if( type_id == PACKETTYPE_FOLLOW ) {
        if( g_game_paused ) return 0;        
        assert( data_len == sizeof(FollowPacket) );
        FollowPacket *pkt = (FollowPacket*) data;
        print("received FollowPacket. following:%d", pkt->following_user_id );
        if( pkt->followed_user_id == g_user_id ) {
            addFollowerContact( pkt->following_user_id, pkt->nickname_following, pkt->face_ind, pkt->hair_ind );
            dbSaveFollowers();
            if( isAFriend( pkt->followed_user_id ) || isAFriend(pkt->following_user_id) ) {
                g_log->printf(WHITE, "NOW %s IS FOLLOWING YOU, MADE A FRIEND!", pkt->nickname_following );                
            } else {
                g_log->printf(WHITE, "NOW %s IS FOLLOWING YOU.", pkt->nickname_following );
            }
        }
    } else if( type_id == PACKETTYPE_CHAR_DELETE ) {
        assert( data_len == sizeof(CharSyncPacket) );
        CharSyncPacket *pkt = (CharSyncPacket*) data;
        Char *ch;
        if( pkt->client_id == g_realtime_client_id ) {
            ch = (Char*) g_char_layer->findPropById(pkt->internal_id );
        } else {
            ch = Char::getByNetworkId( pkt->client_id, pkt->internal_id );
        }
        if(ch) ch->to_clean = true;
    }
    
    return 0;
}


int ssproto_join_channel_result_recv( conn_t _c, int channel_id, int retcode ) {
    assertmsg( retcode == SSPROTO_OK, "can't join the channel. rc:%d", retcode);
    print("join_channel result:%d", retcode);
    g_last_join_channel_id = channel_id;
    return 0;
}
int ssproto_leave_channel_result_recv( conn_t _c, int retcode ) {
    return 0;
}
int realtimeGetChannelMemberCountSync( int channel_id, int *maxnum ) {
    ssproto_get_channel_member_count_send( g_rtconn, channel_id );
    waitForReply(false);
    print("realtimeGetChannelMemberCountSync: max:%d cur:%d", g_net_result_value_option, g_net_result_value );
    *maxnum = g_net_result_value_option;
    return g_net_result_value;    
}
int ssproto_get_channel_member_count_result_recv( conn_t _c, int channel_id, int maxnum, int curnum ) {
    print("ssproto_get_channel_member_count_result_recv. %d,%d,%d",channel_id, maxnum, curnum );
    g_net_result_code = SSPROTO_OK; 
    g_net_result_value = curnum;
    g_net_result_value_option = maxnum;
    g_wait_for_net_result = false;
    return 0;
}

void networkNewEnemy( ENEMYTYPE et, Vec2 at, int client_id, int internal_id, Vec2 v ) {
    if( g_fld->isChunkLoaded(at) == false ) return;
    
#define NEWENEMY(f) e = new f(at, client_id, internal_id )
    Enemy *e = NULL;
    switch(et) {
    case ET_FLY: NEWENEMY(Fly); break;
    case ET_WORM: NEWENEMY(Worm); break;
    case ET_EGG: NEWENEMY(Egg); break;
    case ET_TAKWASHI: NEWENEMY(Takwashi); break;
    case ET_REPAIRER: NEWENEMY(Repairer); break;
    case ET_CHASER: NEWENEMY(Chaser); break;
    case ET_BUILDER: NEWENEMY(Builder); break;
    case ET_GIREV: NEWENEMY(Girev); break;
    case ET_HYPERPUTTER: NEWENEMY(HyperPutter); break;
    case ET_VOIDER: NEWENEMY(Voider); break;
    case ET_FIREGENERATOR: NEWENEMY(FireGenerator); break;
    case ET_FIRECOPTER: NEWENEMY(FireCopter); break;
    case ET_SNOWSLIME: NEWENEMY(SnowSlime); break;
    case ET_BIRD: NEWENEMY(Bird); break;
    case ET_SHRIMP: NEWENEMY(Shrimp); break;
    case ET_BEE: NEWENEMY(Bee); break;
    case ET_DEFENDER: NEWENEMY(Defender); break;
    case ET_TELEPORTER: NEWENEMY(Teleporter); break;
    case ET_COREBALL: NEWENEMY(CoreBall); break;
    default:
        assertmsg(false, "networkNewEnemy: invalid enemy type:%d", et );
    }
    e->v = v;
}        

void packetCount(int pktt, size_t sz ) {
    static size_t alltotalsz = 0;
    static int allcount = 0;
    static size_t sectotalsz = 0;
    static int seccount = 0;    
    
    static int count[256];
    static size_t totsz[256];

    static double count_start_at = 0;
    static double last_sec_count_at = 0;

    if( count_start_at == 0 ) count_start_at = now();

    alltotalsz += sz;
    allcount ++;
    sectotalsz += sz;
    seccount ++;    
    
    if( pktt >=0 && pktt < 256 ) {
        count[pktt]++;
        totsz[pktt]+=sz;
        if( count[pktt]%1000 == 0 ) {
            double elt = now() - count_start_at;
            print("%2d:PKT[%d] : %.1fkq size:%.1fKB Tot:%.1fkq %.1fKB Avg:%.1fkq %.1fKB Sec:%.2fkq %.1fKB",
                  g_realtime_client_id,
                  pktt, (float)count[pktt]/1000.0f, totsz[pktt]/1000.0f,
                  (float)allcount/1000.0f, (float)alltotalsz/1000.0f,
                  (float)allcount/1000.0f/elt, (float)alltotalsz/1000.0f/elt,
                  (float)seccount/1000.0f, (float)sectotalsz/1000.0f
                  );
            if( last_sec_count_at < now() - 1.0 ) {
                last_sec_count_at = now();
                sectotalsz = seccount = 0;
            }
        }
    } else {
        print("unknown packet type:%d", pktt );
    }
}

int ssproto_nearcast_notify_recv( conn_t _c, int channel_id, int sender_cli_id, int x, int y, int range, int type_id, const char *data, int data_len ) {
    if( g_game_paused ) return 0;

    packetCount(type_id, data_len );
    
    Vec2 at( x*PPC,y*PPC );
    if( type_id == PACKETTYPE_CELL ) {
        assert( data_len == sizeof(Cell));
        Cell *outc = g_fld->get(x,y);
        //        if( outc->bt == BT_POLE ) {
        if(outc) {
            //            Cell *c = (Cell*)data;
            //            print("CELL: sync: sync_cell cli:%d ch:%d (%d,%d) r:%d t:%d data_len:%d bt:%d",
            //                  sender_cli_id, channel_id, x,y,range,type_id, data_len, c->bt );
        }
        memcpy( (char*)outc, data, data_len );
        int chx = x/CHUNKSZ, chy = y/CHUNKSZ;
        g_mapview->notifyChanged(chx,chy);
    } else if( type_id == PACKETTYPE_CHUNK_SNAPSHOT ) {
        char uncompbuf[sizeof(Cell) * CHUNKSZ * CHUNKSZ];
        int uncomplen = memDecompressSnappy( uncompbuf, sizeof(uncompbuf), (char*)data, data_len );
        assert( uncomplen == sizeof(uncompbuf) ); 
        assert( x % CHUNKSZ == 0 );
        assert( y % CHUNKSZ == 0 );
        //        print( "ssproto_nearcast_notify_recv: sync_snapshot xy:%d,%d",x,y);
        int ind=0;
        for(int yy=y;yy<y+CHUNKSZ;yy++) {
            for(int xx=x;xx<x+CHUNKSZ;xx++) {
                Cell *toc = g_fld->get(xx,yy);
                assert(toc);
                const char *ptr = uncompbuf + sizeof(Cell) * ind;
                memcpy( (char*)toc, ptr, sizeof(Cell));
                assertmsg( toc->bt >= 0 && toc->bt <= 255, "invalid bt:%d",toc->bt );
                ind++;
            }
        }
        g_mapview->notifyChanged( x / CHUNKSZ, y / CHUNKSZ );
    } else if( type_id == PACKETTYPE_PC ) {
        assert( data_len == sizeof(PCSyncPacket) );
        PCSyncPacket *pkt = (PCSyncPacket*) data;
        PC *pc = (PC*) Char::getByNetworkId( pkt->client_id, pkt->internal_id );
        //            print("pc packet. loc:%f %f cid:%d iid:%d", pkt->loc.x, pkt->loc.y, pkt->client_id, pkt->internal_id );
        if(pc==NULL) {
            PC *newpc = new PC(pkt->loc);
            newpc->setNetworkId( pkt->client_id, pkt->internal_id );
            newpc->shoot_sound_index = pkt->shoot_sound_index;
            newpc->hair_base_index = pkt->hair_base_index;
            newpc->body_base_index = pkt->body_base_index;
            newpc->face_base_index = pkt->face_base_index;
            newpc->beam_base_index = pkt->beam_base_index;
            newpc->enable_enebar = newpc->enable_hpbar = true;
        } else {
            pc->warping = pkt->warping;
            pc->loc = pkt->loc;
            pc->dir = pkt->dir;
            if( pkt->equip_itt == ITT_EMPTY ) {
                pc->items[PC_SUIT_NUM].setEmpty();
            } else {
                pc->items[PC_SUIT_NUM].set( pkt->equip_itt, 1, 1);
                pc->selected_item_index = PC_SUIT_NUM;
            }
            pc->hp = pkt->hp;
            pc->maxhp = pkt->maxhp;
            pc->ene = pkt->ene;
            pc->maxene = pkt->maxene;
            if( pkt->died ) {
                if( pc->died_at == 0 ) pc->died_at = pc->accum_time;
            } else {
                if( pc->died_at > 0 ) pc->died_at = 0;
            }
            pc->last_network_update_at = pc->accum_time;
            ensurePartyIndicator( pkt->client_id, pkt->loc, pkt->face_base_index, pkt->hair_base_index );
            g_wm->ensureRemotePC( pkt->client_id, NULL, pkt->face_base_index, pkt->hair_base_index, pkt->loc );
        }
    } else if( type_id == PACKETTYPE_NEW_BEAM ) {
        assert( data_len == sizeof(NewBeamPacket) );
        NewBeamPacket *pkt = (NewBeamPacket*) data;
        new Beam( pkt->loc, pkt->loc + pkt->v, pkt->type, pkt->base_index, pkt->ene, 0, pkt->client_id, pkt->internal_id );
    } else if( type_id == PACKETTYPE_CREATE_BEAM_SPARK_EFFECT ) {
        assert( data_len == sizeof(CharSyncPacket) );
        CharSyncPacket *pkt = (CharSyncPacket*) data;
        Beam *b = (Beam*) Char::getByNetworkId( pkt->client_id, pkt->internal_id );
        if(b) b->createSparkEffect();
    } else if( type_id == PACKETTYPE_CHAR_DELETE ) {
        // moved to channelcast
    } else if( type_id == PACKETTYPE_PLAY_SOUND ) {
        assert( data_len == sizeof(PlaySoundPacket) );
        PlaySoundPacket *pkt = (PlaySoundPacket*)data;
        Sound *s = g_sound_system->getById( pkt->sound_id );
        if(s) {
            soundPlayAt( s, pkt->at,1, false );
        } else {
            print("playsoundpacket: sound id: %d is not found", pkt->sound_id );
        }
        
    } else if( type_id == PACKETTYPE_EFFECT ) {
        assert( data_len == sizeof(EffectPacket) );
        EffectPacket *pkt = (EffectPacket*) data;
        switch(pkt->type){
        case EFT_DIGSMOKE:
            createDigSmoke( pkt->at, pkt->opts[0], false);
            break;
        case EFT_BLOOD:
            createBloodEffect( pkt->at, pkt->opts[0], false );
            break;
        case EFT_SMOKE:
            createSmokeEffect( pkt->at, false );
            break;
        case EFT_POSITIVE:
            createPositiveEffect( pkt->at, pkt->opts[0], false );
            break;
        case EFT_CHARGE:
            createChargeEffect( pkt->at, pkt->opts[0], false );
            break;
        case EFT_SPARK:
            createSpark( pkt->at, pkt->opts[0], false );
            break;
        case EFT_SHIELD_SPARK:
            createShieldSpark( pkt->at, false );
            break;
        case EFT_LEAF:
            createLeafEffect( pkt->at, false );
            break;
        case EFT_CELL_BUBBLE:
            if( pkt->opts[0] == 0 ) {
                createCellBubbleRed( pkt->at, false );                
            } else {
                createCellBubbleBlue( pkt->at, false  );
            }
            break;
        case EFT_EXPLOSION:
            createExplosion( pkt->at, pkt->opts[0], pkt->opts[1], false );
            break;
        case EFT_BOSS_EXPLOSION:
            createBossExplosion( pkt->at, false );
            break;
        default:
            assertmsg(false, "invalid effect type:%d",pkt->type);
        }
    } else if( type_id == PACKETTYPE_NEW_DEBRI ) {
        assert( data_len == sizeof(NewDebriPacket) );
        NewDebriPacket *pkt = (NewDebriPacket*) data;
        new Debri( pkt->loc, pkt->iniv, pkt->index, pkt->client_id, pkt->internal_id );
    } else if( type_id == PACKETTYPE_NEW_BULLET ) {
        assert( data_len == sizeof(NewBulletPacket) );
        NewBulletPacket *pkt = (NewBulletPacket*) data;
        if( g_fld->isChunkLoaded( pkt->loc ) ) {
            new Bullet( pkt->type, pkt->loc, pkt->loc + pkt->v, pkt->client_id, pkt->internal_id );
        }
    } else if( type_id == PACKETTYPE_NEW_ENEMY ) {
        assert( data_len == sizeof(NewEnemyPacket) );
        NewEnemyPacket *pkt = (NewEnemyPacket*) data;
        if( g_fld->isChunkLoaded( pkt->loc ) ) {
            networkNewEnemy( pkt->type, pkt->loc, pkt->client_id, pkt->internal_id, pkt->iniv );
        }
    } else if( type_id == PACKETTYPE_ENEMY_MOVE ) {
        assert( data_len == sizeof(EnemyMovePacket) );
        EnemyMovePacket *pkt = (EnemyMovePacket*) data;
        Enemy *e = (Enemy*) Char::getByNetworkId( pkt->client_id, pkt->internal_id );
        if(e) {
            e->loc = pkt->loc;
            e->v = pkt->v;
            e->target = pkt->target;
        } else {
            networkNewEnemy( pkt->type, pkt->loc, pkt->client_id, pkt->internal_id, pkt->v );
        }
    } else if( type_id == PACKETTYPE_ENEMY_KNOCKBACK ) {
        assert( data_len == sizeof(EnemyKnockbackPacket) );
        EnemyKnockbackPacket *pkt = (EnemyKnockbackPacket*) data;
        Enemy *e = (Enemy*) Char::getByNetworkId( pkt->client_id, pkt->internal_id );
        //        g_log->printf(WHITE,"KB! e:%p clid:%d intid:%d", e, pkt->client_id, pkt->internal_id );
        if(!e) {
            // Someone shot my local enemie.
            e = (Enemy*) g_char_layer->findPropById( pkt->internal_id);
        }
        if(e) {
            e->loc = pkt->loc;
            e->v = pkt->v;
            if(e->enemy_type == ET_WORM){
                Worm *w = (Worm*) e;
                w->turn_at = w->accum_time + KNOCKBACK_DELAY;
            }
        }
    } else if( type_id == PACKETTYPE_ENEMY_DAMAGE ) {
        assert( data_len == sizeof(EnemyDamagePacket) );
        EnemyDamagePacket *pkt = (EnemyDamagePacket*) data;
        Enemy *e = (Enemy*) Char::getByNetworkId( pkt->client_id, pkt->internal_id );
        if(e) {
            e->applyDamage(pkt->damage);
        } else {
            if( pkt->client_id == g_realtime_client_id ) {
                // Someone shot my local enemie.
                Enemy *e = (Enemy*) g_char_layer->findPropById( pkt->internal_id );
                if(e) {
                    e->applyDamage(pkt->damage);
                }
            }
        }
    } else if( type_id == PACKETTYPE_NEW_FLYER ) {
        assert( data_len == sizeof(NewFlyerPacket) );
        NewFlyerPacket *pkt = (NewFlyerPacket*) data;
        
        Flyer *f = NULL;
        switch(pkt->type) {
        case FLT_BLASTER:
            f = new Blaster( pkt->loc, pkt->loc + pkt->v, 0, pkt->client_id, pkt->internal_id );
            break;
        case FLT_BOMBFLOWERSEED:
            f = new BombFlowerSeed( pkt->loc, pkt->client_id, pkt->internal_id );
            break;
        case FLT_VOLCANICBOMB:
            f = new VolcanicBomb( pkt->loc, pkt->client_id, pkt->internal_id );
            break;
        }
        assert(f);
        f->v = pkt->v;
        f->h = pkt->h;
        f->v_h = pkt->v_h;
        f->gravity = pkt->gravity;
    } else if( type_id == PACKETTYPE_ENERGY_CHAIN ) {
        assert( data_len == sizeof( EnergyChainPacket) );
        EnergyChainPacket *pkt = (EnergyChainPacket*) data;
        if( pkt->client_id == g_realtime_client_id ) {
            PC *pc = (PC*) g_char_layer->findPropById( pkt->internal_id );
            if(pc->id == g_pc->id ) {
                g_pc->charge(pkt->ene_to_charge);
            }
        }
        // 
        
    } else {
        assertmsg( false, "invalid packet type:%d",type_id );
    }
    
    return 0;
}

////////
bool realtimeUnlockGridSend( int chx, int chy ) {
    //    print("realtimeUnlockSend: %d,%d",chx,chy);
    return ssproto_unlock_grid_send( g_rtconn, g_current_project_id, chx, chy );
}
bool realtimeLockGridSend( int chx, int chy ) {
    //    print("realtimeLockSend: %d,%d",chx,chy);    
    return ssproto_lock_grid_send( g_rtconn, g_current_project_id, chx, chy );
}
bool realtimeLockKeepGridSend( int chx, int chy ) {
    return ssproto_lock_keep_grid_send( g_rtconn, g_current_project_id, chx, chy );
}
int ssproto_lock_grid_result_recv( conn_t _c, int grid_id, int x, int y, int retcode ) {
    //    print("ssproto_lock_grid_result_recv: gid:%d xy:%d,%d rc:%d", grid_id,x,y,retcode);
    assertmsg( grid_id == g_current_project_id, "current project id:%d from server:%d", g_current_project_id, grid_id  );
    if( retcode == SSPROTO_OK ) {
        g_fld->setLockGot( x, y );
    } 
    return 0;
}
int ssproto_unlock_grid_result_recv( conn_t _c, int grid_id, int x, int y, int retcode ) {
    //    print("ssproto_unlock_grid_result_recv: gid:%d xy:%d,%d, rc:%d", grid_id, x,y,retcode);
    if( retcode == SSPROTO_OK ) {
        //        print("UNLOCKED CHUNK %d,%d",x,y);
    } 
    return 0;
}

int ssproto_conn_serial_result_recv( conn_t _c, int serial ) {
    print("ssproto_conn_serial_result_recv. serial:%d",serial);
    assert( g_realtime_client_id == 0 );
    g_realtime_client_id = serial;
    return 0;
}

int ssproto_clean_all_result_recv( conn_t _c ) {
    print("ssproto_clean_all_result_recv.");
    assert( g_runstate == RS_LEAVING_PROJECT );
    g_runstate = RS_LEFT_PROJECT;
    return 0;
}



/////////////

// Project-wide lock.
// Used for reactors generating electricity, in far place (no chunks loaded)

extern double g_powersystem_lock_obtained_at;

int ssproto_lock_project_result_recv( conn_t _c, int project_id, int category, int retcode ) {
    //    print("ssproto_lock_project_result_recv: pid:%d cat:%d ret:%d", project_id, category, retcode );
    if( retcode == SSPROTO_OK ) {
        if( project_id == g_current_project_id ) {
            switch(category) {
            case LOCK_POWERSYSTEM:
                g_powersystem_lock_obtained_at = now();
                break;
            case LOCK_RESOURCEDEPO:
                if( g_exwin->visible ) g_exwin->onLockObtained();
                break;
            case LOCK_RESEARCH:
                if( g_labwin->visible ) g_labwin->onLockObtained();
                break;                
            default:
                assert(false);
            }            
        }
    }
    return 0;
}

int ssproto_unlock_project_result_recv( conn_t _c, int project_id, int category, int retcode ) {
    //    print("ssproto_unlock_project_result_recv: pid:%d cat:%d ret:%d", project_id, category, retcode );    
    if( retcode == SSPROTO_OK ) {
        if( project_id == g_current_project_id ) {
            switch(category) {
            case LOCK_POWERSYSTEM:
                assert( g_powersystem_lock_obtained_at > 0 );
                g_powersystem_lock_obtained_at = 0;
                break;
            case LOCK_RESOURCEDEPO:
                g_exwin->onLockReleased();
                break;
            case LOCK_RESEARCH:
                g_labwin->onLockReleased();
                break;
            default:
                assert(false);
            }
        } 
    }
    return 0;
}

// Project-wide locks
void realtimeLockProjectSend( int pjid, LOCKCATEGORY lc ) {
    ssproto_lock_project_send( g_rtconn, pjid, lc );
}
void realtimeUnlockProjectSend( int pjid, LOCKCATEGORY lc ) {
    ssproto_unlock_project_send( g_rtconn, pjid, lc );
}
void realtimeLockKeepProjectSend( int pjid, LOCKCATEGORY lc ) {
    ssproto_lock_keep_project_send( g_rtconn, pjid, lc );
}

void realtimeCleanAllSend() {
    ssproto_clean_all_send( g_rtconn );
}

// Try to get project-wide lock every some seconds.
void pollPowerSystemLock() {
    static double last_pwgrid_save_at = 0;
    
    if( g_current_project_id != 0 ) {
        double nt = now();
        if( g_powersystem_lock_obtained_at == 0 ) {
            realtimeLockProjectSend( g_current_project_id, LOCK_POWERSYSTEM );
        } else {
            if( g_powersystem_lock_obtained_at < nt - 2 ) {
                realtimeLockKeepProjectSend( g_current_project_id, LOCK_POWERSYSTEM );
            }
            if( last_pwgrid_save_at < nt - POWERSYSTEM_SAVE_INTERVAL ) {
                last_pwgrid_save_at = nt;
                savePowerSystem();
                refreshExchangeLinks();
            }
        }
    }
}

void reloadPowerSystem() {
    static PowerSystemDump psd;
    bool res = dbLoadPowerSystem( g_current_project_id, &psd );
    if(res) {
        delete g_ps;
        g_ps = new PowerSystem();
        g_ps->applyDump( &psd );
        g_ps->dumpAll(NULL);
    }
}
void savePowerSystem() {
    static PowerSystemDump psd;
    g_ps->dumpAll(&psd);
    bool res = dbSavePowerSystem( g_current_project_id, &psd );
    if(!res) {
        g_log->print(WHITE, (char*)"CAN'T SAVE POWERSYSTEM");
    }
}

void networkLoop( double dur ) {
    double st = now();
    while( now() < st + dur ) {
        vce_heartbeat();
    }
}

///////

// Save/Load per-user files 
bool dbLoadUserFile( int user_id, const char *prefix, char *ptr, size_t sz ) {
    Format f( "%s_%d", prefix, user_id );
    size_t loadsz = sz;
    bool res = dbLoadFileSync( f.buf, ptr, &loadsz );
    if(!res) return false;
    if( sz != loadsz ) return false;
    return true;
}
bool dbSaveUserFile( int user_id, const char *prefix, const char *ptr, size_t sz ) {
    Format f( "%s_%d", prefix, user_id );
    return dbSaveFileSync( f.buf, ptr, sz );
}

Contact g_followings[SSPROTO_SHARE_MAX];
Contact g_followers[SSPROTO_SHARE_MAX];

bool isFollowing(int uid) {
    for(int i=0;i<elementof(g_followings);i++) {
        if( g_followings[i].user_id == uid ) return true;
    }
    return false;
}
bool isAFollower( int uid ) {
    for(int i=0;i<elementof(g_followers);i++) {
        if( g_followers[i].user_id == uid ) return true;
    }
    return false;
}

// Returns number of UIDs (requires g_followers and g_followings are up to date )
int getAllFriendContacts( Contact *contacts, int outmax ) {
    int outind = 0;
    for(int i=0;i<elementof(g_followings);i++) {
        if( g_followings[i].user_id > 0 && isAFriend( g_followings[i].user_id ) ) {
            contacts[outind] = g_followings[i];
            outind++;
            if(outind==outmax) break;
        }
    }
    return outind;        
}

// "Friend" means mutually following.
int countFriends() {
    int cnt=0;
    for(int i=0;i<elementof(g_followers);i++) {
        if( g_followers[i].user_id != 0 ) {
            for(int j=0;j<elementof(g_followings);j++) {
                if( g_followings[j].user_id == g_followers[i].user_id ) {
                    cnt ++;
                    break;
                }
            }
        }
    }
    return cnt;
}
int getAllFriendUIDs( int *out, int maxn ) {
    int cnt=0;
    for(int i=0;i<elementof(g_followers);i++) {
        if( g_followers[i].user_id != 0 ) {
            for(int j=0;j<elementof(g_followings);j++) {
                if( g_followings[j].user_id == g_followers[i].user_id ) {
                    out[cnt] = g_followers[i].user_id;
                    print("getAllFollowerUIDs found friend: [%d]=%d", cnt, out[cnt] );
                    cnt++;
                    if(cnt==maxn) return cnt;
                    break;
                }
            }
        }
    }
    return cnt;
}
bool isAFriend( int uid ) {
    return isFollowing(uid) && isAFollower(uid);
}
void removeFollower(int uid) {
    for(int i=0;i<elementof(g_followers);i++) {
        if( g_followers[i].user_id == uid ) {
            print("removeFollower: removing %d",uid);            
            g_followers[i].user_id = 0;
        }
    }
}
void removeFollowing(int uid) {
    for(int i=0;i<elementof(g_followings);i++) {
        if( g_followings[i].user_id == uid ) {
            print("removeFollowing: removing %d",uid);
            g_followings[i].user_id = 0;
        }
    }    
}

// Remove only from my list, not friend's list
void unfriendUser( int uid ) {
    removeFollower(uid);
    removeFollowing(uid);
    dbSaveFollowers();
    dbSaveFollowings();
    dbAppendSaveUnfriendLog( uid );
}

bool dbLoadFollowings() {
    assert(g_user_id>0);
    return dbLoadUserFile( g_user_id, "followings", (char*) g_followings, sizeof(g_followings));
}
bool dbSaveFollowings() {
    assert(g_user_id>0);    
    return dbSaveUserFile( g_user_id, "followings", (const char*) &g_followings, sizeof(g_followings) );
}
bool dbLoadFollowers() {
    assert(g_user_id>0);    
    return dbLoadUserFile( g_user_id, "followers", (char*) g_followers, sizeof(g_followers));
}
bool dbSaveFollowers() {
    assert(g_user_id>0);    
    return dbSaveUserFile( g_user_id, "followers", (const char*) &g_followers, sizeof(g_followers) );
}

int ssproto_share_project_result_recv( conn_t _c, int project_id ) {
    print("ssproto_share_project_result_recv pjid:%d",project_id);
    return 0;
}
void dbShareProjectSend( int pjid ) {
    int members[SSPROTO_SHARE_MAX];
    int n = getAllFriendUIDs(members, elementof(members) );
    ssproto_share_project_send( g_dbconn, g_user_id, pjid, members, n );
}
void dbPublishProjectSend( int pjid ) {
    ssproto_publish_project_send( g_dbconn, g_user_id, pjid );
}
bool dbCheckProjectIsJoinable( int pjid, int userid ) {
    ssproto_project_is_joinable_send( g_dbconn, pjid, userid );
    waitForReply();
    print("dbCheckProjectIsJoinable: return code:%d", g_net_result_code );
    if( g_net_result_code == SSPROTO_OK ) {
        return true;
    } else {
        return false;
    }
}
int ssproto_project_is_joinable_result_recv( conn_t _c, int project_id, int user_id, int result ) {
    g_net_result_code = result;
    g_wait_for_net_result = false;
    return 0;
}
int ssproto_is_published_project_result_recv( conn_t _c, int project_id, int published ) {
    g_net_result_value = published;
    g_wait_for_net_result = false;
    return 0;
}
bool dbCheckProjectIsPublished( int project_id ) {
    ssproto_is_published_project_send( g_dbconn, project_id );
    waitForReply();
    print("dbCheckProjectIsPublished: return value:%d", g_net_result_value );
    if( g_net_result_value == 1 ) {
        return true;
    } else {
        return false;
    }
 
}
int ssproto_is_shared_project_result_recv( conn_t _c, int project_id, int shared ) {
    print("ssproto_is_shared_project_result_recv: pjid:%d shared:%d",project_id, shared );
    g_net_result_value = shared;
    g_wait_for_net_result = false;
    return 0;
}
bool dbCheckProjectIsShared( int project_id, int owner_uid ) {
    ssproto_is_shared_project_send( g_dbconn, project_id, owner_uid );
    waitForReply();
    print("dbCheckProjectIsShared: val:%d", g_net_result_value );
    switch( g_net_result_value ) {
    case SSPROTO_PROJECT_IS_SHARED:
        return true;
    default:
        return false;
    }
}

bool followContact( int user_id, const char * nickname, int face_index, int hair_index ) {
    for(int i=0;i<elementof(g_followings);i++) {
        if( g_followings[i].user_id == user_id ) {
            return false;            
        }
    }
    for(int i=0;i<elementof(g_followings);i++) {
        if( g_followings[i].user_id == 0 ){
            g_followings[i].user_id = user_id;
            strncpy( g_followings[i].nickname, nickname, sizeof( g_followings[i].nickname ) );
            g_followings[i].face_index = face_index;
            g_followings[i].hair_index = hair_index;
            print("followContact: added user_id:%d in contact list ind:%d", user_id, i );
            return true;
        }
    }
    return false;        // full
}

bool addFollowerContact( int following_uid, const char *nick, int fi, int hi ) {
    for(int i=0;i<elementof(g_followers);i++){
        if( g_followers[i].user_id == following_uid ) {
            return false;
        }
    }
    for(int i=0;i<elementof(g_followers);i++){
        if( g_followers[i].user_id == 0 ) { 
            g_followers[i].user_id = following_uid;
            strncpy( g_followers[i].nickname, nick, sizeof(g_followers[i].nickname));
            g_followers[i].face_index = fi;
            g_followers[i].hair_index = hi;
            print("addFollowerContact: added userid:%d in ind:%d", following_uid, i );
            return true;
        }
    }
    return false;    // full
}

int ssproto_publish_project_result_recv( conn_t _c, int project_id ) {
    return 0;
}

int *g_db_int_array_result_store_ptr; // Pointer to a buffer to store result from database
int *g_db_int_array_result_size_ptr; // Pass max size and get loaded size as a result

int ssproto_search_published_projects_result_recv( conn_t _c, const int *project_ids, int project_ids_len ) {
    print("ssproto_search_published_projects_result_recv. got_n:%d", project_ids_len );
    int cp_num = MIN( project_ids_len, *g_db_int_array_result_size_ptr );
    for(int i=0;i<cp_num;i++) {
        print("  pid:%d", project_ids[i] );
        g_db_int_array_result_store_ptr[i] = project_ids[i];
    }
    *g_db_int_array_result_size_ptr = cp_num;
    g_wait_for_net_result = false;
    return 0;
}
void dbSearchPublishedProjects( int *pjids, int *num ) {
    ssproto_search_published_projects_send( g_dbconn );
    g_wait_for_net_result = true;
    g_db_int_array_result_store_ptr = pjids;
    g_db_int_array_result_size_ptr = num;
    waitForReply();
    print("dbSearchPublishedProjects done. got num:%d", *num );
}
int ssproto_search_shared_projects_result_recv( conn_t _c, int user_id, const int *project_ids, int project_ids_len ) {
    print("ssproto_search_shared_projects_result_recv. got_n:%d", project_ids_len );
    int cp_num = MIN( project_ids_len, *g_db_int_array_result_size_ptr );
    for(int i=0;i<cp_num;i++) {
        print("  pid:%d", project_ids[i] );
        g_db_int_array_result_store_ptr[i] = project_ids[i];
    }
    *g_db_int_array_result_size_ptr = cp_num;
    g_wait_for_net_result = false;
    return 0;
}
void dbSearchSharedProjects( int *pjids, int *num ) {
    ssproto_search_shared_projects_send( g_dbconn, g_user_id );
    g_wait_for_net_result = true;
    g_db_int_array_result_store_ptr = pjids;
    g_db_int_array_result_size_ptr = num;
    waitForReply();
    print("dbSearchSharedProjects done. got num:%d", *num );
}

//////////////////

void dbEnsureImage( int imgid ) {
    ssproto_ensure_image_send( g_dbconn, 0, imgid, 2048, 2048 );
    g_wait_for_net_result = true;
    waitForReply();
    print("dbEnsureImage done. result:%d", g_net_result_code );
}
int ssproto_ensure_image_result_recv( conn_t _c, int query_id, int result, int image_id ) {
    print("ssproto_ensure_image_result_recv: qid:%d res:%d image_id:%d",query_id, result, image_id);
    assert( result == SSPROTO_OK );
    g_net_result_code = result;
    g_wait_for_net_result = false;
    return 0;
}
// Update image data in backend server
void dbUpdateChunkImageSend( int imgid, int chx, int chy ) {
    //    print("dbUpdateChunkImageSend: %d,%d",chx,chy);
    
    unsigned char rbuf[CHUNKSZ*CHUNKSZ], gbuf[CHUNKSZ*CHUNKSZ], bbuf[CHUNKSZ*CHUNKSZ];
    int i=0;
    int x0 = chx * CHUNKSZ, y0 = chy * CHUNKSZ;

    for(int y=0;y<CHUNKSZ;y++) {
        for(int x=0;x<CHUNKSZ;x++) {
            Cell *c = g_fld->get((x0+x), (y0 + CHUNKSZ - 1 - y) ); // upside down
            Color col = c->toColor();            
            unsigned char r,g,b;
            col.toRGB( &r, &g, &b );
            rbuf[i] = r; gbuf[i] = g; bbuf[i] = b;
            i++;
        }
    }
    //    print("dbUpdateChunkImageSend: ch:%d,%d img:%d,%d",chx,chy, x0,y0);
    ssproto_update_image_part_send( g_dbconn, 0, imgid, x0, 2048 - y0 - CHUNKSZ, CHUNKSZ, CHUNKSZ, rbuf, i,  gbuf, i, bbuf, i );
}
void dbUpdateImageAllRevealedSend( int imgid ) {
    for(int y=0;y<g_fld->chh;y++) {
        for(int x=0;x<g_fld->chw;x++) {
            bool rev = g_fld->getChunkRevealed(x,y);
            if(rev) {
                dbUpdateChunkImageSend( imgid, x,y);
            }
        }
    }
}

int ssproto_update_image_part_result_recv( conn_t _c, int query_id, int result, int image_id ) {
    print("ssproto_update_image_part_result_recv: qid:%d res:%d image_id:%d", query_id, result, image_id );
    return 0;
}
void dbLoadPNGImageSync( int imgid, Image *outimg ) {
    ssproto_get_image_png_send( g_dbconn, QID_PNGIMAGELOAD, imgid );
    size_t sz = SSPROTO_PNG_SIZE_MAX;
    char *buf = (char*) MALLOC(sz);
    g_wait_for_net_result = true;
    g_db_get_file_size = sz;
    g_db_get_file_store_buffer = buf;
    waitForReply();
    sz = g_db_get_file_size;
    g_db_get_file_store_buffer = NULL;
    g_db_get_file_size = 0;
    
    if( g_net_result_code == SSPROTO_OK ) {
        bool ret = outimg->loadPNGMem( (unsigned char*) buf, sz );
        assert(ret);
        print("dbLoadPNGImageSync: image ok! data size:%d", sz );
    } else {
        print("dbLoadPNGImageSync: g_net_result_code: %d", g_net_result_code);
    }
    FREE(buf);        
}

int ssproto_get_image_png_result_recv( conn_t _c, int query_id, int result, int image_id, const char *png_data, int png_data_len ) {
    if( query_id == QID_PNGIMAGELOAD ) {
        g_net_result_code = result;
        g_wait_for_net_result = false;        
        assertmsg( png_data_len <= g_db_get_file_size, "file too large! gotsize:%d maxexpect:%d", png_data_len, g_db_get_file_size );
        g_db_get_file_size = png_data_len;
        memcpy( g_db_get_file_store_buffer, png_data, png_data_len );
        print("ssproto_get_image_png_result_recv: pngdatalen:%d res:%d imgid:%d", png_data_len, result, image_id );
    } else {
        assertmsg(false, "invalid query id:%d", query_id );
    }
    return 0;
}

// Load an image (part size: 256x256 pixels)
void dbLoadRawImageSync( int imgid, Image *outimg) {
    int part_size = 256;
    int ph = outimg->height/part_size, pw = outimg->width/part_size;
    size_t sz = part_size * part_size * 4;
    unsigned char *buf = (unsigned char*) MALLOC(sz);
    assert(buf);
    for(int py=0;py<ph;py++) {
        for(int px=0;px<pw;px++) {
            dbLoadRawImageSync( imgid, px*part_size,py*part_size,part_size,part_size, buf, sz );
            for(int y=0;y<part_size;y++) {
                for(int x=0;x<part_size;x++) {
                    int ind = (x+y*part_size)*4;
                    unsigned char r = buf[ind], g = buf[ind+1], b = buf[ind+2], a = buf[ind+3];
                    outimg->setPixelRaw(px*part_size+x,py*part_size+y,r,g,b,a);
                }
            }
        }
    }
}
// Load part of an image
void dbLoadRawImageSync( int imgid, int x, int y, int w, int h, unsigned char *out, size_t outsz ) {
    print("dbLoadRawImageSync: xywh:%d,%d,%d,%d",x,y,w,h);
    ssproto_get_image_raw_send( g_dbconn, QID_RAWIMAGELOAD, imgid, x,y,w,h );
    assert( outsz <= SSPROTO_RAW_IMAGE_SIZE_MAX );
    g_wait_for_net_result = true;
    g_db_get_file_size = outsz;
    g_db_get_file_store_buffer = (char*)out;
    waitForReply();
    g_db_get_file_store_buffer = NULL;
    g_db_get_file_size = 0;
    print("dbLoadRawImageSync: g_net_result_code: %d",g_net_result_code);
}


int ssproto_get_image_raw_result_recv( conn_t _c, int query_id, int result, int image_id, int x0, int y0, int w, int h, const char *raw_data, int raw_data_len ) {
    if( query_id == QID_RAWIMAGELOAD ) {
        g_net_result_code = result;
        g_wait_for_net_result = false;
        assertmsg( raw_data_len <= g_db_get_file_size, "buffer too large! gotsize:%d expect:%d", raw_data_len, g_db_get_file_size );
        g_db_get_file_size = raw_data_len;
        memcpy( g_db_get_file_store_buffer, raw_data, raw_data_len );
        print("ssproto_get_image_raw_result_recv: datalen:%d res:%d imgid:%d", raw_data_len, result, image_id );
    } else {
        assertmsg(false, "invalid query id:%d", query_id );
    }
    return 0;
}


#define PLAYTIME_COUNTER_CATEGORY 100

void dbIncrementPlaytimeSend( int pjid, int sec ) {
    if( !isDBNetworkActive()) return;
    ssproto_counter_add_send( g_dbconn, PLAYTIME_COUNTER_CATEGORY, pjid, sec );
}
int dbLoadPlaytime( int pjid ) {
    ssproto_counter_get_send( g_dbconn, PLAYTIME_COUNTER_CATEGORY, pjid );
    g_wait_for_net_result = true;
    waitForReply();
    if( g_net_result_code == SSPROTO_OK ) {
        if( g_net_result_value < 0 ) print( "dbloadplaytime: invalid value:%d", g_net_result_value  );
        return g_net_result_value;
    } else {
        return 0;
    }
}

int ssproto_counter_get_result_recv( conn_t _c, int counter_category, int counter_id, int result, int curvalue ) {
    if( counter_category == PLAYTIME_COUNTER_CATEGORY ) {
        g_wait_for_net_result = false;
        g_net_result_code = result;
        g_net_result_value = curvalue;        
    } else {
        assertmsg(false, "invalid counter category:%d", counter_category );
    }
    return 0;
}


void dbSaveAllClearLog( int pjid, int accum_time_sec ) {
    Format fmt( "ZADD %s %d %d", g_all_clear_ranking_z_key, accum_time_sec, pjid );
    print("dbSaveAllClearLog: sending '%s'", fmt.buf );
    ssproto_kvs_command_str_oneway_send( g_dbconn, fmt.buf );
}
void dbLoadAllClearLogs( AllClearLogEntry *out, size_t *sz ) {
    Format fmt( "ZRANGE %s 0 10000000000 WITHSCORES", g_all_clear_ranking_z_key );
    print("dbLoadAllClearLogs: sending '%s'", fmt.buf );
    ssproto_kvs_command_str_send( g_dbconn, QID_LOAD_ALL_CLEAR_LOGS, fmt.buf );
    waitForReply();
    if( g_net_result_code == SSPROTO_OK ) {
        int outmax = *sz;
        int outi=0;
        print("dbLoadAllClearLogs: got result num:%d", g_kvs_result_num );
        for(int i=0;i< g_kvs_result_num; i++ ) {
            print("dbLoadAllClearLogs: result[%d]=%d", i, strtoullen(g_kvs_results[i], g_kvs_results_size[i] ) );
            if( (i%2) == 0 ) {
                out[outi].project_id = strtoullen( g_kvs_results[i], g_kvs_results_size[i] );
            } else if( (i%2)==1 ) {
                out[outi].play_sec = strtoullen( g_kvs_results[i], g_kvs_results_size[i] );
                outi++;
                if( outi == outmax ) {
                    print("dbLoadAllClearLogs: out full. n:%d", outi );
                    return;
                }
            }
            
        }
        *sz = outi;
        print("dbLoadAllClearLogs: outnum:%d",outi);
    } else {
        *sz = 0;        
        print("dbLoadAllClearLogs: can't load");
    }
}


void dbSaveMilestoneProgressLog( int pjid ) {
    long long t = now_msec();
    Format fmt( "ZADD %s %d %lld", g_ms_progress_z_key, pjid, t );
    print("dbSaveMilestoneProgressLog: cmd:'%s'", fmt.buf );
    ssproto_kvs_command_str_oneway_send( g_dbconn, fmt.buf );
}


void dbLoadMilestoneProgressLogs( MilestoneProgressLogEntry *out, size_t *sz ) {
    Format fmt( "ZREVRANGE %s 0 100000000000000 WITHSCORES", g_ms_progress_z_key );
    print("dbLoadMilestoneProgressLogs: cmd:'%s'", fmt.buf );
    ssproto_kvs_command_str_send( g_dbconn, QID_LOAD_MS_PROGRESS_LOGS, fmt.buf );
    waitForReply();
    if( g_net_result_code == SSPROTO_OK ) {
        int outmax = *sz;
        int outi=0;
        for(int i=0;i<g_kvs_result_num;i++) {
            if( (i%2) == 0) {
                out[outi].progress_at = strtoulllen( g_kvs_results[i], g_kvs_results_size[i] );
            } else {
                out[outi].project_id = strtoullen( g_kvs_results[i], g_kvs_results_size[i] );
                outi++;
                if(outi==outmax) return;
            }
        }
        *sz = outi;
        print("dbLoadMilestoneProgressLogs: outnum:%d",outi);
    } else {
        *sz = 0;
        print("dbLoadMilestoneProgressLogs: cant load");
    }
}

void dbLoadMilestoneProgressRanking( MilestoneProgressRankEntry *out, size_t *sz ) {
    MilestoneProgressLogEntry ents[2048];
    size_t n = elementof(ents);
    dbLoadMilestoneProgressLogs( ents, &n );
    if(n==0){
        *sz=0;
        return;
    }

    MilestoneProgressRankEntry ranks[2048];
    memset(ranks,0,sizeof(ranks));

    long long nt = now_msec();
    for(int i=0;i<n;i++) {
        if( ents[i].progress_at < (nt - 24*60*60*1000) ) continue;
        
        bool found=false;
        for(int j=0;j<elementof(ranks);j++) { 
            if( ranks[j].project_id == ents[i].project_id ) {
                ranks[j].progress ++;
                found = true;
                break;
            }
        }
        if(!found) { 
            for(int j=0;j<elementof(ranks);j++) {
                if( ranks[j].project_id == 0 ) {
                    ranks[j].project_id = ents[i].project_id;
                    ranks[j].progress = 1;
                    break;
                }
            }
        }
    }
    // sort
    SorterEntry sents[elementof(ents)];
    int si=0;
    for(int i=0;i<elementof(ranks);i++ ) {
        if( ranks[i].project_id != 0 ) {
            sents[si].val = ranks[i].progress;
            sents[si].ptr = & ranks[i];
            si++;
        }
    }
    quickSortF( sents, 0, si-1 );
    int outmax = *sz;    
    for(int i=0;i<si && i < outmax; i++) {
        MilestoneProgressRankEntry *re = (MilestoneProgressRankEntry*) sents[i].ptr;
        out[i].project_id = re->project_id;
        out[i].progress = re->progress;
    }
    *sz = si;
    print("dbLoadMilestoneProgressRanking: out n:%d", si );
}

//////////////
bool dbLoadUnfriendLog( int uid, UnfriendLog *out ) {
    return dbLoadUserFile( uid, "unfriendlog", (char*)out, sizeof(*out) );
}
bool dbSaveUnfriendLog( int uid, UnfriendLog *log ) {
    return dbSaveUserFile( uid, "unfriendlog", (char*)log, sizeof(*log) );
}
void dbAppendSaveUnfriendLog(int unfriended_uid ) {
    UnfriendLog ufl;
    bool res = dbLoadUnfriendLog( unfriended_uid, &ufl );
    if(!res) {
        print("dbAppendUnfriendLog: no unfriendlog file for user %d. creating.", unfriended_uid );
    }
    ufl.add(g_user_id);
    dbSaveUnfriendLog( unfriended_uid, &ufl );
    print("dbAppendUnfriendLog: saved for user %d.", unfriended_uid );
}
void dbExecuteUnfriendLog() {
    UnfriendLog ufl;
    bool res = dbLoadUnfriendLog( g_user_id, &ufl);
    if(res) {
        for(int i=0;i<elementof(ufl.uids);i++) {
            if( ufl.uids[i] > 0 ) {
                print("dbExecuteUnfriendLog: found uid %d in unfriend log.", ufl.uids[i] );
                unfriendUser(ufl.uids[i]);
                ufl.uids[i] = 0;
            }
        }
        dbSaveUnfriendLog(g_user_id, &ufl);
    }
}
