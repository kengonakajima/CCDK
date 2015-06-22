#include "moyai/common.h"
#include "dimension.h"
#include "util.h"



LinkScanner::LinkScanner(int maxnum) : links(NULL), numlinks(maxnum), used_so_far(0) {
    size_t sz = sizeof(LinkScannerEntry) * numlinks;
    links = (LinkScannerEntry*) MALLOC( sz );
    memset( links, 0, sz );        
}

LinkScannerEntry *LinkScanner::addLink( int a_id, int b_id ) {
    if( used_so_far >= numlinks ) return NULL;
    links[used_so_far].setIDs( a_id, b_id );
    LinkScannerEntry *out = & links[used_so_far];
    used_so_far ++;
    return out;
}
LinkScannerEntry *LinkScanner::findLink(int a_id, int b_id ) {
    for(int i=0;i<used_so_far;i++) {
        if( links[i].ids[0] == a_id && links[i].ids[1] == b_id ) {
            return & links[i];
        }
    }
    return NULL;
}
LinkScannerEntry *LinkScanner::ensureLink(int a_id, int b_id ) {
    LinkScannerEntry *lse = findLink(a_id, b_id);
    if(lse) return lse;
    return addLink( a_id, b_id );
}

// scan up to the level of maxdepth 
void LinkScanner::paintFromStartingLink( int maxdepth, int gid ) {
    for(int depth = 0; depth < maxdepth; depth ++ ) {
        bool set_done = false;
        for(int i=0;i<used_so_far;i++) {
            if( links[i].connected ) {
                for(int j=0;j<used_so_far;j++) {
                    if( links[j].connected == false ) {
                        if( links[j].ids[0] == links[i].ids[0] || links[j].ids[0] == links[i].ids[1] ||
                            links[j].ids[1] == links[i].ids[0] || links[j].ids[1] == links[i].ids[1] ) {
                            links[j].connected = true;
                            links[j].group_id = gid;
                            print("paintFromStartingLink: set connected at index:%d id0:%d id1:%d", j, links[j].ids[0], links[j].ids[1] );
                            set_done = true;
                        }
                    }
                }
            }
        }
        if( set_done == false ) return;
    }
}
LinkScannerEntry *LinkScanner::getEntry(int index) {
    if( index >= used_so_far ) return NULL; else return & links[index];
}

bool LinkScanner::isAllConnected() {
    if( used_so_far == 0 )return false;
    
    for(int i=0;i<used_so_far;i++) {
        if( links[i].connected == false ) return false;
    }
    return true;
}

void LinkScanner::dumpLinks() {
    print("LinkScanner::dumpLink:");
    for(int i=0;i<used_so_far;i++) {
        print("  link[%d] group:%d : %d - %d", i, links[i].group_id, links[i].ids[0], links[i].ids[1] );
    }
    print("LinkScanner::dumpLink done");    
}
// Returns true if ID is included in ids[0] or ids[1]
bool LinkScanner::isIncluded( int any_id ) {
    for(int i=0;i<used_so_far;i++) {
        if( links[i].ids[0] == any_id || links[i].ids[1] == any_id ) return true;
    }
    return false;
}

// Divide into groups.
// Returns number of groups
void LinkScanner::makeGroups( LinkScannerEntry *tops, int *topnum ) {
    if( used_so_far == 0 ) {
        *topnum = 0;
        return;
    }
    
    int gid = 1;
    while(1) {
        if( isAllConnected() ) break;
        for(int i=0;i<used_so_far;i++) {
            if( links[i].connected == false ) {
                print("makeGroups: top_index: %d gid:%d", i, gid );
                setStartingLink( &links[i], gid );
                paintFromStartingLink( 1000, gid ); // 1000は深いが。。
                tops[gid-1] = links[i];
                gid++;
                break;
            }
        }
    }
    
    *topnum = gid-1;
}

// Find all links in a group
void LinkScanner::getAllLinksInGroup( int group_id, LinkScannerEntry *out, int *outnum ) {
    int outi=0;
    for(int i=0;i<used_so_far;i++) {
        if( links[i].group_id == group_id ) {
            print("getAllLinksInGroup: i:%d  %d - %d", i, links[i].ids[0], links[i].ids[1] );
            out[outi] = links[i];
            outi++;
        }
    }
    *outnum = outi;
}


