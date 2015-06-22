#ifndef _MAPVIEW_H_
#define _MAPVIEW_H_

class Char;


#define PIXEL_PER_CELL 24
#define PPC PIXEL_PER_CELL


const int PIXEL_PER_CHUNK = (CHUNKSZ * PPC);

class DurBar : public Prop2D {
public:
    Pos2 at; // Need integer because it's searchable
    int target_char_id;
    double last_update;
    DurBar(Pos2 at, TileDeck *dk);
    virtual bool prop2DPoll( double dt );
    static DurBar* ensure( Pos2 at, int curhp, int maxhp, TileDeck *dk );
    static DurBar* ensure( Char *tgt, int curhp, int maxhp, TileDeck *dk );
    void update( int curhp, int maxhp );
    static void clear( Pos2 at );
    static void clear( Char *tgt );
};

class Chunk {
public:
    int chx, chy;
    bool changed;
    
    Grid *ggrid;
    Prop2D *gprop;
    Grid *egrid;
    Prop2D *eprop;
    Grid *fgrid;
    Prop2D *fprop;
    Grid *bgrid;
    Prop2D *bprop;

    Grid *igrid;
    Prop2D *iprop;
    
    Chunk( int chx, int chy );
    ~Chunk();
    void update();
    void updateMovers();
    
};

class PowerGrid;

class MapView {
public:
    int chw, chh;
    Chunk **chunks;
    Chunk **active_chunks;
    void poll( Vec2 center, float zoom_rate );
    MapView( int w, int h );
    Chunk *allocateChunk( int chx, int chy );
    Chunk *getChunk( int chx, int chy );
    void checkCleanChunks( int minchx, int minchy, int maxchx, int maxchy );    
    void notifyChanged( int chx, int chy );
    void notifyChanged( Vec2 at );
    void notifyChangedAll();
};


class Pole : public Prop2D {
public:
    Pos2 p;
    Pos2 destcache[2];
    Pole( Pos2 p );
    virtual bool prop2DPoll( double dt);
    void addCable( Pos2 rel_to );
    void updateCables();
};

class ExchangeLink {
public:
    Pos2 from;
    Pos2 to[3];
    ExchangeLink() : from(-1,-1) {
        for(int i=0;i<elementof(to);i++) to[i].x = to[i].y = -1;
    }
};



void catenaryCurve( Prop2D *p, Vec2 from, Vec2 to, Color c, float width );

void refreshExchangeLinks() ;
class ExchangeLink;
ExchangeLink *findExchangeLink( Pos2 at ) ;
void exchangeEffect( Pos2 at );
void pollExchangeEffect();

#endif
