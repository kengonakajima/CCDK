#ifndef _DIMENSION_H_
#define _DIMENSION_H_

class Pos2 {
 public:
    int x;
    int y;
    inline Pos2() : x(0),y(0){}
    inline Pos2(int _x,int _y): x(_x), y(_y) {}
    inline Pos2(const Pos2& p ) : x(p.x), y(p.y){}
    inline Pos2(Vec2 v){ x = v.x; y = v.y; }
    inline void clear() { x=y=0; }
    inline void add(Pos2 d){ this->x += d.x; this->y += d.y; }
    inline Pos2 right(){ return Pos2( this->x+1, this->y ); }
    inline Pos2 left(){ return Pos2( this->x-1, this->y ); }
    inline Pos2 down(){ return Pos2( this->x, this->y-1 ); }
    inline Pos2 up(){ return Pos2( this->x, this->y+1 ); }
    inline Pos2 add(int dx,int dy) { return Pos2(this->x+dx,this->y+dy); }
    inline Pos2 to( Pos2 tgt ) { return Pos2( tgt.x - this->x, tgt.y - this->y ); }
    inline bool equals(Pos2 &p){ return ( this->x == p.x && this->y == p.y ); }
    inline bool valid(){ return ( this->x >= 0 && this->y >= 0 ); }
    static inline Pos2 getInvalid(){ return Pos2(-1,-1); }
    static inline Pos2 fromDir( DIR d ) {
        switch(d){
        case DIR_UP: return Pos2( 0, 1 ); 
        case DIR_RIGHT: return Pos2(1, 0 );
        case DIR_DOWN: return Pos2(0,-1);
        case DIR_LEFT: return Pos2(-1,0);
        default:
            assert(false);
            return Pos2(0,0);
        }
    }
    inline float len(Pos2 to){
        return ::len( x,y,to.x,to.y );
    }
    inline Pos2 operator+=(Pos2 arg) { return Pos2( this->x += arg.x, this->y += arg.y); }
    inline Pos2 operator-=(Pos2 arg) { return Pos2( this->x -= arg.x, this->y -= arg.y); }    
    inline bool operator==(Pos2 arg) { return ( this->x == arg.x && this->y == arg.y ); }
    inline bool operator!=(Pos2 arg) { return ( this->x != arg.x || this->y != arg.y ); }
    inline Pos2 operator+(Pos2 arg) { return Pos2( x+arg.x, y+arg.y ); }
    inline Pos2 operator-(Pos2 arg) { return Pos2( x-arg.x, y-arg.y ); }    
    inline bool nextTo(Pos2 tgt) {
        int dx = tgt.x - x, dy = tgt.y - y;
        if( dx==0 ) {
            if( dy == 1 || dy == -1 ) return true;
        } else if( dx == 1 || dx == -1 ) {
            if( dy == 0 ) return true;
        }
        return false;
    }
    inline Pos2 randomize( int r ) { return Pos2( x + irange(-r,r+1), y + irange(-r,r+1) ); }
};


class Rect {
 public:
    int id;
    Pos2 min;
    Pos2 max;
    Rect( Pos2 _min, Pos2 _max ) : min(_min), max(_max) { bless(); }
        Rect( const Rect& r ) : min(r.min), max(r.max) { bless(); }
    Rect() : min(Pos2(0,0)),max(Pos2(0,0)){}
    inline void bless() {
        if( min.x > max.x ) {
            int tmp = min.x;
            min.x = max.x;
            max.x = tmp;
        }
        if( min.y > max.y ) {
            int tmp = min.y;
            min.y = max.y;
            max.y = tmp;
        }
    }
    void grow( int x, int y ) {this->grow( Pos2(x,y) );}
    void grow( Pos2 p ) {
        if(p.x < this->min.x ) this->min.x = p.x;
        if(p.x > this->max.x ) this->max.x = p.x;
        if(p.y < this->min.y ) this->min.y = p.y;
        if(p.y > this->max.y ) this->max.y = p.y;
        //        std::cerr<< "grow:" << p.x << "<" << p.y << " min:"<<this->min.x <<","<<this->min.y << std::endl;
    }
    bool includes(Pos2 p){
        return (p.x >= this->min.x && p.y >= this->min.y &&
                p.x <= this->max.x && p.y <= this->max.y  );
    }
    bool intersect(Rect r){
        return ( this->min.x <= r.max.x && this->min.y <= r.max.y
                 && this->max.x >= r.min.x && this->max.y >= r.min.y );
    }
    bool valid(){ return ( this->min.valid() && this->max.valid() ); }
    static Rect getInvalid(){ return Rect( Pos2::getInvalid(), Pos2::getInvalid()); }

