#include "moyai/common.h"
#include "noise.h"

/////////////

// -0->
// -1->
// -n->
NoiseTable::NoiseTable( int w, int h ) : width(w), height(h) {
    size_t sz = sizeof(float) * w * h;
    data = (float*)MALLOC(sz);
    for(int i=0;i<w*h;i++) data[i] = 0;
}

void NoiseTable::add( int granu, float min, float max ) {
    assertmsg( width % granu == 0, "invalid granu");
    assertmsg( height % granu == 0, "invalid granu");

    int tmpw = width / granu + 1, tmph = height / granu + 1;
    size_t sz = sizeof(float) * tmpw * tmph;
    float *tmptbl = (float*) MALLOC( sz );
    for(int y=0;y<tmph;y++){
        //        fprintf(stderr, "tmp[%d]: ", y );
        for(int x=0;x<tmpw;x++){
            tmptbl[x+y*tmpw] = range(min,max);
            //            fprintf(stderr, "%.4f", tmptbl[x+y*tmpw]);
        }
        //        fprintf(stderr,"\n");
    }
    for(int y=0;y<height;y++){
        for(int x=0;x<width;x++){
            int tmpx = x/granu, tmpy = y/granu;
            float ltv = tmptbl[ tmpx + tmpy * tmpw ];
            float lbv = tmptbl[ tmpx + (tmpy+1) * tmpw ];
            float rtv = tmptbl[ tmpx+1 + tmpy * tmpw ];
            float rbv = tmptbl[ tmpx+1 + (tmpy+1) * tmpw ];
            float yratio = (float)(y%granu) / (float)(granu);
            float xratio = (float)(x%granu) / (float)(granu);
            float left_val = interpolate( ltv, lbv, yratio );
            float right_val = interpolate( rtv, rbv, yratio );
            float out = interpolate( left_val, right_val, xratio );
            set( x,y, get(x,y) + out );
        }
    }
    FREE(tmptbl);
}
void NoiseTable::noise( int maxgranu, float min, float max ) {
    assertmsg( isPowerOf2( (unsigned int) maxgranu), "need power of 2" );
    float m = max;
    int granu = maxgranu;
    for(;;){
        m /= 2.0;
        add( granu, min, m );
        granu /= 2;
        if( granu == 0 ) break;
    }
}

void NoiseTable::dump() {
    for(int y=0;y<height;y++){
        fprintf(stderr, "[%d]: ", y );        
        for(int x=0;x<width;x++){
            float v = get(x,y);
            fprintf(stderr,"%.4f ", v );
        }
        fprintf(stderr, "\n" );
    }
}

