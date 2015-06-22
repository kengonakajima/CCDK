#ifndef _UTIL_H_
#define _UTIL_H_

class Link {
public:
    int ids[2];
    Link() { ids[0] = ids[1] = 0; }
    bool isValid() { return ( ids[0] > 0 && ids[1] > 0 ); }
    void setIDs( int id_a, int id_b ) { ids[0] = id_a; ids[1] = id_b; }    
};


class LinkScannerEntry : public Link {
public:
    bool connected;
    int group_id;
    LinkScannerEntry() : Link(), connected(false), group_id(0) {}
};
class LinkScanner {
public:
    LinkScannerEntry *links;
    int numlinks;
    int used_so_far;
    LinkScanner(int maxnum);
    LinkScannerEntry *addLink( int a_id, int b_id );
    LinkScannerEntry *ensureLink(int a_id, int b_id );
    LinkScannerEntry *findLink(int a_id, int b_id );
    void paintFromStartingLink( int maxdepth, int gid = 0 );
    int getNumUsed() {
        return used_so_far;
    }
    void setStartingLink( LinkScannerEntry *lse, int gid = 0 ) {
        lse->connected = true;
        lse->group_id = gid;
    }
    LinkScannerEntry *getEntry(int index);

    bool isAllConnected();

    void dumpLinks();

    bool isIncluded( int any_id );

    void makeGroups( LinkScannerEntry *tops, int *topnum );
    void getAllLinksInGroup( int groupid, LinkScannerEntry *out, int *outnum );
};

class MazeNode {
public:
    int type;
    bool con_up, con_down, con_right, con_left;
    Rect rect;
    DIR entrance_hint;
    MazeNode() {
        clear();
    }
    void clear() {
        type = 0;
        con_up = false;
        con_down = false;
        con_right = false;
        con_left = false;
        entrance_hint = DIR_NONE;
    }
    void setConnection(DIR d) {
        switch(d) {
        case DIR_UP: con_up = true; break;
        case DIR_DOWN: con_down = true; break;
        case DIR_RIGHT: con_right = true; break;
        case DIR_LEFT: con_left = true; break;
        default:break;
        }
    }
    bool hasConnection( DIR d ) {
        switch(d) {
        case DIR_UP: return con_up;
        case DIR_DOWN: return con_down;
        case DIR_LEFT: return con_left;
        case DIR_RIGHT: return con_right;
        default:assertmsg(false, "notimpl");
        }
        return false;
    }
    bool hasAnyConnection() {
        return con_up || con_down || con_right || con_left;
    }
        
};


class Maze {
public:
    MazeNode *nodes; // left-bottom is 0 and the number increases as it goees up (same as index of cells in Field.) 
    int width,height;
    Maze(int w, int h);
    ~Maze();
    inline int getIndex(int x,int y) { return x + y * width; }
    inline MazeNode *get(int x, int y) {
        if(x<0||y<0||x>=width||y>=height) return NULL;
        return & nodes[ getIndex(x,y) ];
    }
    void clear();
    void setConnection( int x, int y, DIR d, int node_type );
    void setConnection( int x, int y, DIR d );    
    void setConnection4( int x, int y, int node_type ) {
        setConnection( x,y, DIR_UP, node_type );
        setConnection( x,y, DIR_DOWN, node_type );
        setConnection( x,y, DIR_RIGHT, node_type );
        setConnection( x,y, DIR_LEFT, node_type );        
    }
    
    void setType( int x, int y, int t );
    void makeAllConnected();
    void makeAllConnectionsPair();
    void dump();
    int findNeighbourSetConnection( int x, int y );
};

inline int dirToChar( DIR d ) {
    switch(d) {
    case DIR_UP: return 'U';
    case DIR_DOWN: return 'D';
    case DIR_RIGHT: return 'R';
    case DIR_LEFT: return 'L';
    default: return 'X';
    }
}

#endif