///////////////////////
Maze::Maze(int w, int h) : width(w), height(h) {
    size_t sz = sizeof(MazeNode) * w * h;
    nodes = (MazeNode*) MALLOC( sz );
    clear();
}
Maze::~Maze() {
    FREE(nodes);
}
void Maze::clear() {
    for(int i=0;i<width*height;i++) nodes[i].clear();
}
void Maze::setConnection( int x, int y, DIR d ) {
    MazeNode *node = get(x,y);
    node->setConnection(d);
}
void Maze::setConnection( int x, int y, DIR d, int node_type ) {
    MazeNode *node = get(x,y);
    node->setConnection(d);
    node->type = node_type;
}
void Maze::setType( int x, int y, int t ) {
    MazeNode *node = get(x,y);    
    node->type = t;
}
void Maze::makeAllConnected() {
    int nloop = width * height * 100;
    for(int i=0;i<nloop;i++) {
        Pos2 p( irange(0,width),  irange(0,height) );
        MazeNode *node = get(p.x,p.y);
        if( ! node->hasAnyConnection() ) {
            if( findNeighbourSetConnection(p.x,p.y) > 0 ) {
                //                dump();
                //                sleep(1);
            }
        }
    }
    // To avoid a leak 
    int nocon_cnt=0;
    for(int y=0;y<height;y++) {
        for(int x=0;x<width;x++) {
            MazeNode *node = get(x,y);
            if( !node->hasAnyConnection() ) {
                nocon_cnt++;
                findNeighbourSetConnection(x,y);
            }
        }
    }
    assertmsg(nocon_cnt < width*height, "no entrance?");
        
}
int Maze::findNeighbourSetConnection( int x, int y ) {
    int cand_n = 0;
    DIR cand_dirs[4];
        
    MazeNode *unode = get(x,y+1);
    if(unode && unode->hasAnyConnection() ) cand_dirs[cand_n++] = DIR_UP;
    MazeNode *dnode = get(x,y-1);
    if(dnode && dnode->hasAnyConnection() ) cand_dirs[cand_n++] = DIR_DOWN;
    MazeNode *rnode = get(x+1,y);
    if(rnode && rnode->hasAnyConnection() ) cand_dirs[cand_n++] = DIR_RIGHT;
    MazeNode *lnode = get(x-1,y);
    if(lnode && lnode->hasAnyConnection() ) cand_dirs[cand_n++] = DIR_LEFT;

    if( cand_n > 0 ) {
        int ind = irange(0,cand_n);
        MazeNode *node = get(x,y);
        node->setConnection( cand_dirs[ind] );
        node->entrance_hint = cand_dirs[ind];
        
    }
    return cand_n;
}
void Maze::makeAllConnectionsPair() {
    for(int y=0;y<height;y++) {
        prt("[%d]", height-1-y);
        for(int x=0;x<width;x++) {
            MazeNode *node = get(x,y);
            if( node->con_up ) {
                MazeNode *unode = get(x,y+1);
                if(unode) unode->con_down = true;
            }
            if( node->con_down ) {
                MazeNode *dnode = get(x,y-1);
                if(dnode) dnode->con_up = true;
            }
            if( node->con_right ) {
                MazeNode *rnode = get(x+1,y);
                if(rnode) rnode->con_left = true;
            }
            if( node->con_left ) {
                MazeNode *lnode = get(x-1,y);
                if(lnode) lnode->con_right = true;
            }
        }
    }

}
void Maze::dump() {
    print("Maze::dump:");
    for(int y=0;y<height;y++) {
        prt("[%02d]", height-1-y);
        for(int x=0;x<width;x++) {
            MazeNode *node = get(x,height-1-y);
            char msg[8] = "*..../d";
            if( node->type != 0 ) msg[0] = node->type;
            if( node->con_up ) msg[1] = 'U';
            if( node->con_down ) msg[2] = 'D';
            if( node->con_right ) msg[3] = 'R';
            if( node->con_left ) msg[4] = 'L';
            msg[6] = dirToChar( node->entrance_hint ); 

            prt( "%s ", msg );
        }
        print("");
    }
    print("");
}
