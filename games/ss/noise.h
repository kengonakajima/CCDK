#ifndef _NOISE_H_
#define _NOISE_H_


class NoiseTable {
public:
    float *data;
    int width, height;
    NoiseTable( int w, int h );
    ~NoiseTable() { FREE(data); }
    void add( int granu, float min, float max );
    void noise( int maxgranu, float min ,float max );
    inline int index(int x, int y) { return x + y * width; }
    inline float get(int x, int y) { return data[ index(x,y) ];}
    inline void set(int x, int y, float v){ data[ index(x,y) ] = v; }
    void dump();
};

#endif