    Pos2 center() {
        int x = avg( this->min.x , this->max.x );
        int y = avg( this->min.y , this->max.y );
        return Pos2(x,y);
    }
    int area() {
        int w = max.x - min.x + 1;
        int h = max.y - min.y + 1;
        return w * h;
    }
    // Get a point on the edge of a rectangle.
    // mgn: avoid choosing near corner.
    Pos2 getRandomEdge( DIR d, int mgn = 1 ) {
        DIR dir = d;
        if(dir == DIR_NONE ) dir = randomDir();

        if(dir==DIR_UP || dir==DIR_DOWN) {
            if( width() < (mgn+mgn) ) mgn=0;
        } else {
            if( height() < (mgn+mgn) ) mgn=0;
        }
        
        
        switch(dir) { // +Y : UP
        case DIR_UP: return Pos2( irange(min.x+mgn,max.x-mgn), max.y );
        case DIR_DOWN: return Pos2( irange(min.x+mgn,max.x-mgn), min.y );
        case DIR_RIGHT: return Pos2( max.x, irange(min.y+mgn,max.y-mgn) );
        case DIR_LEFT: return Pos2( min.x, irange(min.y+mgn,max.y-mgn) );
        default: assert(false); return Pos2(0,0);
        }
    }
    Pos2 getCenterEdge( DIR d ) {
        DIR dir = d;
        if(dir == DIR_NONE ) dir = randomDir();
        switch(dir){
        case DIR_UP: return Pos2( center().x, max.y );
        case DIR_DOWN: return Pos2( center().x, min.y );
        case DIR_RIGHT: return Pos2( max.x, center().y );
        case DIR_LEFT: return Pos2( min.x, center().y );
        default: assert(false); return Pos2(0,0);
        }
    }
    int width() { return max.x - min.x + 1; }
    int height() { return max.y - min.y + 1; }
    inline Pos2 in( float r ) {
        return Pos2( min.x + (max.x-min.x) * r, min.y + (max.y-min.y) * r );
    }
    inline Pos2 in( float xr, float yr ) {
        return Pos2( min.x + (max.x-min.x) * xr, min.y + (max.y-min.y) * yr );
    }
    inline Rect translate( int x, int y ) {
        return Rect( min + Pos2(x,y), max + Pos2(x,y) );
    }
    inline Rect expand( int dx, int dy ) {
        return Rect( min - Pos2(dx,dy), max + Pos2(dx,dy) );
    }
    inline Pos2 getRandomPos() {
        return Pos2( irange( min.x, max.x), irange(min.y,max.y) );
    }
};

class PosSet {
public:
    Pos2 *ary;
    int nmax, nused;
    PosSet(int maxnum) : nused(0) {
        nmax = maxnum;
        ary = (Pos2*)MALLOC( sizeof(Pos2) * nmax );
        for(int i=0;i<maxnum;i++) ary[i].clear();
    }
    ~PosSet() {
        FREE(ary);
    }
    // true if added
    bool add(Pos2 p) {
        for(int i=0;i<nmax;i++) {
            if( ary[i] == p ) return false;
        }
        if( nused >= nmax ) return false; // full
        ary[nused] = p;
        nused++;
        return true;
    }
    bool find(Pos2 p) {
        for(int i=0;i<nmax;i++) {
            if( ary[i] == p ) return true;
        }
        return false;
    }
  
  
};

#endif
