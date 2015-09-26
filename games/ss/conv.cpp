
#include "moyai/common.h"
#include "dimension.h"
#include "ids.h"
#include "conf.h"

class TileDeck;
#include "item.h"
#include "net.h"
#include "util.h"
#include "field.h"


void makeFullPath( char *topdir, char *out, size_t outlen, const char *filename ) {
    unsigned int h = hash_pjw(filename);
    snprintf( out, outlen, "%s/%02x/%02x/%s", topdir, h%0xff, (h/256)%0xff, filename );
}
bool fileExist( const char *path ) {
    char buf[1];
    size_t sz = 1;
    return readFile( path, buf, &sz);
}

void convertFieldData( char *indir, char *outdir, int pjid ) {
    char outpath[400];
    snprintf(outpath, sizeof(outpath), "field_data_%d_%d_%d_%d", pjid, FIELD_W, FIELD_H, (int)sizeof(Cell) );
    assertmsg( sizeof(Cell) == 48, "invalid Cell size compiled:%d. Expect 48. incorrect version?", sizeof(Cell) );
    char outfullpath[400];
    makeFullPath( outdir, outfullpath, sizeof(outfullpath), outpath );

    if( fileExist(outfullpath) ) {
        print("FATAL: File exists:'%s'", outfullpath);
        return;
    }
    
    size_t read_total = 0;
    int chw = FIELD_W / CHUNKSZ;
    int chh = FIELD_H / CHUNKSZ;
    for(int chy=0;chy<chh;chy++) {
        for( int chx=0;chx<chw;chx++) {
            char path[400];
            snprintf( path, sizeof(path), "field_data_%d_%d_%d_%d_%d", pjid, chx*CHUNKSZ, chy*CHUNKSZ, CHUNKSZ, CHUNKSZ );
            char fullpath[400];
            makeFullPath( indir, fullpath, sizeof(fullpath), path );
            char buf[32*1024];
            size_t sz = sizeof(buf);
            bool res = readFile( fullpath, buf, &sz );
            assertmsg( res, "can't read file:'%s'", fullpath);
            if(chx%8==0)prt(".");
            read_total += sz;
            if( chx==chw-1 ) print("chunk y:%d total:%d",chy, read_total );
            assertmsg( sz < sizeof(buf), "saved file is too large:'%s'", fullpath );

            char decompressed[sizeof(Cell)*CHUNKSZ*CHUNKSZ];
            int decomplen = memDecompressSnappy( decompressed, sizeof(decompressed), buf, sz );
            assertmsg(decomplen == sizeof(decompressed), "invalid decompressed size:%d file:'%s'", decomplen, fullpath );

            int chunk_index = chx + chy*chw;
            size_t offset = chunk_index * sizeof(Cell)*CHUNKSZ*CHUNKSZ;
            //            print("write. ofs:%d chx:%d chy:%d", offset, chx, chy );
            res = writeFileOffset( outfullpath, decompressed, decomplen, offset, false );
            assertmsg(res, "writeFileOffset failed for '%s'", outfullpath );
            
        }
    }
}


// ssconv DIRNAME PROJID
int main( int argc, char **argv ) {
    if( argc != 4 ) {
        print("Usage: ssconv DATADIR_IN DATADIR_OUT PROJID");
        return 1;
    }

    char *datadir_in = argv[1];
    char *datadir_out = argv[2];
    int project_id = atoi( argv[3] );

    print("datadir_in:'%s' datadir_out:'%s' project_id:%d", datadir_in, datadir_out, project_id );

    convertFieldData( datadir_in, datadir_out, project_id );
    
    return 0;
}
