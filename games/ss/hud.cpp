#include "moyai/client.h"
#include "conf.h"

#include "ids.h"
#include "dimension.h"
#include "item.h"
#include "atlas.h"
#include "net.h"
#include "char.h"
#include "hud.h"
#include "pc.h"
#include "effect.h"
#include "enemy.h"

#include "util.h"
#include "field.h"
#include "mapview.h"
#include "pwgrid.h"
#include "globals.h"


/////////////////////

void makeHourMinSecString( int s, char *out, size_t outsz ) {
    int hr = s/3600, min = (s%3600) / 60, sec = s%60;
    snprintf( out, outsz, "%02d:%02d:%02d", hr, min, sec );
}


/////////////////////

// Do not insertProp, because this is used in many layers
ItemPanel::ItemPanel( bool show_durbar ) : show_durbar(show_durbar) {
    setScl(48,48);
    setDeck(g_base_deck);
    setIndex( B_ATLAS_BELT_OFF );
    setColor(1,1,1,0.9);

    item_prop = NULL;
    num_prop = NULL;
    num_grid = NULL;
    cap_prop = NULL;
    cap_grid = NULL;
    dur_prop = NULL;
    
}
void ItemPanel::setCaption( Color c, const char *s ) {
    assertmsg( getParentLayer(),  "need parent layer" );
    if( !cap_prop ) {
        cap_prop = new Prop2D();
        cap_grid = new CharGrid(32,1);
        cap_grid->setDeck( g_ascii_deck );
        cap_prop->addGrid(cap_grid);
        getParentLayer()->insertProp(cap_prop);
    }
    cap_prop->setScl(16);
    cap_prop->setLoc(loc.add(32,-8));
    cap_grid->clear();
    cap_grid->printf( 0,0, c, "%s", s );
}


void ItemPanel::setItemDirect( TileDeck *deck, int index, float scl ) {
    if( !item_prop ) {
        item_prop = new Prop2D();
        getParentLayer()->insertProp(item_prop);
    }
    item_prop->setScl(scl);
    item_prop->setIndex(index);
    item_prop->setDeck(deck);
    item_prop->setLoc(loc);
}

void ItemPanel::update( Item *itm ){
    assertmsg( itm->validate(), "validation failed" );
    ItemConf *conf = itm->conf;
    if(itm->itt == ITT_EMPTY ){
        if( item_prop ){
            item_prop->to_clean = true;
            item_prop = NULL;
        }
        if( num_prop ){
            num_prop->to_clean = true;
            num_prop = NULL;
        }
        if( dur_prop) {
            dur_prop->to_clean = true;
            dur_prop = NULL;
        }
        if( cap_prop ) {
            cap_prop->to_clean = true;
            cap_prop = NULL;
        }
        return;
    }

    assertmsg(conf, "conf is null. itt:%d num:%d dur:%d", itm->itt, itm->num, itm->dur );
    
    // item
    if( !item_prop){
        item_prop = new Prop2D();
        item_prop->setScl(48,48);
        item_prop->setLoc(loc);
        getParentLayer()->insertProp(item_prop);
    }
    item_prop->setDeck(conf->deck);        
    item_prop->setIndex(conf->index);
    
    // number
    if( !num_prop ){
        num_prop = new Prop2D();
        num_prop->setScl(16,16);
        num_prop->setLoc( item_prop->loc.add( 32, -32/2 ) );
        getParentLayer()->insertProp(num_prop);
        num_grid = new CharGrid(4,1);
        num_grid->setDeck(g_ascii_deck);
        num_prop->addGrid(num_grid);
    }
    num_grid->clear();
    if( itm->num > 1 ) {
        num_grid->printf(0,0, Color(1,1,1,0.5) , "%d", itm->num );
    }
    // durability
    if( conf->dur > 0 && itm->dur < conf->dur && show_durbar ) {
        if( !dur_prop ) {
            dur_prop = new Prop2D();
            dur_prop->setScl(32,8);
            dur_prop->setDeck( g_durbar_deck );
            dur_prop->setLoc(loc + Vec2(52,0) );
            getParentLayer()->insertProp(dur_prop);
        }
        float r = (float) itm->dur / (float) conf->dur;
        int i = (int)(r*32);
        dur_prop->setIndex(i);
    } else {
        if(dur_prop) {
            dur_prop->to_clean = true;
            dur_prop = NULL;
        }
    }
}

void ItemPanel::setSelected(bool selected ) {
    if(selected){
        setIndex( B_ATLAS_BELT_ON );
    } else {
        setIndex( B_ATLAS_BELT_OFF );
    }
}

void ItemPanel::toggle( bool visibility ) {
    setVisible(visibility);
    if(item_prop) item_prop->setVisible(visibility);
    if(num_prop) num_prop->setVisible(visibility);
    if(dur_prop) dur_prop->setVisible(visibility);
    if(cap_prop) cap_prop->setVisible(visibility);
}

////////////////////

Belt::Belt() {
    for(int i=0;i<elementof(items);i++){
        items[i] = new ItemPanel(true);
        items[i]->setLoc( getPosByIndex(i) );
        g_belt_layer->insertProp(items[i]);
    }    
}
Vec2 Belt::getPosByIndex( int ind ) {
    return Vec2( - SCRW/2 + 40, -80 - (48+2) * ind );    
}


void Belt::update(){
    for(int i=0;i<elementof(items);i++){
        items[i]->update( & g_pc->items[PC_SUIT_NUM+i] );
        if( i == g_pc->selected_item_index - PC_SUIT_NUM ){
            items[i]->setSelected( true );
        } else {
            items[i]->setSelected( false );
        }
    }
}

void Belt::spikeEffect(int ind ){
    items[ind]->spikeEffect();
}

//////////////
Suit::Suit() {
    for(int i=0;i<elementof(items);i++) {
        items[i] = new ItemPanel(true);
        items[i]->setLoc( getPosByIndex(i) );
        items[i]->setIndex( B_ATLAS_WHITE_FILL );
        items[i]->setColor( Color(0.2,0.2,0.6,0.4) );
        g_belt_layer->insertProp(items[i]);
    }
}
Vec2 Suit::getPosByIndex( int ind ) {
    return Vec2( -SCRW/2 + 40, 200 - (48+2) * ind );
}
// PC items are sorted from SUIT, BELT, INVENTORY
void Suit::update() {
    for(int i=0;i<elementof(items);i++) {
        items[i]->update( & g_pc->items[i]);
    } 
}
//////////////

Gauge::Gauge( const char *nm, Vec2 lc, Color c, int glen ) : Prop2D(), color(c), loc(lc), grid_len(glen), high_thres_rate(0), low_thres_rate(0) {
    snprintf( name, sizeof(name), "%s", nm );

    // base
    base_grid = new Grid(glen,1);
    setLoc(lc);
    addGrid(base_grid);
    base_grid->setDeck( g_base_deck );
    base_grid->set(0,0,B_ATLAS_GAUGE_LEFT);    
    for(int i=1;i<glen-1;i++){
        base_grid->set(i,0,B_ATLAS_GAUGE_CENTER);
    }
    base_grid->set(glen-1,0,B_ATLAS_GAUGE_RIGHT);
    g_gauge_layer->insertProp(this);

    // bar
    bar_prop = new Blinker();
    bar_prop->setLoc( loc );
    g_gauge_layer->insertProp(bar_prop);
    
    // caption
    cap_prop = new Prop2D();
    cap_grid = new CharGrid(32,1);
    cap_grid->setDeck( g_ascii_deck );
    cap_prop->addGrid(cap_grid);
    cap_prop->setScl(12,12);
    cap_prop->setLoc(lc.add(8,12));
    g_gauge_layer->insertProp(cap_prop);

    update( 1,2 );
}

void Gauge::update( int cur_num, int max_num ) {
    cap_grid->clear();
    cap_grid->printf(0,0, WHITE, "%s %d/%d", name, cur_num, max_num );

    float rate = (float)cur_num / (float)max_num;

    bar_prop->clearPrims();
    int pxunit = 4;
    float max_x = pxunit + grid_len*pxunit*8-2*pxunit;
    float min_x = pxunit;
    float cur_x = min_x + ( max_x - min_x ) * rate;
    bar_prop->addRect(Vec2(min_x,pxunit), Vec2(cur_x,  (8-1)*pxunit), color );

    if( high_thres_rate != 0 && cur_num > max_num * high_thres_rate ){
        float thres_x = min_x + ( max_x - min_x ) * high_thres_rate;
        bar_prop->addRect(Vec2(thres_x,pxunit), Vec2(cur_x,  (8-1)*pxunit), high_color );
    }
    if( low_thres_rate != 0 && cur_num < max_num * low_thres_rate ){
        bar_prop->startBlink(0.2);
    } else {
        bar_prop->stopBlink();
        bar_prop->setVisible(true);
    }
    
}
void Gauge::setThresColorHigh( float rate, Color c ) {
    high_thres_rate = rate;
    high_color = c;
}
void Gauge::setLowThres( float rate ){
    low_thres_rate = rate;
}
void Gauge::toggle(bool visibility) {
    if(bar_prop) bar_prop->setVisible(visibility);
    if(cap_prop) cap_prop->setVisible(visibility);
    setVisible(visibility);
}
void hudUpdatePCStatus() {
    g_hp_gauge->update( g_pc->hp, g_pc->maxhp );
    g_elec_gauge->update( g_pc->ene, g_pc->maxene );
    if( g_pc->armor > 0 ) {
        g_armor_gauge->update( g_pc->armor, g_pc->maxarmor );
        g_armor_gauge->toggle(true);
    } else {
        g_armor_gauge->toggle(false);
    }
}
void hudUpdateItems() {
    if(g_belt) g_belt->update();
    if(g_suit) g_suit->update();
    if(g_invwin) g_invwin->update();
}
void hudUpdateMilestone( int cleared ) {
    g_milestone_display->updateMilestone(cleared);
    g_flagarrow->updateLevel(cleared+1);
}

////////////////////

PictLabel::PictLabel( Vec2 lc, int pict_index ) : Prop2D(), loc(lc) {

    setDeck(g_base_deck);
    setIndex(pict_index);
    setScl(PIXEL_PER_CELL,PIXEL_PER_CELL);
    setLoc(lc);

    num_prop = new Prop2D();
    num_prop->setLoc(lc.add(0,-8));
    num_prop->setScl(16,16);
    addChild(num_prop);
    
    num_grid = new CharGrid(24,1);
    num_grid->setDeck(g_ascii_deck);
    num_prop->addGrid( num_grid );
    setNumber(0);
}

void PictLabel::setNumber(int n ){
    num_grid->clear();
    num_grid->printf( 0,0, WHITE, " %d", n ); // Space not to overlap
}
void PictLabel::setString( const char *s ) {
    num_grid->clear();
    num_grid->printf( 0,0, WHITE, " %s", s );
}


////////////////
GridCursor::GridCursor() : Prop2D(), attachment(NULL) {
    setBright(true);
    setScl(PPC);
    setDeck( g_base_deck );
    g_cursor_layer->insertProp(this);
}
Prop2D *GridCursor::ensureAttachment( Vec2 at, TileDeck *dk, int index ) {
    if( attachment == NULL ) {
        attachment = new Prop2D();
        attachment->setScl(PPC);
        g_cursor_layer->insertProp(attachment);
        attachment->ensurePriority(this);
    }
    attachment->setDeck(dk);
    attachment->setIndex(index);
    attachment->setLoc(at);
    return attachment;
}
void GridCursor::clearAttachment() {
    if(attachment) {
        attachment->to_clean = true;
        attachment = NULL;
    }
}
void GridCursor::setBright(bool bright) {
    if(bright) setIndex( B_ATLAS_GRID_CURSOR_BRIGHT ); else setIndex( B_ATLAS_GRID_CURSOR_DARK );
}
bool GridCursor::prop2DPoll(double dt) {
    if( index == B_ATLAS_GRID_CURSOR_BRIGHT || index == B_ATLAS_GRID_CURSOR_DARK ) {
        float alpha = 0.2 + sin(accum_time*7)*0.2;
        setColor( Color(1,1,1,alpha) );
    } else {
        setColor( Color(1,1,1,0.2) );
    }
    
    return true;
}
                              
///////////////////
Window::Window( int w, int h, int base_frame_index, float alpha ) {
    setDeck( g_base_deck );
    setScl(PPC);
    bg = new Grid(w,h);
    addGrid(bg);
    setupGridWindowFrame( bg, w, h, base_frame_index, alpha );
}

bool Window::prop2DPoll(double dt) {
    if(windowPoll(dt)==false)return false;
    return true;
}

void setupGridWindowFrame( Grid *g, int w, int h, int base_frame_index, float alpha ) {
    static const int rel_inds[9] = {
        // Be careful upside-down
        2*AU, 2*AU+1, 2*AU+2,
        1*AU, 1*AU+1, 1*AU+2,
        0,1,2
    };
    
    for(int x=0;x<w;x++){
        for(int y=0;y<h;y++) {
            int rel_ind = 0;
            if(x==0){
                if(y==0 ){
                    rel_ind = 0;
                } else if(y==h-1) {
                    rel_ind = 6;
                } else {
                    rel_ind = 3;
                }
            } else if(x==w-1) {
                if(y==0){
                    rel_ind = 2;
                } else if( y==h-1) {
                    rel_ind = 8;
                } else {
                    rel_ind = 5;
                }
            } else {
                if(y==0) {
                    rel_ind = 1;
                } else if(y==h-1) {
                    rel_ind = 7;
                } else {
                    rel_ind = 4;
                }
            }
            g->set(x,y, base_frame_index + rel_inds[rel_ind] );
        }
    }
    g->fillColor( Color(1,1,1,alpha) ); 
}


//////////////////////////

PropGroup::PropGroup( int num ) : num(num) {
    props = (Prop2D**) MALLOC( sizeof(Prop2D*) * num );
    assert(props);
    for(int i=0;i<num;i++) props[i] = NULL;
}
PropGroup::~PropGroup() {
    FREE(props);
}
int PropGroup::reg( Prop2D *p ) {
    assert(p);
    assertmsg(! find(p->id), "already registered. propid:%d", p->id);

    for(int i=0;i<num;i++){
        if( ! props[i] ) {
            props[i] = p;
            return i;
        }
    }
    assertmsg( false, "no room in panelgroup");
    return -1;
}
Prop2D *PropGroup::find(int id) {
    for(int i=0;i<num;i++) {
        if( props[i] && props[i]->id == id  ) {
            return props[i];
        }
    }
    //    print("PropGroup::find(%d) : not found.", id );
    return NULL;
}
void PropGroup::clear() {
    for(int i=0;i<num;i++) {
        props[i] = NULL;
    }
}
// search by position
Prop2D *PropGroup::find( Vec2 lc ) {
    for(int i=0;i<num;i++) {
        if( props[i] && props[i]->loc == lc ) {
            return props[i];
        }
    }
    return NULL;
}
// get next button by id
Prop2D *PropGroup::getNext( int cur_id, DIR dir ) {
    Prop2D *p = find(cur_id);
    if(!p)return NULL;
    return getNext( p, dir );
}
bool PropGroup::isBottom( Prop2D *to_test ) {
    if(!to_test)return false;
    for(int i=0;i<num;i++) {
        if( props[i] && props[i]->loc.y < to_test->loc.y ) return false;
    }
    return true;
}
bool PropGroup::isTop( Prop2D *to_test ) {
    if(!to_test)return false;    
    for(int i=0;i<num;i++) {
        if( props[i] && props[i]->loc.y > to_test->loc.y ) return false;
    }
    return true;    
}
Prop2D *PropGroup::getTop() {
    Prop2D *out = NULL;
    float maxy = -999999;
    for(int i=0;i<num;i++) {
        if( props[i] && props[i]->loc.y > maxy ) { maxy = props[i]->loc.y; out = props[i]; }
    }
    return out;
}
Prop2D *PropGroup::getBottom() {
    Prop2D *out = NULL;
    float miny = 999999;
    for(int i=0;i<num;i++) {
        if( props[i] && props[i]->loc.y < miny ) { miny = props[i]->loc.y; out = props[i]; }
    }
    return out;
}

// get nearby button at the direction
Prop2D *PropGroup::getNext( Prop2D *from, DIR dir ) {
    int dx,dy;
    dirToDXDY(dir,&dx,&dy);
    float unit = 24;
    float distance  = unit;
    for(int i=0;i<20;i++) {
        Vec2 at = from->loc + Vec2( dx,dy ) * distance;
        //                print("getnext: dist:%f at:%f %f from:%f %f", distance, at.x, at.y, from->loc.x, from->loc.y );
        bool out_of_area = false;
        Vec2 minv, maxv;
        getMinMax( &minv, &maxv );
        if( dir == DIR_RIGHT && at.x > maxv.x ) { // going over right edge
            at.x = minv.x;
            out_of_area = true;
        } else if( dir == DIR_LEFT && at.x < minv.x ) {
            at.x = maxv.x;
            out_of_area = true;
        } else if( dir == DIR_UP && at.y > maxv.y ) {
            at.y = minv.y;
            out_of_area = true;
        } else if( dir == DIR_DOWN && at.y < minv.y ) {
            at.y = maxv.y;
            out_of_area = true;
        }
        
        if( out_of_area ) {
            print("outofarea: at:%f,%f", at.x, at.y);
            return getNearest(at,from);                
        } else {
            Prop2D *p = getNearest(at, from );
            if(!p) return NULL;
            switch( dir ) {
            case DIR_RIGHT:
                if( p->loc.x <= from->loc.x ) { distance += unit; continue; }
                break;
            case DIR_LEFT:
                if( p->loc.x >= from->loc.x ) { distance += unit; continue; }
                break;
            case DIR_UP:
                if( p->loc.y <= from->loc.y ) { distance += unit; continue; }
                break;
            case DIR_DOWN:
                if( p->loc.y >= from->loc.y ) { distance += unit; continue; }
                break;
            default:
                break;
            }
            return p;
        }
        distance += unit;
    }
    assertmsg( false, "cant find? must be found because nearest..(fatal bug)");
    return NULL;
}

Prop2D *PropGroup::getNearest( Vec2 at, Prop2D *except ) {
    float minl = 9999999999;
    Prop2D *ans = NULL;
    for(int i=0;i<num;i++) {
        Prop2D *p = props[i];
        if(p && p->visible ) {
            float l = at.len(p->loc);
            if( l < minl && p != except) {
                minl = l;
                ans = p;
            }
        }
    }
    return ans;
}
void PropGroup::getMinMax( Vec2 *min, Vec2 *max ) {
    *min = Vec2(99999999,99999999);
    *max = Vec2(-99999999,-99999999);
    
    for(int i=0;i<num;i++) {
        Prop2D *p = props[i];
        if(p) {
            Vec2 at = p->loc;
            if( at.x > max->x ) max->x = at.x;
            if( at.x < min->x ) min->x = at.x;
            if( at.y > max->y ) max->y = at.y;
            if( at.y < min->y ) min->y = at.y;        
        }
    }
}



//////////////////
int calcMaxCraftPage() {
    int craftable_num = ItemConf::countCraftable();
    int lastpage = ( craftable_num % Inventory::CRAFT_PER_PAGE ) == 0 ? 0 : 1;
    return craftable_num / Inventory::CRAFT_PER_PAGE + lastpage;
}


Inventory::Inventory() : Window(40,25, B_ATLAS_WINDOW_FRAME_BASE ), cursor_at( PC_BELT_NUM ), craft_cursor_at(-1), selected_at(-1), craftpage(0) {
    setLoc( -SCRW/2+4,-300 );

    group = new PropGroup( elementof(items));
    
    for(int i=0;i<elementof(items);i++){
        ItemPanel *p = new ItemPanel(true);
        items[i] = p;
        group->reg(p);
        g_hud_layer->insertProp(p);
    }
    for(int i=0;i<PC_SUIT_NUM;i++) {
        items[i]->setLoc( Suit::getPosByIndex(i) );
    }
    for(int i=0;i<PC_BELT_NUM;i++) {
        items[PC_SUIT_NUM+i]->setLoc( Belt::getPosByIndex(i) );
    }
    Vec2 base = items[PC_BELT_NUM+PC_SUIT_NUM-1]->loc + Vec2( 100,0 );
    for(int i=PC_BELT_NUM+PC_SUIT_NUM;i<elementof(items);i++) {
        int x = (i-PC_BELT_NUM-PC_SUIT_NUM) % 3;
        int y = (i-PC_BELT_NUM-PC_BELT_NUM) / 3;
        items[i]->setLoc( base + Vec2( x * 100, y * (48+2) ) );
    }

    // craft candidates
    craftgroup = new PropGroup( elementof(crafts) );
    for(int i=0;i<elementof(crafts);i++) {
        ItemConf *itc = ItemConf::getCraftable(craftpage * CRAFT_PER_PAGE + i);
        if(!itc) {
            crafts[i] = NULL;
            continue;
        }
        CRAFTOPTMODE mode = ResearchState::itemIsLocked( itc->itt ) ? CRAFTOPTMODE_LOCKED : CRAFTOPTMODE_CRAFTABLE;
        crafts[i] = new CraftOption( itc, loc + Vec2( 460, 520 - (i*52) ), mode, g_hud_layer );
        craftgroup->reg(crafts[i]);
    }
    craft_caption = new CharGridTextBox(20);
    craft_caption->setString( WHITE, "not set" );
    craft_caption->setLoc( loc + Vec2(420,560) );
    g_hud_layer->insertProp(craft_caption);

    // show name of selected item
    itemname_caption = new CharGridTextBox(20);
    itemname_caption->setString(  WHITE, "HOGE");
    itemname_caption->setLoc( loc + Vec2(20, 20 ) );
    g_hud_layer->insertProp(itemname_caption);

    
    g_hud_layer->insertProp(this);

    update();
}

bool Inventory::prop2DPoll( double dt ) {
    return true;
}

void Inventory::toggle( bool visibility ) {
    setVisible(visibility);
    for(int i=0;i<elementof(items);i++) {
        items[i]->toggle(visibility);
    }
    for(int i=0;i<elementof(crafts);i++) {
        if(crafts[i]) {
            crafts[i]->toggle(visibility);
        }
    }
    craft_caption->setVisible(visibility);
    if(visibility) {
        update();
    } else {
        g_belt->update();
        g_suit->update();
    }
    itemname_caption->setVisible(visibility);
}

void Inventory::update() {
    if( !getVisible() ) return; 
    for(int i=0;i<elementof(items);i++) {
        items[i]->update( & g_pc->items[i] );
        items[i]->setSelected( i == cursor_at || i == selected_at );
    }
    for(int i=0;i<elementof(crafts);i++) {
        if(crafts[i]) crafts[i]->setSelected(false);
    }
    for(int i=0;i<elementof(crafts);i++) {
        if(crafts[i]) {
            ItemConf *itc = ItemConf::getCraftable( CRAFT_PER_PAGE * craftpage + i );

            if(itc) {
                CRAFTOPTMODE mode = ResearchState::itemIsLocked( itc->itt ) ? CRAFTOPTMODE_LOCKED : CRAFTOPTMODE_CRAFTABLE;
                crafts[i]->update( itc, mode );
                crafts[i]->updateColor(mode);
            } else {
                crafts[i]->update( NULL, CRAFTOPTMODE_EMPTY );
                crafts[i]->updateColor( CRAFTOPTMODE_EMPTY );
            }
            if( i == craft_cursor_at ) crafts[i]->setSelected(true);
        }
    }
    char *name = NULL;
    if( cursor_at >= 0 ) {
        ItemConf *itc = g_pc->items[cursor_at].conf;
        if(itc && g_pc->items[cursor_at].isEmpty()==false) {
            name = itc->name;
        } else {
            name = (char*)"";
        }
    } else if( craft_cursor_at >= 0 ) {
        CraftOption *copt = crafts[craft_cursor_at];
        if(copt->itc) {
            ItemConf *itc = copt->itc;
            name = itc->name;
        } else {
            name = (char*)"";
        }
    }
    itemname_caption->setString( WHITE, name );

    char capstr[32];
    snprintf( capstr, sizeof(capstr), "CRAFT:  PAGE %d/%d", craftpage+1, calcMaxCraftPage() );
    craft_caption->setString(WHITE,capstr);
    
}

// Cursor index
// 0,1,2,3 : belt
// 4,5,6,  7,8,9, ... ( PC_ITEM_NUM - 1 ) : inventory items
// 
void Inventory::moveCursor( DIR dir ) {
    if( getVisible() == false ) return;
    g_cursormove_sound->play();

    Prop2D *from_panel = NULL;
    Prop2D *to_panel = NULL;
    bool target_is_crafting = false;

    //    print("dir:%d ccat:%d cat:%d", dir, craft_cursor_at, cursor_at );
    // Go to craft candidates when cursor is at the right most.
    if( dir == DIR_RIGHT && cursor_at >= (PC_BELT_NUM+PC_SUIT_NUM) && ((cursor_at-PC_BELT_NUM-PC_SUIT_NUM)%3) == 2 ) {
        // move from item to craft
        from_panel = items[cursor_at];
        to_panel = craftgroup->getNext( from_panel, dir );
        target_is_crafting = true;
    } else if( (dir == DIR_RIGHT||dir==DIR_LEFT) && craft_cursor_at >= 0 ) {
        // move from craft to item
        from_panel = crafts[craft_cursor_at];
        to_panel = group->getNext( from_panel, dir );
    } else if( dir == DIR_DOWN && craft_cursor_at == (elementof(crafts)-1) ) {
        // downward scrolling of crafts
        from_panel = crafts[craft_cursor_at];
        to_panel = crafts[0];
        craftpage ++;
        if( craftpage == calcMaxCraftPage()) craftpage = 0;
        target_is_crafting = true;
    } else if( dir == DIR_UP && craft_cursor_at == 0 ) {
        // upward scroll of crafts
        from_panel = crafts[0];
        to_panel = crafts[elementof(crafts)-1];
        craftpage--;
        if(craftpage<0) craftpage = calcMaxCraftPage() - 1;
        target_is_crafting = true;        
    } else if( (dir == DIR_DOWN||dir == DIR_UP) && craft_cursor_at >= 0 ) {
        // cursor goes up inside crafts
        from_panel = crafts[craft_cursor_at];
        to_panel = craftgroup->getNext( from_panel, dir );
        target_is_crafting = true;
    } else if( dir == DIR_LEFT && cursor_at >= 0 && cursor_at < PC_BELT_NUM+PC_SUIT_NUM ) {
        from_panel = items[cursor_at];
        to_panel = craftgroup->getNext( from_panel, dir );
        target_is_crafting = true;
    } else {
        from_panel = items[cursor_at];
        to_panel = group->getNext( from_panel, dir );
    }

    if( target_is_crafting ) {
        cursor_at = -1;
        for(int i=0;i<elementof(crafts);i++) {
            if( crafts[i] == to_panel ) {
                craft_cursor_at = i;
                break;
            }
        }        
    } else {
        craft_cursor_at = -1;
        for(int i=0;i<elementof(items);i++) {
            if( items[i] == to_panel ) {
                cursor_at = i;
                break;
            }
        }
    }
    
    update();

}
// select at the current cursor
void Inventory::selectAtCursor() {
    if( craft_cursor_at >= 0 ) {
        CraftOption *cop = crafts[ craft_cursor_at ];
        if(cop->itc) g_pc->craft( cop->itc );
    } else {
        if( selected_at == cursor_at ) {
            // ask player to really abandon
            hudShowConfirmAbandonMessage(selected_at);
            selected_at = -1;
        } else {
            if( selected_at == -1 ) {
                g_craft_sound->play();
                selected_at = cursor_at;            
            } else {
                if( g_pc->isSwappable( selected_at, cursor_at ) ) {
                    g_pc->swapItem( selected_at, cursor_at );
                    g_pc->onItemUpdated(selected_at);
                    g_pc->onItemUpdated(cursor_at);
                    g_craft_sound->play();
                } else {
                    g_cant_sound->play();
                }                
                selected_at = -1;
            }
        }
    }
    update();
}

void Inventory::spikeEffect( int ind ) {
    items[ind]->spikeEffect();
}
////////////////////////
CharGridTextBox::CharGridTextBox( int w ) : Prop2D() {
    setDeck( g_ascii_deck );
    setScl(16);
    bg = new Grid(w,1);
    addGrid(bg);
    bg->setDeck( g_base_deck );
    fillBG(Grid::GRID_NOT_USED);
    cg = new CharGrid(w,1);
    addGrid(cg);
}
void CharGridTextBox::fillBG( int ind ) {
    for(int i=0;i<bg->width;i++) {
        bg->set(i,0,ind);
    }
}
void CharGridTextBox::setString( Color c, const char *s ) {
    cg->clear();
    cg->printf(0,0,c, "%s",s);
}
bool CharGridTextBox::isEmpty() {
    for(int i=0;i<cg->width;i++) {
        if( cg->get(i,0) != Grid::GRID_NOT_USED ) return false;
    }
    return true;
}
bool CharGridTextBox::isEqual( const char *s, int l ) {
    for(int i=0;i<cg->width && i < l ;i++) {
        if( s[i]==0 )return true;
        int ind = cg->get(i,0);
        if( s[i] != ind ) return false;
    }
    return true;
}

////////////////////////
CraftOption::CraftOption( ItemConf *itc, Vec2 lc, CRAFTOPTMODE mode, Layer *l ) : ItemPanel(false), itc(itc), mode(mode) {
    l->insertProp(this);
    setLoc(lc);
    ene = NULL;
    memset( ingrs, 0, sizeof(ingrs));
    update(itc, mode);
}

void CraftOption::update( ItemConf *itc_given, CRAFTOPTMODE md ) {
    mode = md;
    
    Item itm;
    if( itc_given ) {
        itc = itc_given;
        itm.set(itc->itt, 1);
    } else {
        itc = NULL;
    }
    ItemPanel::update( &itm );

    // clean
    if(ene) {
        ene->to_clean = true;
        ene = NULL;
    }
    if( mode == CRAFTOPTMODE_LOCKED ) {
        setCaption( Color(1,0.4,0.4,1), "RESEARCH REQUIRED" );
        clearIngrs();
    } else if( mode == CRAFTOPTMODE_CRAFTABLE ){
        setCaption( Color(1,0.2,0.2,1), "");
        //
        if(!ene) {
            ene = new PictLabel( loc + Vec2(50,0), B_ATLAS_ELECTRO );
            getParentLayer()->insertProp(ene);
        }
        if(itc) {
            ene->setNumber( itc->required_ene );
            //
            for(int i=0;i<ITEM_INGREDIENT_NUM; i++ ) {
                if( itc->ingredients[i].itt > ITT_ERROR ) {
                    ItemConf *ingitc = ItemConf::getItemConf( itc->ingredients[i].itt );
                    if( !ingrs[i]) {
                        ingrs[i] = new PictLabel( loc +  Vec2( 160 + i * 80, 0 ), ingitc->index );
                        getParentLayer()->insertProp(ingrs[i]); 
                    }
                    ingrs[i]->setIndex( ingitc->index );
                    ingrs[i]->setScl(24);
                    ingrs[i]->setNumber( itc->ingredients[i].n );
                } else {
                    if(ingrs[i]) {
                        ingrs[i]->setIndex(-1);
                        ingrs[i]->setString("");
                    }
                }
            }
        }
    } else if( mode == CRAFTOPTMODE_ONRESEARCH ) {
        setCaption(WHITE,"");
        setIndex(-1);
        ResearchState *rs = ResearchState::getResearchState( itc->itt );
        for(int i=0;i<ITEM_INGREDIENT_NUM;i++) {
            if( rs->ingrs[i].itt > ITT_ERROR ) {
                ItemConf *ingitc = ItemConf::getItemConf( rs->ingrs[i].itt );
                if( !ingrs[i]) {
                    ingrs[i] = new PictLabel( loc + Vec2( 60 + i * 140, 0), ingitc->index );
                    
                    getParentLayer()->insertProp(ingrs[i]); // addChild(ingrs[i]);
                }
                ingrs[i]->setIndex( ingitc->index );
                ingrs[i]->setScl(24);
                
                setIngredientNum( i, rs->ingrs[i].current, rs->ingrs[i].n );
            } else {
                if(ingrs[i]) {
                    ingrs[i]->setIndex(-1);
                    ingrs[i]->setString("");
                }
            }
        }
    } else if( mode == CRAFTOPTMODE_RESEARCHDONE ) {
        clearIngrs();
        setIndex(-1);        
        setCaption( Color(0.7,1,0.7,1), "RESEARCH DONE" );
    } else if( mode == CRAFTOPTMODE_EMPTY ) {
        clearIngrs();
        setIndex(B_ATLAS_BELT_OFF );
        setCaption( WHITE,"");
    } else {
        assertmsg( false, "invalid craft opt mode: %d", mode );
    }
}
void CraftOption::clearIngrs() {
    for(int i=0;i<elementof(ingrs);i++) {
        if( ingrs[i] ) {
            ingrs[i]->to_clean = true;
            ingrs[i] = NULL;
        }
    }
}
void CraftOption::toggle(bool visibility) {
    ItemPanel::toggle(visibility);
    if(ene) ene->setVisible(visibility);
    for(int i=0;i<elementof(ingrs);i++) {
        if( ingrs[i] )ingrs[i]->toggle(visibility);
    }
}
void CraftOption::updateColor( CRAFTOPTMODE mode ) {
    bool can_craft = true;
    Color red(1,0.4,0.4,1), white(1,1,1,1);
    if( itc && g_pc->ene >= itc->required_ene ) {
        if(ene) ene->setNumColor(white);
    } else {
        if(ene) ene->setNumColor(red);
        can_craft = false;
    }
    if( mode != CRAFTOPTMODE_CRAFTABLE ) can_craft = false;

    for(int i=0;i<ITEM_INGREDIENT_NUM;i++){
        if(!ingrs[i])continue;
        if( itc && itc->ingredients[i].itt > ITT_ERROR ) {
            if( g_pc->hasEnough( itc->ingredients[i].itt, itc->ingredients[i].n ) ) {
                ingrs[i]->setNumColor(white);
            } else {
                ingrs[i]->setNumColor(red);
                can_craft = false;
            }
        }
    }
    if( item_prop) {
        if(can_craft) {
            item_prop->setColor(white);
            setIndex( B_ATLAS_BELT_OFF );
        } else {
            item_prop->setColor(red);
            setIndex( B_ATLAS_BELT_RED );
        }        
    }
}
void CraftOption::updateResearchIngr() {
    if( mode == CRAFTOPTMODE_ONRESEARCH ) {
        ResearchState *rs = ResearchState::getResearchState( itc->itt );
        for( int i=0;i<elementof(ingrs);i++){
            if( rs->ingrs[i].n > 0 ) {
                setIngredientNum( i, rs->ingrs[i].current, rs->ingrs[i].n );
            } else {
                if(ingrs[i]) ingrs[i]->setString("");
            }
        }
    }
}
void CraftOption::setIngredientNum( int i, int cur, int n ) {
    assert( n>0 );
    char numstr[32];
    snprintf(numstr,sizeof(numstr), "%d/%d", cur, n );
    if(ingrs[i]) ingrs[i]->setString(numstr);
}

    
////////////////////////////

BlinkCursor::BlinkCursor( DIR dir ) : Blinker() {
    int ind = B_ATLAS_UP_ARROW;
    switch(dir) {
    case DIR_UP: ind = B_ATLAS_UP_ARROW; break;
    case DIR_RIGHT: ind = B_ATLAS_RIGHT_ARROW; break;
    case DIR_LEFT: ind = B_ATLAS_LEFT_ARROW; break;
    case DIR_DOWN: ind = B_ATLAS_DOWN_ARROW; break;
    default:
        assertmsg(false,"invalid cursor direction");
    }
    setIndex(ind);
    setScl(24);
    setDeck( g_base_deck );
    startBlink(0.15);    
}


////////////////////////////

int calcMaxResearchPage() {
    int craftable_num = ItemConf::countCraftable();
    int lastpage = ( craftable_num % LabWindow::RESEARCH_PER_PAGE ) == 0 ? 0 : 1;
    return craftable_num / LabWindow::RESEARCH_PER_PAGE + lastpage;
}
LabWindow::LabWindow() : Window( 30,25, B_ATLAS_WINDOW_FRAME_BASE ), cursor_at(0), page(0), research_lock_obtained_at(0)  {
    g_hud_layer->insertProp(this);    
    memset( opts, 0, sizeof(opts));
    setLoc( -SCRW/2+120,-300 );    
    group = new PropGroup( elementof(opts) * RESEARCH_INGREDIENT_NUM );
    for(int i=0;i<elementof(opts);i++) {
        opts[i] = NULL;
        ItemConf *itc = ItemConf::getCraftable( RESEARCH_PER_PAGE * page + i);
        if(!itc) continue;
        ResearchState *rs = ResearchState::getResearchState(itc->itt);
        CRAFTOPTMODE mode;
        if( rs->locked ) mode = CRAFTOPTMODE_ONRESEARCH; else mode = CRAFTOPTMODE_RESEARCHDONE;
        
        opts[i] = new CraftOption( itc, loc + Vec2(60, 500 - (i*52) ), mode, g_hud_layer );
    }


    updateGroup();

    // initial cursor position
    Vec2 minv, maxv;
    group->getMinMax( &minv, &maxv );
    Vec2 left_top( minv.x, maxv.y );
    Prop2D *left_top_prop = group->getNearest( left_top, NULL );
    cursor_id_at = left_top_prop->id;
    
    // window title
    title_tb = new CharGridTextBox(30);
    title_tb->setString( WHITE, "not set" );
    title_tb->setLoc( loc + Vec2(30, 560 ) );
    g_hud_layer->insertProp(title_tb);

    state_caption = new CharGridTextBox(48);
    state_caption->setString( WHITE, "(not set)");
    state_caption->setLoc( Vec2(-280,0) );
    g_hud_layer->insertProp(state_caption);
    
    // name of current selected item
    itemname_caption = new CharGridTextBox(20);
    itemname_caption->setString(  Color(1,1,1,1), "HOGE");
    itemname_caption->setLoc( loc + Vec2(20, 20 ) );
    g_hud_layer->insertProp(itemname_caption);

    // cursor
    cursor = new BlinkCursor();
    g_hud_layer->insertProp(cursor);

    // number of items left
    item_left = new PictLabel( loc + Vec2(500,40), 1 );
    item_left->setScl(PPC*1.5);
    g_hud_layer->insertProp( item_left );
}
void LabWindow::updateCaption() {
    if( research_lock_obtained_at == 0 ) {
        state_caption->setString( WHITE, MSG_OCCUPIED_BY_USER );
    } 
}

// Avoid playing a SE many times via network
void playWindowOpenSoundOnce() {
    if(g_windowopen_sound->isPlaying() == false ) g_windowopen_sound->play();
}

void LabWindow::toggle(bool vis) {
    setVisible(vis);

    if(vis) {
        playWindowOpenSoundOnce();
        update();
    }
    if( isDBNetworkActive() == false ) {
        // offline play for debugging
        state_caption->setVisible(false);
        toggleContent(vis);        
    } else { 
        if( research_lock_obtained_at == 0) {
            state_caption->setVisible(vis);
            toggleContent(false);
        } else {
            state_caption->setVisible(false);
            toggleContent(vis);
        }
    }
    
    if(!vis) {
        // Save PC when closing
        if( g_current_project_id > 0 ) {
            dbSavePC();
            dbSaveResearchState(g_current_project_id);
            realtimeUnlockProjectSend( g_current_project_id, LOCK_RESEARCH );
        }
    }    
}
void LabWindow::toggleContent(bool vis) { 
    for(int i=0;i<elementof(opts);i++) {
        if(opts[i])  opts[i]->toggle(vis);
    }
    title_tb->setVisible(vis);
    itemname_caption->setVisible(vis);
    
    cursor->setVisible(vis);
    item_left->toggle(vis);
}

void LabWindow::update() {
    for(int i=0;i<elementof(opts);i++) {
        if(opts[i]) {
            ItemConf *itc = ItemConf::getCraftable( RESEARCH_PER_PAGE * page + i);
            if(itc) {
                ResearchState *rs = ResearchState::getResearchState(itc->itt);
                CRAFTOPTMODE mode;
                if( rs->locked ) mode = CRAFTOPTMODE_ONRESEARCH; else mode = CRAFTOPTMODE_RESEARCHDONE;
                //                print("LW UPD: i:%d itt:%d page:%d ingr:%d,%d,%d,%d",i, itc->itt, page, itc->ingredients[0].itt, itc->ingredients[1].itt, itc->ingredients[2].itt, itc->ingredients[3].itt );
                opts[i]->update( itc,mode );
                opts[i]->updateResearchIngr();
            } else {
                opts[i]->update( NULL, CRAFTOPTMODE_EMPTY );
                opts[i]->updateResearchIngr();
            }
        }
    }
    updateCursor();

    // show name of crafted item and resouce nums
    Prop2D *cur = (Prop2D*) g_hud_layer->findPropById( cursor_id_at );
    for(int i=0;i<elementof(opts);i++) {
        if(opts[i]) {
            for(int j=0;j<elementof(opts[i]->ingrs);j++) {
                if( cur && opts[i]->ingrs[j] == cur ) {
                    ItemConf *itc = opts[i]->itc;
                    // name
                    itemname_caption->setString( Color(1,1,1,1), itc->name );

                    // num left
                    ResearchState *rs = ResearchState::getResearchState(itc->itt);
                    ITEMTYPE need_itt = rs->ingrs[j].itt;
                    int leftnum = g_pc->countItem(need_itt);
                    char s[100];
                    snprintf(s,sizeof(s)," %d LEFT", leftnum );
                    item_left->setString(s);
                    ItemConf *need_itc = ItemConf::getItemConf(need_itt);
                    item_left->setIndex( need_itc->index );
                    bool not_enough = (leftnum == 0);
                    if( need_itc->itt == ITT_BATTERY1 || need_itc->itt == ITT_BATTERY2 ) {
                        if( leftnum==1 ) not_enough = true;
                    }

                    
                    if( not_enough ) {
                        item_left->setColor( Color(1,0.4,0.4,1) );
                        item_left->setNumColor( Color(1,0.4,0.4,1) );                        
                    } else {
                        item_left->setColor( Color(1,1,1,1 ) );
                        item_left->setNumColor( Color(1,1,1,1 ) );                        
                    }
                    break;
                }
            }
        }
    }

    updateGroup();

    // update page number
    char tstr[32];
    snprintf( tstr, sizeof(tstr), "RESEARCH STATUS:  PAGE %d/%d", page+1, calcMaxResearchPage() );
    title_tb->setString( WHITE, tstr );
    
}
void LabWindow::updateGroup() {
    group->clear();
    // Register all pictlabel positions
    for(int i=0;i<elementof(opts);i++ ) {
        if( !opts[i] ) continue;
        CraftOption *opt = opts[i];
        for(int j=0;j<elementof(opt->ingrs);j++){
            if( opt->ingrs[j] ) {
                group->reg( opt->ingrs[j] );
            }
        }
    }
}

void LabWindow::moveCursor(DIR dir) {
    if( research_lock_obtained_at == 0 ) {
        print("require research lock!");
        return;
    }
    Prop2D *cur = (Prop2D*) g_hud_layer->findPropById( cursor_id_at );

    bool full_comp = false;
    Prop2D *any_selectable = group->getNearest( Vec2(-SCRW/2,SCRH/2), NULL );
    if( !any_selectable ) {
        full_comp = true;
    }
            
    if( dir == DIR_DOWN && (full_comp || group->isBottom(cur) ) ) {
        page++; if( page>2)page=0;
        update(); // update position
        Prop2D *top = group->getTop();
        if(top) {
            cursor_id_at = top->id;
        } else {
            cursor_id_at = -1;
        }
        print("PAGE++");
    } else if( dir == DIR_UP && (full_comp || group->isTop(cur) ) ){
        page--; if(page<0)page=2;
        update();        // update position
        Prop2D *btm = group->getBottom();
        if(btm) {
            cursor_id_at = btm->id;            
        } else {
            cursor_id_at = -1;
        }

        print("PAGE--");
    } else {
        if(cur) {
            Prop2D *next = group->getNext( cur, dir);
            if(next) cursor_id_at = next->id;
        } else {
            cursor_id_at = -1;
        }
    }
    update();
}
void LabWindow::updateCursor() {
    Prop2D *target_panel = NULL;
    
    if( cursor_id_at < 0 ) {
        target_panel = group->getNearest( Vec2(-SCRW/2,SCRH/2), NULL );
        if(!target_panel) {
            print("no selectable in a group! full comp in this page?");
        } else {
            cursor_id_at = target_panel->id;
        }
        
    } else {
        target_panel = (Prop2D*) g_hud_layer->findPropById( cursor_id_at );
    }

    if( target_panel ) {
        cursor->setLoc( target_panel->loc + Vec2(24,-16) );
    } else {
        print("LabWindow::updateCursor: FULL COMP?");
    }
    
}

void LabWindow::selectAtCursor() {
    Prop2D *p = (Prop2D*) g_hud_layer->findPropById( cursor_id_at );
    for(int i=0;i<elementof(opts);i++) {
        if( opts[i] ) {
            for(int j=0;j< elementof(opts[i]->ingrs); j++ ) {
                if( p == opts[i]->ingrs[j] ) {
                    ItemConf *itc = opts[i]->itc;
                    ResearchState *rs = ResearchState::getResearchState( itc->itt );
                    ITEMTYPE ingr_itt = rs->ingrs[j].itt;

                    int cnt = g_pc->countItem( ingr_itt );

                    // To avoid stuck
                    if( (ingr_itt == ITT_BATTERY1 || ingr_itt == ITT_BATTERY2 ) && cnt == 1 ) {
                        print("can't use the last battery!");
                        return;
                    }

                    //
                    if( cnt > 0 && rs->ingrs[j].current < rs->ingrs[j].n ) {
                        rs->ingrs[j].current ++;
                        g_pc->decItem( ingr_itt, 1 );
                        g_craft_sound->play();
                        //                        print("III:%d j:%d itt:%d cnt:%d", i,j, ingr_itt, cnt );
                        if( rs->isComplete() ) {
                            rs->locked = false;
                            print("COMP!");
                            opts[i]->mode = CRAFTOPTMODE_RESEARCHDONE;
                            g_researchdone_sound->play();
                            cursor_id_at = -1;
                            realtimeEventSend( EVT_RESEARCH_DONE, g_pc->nickname, itc->itt );
                        }
                        update();
                        return;
                    }
                }
            }
        }
    }
}
void LabWindow::poll() {
    if( visible ) {
        if( research_lock_obtained_at == 0 ) {
            realtimeLockProjectSend( g_current_project_id, LOCK_RESEARCH );
        } else if( research_lock_obtained_at < now() - 0.5 ) {
            if( updateInterval( 0, 1 ) ) realtimeLockKeepProjectSend( g_current_project_id, LOCK_RESEARCH );
        }
        updateCaption();
    }
    
}
void LabWindow::onLockObtained() {
    research_lock_obtained_at = now();
    dbLoadResearchState( g_current_project_id );
    toggle(true);
}
void LabWindow::onLockReleased() {
    research_lock_obtained_at = 0;
}



/////////////////////////

FacePanel::FacePanel() : Prop2D() {
    face_prop = new Prop2D();
    face_prop->setScl(24);
    face_prop->setDeck(g_base_deck);
    hair_prop = new Prop2D();
    hair_prop->setScl(24);
    hair_prop->setDeck(g_base_deck);        
        
    face_prop->priority = 1;
    hair_prop->priority = 2;                
    addChild(face_prop);
    addChild(hair_prop);    
}

void FacePanel::update( int faceind, int hairind ) {
    face_prop->setIndex( faceind );
    hair_prop->setIndex( hairind );    
}
void FacePanel::setScl(float s) {
    face_prop->setScl(s);
    hair_prop->setScl(s);    
}
bool FacePanel::prop2DPoll( double dt ) {
    face_prop->setLoc(loc);
    hair_prop->setLoc(loc);
    return true;
}

//////////////////    

WorldMapIndicator::WorldMapIndicator( WMINDTYPE t ) : type(t), face(NULL), client_id(0), name_tb(NULL), last_packet_at(0) {
    setDeck( g_base_deck );

    setScl(24);

    if(t == WMIND_PC){
        face = new FacePanel();
        g_wm_layer->insertProp(face);
        name_tb = new CharGridTextBox(PC_NICKNAME_MAXLEN);
        name_tb->setString( WHITE,"");
        name_tb->priority = 3;        
        addChild(name_tb);
    }
}
void WorldMapIndicator::setVisible(bool vis) {
    Prop2D::setVisible(vis);
    if(face) face->setVisible(vis);
}

bool WorldMapIndicator::prop2DPoll( double dt ) {
    bool to_show = (int)(g_wm->accum_time*2) % 2;
    if(to_show) {
        setColor( Color(0,1,0,1) );
    } else {
        setColor( Color(0,0,0,0));
    }
    to_show &= visible;
    if(face) face->setVisible(to_show);
    if(name_tb) name_tb->setVisible(to_show);
    
    switch(type) {
    case WMIND_LOCALPC:
        setIndex( B_ATLAS_WHITE_ANGLED_CROSS );
        break;
    case WMIND_FLAG:
        setIndex( B_ATLAS_FLAG_BASE + ( (int)(accum_time*6)%2) );
        if( g_flag->all_cleared ) setVisible(false);
        break;
    case WMIND_PC:
        face->setLoc(loc);
        name_tb->setLoc(loc+Vec2(10,-6));
        break;
    case WMIND_MOTHERSHIP:
        setIndex( BT_MOTHERSHIP );
        break;
    }
    return true;
}

WorldMap::WorldMap() : Prop2D(), img(NULL), tex(NULL) { 
    setScl(512,512);
    setLoc(0,0);
    setIndex(0);
    
    img = new Image();
    img->setSize(512,512);
    tex = new Texture();
    tex->setImage(img);
    setTexture(tex);

    img->fill( Color(0,0,0,1) );
    g_hud_layer->insertProp(this);

    //
    localpc = new WorldMapIndicator( WMIND_LOCALPC );
    localpc->setLoc(0,0);
    g_wm_layer->insertProp(localpc);

    flag = new WorldMapIndicator( WMIND_FLAG );
    g_wm_layer->insertProp(flag);
    mothership = new WorldMapIndicator( WMIND_MOTHERSHIP );
    g_wm_layer->insertProp(mothership);
    
    for(int i=0;i<elementof(remotepcs);i++) remotepcs[i]=NULL;

    playtime_tb = new CharGridTextBox(32);
    playtime_tb->setString( WHITE, "NOT SET");
    playtime_tb->setScl(16);
    playtime_tb->setLoc( Vec2(-100,-250) );
    g_hud_layer->insertProp(playtime_tb);
}


void WorldMap::toggle(bool vis) {
    setVisible(vis);
    localpc->setVisible(vis);
    flag->setVisible(vis);
    mothership->setVisible(vis);
    playtime_tb->setVisible(vis);
    for(int i=0;i<elementof(remotepcs);i++) if(remotepcs[i]) remotepcs[i]->setVisible(vis);
    if(vis) update();    
}
void WorldMap::update() {
    Pos2 center( g_pc->loc.x/PPC, g_pc->loc.y/PPC);
    int x0 = center.x - 256, y0 = center.y - 256;
    size_t bufsz = 512 * 512 * 4;
    unsigned char *buf = (unsigned char*) MALLOC(bufsz);
    // upside down! in server, (0,0) is left-top
    int sv_y0_lt = FIELD_H - 1 - y0;
    int sv_y0_lb = sv_y0_lt - 512;
#define LOADIMAGESYNC(_dx,_dy) \
    dbLoadRawImageSync(g_current_project_id, x0+(_dx), sv_y0_lb+(_dy), 256, 256, buf, bufsz ); \
    for(int y=0; y<256; y++ ) { \
        for(int x=0; x<256; x++ ) { \
            int ind = (x + y * 256)*4; \
            unsigned char r = buf[ind], g = buf[ind+1], b = buf[ind+2], a = buf[ind+3]; \
            img->setPixelRaw(x+(_dx),y+(_dy), r,g,b,a ); \
        } \
    }
    img->fill(Color(0.5,0,0,1));
    LOADIMAGESYNC(0,0);
    LOADIMAGESYNC(0,256);
    LOADIMAGESYNC(256,0);    
    LOADIMAGESYNC(256,256);    
    //    for(int t=0; t<512; t++ ) img->setPixelRaw(t,t,0xff,0,0,0xff);
    tex->setImage(img); // 3ms

    // flag position
    Vec2 rel_to_flag = (g_flag->loc - g_pc->loc)/PPC;
    if( rel_to_flag.x >= -256 && rel_to_flag.x <= 256 &&
        rel_to_flag.y >= -256 && rel_to_flag.y <= 256 ) {
        flag->loc = rel_to_flag;
        flag->setColor(1,1,1,1);
    } else {
        flag->loc = HUD_OUT_OF_SCREEN;
    }
    Vec2 rel_to_ms = (pos2ToVec2(g_fld->getMotherShipPoint()) - g_pc->loc )/PPC;
    if( rel_to_ms.x >= -256 && rel_to_ms.x <= 256 &&
        rel_to_ms.y >= -256 && rel_to_ms.y <= 256 ) {
        mothership->loc = rel_to_ms;
        mothership->setColor(1,1,1,1);
    } else {
        mothership->loc = HUD_OUT_OF_SCREEN;
    }


    updatePlayTime();
}
void WorldMap::updatePlayTime() {
    int playsec = dbLoadPlaytime(g_current_project_id);
    char hmsstr[32];
    makeHourMinSecString( playsec, hmsstr, sizeof(hmsstr));
    Format fmt( "TOTAL TIME: %s", hmsstr );
    playtime_tb->setString( WHITE, fmt.buf );
}

bool WorldMap::prop2DPoll( double dt ) {
    for(int i=0;i<elementof(remotepcs);i++ ) {
        if( remotepcs[i] && remotepcs[i]->last_packet_at < accum_time - 5 ) { // use accum_time, not global time
            remotepcs[i]->to_clean = true;
            remotepcs[i] = NULL;
        }
    }
    if( playtime_tb->visible ) {
        float f = fabs(sin(accum_time));
        Color c(1,1,1,f);
        playtime_tb->setColor(c);
        if( updateInterval( 0, INCREMENT_PLAYTIME_INTERVAL_SEC ) ) updatePlayTime();
    }
    return true;
}
void WorldMap::ensureRemotePC( int client_id, const char *nickname, int face_ind, int hair_ind, Vec2 lc ) {
    // Update existing player icon
    WorldMapIndicator *tgt = NULL;
    for( int i=0;i<elementof(remotepcs);i++) {
        if( remotepcs[i] && remotepcs[i]->client_id == client_id ) {
            tgt = remotepcs[i];
            break;
        }
    }
    if(!tgt) {
        for( int i=0;i<elementof(remotepcs);i++) {
            if( remotepcs[i] == NULL ) {
                tgt = remotepcs[i] = new WorldMapIndicator( WMIND_PC );
                g_wm_layer->insertProp(tgt);
                break;
            }
        }
    }
    if(!tgt) return; // full
    tgt->last_packet_at = accum_time; 
    tgt->client_id = client_id;
    tgt->face->update(face_ind,hair_ind);
    tgt->loc = ( lc - g_pc->loc ) / PPC;
    if(nickname) {
        strncpy( tgt->nickname, nickname, sizeof(tgt->nickname) );
        tgt->name_tb->setString(WHITE, nickname);
    }
    tgt->setVisible( visible );
    
}


/////////////////////////////

ExchangeWindow::ExchangeWindow() : Window(40,25, B_ATLAS_WINDOW_FRAME_BASE ), cursor_id_at(-1), depo_lock_obtained_at(0) {
    g_hud_layer->insertProp(this);
    setLoc( -SCRW/2+4,-300 );
    group = new PropGroup( elementof(pcitems) + elementof(storeditems) );
    // Items that PC have
    for(int i=0;i<elementof(pcitems);i++ ) {
        ItemPanel *p = new ItemPanel(true);
        pcitems[i] = p;
        group->reg(p);
        g_hud_layer->insertProp(p);
    }
    for(int i=0;i<PC_SUIT_NUM;i++) {
        pcitems[i]->setLoc( Suit::getPosByIndex(i) );
    }
    for(int i=0;i<PC_BELT_NUM;i++) {
        pcitems[PC_SUIT_NUM+i]->setLoc( Belt::getPosByIndex(i) );
    }    
    Vec2 base = pcitems[PC_BELT_NUM+PC_SUIT_NUM-1]->loc + Vec2( 120,0 );
    for(int i=PC_BELT_NUM+PC_SUIT_NUM;i<elementof(pcitems);i++ ) {    
        int x = (i-PC_BELT_NUM-PC_SUIT_NUM) % 3;
        int y = (i-PC_BELT_NUM-PC_SUIT_NUM) / 3;
        pcitems[i]->setLoc( base + Vec2( x * 100, y * (48+2) ) );
    }
    cursor_id_at = pcitems[0]->id;
    
    // Items that are deposit
    base = pcitems[PC_SUIT_NUM+PC_BELT_NUM-1]->loc + Vec2(500,0);
    for(int i=0;i<elementof(storeditems);i++) {
        ItemPanel *p = new ItemPanel(true);
        storeditems[i] = p;
        group->reg(p);
        int x = i % 3;
        int y = i / 3;
        p->setLoc( base + Vec2(x*140, y*(48+2) ) );
        g_hud_layer->insertProp(p);
    }
    deposit_caption = new CharGridTextBox(20);
    deposit_caption->setString( WHITE, "(not set)");
    deposit_caption->setLoc( loc + Vec2(120,480) );
    g_hud_layer->insertProp( deposit_caption );

    withdraw_caption = new CharGridTextBox(20);
    withdraw_caption->setString( WHITE, "(not set)");
    withdraw_caption->setLoc( loc + Vec2(500, 480 ) );
    g_hud_layer->insertProp( withdraw_caption );

    title_tb = new CharGridTextBox(48);
    title_tb->setString( WHITE, "(not set)");
    title_tb->setLoc( loc + Vec2(40,550) );
    g_hud_layer->insertProp( title_tb );

    state_caption = new CharGridTextBox(48);
    state_caption->setString( WHITE, "(not set)");
    state_caption->setLoc( Vec2(-380,0) );
    g_hud_layer->insertProp( state_caption );
    
    
    updateCaption(0,EXCHANGE_CONSUME_ENE_PER_ITEM);
    update();
}

void ExchangeWindow::updateCaption( int cur_ene, int req_ene ) {
    Format fmt( "RESOURCE EXCHANGE TERMINAL ENE:%d REQUIRED:%d", cur_ene, req_ene );
    Color c;
    if( cur_ene < EXCHANGE_CONSUME_ENE_PER_ITEM ) {
        c = RED;
    } else {
        c = WHITE;
    }
    deposit_caption->setString( c, "TO DEPOSIT:" );
    withdraw_caption->setString( c, "TO WITHDRAW:" );
    title_tb->setString(c, fmt.buf );

    if( depo_lock_obtained_at == 0 ) {
        state_caption->setString( WHITE, MSG_OCCUPIED_BY_USER );
    }
}

void ExchangeWindow::toggle( bool vis ) {
    setVisible(vis);

    if(vis) {
        playWindowOpenSoundOnce();
        update();
    }
    if( isDBNetworkActive() == false ) {
        state_caption->setVisible(false);
        toggleContent(vis);        
    } else {
        if( depo_lock_obtained_at == 0 ) {
            state_caption->setVisible(vis);
            toggleContent(false);
        } else {
            state_caption->setVisible(false);
            toggleContent(vis);
        }
    }
    if(!vis) {
        if( g_current_project_id > 0 ) {
            dbSavePC();
            dbSaveResourceDeposit( g_current_project_id );
            realtimeUnlockProjectSend( g_current_project_id, LOCK_RESOURCEDEPO );
        }
    }
}
void ExchangeWindow::toggleContent(bool vis) {
    
    for(int i=0;i<elementof(pcitems);i++) {
        pcitems[i]->toggle(vis);
    }
    for(int i=0;i<elementof(storeditems);i++) {
        storeditems[i]->toggle(vis);
    }
    deposit_caption->setVisible(vis);
    withdraw_caption->setVisible(vis);
    title_tb->setVisible(vis);
}

void ExchangeWindow::update() {
    if(!visible) return;
    for(int i=0;i<elementof(pcitems);i++) {
        pcitems[i]->update( & g_pc->items[i] );
        pcitems[i]->setSelected( pcitems[i]->id == cursor_id_at );
    }

    for(int i=0;i<g_depo->item_num;i++) {
        Item itm;
        if( g_depo->items[i].isEmpty() == false ) {
            itm.set( g_depo->items[i].itt, g_depo->items[i].num, g_depo->items[i].dur );
        } 
        storeditems[i]->update( &itm );
        storeditems[i]->setSelected( storeditems[i]->id == cursor_id_at );
    }
}
void ExchangeWindow::cancel() {
    hide();
}

void ExchangeWindow::moveCursor(DIR dir) {
    if( getVisible() == false ) return;

    Prop2D *cur = (Prop2D*) g_hud_layer->findPropById( cursor_id_at );
    if(!cur)return;
    Prop2D *next = group->getNext( cur, dir );
    if(!next)return;
    cursor_id_at = next->id;
    update();
}
void ExchangeWindow::selectAtCursor() {
    Prop2D *p = (Prop2D*) g_hud_layer->findPropById( cursor_id_at );
    assert(p);
    
    Pos2 from( g_pc->loc.x/PPC, g_pc->loc.y/PPC);
    PowerEquipmentNode *nd = g_fld->findNearestExchange( from );
    if(!nd)return;
    PowerGrid *pg = g_ps->findGrid(nd->at);
    if(!pg) return;

    if( pg->ene < EXCHANGE_CONSUME_ENE_PER_ITEM ) {
        soundPlayAt( g_cant_sound, g_pc->loc,1 );
        return;
    } 
    
    // Selected items that PC have
    for(int i=0;i<elementof(pcitems);i++) {
        if( pcitems[i] == p ) {
            print("PCITEM[%d] dur:%d", i, g_pc->items[i].dur );
            if( g_pc->items[i].isEmpty() == false && g_pc->items[i].isDamaged() ) {
                g_cant_sound->play();
                hudShowWarningMessage( "CAN'T DEPOSIT DAMAGED ITEMS!" );
                return;
            }
            if( g_pc->items[i].isEmpty() == false ) {
                g_craft_sound->play();
                if( g_depo->incItem( g_pc->items[i].itt, 1, 0, false ) ) {
                    g_pc->decItemIndex(i, 1);
                    pg->modEne( -EXCHANGE_CONSUME_ENE_PER_ITEM, true ); // sync to other game servers
                    return;
                } else {
                    print("exchange full!"); 
                    return;
                }
            }
        }
    }
    // Selected items that are deposit
    for(int i=0;i<g_depo->item_num;i++) {
        if( storeditems[i] == p ) {
            print("STOREDITEM[%d]", i );
            if( g_depo->items[i].isEmpty() == false ) {
                g_craft_sound->play();
                if( g_pc->incItem( g_depo->items[i].itt, 1, PC_SUIT_NUM, false ) ) {
                    // Try to remove an item from nearby exchange
                    if( g_depo->decItem( g_depo->items[i].itt, 1 ) ) {
                        print("removed from depo." );
                    } 
                    update();
                    pg->modEne( -EXCHANGE_CONSUME_ENE_PER_ITEM, true ); //  sync to other game servers
                    return;
                }
            }
        }
    }
}
void ExchangeWindow::poll() {
    Pos2 from( g_pc->loc.x/PPC, g_pc->loc.y/PPC);
    PowerEquipmentNode *nd = g_fld->findNearestExchange( from );
    if(nd) {
        PowerGrid *pg = g_ps->findGrid(nd->at);
        if(pg) {
            updateCaption( pg->ene, EXCHANGE_CONSUME_ENE_PER_ITEM );
        }
    } else {
        updateCaption( 0, EXCHANGE_CONSUME_ENE_PER_ITEM );        
    }

    if( visible ) {
        if( depo_lock_obtained_at == 0 ) {
            realtimeLockProjectSend( g_current_project_id, LOCK_RESOURCEDEPO );
        } else if( depo_lock_obtained_at < now() - 0.5 ) {
            if( updateInterval(0,1.0f) ) realtimeLockKeepProjectSend( g_current_project_id, LOCK_RESOURCEDEPO );
        }
    }
}

void ExchangeWindow::onLockObtained() {
    depo_lock_obtained_at = now();
    dbLoadResourceDeposit( g_current_project_id );
    toggle(true);
}
void ExchangeWindow::onLockReleased() {
    depo_lock_obtained_at = 0;
}

//////////////////////////////
PortalWindow::PortalWindow() :Window(25,15, B_ATLAS_WINDOW_FRAME_BASE), cursor_at(0) {
    g_hud_layer->insertProp(this);
    setLoc(-300,-200);

    Vec2 center = loc + Vec2(250,180);
    // UP
    directions[0] = new ItemPanel(false);
    directions[0]->setLoc( center + Vec2(0,100) );
    // RIGHT
    directions[1] = new ItemPanel(false);
    directions[1]->setLoc( center + Vec2(150,0) );
    // DOWN
    directions[2] = new ItemPanel(false);
    directions[2]->setLoc( center + Vec2(0,-100) );
    // LEFT
    directions[3] = new ItemPanel(false);
    directions[3]->setLoc( center + Vec2(-150,0) );
    for(int i=0;i<elementof(directions);i++) {
        g_hud_layer->insertProp(directions[i] );
    }

    title_tb = new CharGridTextBox(40);
    title_tb->setString( WHITE, "(not set)");
    title_tb->setLoc( loc + Vec2(30,330) );
    g_hud_layer->insertProp( title_tb );
        
    update();
}
void PortalWindow::updateCaption( int cur_ene, int req_ene ) {
    Color c;
    if( cur_ene < req_ene ) c = RED; else c = WHITE;
    Format f("PORTAL TERMINAL: ENE: %d/%d", cur_ene, req_ene );
    title_tb->setString( c, f.buf );
    
    directions[0]->setCaption( c, "NORTH" );
    directions[0]->setItemDirect( g_base_deck, B_ATLAS_UP_ARROW, 48 );
    directions[1]->setCaption( c, "EAST" );
    directions[1]->setItemDirect( g_base_deck, B_ATLAS_RIGHT_ARROW, 48 );    
    directions[2]->setCaption( c, "SOUTH" );
    directions[2]->setItemDirect( g_base_deck, B_ATLAS_DOWN_ARROW, 48 );        
    directions[3]->setCaption( c, "WEST" );
    directions[3]->setItemDirect( g_base_deck, B_ATLAS_LEFT_ARROW, 48 );            
    
}

void PortalWindow::toggle( bool vis ) {
    setVisible(vis);
    if(vis) playWindowOpenSoundOnce();

    for(int i=0;i<elementof(directions);i++) {
        directions[i]->toggle(vis);
    }
    title_tb->setVisible(vis);
}
void PortalWindow::update() {
    if( !getVisible() ) return;
    for(int i=0;i<elementof(directions);i++) {
        directions[i]->setSelected( cursor_at == i );
    }

}
void PortalWindow::moveCursor( DIR dir ) {
    switch(dir) {
    case DIR_UP:  cursor_at = 0; break;
    case DIR_RIGHT: cursor_at = 1; break;
    case DIR_DOWN: cursor_at = 2; break;
    case DIR_LEFT: cursor_at = 3; break;
    default: break;
    }
    update();
}

void PortalWindow::selectAtCursor() {
    print("portalwindow: cursor: %d", cursor_at );
    DIR d = DIR_NONE;
    switch( cursor_at ) {
    case 0: d = DIR_UP; break;
    case 1: d = DIR_RIGHT;break;
    case 2: d = DIR_DOWN; break;
    case 3: d = DIR_LEFT; break;
    default:
        assertmsg(false, "invalid cursor pos" ); break;
    }
    Pos2 p;
    if( g_fld->findNearestBlock( vec2ToPos2(g_pc->loc), BT_PORTAL_ACTIVE, 2, &p ) ) {
        Cell *c = g_fld->get(p);
        PowerGrid *pg = g_ps->findGridById( c->powergrid_id );
        if(pg && pg->ene >= PORTAL_ENE_REQUIRED_TO_WARP ) {
            pg->modEne( - PORTAL_ENE_REQUIRED_TO_WARP, true );
            g_pc->startWarp( p, d);
        }
    }
    hide();
}
void PortalWindow::poll() {
    Pos2 p;
    Pos2 from = vec2ToPos2( g_pc->loc );

    if( g_fld->findNearestBlock( from, BT_PORTAL_ACTIVE, 2, &p ) ) {
        Cell *c = g_fld->get(p);
        if(c && c->powergrid_id > 0 ) {
            PowerGrid *pg = g_ps->findGridById(c->powergrid_id);
            if(!pg) {
                updateCaption( 0, PORTAL_ENE_REQUIRED_TO_WARP );
            } else {
                updateCaption( pg->ene, PORTAL_ENE_REQUIRED_TO_WARP );
            }
        } else {
            updateCaption( 0, PORTAL_ENE_REQUIRED_TO_WARP );
        }
    } else {
        updateCaption( 0, PORTAL_ENE_REQUIRED_TO_WARP );
    }
}

/////////////////////////

FlagArrow::FlagArrow(){
    setDeck( g_base_deck );
    setIndex( B_ATLAS_LONG_ARROW );
    setScl(PIXEL_PER_CELL*1.5,PIXEL_PER_CELL*1.5);
    setColor( 1,1,0,0.5 );
    setLoc(0,0);

    lv_caption = new CharGridTextBox(20);
    lv_caption->setString( WHITE, "HOGE");
    lv_caption->setLoc(loc);
    addChild(lv_caption);
    updateLevel(1);

    g_hints_layer->insertProp(this);
}
void FlagArrow::updateLevel(int v){
    char tmp[100];
    snprintf( tmp, sizeof(tmp), "MILESTONE %d", v );
    lv_caption->setString( Color(1,1,1,0.5), tmp );
}

// Convert from world to local axis internally
void FlagArrow::updateLoc(Vec2 from_world, Vec2 to_world ) {
    Vec2 tov = from_world.to(to_world);
    float distance = tov.len();
    float arrow_l, capt_l;
    //    if( distance < SCRW/2/g_zoom_rate && distance < SCRH/2/g_zoom_rate) {
    float maxx = SCRW/2-240;
    if( isInScreen(tov) ) {
        // inside screen
        float distance_in_screen = distance * g_zoom_rate;
        arrow_l = distance_in_screen - 20;
        capt_l = distance_in_screen - 40;
        if( capt_l < 0 ) capt_l = 0;
        setLoc(tov.normalize(arrow_l));
        Vec2 caploc = tov.normalize(capt_l);
        if( caploc.x > maxx ) caploc.x = maxx;
        lv_caption->setLoc(caploc);
    } else {
        // outside screen
        Vec2 edgeloc = hudCalcScreenEdge(tov, 30,30,30,100);
        setLoc(edgeloc);
        Vec2 caploc = tov.normalize(edgeloc.len()-30);
        if( caploc.x > maxx ) caploc.x = maxx;
        lv_caption->setLoc( caploc );
    }
    setRot(atan2(tov.y,tov.x) );
}
///////////////////
MilestoneDisplay::MilestoneDisplay() : CharGridTextBox(16) {
    setLoc( SCRW/2 - 220, -SCRH/2+48);
    g_gauge_layer->insertProp(this);
}

void MilestoneDisplay::updateMilestone( int ms ) {
    char tmp[20];
    snprintf(tmp,sizeof(tmp), "MILESTONE %d", ms );
    setString( WHITE, tmp );
}
///////////////////////
    
static const int face_cand_inds[] = {
    30*AU+24, 30*AU+26, 30*AU+28, 30*AU+30
};
static const int hair_cand_inds[] = {
    29*AU+24, 29*AU+27, 28*AU+24, 28*AU+27
};
static const int body_cand_inds[] = {
    31*AU+24, 32*AU+24, 33*AU+24, 34*AU+24
};
static const int beam_cand_inds[] = {
    B_ATLAS_PC_BEAM, B_ATLAS_PC_BEAM+1, B_ATLAS_PC_BEAM+2, B_ATLAS_PC_BEAM+3
};
#define PREPARATION_WINDOW_LOC Vec2( -SCRW/2+4, -SCRH/2+PIXEL_PER_CELL )
#define PREPARATION_GRID_WIDTH ((SCRW - 8) / PIXEL_PER_CELL)
#define PREPARATION_GRID_HEIGHT ((SCRH / PIXEL_PER_CELL) - 2)

CharMakeWindow::CharMakeWindow() : Window( PREPARATION_GRID_WIDTH, PREPARATION_GRID_HEIGHT, B_ATLAS_WINDOW_FRAME_BASE, WINDOW_ALPHA ), selected_shoot_sound_index(0) {
    g_hud_layer->insertProp(this);
    setLoc(PREPARATION_WINDOW_LOC);

    title_tb = new CharGridTextBox(30);
    title_tb->setString( WHITE, "CREATE CHARACTER" );
    title_tb->setLoc(getTitleLoc());
    g_hud_layer->insertProp( title_tb );

    charloc = loc + Vec2(100,400);
    float charsclrate = 3;
    float charscl = 24*charsclrate;
    body_prop = new Prop2D();
    body_prop->setDeck( g_base_deck );
    body_prop->setIndex( B_ATLAS_PC_BODY_BASE );
    body_prop->setScl(charscl);
    body_prop->setLoc(charloc);
    g_hud_layer->insertProp(body_prop);    
    face_prop = new Prop2D();
    face_prop->setDeck( g_base_deck );
    face_prop->setIndex( B_ATLAS_PC_FACE_FRONT );
    face_prop->setScl(charscl);
    face_prop->setLoc(charloc);
    g_hud_layer->insertProp(face_prop);
    hair_prop = new Prop2D();
    hair_prop->setDeck( g_base_deck );
    hair_prop->setIndex( B_ATLAS_PC_HAIR_FRONT );
    hair_prop->setScl(charscl);
    hair_prop->setLoc(charloc);
    g_hud_layer->insertProp(hair_prop);
    equip_prop = new Prop2D();
    equip_prop->setDeck( g_base_deck );
    equip_prop->setScl(charscl);
    equip_prop->setLoc(charloc + PC::calcEquipPosition(DIR_DOWN) * charsclrate);
    equip_prop->setIndex( B_ATLAS_PC_EQUIPMENT_BEAMGUN_BASE );
    g_hud_layer->insertProp(equip_prop);
    beam_prop = new Prop2D();
    beam_prop->setDeck( g_base_deck );
    beam_prop->setScl( charscl );
    beam_prop->setLoc( charloc + Vec2(-6*charsclrate,-16*charsclrate) );
    beam_prop->setIndex( B_ATLAS_PC_BEAM );
    beam_prop->setRot( -M_PI/2);
    g_hud_layer->insertProp(beam_prop);


    assert( elementof(hair_cand_inds) <= elementof(hair_cands));
    assert( elementof(body_cand_inds) <= elementof(body_cands));
    assert( elementof(face_cand_inds) <= elementof(face_cands));
    assert( elementof(beam_cand_inds) <= elementof(beam_cands));    
    
    for(int i=0;i<elementof(hair_cands);i++) hair_cands[i]=NULL;
    for(int i=0;i<elementof(face_cands);i++) face_cands[i]=NULL;
    for(int i=0;i<elementof(body_cands);i++) body_cands[i]=NULL;
    for(int i=0;i<elementof(beam_cands);i++) beam_cands[i]=NULL;

    group = new PropGroup( elementof(hair_cands) + elementof(body_cands) + elementof(face_cands)+ elementof(beam_cands) +
                           elementof(sound_cands) + 1 /* next button */ );

    float topy = 480;
    for(int i=0;i<elementof(hair_cand_inds);i++) {
        hair_cands[i] = new Prop2D();
        hair_cands[i]->setScl(charscl);
        hair_cands[i]->setDeck( g_base_deck );
        hair_cands[i]->setLoc( loc + Vec2(500 + i * (charscl+8) ,topy) );
        hair_cands[i]->setIndex( hair_cand_inds[i] );
        g_hud_layer->insertProp(hair_cands[i]);
        group->reg( hair_cands[i] );
    }
    cursor_id_at = hair_cands[0]->id;
    
    for(int i=0;i<elementof(face_cand_inds);i++) {
        face_cands[i] = new Prop2D();
        face_cands[i]->setScl(charscl);
        face_cands[i]->setDeck( g_base_deck );
        face_cands[i]->setLoc( loc + Vec2(500 + i * (charscl+8), topy-80 ) );
        face_cands[i]->setIndex( face_cand_inds[i] );
        g_hud_layer->insertProp(face_cands[i]);
        group->reg( face_cands[i] );        
    }

    for(int i=0;i<elementof(body_cand_inds);i++) {
        body_cands[i] = new Prop2D();
        body_cands[i]->setScl(charscl);
        body_cands[i]->setDeck( g_base_deck );
        body_cands[i]->setLoc( loc + Vec2(500 + i * (charscl+8), topy-160 ) );
        body_cands[i]->setIndex( body_cand_inds[i] );
        g_hud_layer->insertProp( body_cands[i] );
        group->reg( body_cands[i] );                
    }
    for(int i=0;i<elementof(beam_cand_inds);i++) {
        beam_cands[i] = new Prop2D();
        beam_cands[i]->setScl(charscl);
        beam_cands[i]->setDeck( g_base_deck );
        beam_cands[i]->setLoc( loc + Vec2(500 + i * (charscl+8), topy-240 ) );
        beam_cands[i]->setIndex( beam_cand_inds[i] );
        g_hud_layer->insertProp( beam_cands[i] );
        group->reg( beam_cands[i] );                        
    }
    for(int i=0;i<elementof(sound_cands);i++){
        sound_cands[i] = new Prop2D();
        sound_cands[i]->setScl(8*2);
        sound_cands[i]->setDeck( g_ascii_deck );
        sound_cands[i]->setLoc( loc + Vec2(500 + i * (16+16), topy-320 ) );
        sound_cands[i]->setIndex( A_ATLAS_BIG_DIGIT_START + i );
        g_hud_layer->insertProp( sound_cands[i]);
        group->reg( sound_cands[i] );                                
    }

    hair_caption = new CharGridTextBox(20);
    hair_caption->setString( WHITE, "HAIR:" );
    hair_caption->setLoc( loc + Vec2(300,topy) );
    g_hud_layer->insertProp(hair_caption);


    face_caption = new CharGridTextBox(20);
    face_caption->setString( WHITE, "FACE:" );
    face_caption->setLoc( loc + Vec2(300,topy-80));
    g_hud_layer->insertProp(face_caption);

    body_caption = new CharGridTextBox(20);
    body_caption->setString( WHITE, "BODY:" );
    body_caption->setLoc( loc + Vec2(300,topy-160));
    g_hud_layer->insertProp(body_caption);
    
    beam_caption = new CharGridTextBox(20);
    beam_caption->setString( WHITE, "BEAM:" );
    beam_caption->setLoc( loc + Vec2(300,topy-240) );
    g_hud_layer->insertProp(beam_caption);
    
    sound_caption = new CharGridTextBox(20);
    sound_caption->setString( WHITE, "BEAM SOUND:" );
    sound_caption->setLoc( loc + Vec2(300,topy-320) );
    g_hud_layer->insertProp(sound_caption);    

    next_button = new CharGridTextBox(8);
    next_button->setString( WHITE, "NEXT" );
    next_button->setLoc( loc + Vec2( 700, 100 ) );
    g_hud_layer->insertProp(next_button);
    group->reg( next_button );

    cursor = new BlinkCursor();
    g_hud_layer->insertProp(cursor);

    update();
}

void CharMakeWindow::toggle(bool vis) {
    setVisible(vis);
    title_tb->setVisible(vis);
    hair_prop->setVisible(vis);
    body_prop->setVisible(vis);
    face_prop->setVisible(vis);
    equip_prop->setVisible(vis);
    beam_prop->setVisible(vis);
    for(int i=0;i<elementof(body_cands);i++) if( body_cands[i] ) body_cands[i]->setVisible(vis);
    for(int i=0;i<elementof(hair_cands);i++) if( hair_cands[i] ) hair_cands[i]->setVisible(vis);
    for(int i=0;i<elementof(face_cands);i++) if( face_cands[i] ) face_cands[i]->setVisible(vis);
    for(int i=0;i<elementof(beam_cands);i++) if( beam_cands[i] ) beam_cands[i]->setVisible(vis);
    for(int i=0;i<elementof(sound_cands);i++) if( sound_cands[i] ) sound_cands[i]->setVisible(vis);    
    hair_caption->setVisible(vis);
    face_caption->setVisible(vis);
    body_caption->setVisible(vis);
    beam_caption->setVisible(vis);
    sound_caption->setVisible(vis);
    next_button->setVisible(vis);
    cursor->setVisible(vis);
}

void CharMakeWindow::update() {
    print( "charmakewindow: cursor_id_at:%d",cursor_id_at);

    Prop2D *curp = group->find( cursor_id_at );
    assert(curp);
    if( cursor_id_at == next_button->id ) {
        cursor->loc = curp->loc + Vec2(24,-24);
    } else {
        cursor->loc = curp->loc + Vec2(0,-32);        
        for(int i=0;i<elementof(body_cands);i++) {
            if(body_cands[i] && cursor_id_at == body_cands[i]->id ) {
                cursor->loc = curp->loc + Vec2(0,-44);
                break;
            }
        }
    }
}

void CharMakeWindow::moveCursor( DIR dir ) {
    print("charmakewindow: moveCursor: dir:%d",dir);
    Prop2D *from_p = group->find( cursor_id_at );
    Prop2D *to_p = group->getNext( from_p, dir );
    cursor_id_at = to_p->id;
    g_cursormove_sound->play();
    update();
}
void CharMakeWindow::selectAtCursor() {
    g_craft_sound->play();
    print("charmakewindow: selectAtCursor: cursor_id_at:%d", cursor_id_at );
    for(int i=0;i<elementof(sound_cands);i++){
        if( sound_cands[i] && sound_cands[i]->id == cursor_id_at ) {
            selected_shoot_sound_index = i;
            g_pc->shoot_sound_index = i;
            break;
        }
    }
    for(int i=0;i<elementof(hair_cands);i++) {
        if( hair_cands[i] && hair_cands[i]->id == cursor_id_at ) {
            hair_prop->setIndex( hair_cand_inds[i] );
            g_pc->hair_base_index = hair_cand_inds[i];
        }
    }
    for(int i=0;i<elementof(body_cands);i++) {
        if( body_cands[i] && body_cands[i]->id == cursor_id_at ) {
            body_prop->setIndex( body_cand_inds[i] );
            g_pc->body_base_index = body_cand_inds[i];
        }
    }
    for(int i=0;i<elementof(face_cands);i++) {
        if( face_cands[i] && face_cands[i]->id == cursor_id_at ) {
            face_prop->setIndex( face_cand_inds[i] );
            g_pc->face_base_index = face_cand_inds[i];            
        }
    }
    for(int i=0;i<elementof(beam_cands);i++) {
        if( beam_cands[i] && beam_cands[i]->id == cursor_id_at ) {
            beam_prop->setIndex( beam_cand_inds[i] );
            g_pc->beam_base_index = beam_cand_inds[i];            
        }
    }

    if( cursor_id_at == next_button->id ) {
        print("GO TO CHARNAME");
        hide();
        g_charnamewin->show();
        return;
    }

    update();
}
bool CharMakeWindow::windowPoll(double dt) {
    if(!visible)return true;

	static double accumTime = 0.0;
	accumTime += dt;
    double step_time = 1.0f / (double) g_ideal_frame_rate;
	while (accumTime >= step_time) {
		accumTime -= step_time;
		beam_prop->loc += Vec2(0,-16);
	}

	if( beam_prop->loc.y < -200 ) {
		beam_prop->loc = charloc + Vec2(-6*3,-16*3);
		g_shoot_sound[ selected_shoot_sound_index ]->play();
	}

    return true;
}
/////////////////
ProjectTypeWindow::ProjectTypeWindow() : Window(PREPARATION_GRID_WIDTH, PREPARATION_GRID_HEIGHT, B_ATLAS_WINDOW_FRAME_BASE, WINDOW_ALPHA ) {
    g_hud_layer->insertProp(this);
    setLoc(PREPARATION_WINDOW_LOC);
    group = new PropGroup( 4 );

    title_tb = new CharGridTextBox(30);
    title_tb->setString( WHITE, "SELECT PROJECT TYPE:" );
    title_tb->setLoc(getTitleLoc());
    g_hud_layer->insertProp(title_tb);

    
    Vec2 base = title_tb->loc + Vec2(50,-80);
    mine = new PictLabel( base, B_ATLAS_LIGHTBLUE_CIRCLE );
    mine->setString("  MY PROJECTS");
    mine->setScl(48);
    cursor_id_at = mine->id;
    g_hud_layer->insertProp(mine);
    group->reg(mine);

    shared = new PictLabel( base + Vec2(0,-100), B_ATLAS_LIGHTGREEN_CIRCLE );
    shared->setString( "  SHARED PROJECTS");
    shared->setScl(48);
    g_hud_layer->insertProp(shared);
    group->reg(shared);
    
    pub = new PictLabel( base + Vec2(0,-200), B_ATLAS_ORANGE_CIRCLE );
    pub->setString( "  PUBLIC PROJECTS" );
    pub->setScl(48);
    g_hud_layer->insertProp(pub);
    group->reg(pub);

    back_tb = new CharGridTextBox(4);
    back_tb->setString( WHITE, "BACK" );
    back_tb->setLoc( pub->loc + Vec2(48,-96) );
    g_hud_layer->insertProp(back_tb);
    group->reg(back_tb);

    cursor = new BlinkCursor( DIR_RIGHT );
    g_hud_layer->insertProp(cursor);

    update();
}
void ProjectTypeWindow::toggle(bool vis) {
    setVisible(vis);
    title_tb->setVisible(vis);

    mine->setVisible(vis);
    shared->setVisible(vis);
    pub->setVisible(vis);
    back_tb->setVisible(vis);

    cursor->setVisible(vis);
}
void ProjectTypeWindow::update() {
    Prop2D *curp = group->find( cursor_id_at );
    cursor->loc = curp->loc + Vec2(-48,0);
}
void ProjectTypeWindow::moveCursor( DIR dir ) {
    Prop2D *from_p = group->find(cursor_id_at);
    Prop2D *to_p = group->getNext( from_p, dir );
    cursor_id_at = to_p->id;
    g_cursormove_sound->play();
    update();
}
void ProjectTypeWindow::selectAtCursor() {
    g_craft_sound->play();
    hide();

    Prop2D *p = group->find(cursor_id_at);
    if( p == pub ) {
        g_projlistwin->list_type = PJLT_PUBLIC;
        g_projlistwin->show();
    } else if( p == shared ) {
        g_projlistwin->list_type = PJLT_SHARED;
        g_projlistwin->show();
    } else if( p == mine ){
        g_projlistwin->list_type = PJLT_PRIVATE;
        g_projlistwin->show();
    } else if( p == back_tb ) {
        g_titlewin->show();
    }

    
}

////////////////////////////
ProjectListWindow::ProjectListWindow() : Window( PREPARATION_GRID_WIDTH, PREPARATION_GRID_HEIGHT, B_ATLAS_WINDOW_FRAME_BASE, WINDOW_ALPHA ){
    g_hud_layer->insertProp(this);
    setLoc(PREPARATION_WINDOW_LOC);
    
    //

    title_tb = new CharGridTextBox(30);
    title_tb->setString( WHITE, "SELECT A PROJECT :" );
    title_tb->setLoc( getTitleLoc());
    g_hud_layer->insertProp(title_tb);

    header = new CharGridTextBox(64);
    header->setString( WHITE,       "ID       OWNER        CREATED   MS.  PLAYERS TOT.TIME" );
    header->setLoc( getHeaderLoc() );
    g_hud_layer->insertProp(header);

    
    Vec2 base = header->loc + Vec2( 0, -40 );
    for(int i=0;i<elementof(lines);i++) {
        lines[i] = new CharGridTextBox(64);
        lines[i]->setString( WHITE, "01234567 0123456789AB 012345678 0123 0123    01:23:45" );
        lines[i]->setLoc( base - Vec2(0, 32 * i));
        g_hud_layer->insertProp(lines[i]);
    }
    project_num = 0;
    for(int i=0;i<elementof(project_id_cache);i++) project_id_cache[i]=0;
    cursor_at = 0;
    cursor = new BlinkCursor( DIR_RIGHT );
    g_hud_layer->insertProp(cursor);

    list_type = PJLT_PRIVATE;
    update();
}

void makeShortHumanDiffTimeSec( char *out, size_t outlen, int dtsec ) {
    if( dtsec < 60 ) {
        snprintf(out,outlen,"NOW");
    } else if( dtsec < 3600 ) {
        snprintf(out,outlen,"%dMIN", (int)(dtsec/60) );
    } else if( dtsec < 3600*24 ) {
        snprintf(out,outlen,"%dHOUR", (int)(dtsec/60/60) );
    } else {
        snprintf(out,outlen,"%dDAY", (int)(dtsec/60/60/24) );
    } 
}


int countMilestoneCleared(ProjectInfo *pinfo ) {
    int n=0;
    for(int i=0;i<elementof(pinfo->flag_cands);i++) {
        if( pinfo->flag_cands[i].finished != 0 ) {
            n++;
        }
    }
    return n;
}



void ProjectListWindow::toggle(bool vis) {
    if(vis==false) {
        // Can't close during saving
        if( g_fsaver->active ) {
            print("ProjectListWindow::toggle: can't close! now saving");
            return;
        }
    }
    setVisible(vis);
    header->setVisible(vis);
    title_tb->setVisible(vis);
    for(int i=0;i<elementof(lines);i++) {
        lines[i]->setVisible(vis);
    }
    cursor->setVisible(vis);
    if(vis) {
        cursor_at = 0;
        updateProjectList();
        update();
    } 
}
void ProjectListWindow::clear() {
    for(int i=0;i<elementof(lines);i++) {
        lines[i]->setString(WHITE,"");
    }
}


// progress_mode: show from.ms, now.ms
void makeProjectInfoLineString( char *out, size_t outlen, const char *prefix, ProjectInfo *pi, int players, int playsec, bool progress_mode, int progress ) {
    assert(pi);
    char id_str[8+1], owner_str[12+1], created_str[9+1], milestone_str[3+1], players_str[3+1];
    snprintf( id_str, sizeof(id_str), "%-8d", pi->project_id );
    truncateString(owner_str, pi->owner_nickname, sizeof(owner_str)-1 );

    unsigned int curtime = dbGetTime(); 
    int dt = curtime - pi->created_at;
    makeShortHumanDiffTimeSec( created_str, sizeof(created_str), dt );
    int ms_cleared = countMilestoneCleared(pi);
    if( ms_cleared == MILESTONE_MAX ) {
        snprintf( milestone_str, sizeof(milestone_str), "FIN" );
    } else {
        snprintf( milestone_str, sizeof(milestone_str), "%d", countMilestoneCleared(pi) );
    }
    snprintf( players_str, sizeof(players_str), "%d", players );

    char hmsstr[32];
    makeHourMinSecString( playsec, hmsstr, sizeof(hmsstr) );
    if( progress_mode ) {
        snprintf( out, outlen, "%s%-8s %-12s %8d/%-5d %-4s", prefix, id_str, owner_str, progress, ms_cleared, players_str );
    } else {
        snprintf( out, outlen, "%s%-8s %-12s %-9s %-4s %-4s    %s", prefix, id_str, owner_str, created_str, milestone_str, players_str, hmsstr );
    }
}

void setupProjectInfoTB( CharGridTextBox *out_tb, const char *prefix, int projid, bool progress_mode=false, int progress=0 ) {
    ProjectInfo pinfo;
    if( dbLoadProjectInfo( projid, &pinfo) ) { // TODO: to avoid loop, you have to implement new backend protocol
        int online_num = dbCountOnlinePlayerSync( projid );
        int playsec = dbLoadPlaytime( projid );
        char msg[100];
        makeProjectInfoLineString( msg, sizeof(msg), prefix, &pinfo, online_num, playsec, progress_mode, progress );
        //                snprintf(msg,sizeof(msg), "ID %d owner:%s", projids[i], pinfo.owner_nickname );
        out_tb->setString(WHITE,msg);
    } else {
        out_tb->setString(WHITE, Format("CANNOT LOAD PROJECT INFO ID:%d", projid ).buf );
    }
}



void ProjectListWindow::updateProjectList() {
    clear();
    project_num = 0;

    if( list_type == PJLT_PRIVATE ) { 
        // JSON forat : [ 112233, 113355, 11491, 5830, 85959 ] array of interger of project_id
        int projids[elementof(lines)-1]; // -1:BACK
        int n = elementof(projids);
        bool dbres = dbLoadIntArrayJSON( g_project_by_owner_ht_key, g_user_name, projids, &n);
        if(dbres == false) {
            print("NO project!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        } else {
            project_num = n;
            for(int i=0;i<n;i++) {
                setupProjectInfoTB( lines[i], "", projids[i] );
                project_id_cache[i] = projids[i];
            }
        }
        if( project_num < MAX_PRIVATE_PROJECT ) {
            lines[ project_num ]->setString( WHITE, "CREATE NEW PROJECT" ); // Last button is always CREATE
        }
        for(int i=0;i<elementof(lines);i++) {
            if( lines[i]->isEmpty() ) {
                lines[i]->setString( WHITE, "BACK" );
                break;
            }
        }
    } else if( list_type == PJLT_SHARED ) {
        int projids[elementof(lines)-1]; // -1:BACK用
        int n=elementof(projids);
        dbSearchSharedProjects(projids,&n);
        project_num = n;
        if(n==0) {
            lines[0]->setString( WHITE, "NO SHARED PROJECT BY FRIENDS");
            lines[1]->setString( WHITE, "BACK" );
        } else {
            for(int i=0;i<n;i++) {
                setupProjectInfoTB( lines[i], "", projids[i] );
                project_id_cache[i] = projids[i];
            }
            lines[n]->setString( WHITE, "BACK" );                        
        }
    } else if( list_type == PJLT_PUBLIC ) {
        // synchronous loading
        int projids[elementof(lines)-1]; // -1:BACK
        int n=elementof(projids);
        dbSearchPublishedProjects(projids,&n);
        project_num = n;
        if(n==0) {
            lines[0]->setString( WHITE, "NO PUBLIC PROJECT");
            lines[1]->setString( WHITE, "BACK" );
        } else {
            for(int i=0;i<n;i++) {
                setupProjectInfoTB( lines[i], "", projids[i] );
                project_id_cache[i] = projids[i];
            }
            lines[n]->setString( WHITE, "BACK" );            
        }
        
    }
}

void ProjectListWindow::update() {
    cursor->setLoc( lines[cursor_at]->loc + Vec2(-16,8));
}
void ProjectListWindow::poll() {
    bool to_hide_cursor = false;
    
    if( g_fld->isGenerateStarted() ) {
        // Started to generate
        bool finished = g_fld->asyncGenerate();
        if( finished ) { 
            bool save_finished;
            g_fsaver->poll( &save_finished );
            if(save_finished && g_fsaver->active ) {
                // Save project metadata
                ProjectInfo pinfo;
                memset( &pinfo, 0, sizeof(pinfo) );
                pinfo.project_id = g_fsaver->project_id;
                pinfo.owner_uid = g_user_id;
                snprintf( pinfo.owner_username, sizeof(pinfo.owner_username), "%s", g_user_name );
                snprintf( pinfo.owner_nickname, sizeof(pinfo.owner_nickname), "%s", g_pc->nickname );
                pinfo.created_at = dbGetTime(NULL);
                for(int i=0;i<MILESTONE_MAX;i++){
                    pinfo.flag_cands[i] = g_fsaver->tgtf->flag_cands[i];
                }
                dbSaveProjectInfo(&pinfo);
                dbSaveFieldEnvSync(pinfo.project_id);
                
                // Save to my project list
                int projids[32];
                int projids_len = elementof(projids);
                bool res = dbLoadIntArrayJSON( g_project_by_owner_ht_key, g_user_name, projids, &projids_len );
                print("===");
                for(int i=0;i<projids_len;i++) {
                    print("PIDS[%d]:%d", i, projids[i]);
                }
                if(!res) {
                    print("saving first project");
                    projids[0] = g_fsaver->project_id;
                    projids_len = 1;
                } else {
                    if( projids_len < MAX_PRIVATE_PROJECT ) {
                        projids[projids_len] = g_fsaver->project_id;
                        projids_len++;
                        print("added project. id:%d newlen:%d", g_fsaver->project_id, projids_len );
                    } else {
                        print("project FULL!");
                    }
                }
                dbEnsureImage( pinfo.project_id );
                dbUpdateImageAllRevealedSend( pinfo.project_id );
                dbSaveFieldRevealSync( pinfo.project_id );
                res = dbSaveIntArrayJSON( g_project_by_owner_ht_key, g_user_name, projids, projids_len );
                //                print("dbSaveIntArrayJSON: ret:%d len:%d",res, projids_len );

                g_fsaver->active = false; // To stop calling this update function                
                updateProjectList();
            } else if( g_fsaver->active ){
                lines[cursor_at]->setString(WHITE,"");
                char msg[100];
                snprintf(msg,sizeof(msg),"SAVING... (%d/%d)", g_fsaver->countUnsent(),  g_fsaver->countTotalNum() );
                lines[cursor_at]->setString( WHITE, msg );
                to_hide_cursor = true;
            }
        } else {
            if( lines[cursor_at]->isEqual( (char*) "CREATE", 6) ||
                lines[cursor_at]->isEqual( (char*) "GENERATING", 10) ) {
                char msg[100];
                snprintf(msg, sizeof(msg), "GENERATING FIELD.. STEP %d-%d", g_fld->generate_step, g_fld->generate_counter );
                lines[cursor_at]->setString( WHITE, msg );
            }
            to_hide_cursor = true;
        }
    }
    
    if( to_hide_cursor) {
        cursor->setVisible(false);
    } else {
        cursor->setVisible(true);
    }
}
int findFinalLine( CharGridTextBox **lines, int n ) {
    for(int i=0;i<n;i++) {
        if( lines[i]->isEmpty() ) {
            return i-1;
        }
    }
    return -1; // list is empty
}


void ProjectListWindow::moveCursor( DIR dir ) {
    if( g_fsaver->active ) return;
    
    int orig_cursor_at = cursor_at;
    if( dir == DIR_DOWN ) {
        cursor_at ++;
        if( cursor_at >= elementof(lines) ) {
            cursor_at = elementof(lines)-1;
        }
        if( lines[cursor_at]->isEmpty() ) cursor_at = 0;
    } else if( dir == DIR_UP ) {
        cursor_at --;
        if( cursor_at < 0 ) {
            cursor_at = findFinalLine(lines,elementof(lines));
            if(cursor_at<0) cursor_at = 0;
        }
    }
    if( cursor_at != orig_cursor_at ) {
        g_cursormove_sound->play();
    }
    cursor->ensureVisible();
    
    update();
}
void ProjectListWindow::cancel() {
    if( g_fsaver->active ) return;
    hide();
    g_projtypewin->show();
}
void ProjectListWindow::selectAtCursor() {
    if( lines[cursor_at]->isEqual( (char*)"CREATE", 6 ) ) {
        g_craft_sound->play();
        
        g_fld->startGenerate();
        
        g_mapview->notifyChangedAll();

        int new_proj_id = dbGetNewProjectId();

        bool res = dbAppendProjectStatus( new_proj_id, Format("CREATED BY %s", g_pc->nickname).buf );
        assert(res);
        
        //        Cell cc;
        //        print("creating new project! new id:%d cell_size:%d bool:%d enum:%d",
        //              new_proj_id, sizeof(Cell), sizeof(cc.untouched), sizeof(cc.bt) );
        g_fsaver->start( g_user_name, new_proj_id );
        
        print("started saving field.." )  ;
    } else if( lines[cursor_at]->isEqual( (char*) "SAVING", 6) ) {
        // no-op
    } else if( lines[cursor_at]->isEqual( (char*) "GENERATING", 10) ) {
        // no-op
    } else if( lines[cursor_at]->isEqual( (char*) "NO PUB", 6) ) {
        // no-op
    } else if( lines[cursor_at]->isEqual( (char*) "NO SHA", 6) ) {
        // no-op
    } else if( lines[cursor_at]->isEqual( (char*) "BACK", 4) ) {
        hide();
        g_projtypewin->show();
    } else if( lines[cursor_at]->isEmpty() == false ) {
        // Selected existing project. Common in every list_type
        g_craft_sound->play();
        assert( cursor_at < project_num );
        print("selected project: id:%d", project_id_cache[cursor_at] );
        hide();
        g_projinfowin->setProjectId( project_id_cache[cursor_at] );
        g_projinfowin->previous_window = this;
        g_projinfowin->show();
    }
}
void ProjectTypeWindow::cancel() {
    hide();
    g_runstate = RS_MAIN_MENU;
    g_titlewin->show();    
}

//////////////////

ProjectInfoWindow::ProjectInfoWindow() : Window( PREPARATION_GRID_WIDTH, PREPARATION_GRID_HEIGHT, B_ATLAS_WINDOW_FRAME_BASE, WINDOW_ALPHA ), project_id(0), previous_window(NULL), last_poll_maxcon_at(0) {
    g_hud_layer->insertProp(this);
    setLoc(PREPARATION_WINDOW_LOC);

    title_tb = new CharGridTextBox(30);
    title_tb->setString(WHITE, "PROJECT INFO:");
    title_tb->setLoc( getTitleLoc());
    g_hud_layer->insertProp(title_tb);
    
    go_tb = new CharGridTextBox(20);
    go_tb->setString( WHITE, "GO" );
    go_tb->setLoc( loc + Vec2( 600,80) );
    g_hud_layer->insertProp(go_tb);

    back_tb = new CharGridTextBox(4);
    back_tb->setString( WHITE, "BACK" );
    back_tb->setLoc( loc + Vec2( 600, 80+32 ) );
    g_hud_layer->insertProp(back_tb);

    share_tb = new CharGridTextBox(40);
    share_tb->setString( WHITE, "SHARE" );
    share_tb->setLoc( loc + Vec2(600, 80+64 ) );
    g_hud_layer->insertProp(share_tb);

    publish_tb = new CharGridTextBox(40);
    publish_tb->setString( WHITE, "PUBLISH" );
    publish_tb->setLoc( loc + Vec2(600, 80+96 ) );
    g_hud_layer->insertProp(publish_tb);        
    
    delete_tb = new CharGridTextBox(20);
    delete_tb->setString( WHITE, "DELETE" );
    delete_tb->setLoc( loc + Vec2(600, 80+128 ) );
    g_hud_layer->insertProp(delete_tb);

    
    cursor = new BlinkCursor( DIR_RIGHT );
    g_hud_layer->insertProp(cursor);

    cursor_at_id = go_tb->id;
    group = new PropGroup(5);
    group->reg( go_tb );
    group->reg( back_tb );    
    group->reg( delete_tb );
    group->reg( share_tb );    
    group->reg( publish_tb );    
    

    map_img = new Image();
    map_img->setSize(2048,2048);
    map_tex = new Texture();
    map_tex->setImage(map_img);
    map_prop = new Prop2D();
    map_prop->setTexture(map_tex);
    map_prop->setScl(512,512);
    map_prop->setLoc( loc + Vec2(256+32,256+100) );
    g_hud_layer->insertProp(map_prop);

    info_ta = new CharGridTextArea( 48, 20 );
    info_ta->setScl(16);
    info_ta->setLoc( loc + Vec2( 580, 320 ) );
    g_hud_layer->insertProp(info_ta);

    for(int i=0;i<elementof(indicators);i++) indicators[i] = NULL;

}
void ProjectInfoWindow::toggle(bool vis) {
    setVisible(vis);
    title_tb->setVisible(vis);
    go_tb->setVisible(vis);
    back_tb->setVisible(vis);
    delete_tb->setVisible(vis);
    share_tb->setVisible(vis);
    publish_tb->setVisible(vis);
    cursor->setVisible(vis);
    map_prop->setVisible(vis);
    info_ta->setVisible(vis);

    if(vis) {
        realtimeJoinChannelSend( project_id );
        dbEnsureImage(project_id);
        cursor_at_id = go_tb->id;
        update();
    } else {
        clearFlagIndicators();
    }
}

void ProjectInfoWindow::moveCursor( DIR dir ) {
    Prop2D *from_p = group->find( cursor_at_id );
    Prop2D *to_p = group->getNext( from_p, dir );
    cursor_at_id = to_p->id;
    g_cursormove_sound->play();
    updateCursor();
}

void ProjectInfoWindow::showPreviousWindow() {
    if( previous_window == NULL ) {
        g_projlistwin->show();
    } else {
        previous_window->show();
    }
}
void delete_project_confirm_callback( bool positive ) {
    if(positive) {
        dbDeleteProject( g_projinfowin->project_id );
        g_projinfowin->hide();
        g_projinfowin->showPreviousWindow();    
    }
}

void hudShowConfirmProjectInfoMessage( const char *msg, void (*cb)(bool positive) ) {
    g_msgwin->clear();
    g_msgwin->writeLine( WHITE, msg );
    g_msgwin->setCloseCallback( cb );
    g_msgwin->setWithCancel(true);
    g_msgwin->show();
    g_msgwin->toggleDelay(false);
}
void share_project_confirm_callback( bool positive ) {
    if(positive) {
        dbShareProjectSend( g_projinfowin->project_id );
        g_projinfowin->update();
    }
}
void publish_project_confirm_callback(bool positive) {
    if(positive) {
        dbPublishProjectSend(g_projinfowin->project_id);
        g_projinfowin->update();        
    }
}
void stop_share_project_confirm_callback( bool positive ) {
    if(positive) {
        dbUnshareProjectSend( g_projinfowin->project_id );
        g_projinfowin->update();        
    }
}
void stop_publish_project_confirm_callback(bool positive) {
    if(positive) {
        dbUnpublishProjectSend(g_projinfowin->project_id);
        g_projinfowin->update();        
    }
}

void ProjectInfoWindow::selectAtCursor() {

    ProjectInfo pinfo;
    dbLoadProjectInfo( project_id, &pinfo );
        
    bool result_ok = true;
    
    Prop2D *curp = group->find( cursor_at_id );
    if(curp == back_tb ) {
        hide();
        showPreviousWindow();
    } else if( curp == delete_tb ) {
        if( pinfo.owner_uid == g_user_id ) {
            hudShowConfirmProjectInfoMessage( "ARE YOU SURE TO DELETE?", delete_project_confirm_callback );
        } else {
            result_ok = false;
        }
    } else if( curp == go_tb ) {
        if( go_tb->isEqual("GO",2)) {
            hide();
            print("SELECTED! pjid:%d runstate:%d",project_id, g_runstate );
            if( g_runstate == RS_SELECTING_PROJECT ) g_runstate = RS_SELECTED_PROJECT;
            g_current_project_id = project_id;
        } else {
            result_ok = false;
        }
    } else if( curp == share_tb ) {
        if( pinfo.owner_uid == g_user_id ) {
            if( share_tb->isEqual( "SHARE",5 ) ) {
                hudShowConfirmProjectInfoMessage( "ARE YOU SURE TO SHARE?", share_project_confirm_callback );
            } else if( share_tb->isEqual( "STOP", 4) ) {
                hudShowConfirmProjectInfoMessage( "ARE YOU SURE TO STOP SHARING?", stop_share_project_confirm_callback );
            }
        } else {
            result_ok = false;
        }
        
    } else if( curp == publish_tb ) {
        if( pinfo.owner_uid == g_user_id ) {
            if( publish_tb->isEqual( "MAKE",4) ) {
                hudShowConfirmProjectInfoMessage( "ARE YOU SURE TO PUBLISH?", publish_project_confirm_callback );
            } else if( publish_tb->isEqual( "STOP",4) ) {
                hudShowConfirmProjectInfoMessage( "ARE YOU SURE TO STOP PUBLISHING?", stop_publish_project_confirm_callback );
            }
        } else {
            result_ok = false;            
        }
    }
    if( result_ok ) g_craft_sound->play(); else g_cant_sound->play();
}
void ProjectInfoWindow::updateCursor() {
    Prop2D *curp = group->find( cursor_at_id );
    cursor->setLoc( curp->loc + Vec2(-16,8) );    
}
void ProjectInfoWindow::cancel() {
    hide();
    showPreviousWindow();
}
void ProjectInfoWindow::update() {
    updateCursor();

    ProjectInfo pinfo;
    if( dbLoadProjectInfo( project_id, &pinfo) == false ) return;
    
    // Button Textboxes
    delete_tb->setString( WHITE, "DELETE" );
    if( dbCheckProjectIsShared( project_id, g_user_id) == false  ) {
        share_tb->setString( WHITE, Format( "SHARE WITH %d FRIENDS", countFriends() ).buf );
    } else {
        share_tb->setString( WHITE, Format( "STOP SHARING" ).buf );
    }
    if( dbCheckProjectIsPublished( project_id ) == false ) {
        publish_tb->setString( WHITE, Format( "MAKE PUBLIC" ).buf );
    } else {
        publish_tb->setString( WHITE, Format( "STOP PUBLISHING" ).buf );
    }
    
    if( pinfo.owner_uid != g_user_id ) {
        delete_tb->setColor(RED);
        share_tb->setColor(RED);
        publish_tb->setColor(RED);                
    } 
    
    char pubstate[32];

    if( dbCheckProjectIsPublished( project_id ) ) {
        strcpy( pubstate, "PUBLIC ");
    } else if( dbCheckProjectIsShared( project_id, g_user_id ) ) {
        strcpy( pubstate, "SHARED " );
    } else {
        strcpy( pubstate, "PRIVATE " );
    }
    
    title_tb->setString(WHITE, Format( "%sPROJECT ID:%d", pubstate, project_id ).buf );

    if( project_id <= 0 ) return;

    print("ProjectInfoWindow: show: pjid:%d",project_id );
    map_img->fill(Color(0.5,0,0,1));
    if( g_skip_projectinfowin_worldmap == false ) {
        dbLoadRawImageSync( project_id, map_img );
    }
    map_tex->setImage(map_img);
    
    info_ta->clear();
            
    info_ta->writeLine( WHITE, Format( "OWNER: %s", pinfo.owner_nickname ).buf );
    info_ta->writeLine( WHITE, "" );
            
    info_ta->writeLine( WHITE, Format( "MILESTONE: %d", countMilestoneCleared(&pinfo) ).buf );
    info_ta->writeLine( WHITE, "" );
            
    info_ta->writeLine( WHITE, Format( "ONLINE PLAYERS: %d", dbCountOnlinePlayerSync( project_id ) ).buf );
    info_ta->writeLine( WHITE, "" );
    
            
    info_ta->writeLine( WHITE, "STATUS:" );
    ProjectEvent events[30];
    int evnum = elementof(events);
    dbLoadProjectStatus( project_id, events, &evnum );
    print("STATUS NUM: %d", evnum );
    unsigned int nowt = dbGetTime();
    for(int i=0;i<MIN(evnum,10);i++) {
        char humantime[10];
        makeShortHumanDiffTimeSec( humantime, sizeof(humantime), nowt - events[i].at );
        Format msg( " %-4s %s", humantime, events[i].caption  );
        msg.trimWith(25);
        info_ta->writeLine( WHITE, msg.buf );
    }

    // Flag
    clearFlagIndicators();
    Pos2 rp = g_fld->getRespawnPoint();
    g_fld->applyFlagCands( &pinfo);
    FlagCandidate *fc = g_fld->findNearestFlagCand(rp);
    if(fc && fc->finished==false) {
        insertFlagIndicator( fc->pos );
    }

    // Check if it's possible to join?
    updateGoButton();
}
void ProjectInfoWindow::clearFlagIndicators() {
    for(int i=0;i<elementof(indicators);i++) {
        if( indicators[i] ) {
            indicators[i]->to_clean = true;
            indicators[i] = NULL;
        }
    }
}

void ProjectInfoWindow::insertFlagIndicator( Pos2 p ) {
    for(int i=0;i<elementof(indicators);i++) {
        if( indicators[i] && indicators[i]->type == WMIND_FLAG && indicators[i]->pos_hint == p ) {
            return;
        }
    }
    int u = FIELD_W / 512;
    for(int i=0;i<elementof(indicators);i++) {
        if( indicators[i] == NULL ) {
            indicators[i] = new WorldMapIndicator( WMIND_FLAG );
            indicators[i]->setLoc( map_prop->loc - Vec2(256,256) + Vec2(p.x/u,p.y/u) );
            print("inserting flag indicator %d : %d,%d prio:%d %d v:%f %f vis:%d",
                  i, p.x, p.y, indicators[i]->priority, map_prop->priority, indicators[i]->loc.x, indicators[i]->loc.y,
                  indicators[i]->visible );
            g_hud_layer->insertProp( indicators[i] );
            return;
        }
    }
    assertmsg(false, "indicators full");
}

void ProjectInfoWindow::updateParty( int userid, const char *nickname, Vec2 wloc, int hair_ind, int face_ind ) {
    assert( FIELD_W == FIELD_H );
    float pixel_per_cell = map_prop->scl.x / (float)FIELD_W ;
    
    Vec2 rel_at = wloc / PPC * pixel_per_cell;
    Vec2 at = rel_at + map_prop->loc - map_prop->scl/2;
    for(int i=0;i<elementof(indicators);i++) {
        if( indicators[i] && indicators[i]->type == WMIND_PC && indicators[i]->client_id == userid ) {
            indicators[i]->last_packet_at = indicators[i]->accum_time;
            indicators[i]->loc = at;
            return;
        }
    }

    // No PC icon of the userid, create!
    for(int i=0;i<elementof(indicators);i++) {
        if( indicators[i] == NULL ) {
            indicators[i] = new WorldMapIndicator( WMIND_PC );
            indicators[i]->face->update( face_ind, hair_ind );
            indicators[i]->face->setScl(16);
            indicators[i]->client_id = userid;
            indicators[i]->loc = at;
            strncpy( indicators[i]->nickname, nickname, sizeof(indicators[i]->nickname));
            indicators[i]->name_tb->setString(WHITE, nickname);
            indicators[i]->name_tb->setScl(8);
            g_hud_layer->insertProp(indicators[i]);
            break;
        }
    }
}
void ProjectInfoWindow::updateGoButton() {
    // Check access right
    ProjectInfo pinfo;
    if( dbLoadProjectInfo( project_id, &pinfo) == false ) return;
    
    bool accessable = false;
    if( g_user_id == pinfo.owner_uid ) {
        print("pj %d is my own project", project_id  );
        accessable = true;
    } else if( dbCheckProjectIsJoinable( project_id, g_user_id ) ) {
        print("dbCheckProjectIsJoinable: return ok");
        accessable = true;
    }

    // How many players are online 
    bool is_full = false;
    int maxnum,curnum;
    curnum = realtimeGetChannelMemberCountSync( project_id, &maxnum );
    if( curnum >= maxnum ) { accessable = false; is_full = true; }
    
    if( accessable ) {
        go_tb->setString( WHITE, "GO" );        
    } else {
        if( is_full ) {
            go_tb->setString( RED, "FULL" );
        } else {
            go_tb->setString( RED, "NOT SHARED" );
        }
        
    }

}
void ProjectInfoWindow::poll() {
    for(int i=0;i<elementof(indicators);i++) {
        if( indicators[i] == NULL || indicators[i]->type != WMIND_PC ) continue;
        if( indicators[i]->last_packet_at < indicators[i]->accum_time - 10 ) {
            indicators[i]->to_clean = true;
            indicators[i] = NULL;
        } 
    }
    // Check number of online players every second
    if( last_poll_maxcon_at < accum_time - 1 ) {
        last_poll_maxcon_at = accum_time;
        updateGoButton();
    }
}

//////////////////

CharGridTextInputBox::CharGridTextInputBox( int w ) : CharGridTextBox( w ), show_cursor(true) {
    s[0] = '\0';
    cursor_char = '_';
    fillBG( B_ATLAS_BLACK_FILL );
}
void CharGridTextInputBox::appendChar( char ch ) {
    int l = strlen(s);
    if( l < cg->width ) {
        s[l] = ch;
        s[l+1] = '\0';
    }
    setString(WHITE,s);
}
void CharGridTextInputBox::deleteTailChar() {
    if( strlen(s) > 0 ) {
        s[ strlen(s)-1 ] = '\0';
    }
    setString(WHITE,s);
}
bool CharGridTextInputBox::prop2DPoll(double dt) {
    int l = strlen(s);
    int cursor_at = l;
    if( l < cg->width ) {
        bool to_show = visible && ((int)(accum_time * 4)%2) && show_cursor;
        if( to_show ) {
            cg->set(cursor_at,0, cursor_char );
        } else {
            cg->set(cursor_at,0, Grid::GRID_NOT_USED);
        }
    }
    
    return true;
}

//////////////////
CharGridTextArea::CharGridTextArea( int w, int h ) : to_write(0) {
    bg = new Grid(w,h);
    bg->setDeck(g_base_deck);
    addGrid(bg);    
    cg = new CharGrid(w,h);
    cg->setDeck(g_ascii_deck);
    addGrid(cg);        
    setScl(16);    
}
void CharGridTextArea::clear() {
    to_write = 0;
    cg->clear();
}
void CharGridTextArea::scrollUp() {
    print("SCROLLUP");
    if( cg->height <= 1 ) return;
    for(int y=cg->height-1;y>=1;y--) {
        for(int x=0;x<cg->width;x++) {
            int ind = cg->get(x,y-1);
            cg->set(x,y,ind);
        }
    }
}
void CharGridTextArea::writeLine( Color c, const char *s ) {
    if( to_write == cg->height ) {
        scrollUp();
    }
    int wy = cg->height - 1 - to_write;
    cg->printf(0, wy, c, "%s", s );
    to_write ++;
    if( to_write == cg->height ) {        
        to_write = cg->height - 1;
    }
}

void CharGridTextArea::fillBG( int ind ) {
    for(int y=0;y<bg->height;y++) {
        for(int x=0;x<bg->width;x++){
            bg->set(x,y,ind);
        }
    }
}

//////////////////

SoftwareKeyboardWindow::SoftwareKeyboardWindow() : Window( PREPARATION_GRID_WIDTH, PREPARATION_GRID_HEIGHT, B_ATLAS_WINDOW_FRAME_BASE, WINDOW_ALPHA) {
    group = new PropGroup( elementof(keys) );
    cursor_at_id = -1;
    input_min_len = 0;
}
const char SoftwareKeyboardWindow::chars[26+26+10+1] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

void SoftwareKeyboardWindow::setupSoftwareKeyboard( int inputminlen, int inputmaxlen ) {
    input_min_len = inputmaxlen;

    for(int i=0;i<elementof(keys);i++) keys[i] = NULL;
    Vec2 curloc(80,350);
    assert( keys[0] == NULL );
    for(int i=0;i<strlen(chars);i++ ){
        keys[i] = new Prop2D();
        keys[i]->setDeck(g_ascii_deck);
        keys[i]->setIndex( chars[i] );
        keys[i]->setLoc( loc + curloc );
        if( (i % 26 == 25) || (i%26 == 51) ) {
            curloc.x = 80;
            curloc.y -= (16+24);
        } else {
            curloc.x += (16+8);
        }
        keys[i]->setScl(16);
        g_hud_layer->insertProp(keys[i]);
        group->reg( keys[i] );
    }

    cursor_at_id = keys[0]->id;
    
    
    //    
    input_tb = new CharGridTextInputBox( inputmaxlen );
    g_hud_layer->insertProp(input_tb);
    //    input_tb->appendChar('A');
    input_tb->setLoc( loc + Vec2(100,450) );

    ok_tb = new CharGridTextBox(4);
    ok_tb->setScl(16);
    Color col = WHITE;
    if( inputminlen > 0 ) col = RED;
    ok_tb->setString( col, "DONE" );
    ok_tb->setLoc( loc + Vec2(620,200) );
    g_hud_layer->insertProp(ok_tb);
    group->reg(ok_tb);

    del_tb = new CharGridTextBox(6);
    del_tb->setScl(16);
    del_tb->setString( WHITE, "DEL" );
    del_tb->setLoc( keys[26+26+10-1]->loc + Vec2(32,-8) );
    g_hud_layer->insertProp(del_tb);
    group->reg(del_tb);
    
    //
    cursor = new BlinkCursor();
    g_hud_layer->insertProp(cursor);

    // 
    update();
    
}

void SoftwareKeyboardWindow::update() {
    Prop2D *atprop = group->find( cursor_at_id );
    assert(atprop);
    cursor->setLoc( atprop->loc + Vec2(0,-16) );
}

////////////
CharNameWindow::CharNameWindow() : SoftwareKeyboardWindow() {
    setLoc(PREPARATION_WINDOW_LOC);
    g_hud_layer->insertProp(this);
    //
    title_tb = new CharGridTextBox(30);
    title_tb->setString( WHITE, "INPUT CHRACTER NAME:" );
    title_tb->setLoc(getTitleLoc());
    g_hud_layer->insertProp(title_tb);

    setupSoftwareKeyboard(PC_NICKNAME_MINLEN,PC_NICKNAME_MAXLEN);
}

void CharNameWindow::toggle(bool vis) {
    setVisible(vis);
    title_tb->setVisible(vis);
    for(int i=0;i<elementof(keys);i++) if(keys[i]) keys[i]->setVisible(vis);
    input_tb->setVisible(vis);
    cursor->setVisible(vis);
    ok_tb->setVisible(vis);
    del_tb->setVisible(vis);
}
void CharNameWindow::moveCursor( DIR d ) {
    g_cursormove_sound->play();
    Prop2D *nextp = group->getNext( cursor_at_id, d );
    cursor_at_id = nextp->id;
    //    print("chnmw:moveCursor:%d nextp:%d",d,cursor_at_id);
    update();
}
void CharNameWindow::selectAtCursor() {
    assert( cursor_at_id >= 0 );
    int namelen = strlen(input_tb->get());
    print("namelen:%d",namelen);
    
    Prop2D *curp = group->find(cursor_at_id);
    if(curp == ok_tb ) {
        if( namelen >= PC_NICKNAME_MINLEN ) {
            hide();
            g_pc->setNickname( input_tb->get() );
            g_pc->addInitialItems();

            if( isDBNetworkActive() ) {
                dbSavePC();
            }
            if( g_runstate == RS_MAKING_CHARACTER ) {
                print("set runstate char-made!");
                g_runstate = RS_MADE_CHARACTER;
            }
        } else {
            g_cant_sound->play();
        }
        return;
    }  else if( curp == del_tb ) {
        g_craft_sound->play();
        input_tb->deleteTailChar();
    }
    
    for(int i=0;i<elementof(keys);i++){
        if(keys[i] == curp ) {
            print("K:%d ch:%c",i, chars[i]);
            input_tb->appendChar(chars[i]);
            g_craft_sound->play();
            break;
        }
    }
    int slen = strlen(input_tb->get());
    bool len_ok = (slen >= PC_NICKNAME_MINLEN);
    if( len_ok ) {
        ok_tb->setColor(WHITE);
    } else {
        ok_tb->setColor(RED);
    }
    if( slen == PC_NICKNAME_MAXLEN ) {
        input_tb->show_cursor = false;
    } else {
        input_tb->show_cursor = true;
    }
}
///////////////////////////

Log::Log( Vec2 lc, int max_ln ) : tbs(NULL), line_len(0), line_num(0), loc(0,0) {
    loc = lc;
    line_num = max_ln;

    tbs = (LogText**) MALLOC( sizeof(LogText*) * max_ln );
    assert(tbs);
    for(int i=0;i<max_ln;i++) {
        tbs[i] = new LogText();
        tbs[i]->setLoc( lc.x, lc.y + 20 * i );
        tbs[i]->setScl(16);
        tbs[i]->setString( WHITE, "" );
        g_hud_layer->insertProp(tbs[i]);
    }
    last_message[0] = '\0';
}


void Log::printf( const char *fmt, ... ) {
    char out[512];
    va_list argptr;
    va_start( argptr, fmt );
    vsprintf( out, fmt, argptr );
    va_end( argptr );
    this->print(Color(1,1,1,1),out);
}

void Log::printf( Color c, const char *fmt, ... ) {
    //
    char out[512];
    va_list argptr;
    va_start( argptr, fmt );
    vsprintf( out, fmt, argptr );
    va_end( argptr );
    this->print( c, out );
}
void Log::print( Color c, char *s ) { 
    ::print("%s",s);
    if( strcmp( last_message, s) == 0 ) {
        // Only spike when the same text is repeated
        tbs[0]->setScl(16,16);
        tbs[0]->setColor(c);
        tbs[0]->seekScl(16,16,0.2);
        tbs[0]->last_updated_at = tbs[0]->accum_time;
        return;
    }
    strncpy( last_message, s, sizeof(last_message) );
    
    // Newer log goes bottom. rotate position
    LogText *tmp = tbs[line_num-1];
    for(int i=line_num-1;i>=1; i-- ){
        tbs[i] = tbs[i-1];
        tbs[i]->loc.y = loc.y + 32 * i;
    }
    tbs[0] = tmp;
    tbs[0]->loc.y = loc.y;
    tbs[0]->setString(WHITE,s);
    tbs[0]->setColor(c);
    tbs[0]->setScl(32,32);
    tbs[0]->seekScl(16,16,0.2);
    tbs[0]->last_updated_at = tbs[0]->accum_time;
}
void Log::poll() {
    for(int i=0;i<line_num;i++){
        if( tbs[i]->last_updated_at < tbs[i]->accum_time - 15 ){
            Color c = tbs[i]->getColor();
            if(c.a > 0.1) {
                tbs[i]->setColor( Color(1,1,1,0) );
            }
        } 
    }
}

void hudMilestoneMessage( const char *nick, int cnt, bool write_db ) {
    Format f( "%s GOT A FLAG! MILESTONE IS NOW %d/%d", nick, cnt, MILESTONE_MAX );
    g_log->printf( WHITE, f.buf );
    if(write_db) {
        Format f( "FLAG[%d] BY %s", cnt, nick );
        dbAppendProjectStatus( g_current_project_id, f.buf );
    }
}
void hudFortressDestroyMessage( const char *nick, int size, bool write_db ) {
    Format f("%s DESTROYED A FORTRESS. SIZE:%d", nick, size );
    g_log->printf( WHITE, f.buf);
    if(write_db) {
        Format f( "FORTRESS BY %s", nick );
        dbAppendProjectStatus( g_current_project_id, f.buf );
    }
}

void hudCoreDestroyMessage( const char *nick, bool write_db ) {
    Format f("%s DESTROYED A CORE", nick );
    g_log->printf( WHITE, f.buf);
    if(write_db) {
        Format f( "CORE BY %s", nick );
        dbAppendProjectStatus( g_current_project_id, f.buf );
    }
}
void hudKilledMessage( const char *nick, bool write_db ) {
    Format f("%s KILLED BY ENEMY", nick );
    g_log->printf( WHITE, f.buf );
    if(write_db) {
        Format f( "KILLED: %s", nick );
        dbAppendProjectStatus( g_current_project_id, f.buf );
    }
}


///////////////////
PartyIndicator::PartyIndicator() : Prop2D(), client_id(0) {
    face = new FacePanel();
    g_hints_layer->insertProp(face);

    distance_tb = new CharGridTextBox(8);
    distance_tb->setString( WHITE, "not set");
    addChild( distance_tb );
    
    g_hints_layer->insertProp(this);
}

bool isInScreen( Vec2 tov ) {
    float scr_w = SCRW/2 / g_zoom_rate, scr_h = SCRH/2 / g_zoom_rate;
    return ( tov.x < scr_w && tov.x > - scr_w && tov.y < scr_h && tov.y > - scr_h  );
}

Vec2 hudCalcScreenEdge( Vec2 tov, float leftmargin, float rightmargin, float topmargin, float bottommargin ) {
    if( tov.x == 0 ) tov.x = 0.1;

    float d = tov.y / tov.x;
    float maxy = SCRH/2 - topmargin;
    float miny = -SCRH/2 + bottommargin;
    float maxx = SCRW/2 - rightmargin;
    float minx = -SCRW/2 + leftmargin;
    float dx,dy;


    if( tov.x > 0 ) {
        // right
        dx = maxx;
        dy = dx * d;
    } else {
        dx = minx;
        dy = dx * d;
    }
            
    if( dy > maxy ) {
        dy = maxy;
        dx = maxy/d;
    } else if( dy < miny ) {
        dy = miny;
        dx = miny/d;
    }
            
    return Vec2(dx,dy);
}

bool PartyIndicator::prop2DPoll( double dt ) {

        
    // angle d is (y/x)
    if( visible ) {
        Vec2 tov = world_loc - g_pc->loc;
        if( isInScreen(tov) ) {
            setVisible(false);
            client_id = 0;
        }
        if( last_packet_at < accum_time - 10 ) {
            setVisible(false);
            client_id = 0;
        }

        loc = hudCalcScreenEdge(tov);

        float l = g_pc->loc.len(world_loc);
        Format f( "%dM", (int)(l/PPC) );
        distance_tb->setString( WHITE, f.buf );
    }
    face->setVisible(visible);
    face->setLoc(loc);
    distance_tb->setLoc(loc + Vec2(16,-2));
    
    return true;
}
void PartyIndicator::setVisible( bool vis ) {
    Prop2D::setVisible(vis);
    face->setVisible(vis);
    distance_tb->setVisible(vis);
}

// Set up everything and set invisible.
PartyIndicator *g_parties[8];
void setupPartyIndicator() {
    for(int i=0;i<elementof(g_parties);i++) {
        g_parties[i] = new PartyIndicator();
        g_parties[i]->setLoc( range(0,200), range(0,200) );
        g_parties[i]->setVisible(false);
    }
}
PartyIndicator *findPartyIndicator( int client_id ) {
    for(int i=0;i<elementof(g_parties);i++) {
        if( g_parties[i]->client_id == client_id ) {
            return g_parties[i];
        }
    }
    return NULL;
}

void ensurePartyIndicator( int client_id, Vec2 wloc, int face_ind, int hair_ind ) {
    PartyIndicator *pi = findPartyIndicator(client_id);

    if(!pi) {
        pi = findPartyIndicator(0); // find indicator that is not in use
        if(!pi) {
            return; // full!  TODO: replace with nearer players
        }
    }
    pi->client_id = client_id;
    pi->last_packet_at = pi->accum_time;
    pi->world_loc = wloc;
    pi->face->update( face_ind, hair_ind );
    pi->setVisible(true);
}

/////////////////////////////

SpecialMenuWindow::SpecialMenuWindow() : Window( 20, 20, B_ATLAS_WINDOW_FRAME_BASE ), cursor_at(0) {
    setLoc( -20*24/2, -20*24/2 );
    g_hud_layer->insertProp(this);
    for(int i=0;i<elementof(options);i++) options[i] = NULL;

    cursor = new BlinkCursor(DIR_RIGHT);
    g_hud_layer->insertProp(cursor);

    setupMenu();
}
void SpecialMenuWindow::setupMenu() {
    Vec2 base = getTitleLoc() + Vec2(32,-32);
    
    int n = 3 + ( g_enable_debug_menu ? 1 : 0 );
    
    for(int i=0;i<n;i++ ) {
        options[i] = new CharGridTextBox(32);
        options[i]->setScl(16);
        options[i]->setLoc( base + Vec2(0,-32*i));
        g_hud_layer->insertProp(options[i]);
    }
    options[0]->setString( WHITE, "RESUME" );
    options[1]->setString( WHITE, "LEAVE THIS PROJECT" );
    options[2]->setString( WHITE, "SUICIDE");
    if( g_enable_debug_menu ) options[3]->setString( WHITE, "DEBUG MENU" );
    
    update();
}

void SpecialMenuWindow::moveCursor( DIR d ) {
    if( d == DIR_UP ) {
        cursor_at--;
        if( cursor_at < 0 ) cursor_at = 0;
    } else if( d == DIR_DOWN ) {
        cursor_at ++;
        if( cursor_at >= elementof(options) ) {
            cursor_at = elementof(options)-1;
        } else if( options[cursor_at] == NULL ) {
            cursor_at--;
        }
    }
    update();
}
void SpecialMenuWindow::update() {
    cursor->setLoc( options[cursor_at]->loc + Vec2(-16,6));
}

void leaveGameNow() {
    g_runstate = RS_LEAVING_PROJECT;        
    realtimeEventSend( EVT_LOGOUT, g_pc->nickname, 0, 0 );
    dbSavePC();
    g_pc->setLoc( pos2ToVec2( g_fld->getRespawnPoint()) );
    clearNormalCharacters();
    g_fld->checkSaveAllUnsaved(0.2);
    dbSaveFieldRevealSync( g_current_project_id );
    g_fld->clear();
    g_flag->reset();
    g_ps->clear();
    g_mapview->notifyChangedAll();
    realtimeCleanAllSend(); // Change runstate to RS_LEFT_PROJECT when receiving responce from backend of this message
    dbDeletePresenceSend();
}

void leave_confirm_callback( bool positive ) {
    if(positive) {
        leaveGameNow();
    }
}
void hudShowConfirmLeaveMessage() {
    g_msgwin->clear();
    g_msgwin->writeLine( WHITE, "ARE YOU SURE TO LEAVE?" );
    g_msgwin->setCloseCallback(leave_confirm_callback);
    g_msgwin->setWithCancel(true);
    g_msgwin->show();
    g_msgwin->toggleDelay(false);
}


void SpecialMenuWindow::selectAtCursor() {
    if( options[cursor_at]->isEqual( "RESUME", 6) ) {
        hide();
    } else if( options[cursor_at]->isEqual( "LEAVE", 5) ) {
        hide();
        if( g_current_project_id > 0 ) {
            hudShowConfirmLeaveMessage();
        }                        
    } else if( options[cursor_at]->isEqual( "DEBUG MENU", 10 ) ) {
        hide();
        g_debugwin->show();
    } else if( options[cursor_at]->isEqual( "SUICIDE", 7 ) ) {
        g_pc->suicide();
        hide();
    }
}
void SpecialMenuWindow::toggle(bool vis) {
    setVisible(vis);
    cursor->setVisible(vis);
    for(int i=0;i<elementof(options);i++) if(options[i]) options[i]->setVisible(vis);
}



////////////////////////
FollowWindow::FollowWindow() : Window( 20,20, B_ATLAS_WINDOW_FRAME_BASE ), cursor_at(0) {
    g_hud_layer->insertProp(this);
    setLoc( -10*24, -10*24 );
    
    title_tb = new CharGridTextBox(40);
    title_tb->setString( WHITE, "(not set)" );
    title_tb->setLoc( getTitleLoc() );
    g_hud_layer->insertProp(title_tb);

    face = new FacePanel();
    face->setScl(48);
    face->setLoc( getTitleLoc() + Vec2(32,-64) );
    g_hud_layer->insertProp(face);
    ok_tb = new CharGridTextBox(2);
    ok_tb->setString(WHITE, "OK" );
    ok_tb->setLoc( getTitleLoc() + Vec2(300,-200) );
    g_hud_layer->insertProp(ok_tb);
    later_tb = new CharGridTextBox(5);
    later_tb->setString(WHITE, "LATER");
    later_tb->setLoc( getTitleLoc() + Vec2(300,-240) );    
    g_hud_layer->insertProp(later_tb);

    cursor = new BlinkCursor( DIR_RIGHT );
    g_hud_layer->insertProp(cursor);

    update( 0, "not set", B_ATLAS_PC_FACE_FRONT, B_ATLAS_PC_HAIR_FRONT );
}
void FollowWindow::toggle( bool vis ) {
    setVisible(vis);
    face->setVisible(vis);
    title_tb->setVisible(vis);
    ok_tb->setVisible(vis);
    later_tb->setVisible(vis);
    cursor->setVisible(vis);
}
void FollowWindow::selectAtCursor() {
    if( cursor_at == 0 ) {
        print("follow:OK. uid:%d", user_id );
        followContact( user_id, nickname, face_index, hair_index );
        realtimeFollowSend( g_user_id, user_id, g_pc->nickname, nickname, face_index, hair_index );
        dbSaveFollowings();
        hide();
        if( isAFriend( user_id ) ) {
            g_log->printf(WHITE, "YOU FOLLOWED %s, MADE A FRIEND!", nickname );            
        } else {
            g_log->printf(WHITE, "YOU FOLLOWED %s", nickname );
        }
        
    } else if( cursor_at == 1 ){
        hide();
    }
}
void FollowWindow::moveCursor( DIR d ) {
    if( cursor_at == 0 ) cursor_at = 1; else cursor_at = 0;
    updateCursor();
}
void FollowWindow::update( int uid, const char *nick, int face_ind, int hair_ind ) {
    user_id = uid;
    strncpy( nickname, nick, sizeof(nickname));
    face_index = face_ind;
    hair_index = hair_ind;
    title_tb->setString(WHITE, Format( "FOLLOW %s ?", nick ).buf );
    face->update( face_ind, hair_ind );
    updateCursor();
}
void FollowWindow::updateCursor() {
    CharGridTextBox *tgt=NULL;
    if(cursor_at == 0 ) tgt = ok_tb; else tgt = later_tb;
    cursor->setLoc( tgt->loc + Vec2(-16,6) );
}
//////////////////

MessageWindow::MessageWindow() : Window( 22,15, B_ATLAS_WINDOW_FRAME_BASE ), show_at(0), close_callback(NULL), cursor_at(0), with_cancel(false) {
    g_msg_layer->insertProp(this);
    setLoc(-250,-100);
    
    msgarea = new CharGridTextArea( 50, 20 );
    msgarea->setScl(16);
    msgarea->setLoc(-220,-80);
    g_msg_layer->insertProp(msgarea);

    ok_tb = new CharGridTextBox(2);
    ok_tb->setString( RED, "OK" );
    ok_tb->setLoc(200,-60);
    g_msg_layer->insertProp(ok_tb);

    cancel_tb = new CharGridTextBox(6);
    cancel_tb->setString(WHITE, "CANCEL" );
    cancel_tb->setLoc(-150,-60);
    g_msg_layer->insertProp(cancel_tb);

    cursor = new BlinkCursor( DIR_UP );
    g_msg_layer->insertProp(cursor);
}

void MessageWindow::toggle( bool vis) {
    setVisible(vis);
    cursor->setVisible(vis);
    ok_tb->setVisible(vis);
    cancel_tb->setVisible(vis);
    msgarea->setVisible(vis);
    if(vis) {
        show_at = accum_time;
        ok_tb->setColor(RED);
        if( with_cancel ) {
            cursor_at = 1;
            cancel_tb->setVisible(vis);
        } else {
            cursor_at = 0;            
            cancel_tb->setVisible(false);
        }
        updateCursor();
    }
}
void MessageWindow::clear() {
    msgarea->clear();
}
void MessageWindow::writeLine( Color c, const char *s ) {
    msgarea->writeLine(c,s);
}
void MessageWindow::selectAtCursor() {
    if( cursor_at == 0 ) { // selected OK
        if( accum_time > show_at + DELAY_TIME ) {
            g_craft_sound->play();
            hide();
            if(close_callback) {
                close_callback(true);
                close_callback = NULL;
            }
        } else {
            g_cant_sound->play();
        }
    } else { // selected cancel
        g_craft_sound->play();
        hide();
        if(close_callback) {
            close_callback(false);
            close_callback = NULL;
        }
    }
}
void MessageWindow::poll() {
    if( accum_time > show_at + DELAY_TIME ) {
        ok_tb->setColor(WHITE);
    }
}
void MessageWindow::cancel() {
    if( close_callback ) close_callback(false);
    hide();    
}

// Make sure call this function after show()
void MessageWindow::toggleDelay( bool to_delay ) {
    if(!to_delay) {
        show_at = accum_time - DELAY_TIME;
    }
}
void MessageWindow::moveCursor( DIR d) {
    int maxind = 0;
    if(with_cancel) maxind = 1; else maxind = 0;
    
    if(d == DIR_RIGHT) {
        cursor_at ++;
        if( cursor_at > maxind ) cursor_at = 0;
    } else if(d==DIR_LEFT){
        cursor_at --;
        if( cursor_at < 0 ) cursor_at = maxind;
    }
    updateCursor();
}
// OK goes right, CANCEL goes left
void MessageWindow::updateCursor() {
    if(cursor_at == 0 ) { 
        cursor->setLoc( ok_tb->loc + Vec2(8,-16));
    } else { 
        cursor->setLoc( cancel_tb->loc + Vec2(8,-16));
    }
}


/////////
void hudCongratulateProjectIsOver() {
    g_all_clear_sound->play();
    
    g_msgwin->clear();
    g_msgwin->setWithCancel(false);
    g_msgwin->writeLine( WHITE, "      CONGRATULATIONS!   " );
    g_msgwin->writeLine( WHITE, "" );
    g_msgwin->writeLine( WHITE, "" );    
    g_msgwin->writeLine( WHITE, "    ALL MILESTONE IS DONE. " );
    g_msgwin->writeLine( WHITE, "" );
    g_msgwin->writeLine( WHITE, "" );    
    g_msgwin->writeLine( WHITE, " TOTAL ACCUMULATED PLAY TIME:" );
    g_msgwin->writeLine( WHITE, "" );    
    int playsec = dbLoadPlaytime(g_current_project_id);
    char s[12];
    makeHourMinSecString( playsec, s, sizeof(s));
    Format fmt( "        %s", s );
    g_msgwin->writeLine( WHITE, fmt.buf );
    g_msgwin->show();

    dbSaveAllClearLog( g_current_project_id, playsec );
}

void hudShowErrorMessage( const char *caption, const char *msg, void (*cbfunc)(bool positive) ) {
    g_msgwin->clear();
    g_msgwin->writeLine( WHITE, caption );
    g_msgwin->writeLine( WHITE, "");
    g_msgwin->writeLine( WHITE, msg );
    g_msgwin->setCloseCallback(cbfunc);
    g_msgwin->setWithCancel(false);
    g_msgwin->show();
    g_msgwin->toggleDelay(false);
}


/////////////
MenuWindow::MenuWindow( const char *titlestr, float alpha ) : Window( PREPARATION_GRID_WIDTH, PREPARATION_GRID_HEIGHT, B_ATLAS_WINDOW_FRAME_BASE, alpha ), cursor_at(0) {
    g_hud_layer->insertProp(this);
    setLoc(PREPARATION_WINDOW_LOC);

    title_tb = new CharGridTextBox(30);
    title_tb->setString( WHITE, titlestr );
    title_tb->setLoc( getTitleLoc() );
    g_hud_layer->insertProp(title_tb);

    for(int i=0;i<elementof(options);i++) options[i] = NULL;

    cursor = new BlinkCursor( DIR_RIGHT );
    g_hud_layer->insertProp(cursor);
}
void MenuWindow::setMenuEntry( int ind, Color col, const char *s ) {
    if( options[ind] ) {
        options[ind]->to_clean = true;
    }
    options[ind] = new CharGridTextBox( strlen(s) );
    options[ind]->setString(col,s);
    options[ind]->setLoc( title_tb->loc + Vec2(40,-80 + ind * -50 ) );
    g_hud_layer->insertProp(options[ind]);
    updateCursor();
}


void MenuWindow::toggle( bool vis ) {
    setVisible(vis);
    for(int i=0;i<elementof(options);i++) if(options[i]) options[i]->setVisible(vis);
    title_tb->setVisible(vis);
    cursor->setVisible(vis);
}
void MenuWindow::selectAtCursor() {
    g_craft_sound->play();
    onEntrySelected(cursor_at);
}
void MenuWindow::moveCursor( DIR d ) {
    switch(d){
    case DIR_UP:
        cursor_at --;
        if(cursor_at<0) {
            bool found=false;
            for(int i=0;i<elementof(options);i++){
                if( options[i] == NULL ) {
                    cursor_at = i-1;
                    found = true;
                    break;
                }
            }
            if(!found) cursor_at = elementof(options)-1;
        }
        g_cursormove_sound->play();        
        break;
    case DIR_DOWN:
        cursor_at ++;
        if(cursor_at>=elementof(options) || options[cursor_at] == NULL ) cursor_at=0;
        g_cursormove_sound->play();
        break;
    default:
        break;
    }
    updateCursor();
}
void MenuWindow::setOptionsLoc( Vec2 lefttop ) {
    for(int i=0;i<elementof(options);i++) {
        if( options[i] ) {
            options[i]->setLoc( lefttop + Vec2(0, -50 * i ) );
        }
    }
}
void MenuWindow::updateCursor() {
    assert( options[cursor_at] );
    cursor->loc = options[cursor_at]->loc + Vec2(-50,8);
}

/////////////////////

TitleWindow::TitleWindow() : MenuWindow( "", 0 ) { // no title caption
    setMenuEntry( 0, WHITE, "PROJECTS" );
    setMenuEntry( 1, WHITE, "RANKING" );
    setMenuEntry( 2, WHITE, "FRIENDS" );
    setMenuEntry( 3, WHITE, "CREDITS" );
    setMenuEntry( 4, WHITE, "QUIT GAME" );
    setOptionsLoc( Vec2(-80,-80) );
    updateCursor();
}
void TitleWindow::onEntrySelected( int ind ) {
    switch(ind) {
    case 0:
        hide();
        g_projtypewin->show();
        g_runstate = RS_SELECTING_PROJECT;
        break;
    case 1:
        hide();
        g_ranktypewin->show();
        g_runstate = RS_SELECTING_PROJECT;
        break;
    case 2:
        hide();
        g_friendwin->show();
        g_runstate = RS_SELECTING_PROJECT;            
        break;
    case 3:
        hide();
        g_creditwin->show();
        break;
    case 4:
        g_program_finished = true;
        break;        
    }
}
void TitleWindow::cancel() {   
}

////////////////////

RankTypeWindow::RankTypeWindow() : MenuWindow( "SELECT RANKING TYPE", WINDOW_ALPHA ) {
    setMenuEntry( 0, WHITE, MSG_RANKTYPE_QUICKEST_ALL_CLEAR );
    setMenuEntry( 1, WHITE, MSG_RANKTYPE_MOST_MILESTONE_PROGRESS );
    setMenuEntry( 2, WHITE, "BACK" );
};
void RankTypeWindow::onEntrySelected( int ind ) {
    hide();
    if( ind >= 0 && ind <= 1 ) {
        RANKTYPE rts[2] = { RANKTYPE_QUICKEST_ALL_CLEAR, RANKTYPE_MILESTONE_PROGRES_DAY };
        g_rankwin->show();
        g_rankwin->updateByType( rts[ind] );
    } else if( ind == 2 ) { // back
        g_titlewin->show();
    }
}
void RankTypeWindow::cancel() {
    g_runstate = RS_MAIN_MENU;
    hide();
    g_titlewin->show();
}
////////////////////
RankingWindow::RankingWindow() : Window( PREPARATION_GRID_WIDTH, PREPARATION_GRID_HEIGHT, B_ATLAS_WINDOW_FRAME_BASE, WINDOW_ALPHA ), cursor_at(0) {
    g_hud_layer->insertProp(this);
    setLoc( PREPARATION_WINDOW_LOC );
    
    title_tb = new CharGridTextBox(64);
    title_tb->setString( WHITE, "-----------------" );
    title_tb->setLoc( getTitleLoc());
    g_hud_layer->insertProp(title_tb);

    header = new CharGridTextBox(64);
    header->setString( WHITE,       "RANK ID       OWNER        CREATED   MS.  PLAYERS TOT.TIME" );
    header->setLoc( getHeaderLoc() );
    g_hud_layer->insertProp(header);

    cursor = new BlinkCursor( DIR_RIGHT );
    cursor->setLoc(0,0);
    g_hud_layer->insertProp(cursor);

    for(int i=0;i<elementof(lines);i++) {
        lines[i] = new CharGridTextBox(64);
        lines[i]->setString( WHITE, "------" );
        lines[i]->setLoc( header->loc + Vec2(0,-32*(i+1)) );
        g_hud_layer->insertProp( lines[i] );
    }
    updateCursor();
}
void RankingWindow::cancel() {
    hide();
    g_ranktypewin->show();
}
void RankingWindow::toggle( bool vis ) {
    setVisible(vis);
    cursor->setVisible(vis);
    title_tb->setVisible(vis);
    header->setVisible(vis);
    for(int i=0;i<elementof(lines);i++) lines[i]->setVisible(vis);
    if(vis) {
        cursor_at = 0;
        updateCursor();
    }
}
void RankingWindow::updateByType( RANKTYPE rt ) {
    memset( project_id_cache, 0, sizeof(project_id_cache) );
    if( rt == RANKTYPE_QUICKEST_ALL_CLEAR ) {
        title_tb->setString( WHITE, MSG_RANKTYPE_QUICKEST_ALL_CLEAR );
        header->setString( WHITE,       "RANK ID       OWNER        CREATED   MS.  PLAYERS TOT.TIME" );        
        AllClearLogEntry ents[ elementof(lines) ];
        size_t n = elementof(ents);
        dbLoadAllClearLogs( ents, &n );
        for(int i=0;i<elementof(lines);i++) lines[i]->clear();
        for(int i=0;i<n;i++) {
            Format fmt( "%-4d ", i+1 );
            setupProjectInfoTB( lines[i], fmt.buf, ents[i].project_id );
            project_id_cache[i] = ents[i].project_id;            
        }
        if(n==0) {
            lines[0]->setString( WHITE, "NO PROJECT FOUND" );
        }
    } else if( rt == RANKTYPE_MILESTONE_PROGRES_DAY ) {
        title_tb->setString( WHITE, MSG_RANKTYPE_MOST_MILESTONE_PROGRESS );
        header->setString( WHITE,       "RANK ID       OWNER        PROGRESS/NOW   PLAYERS" );
        MilestoneProgressRankEntry ranks[16];
        size_t n = elementof(ranks);
        dbLoadMilestoneProgressRanking( ranks, &n );

        for(int i=0;i<elementof(lines);i++) lines[i]->clear();
        for(int i=0;i<n;i++) {
            Format fmt( "%-4d ", i+1);
            setupProjectInfoTB( lines[i], fmt.buf, ranks[i].project_id, true, ranks[i].progress );
            project_id_cache[i] = ranks[i].project_id;                        
        }
        if(n==0) {
            lines[0]->setString( WHITE, "NO PROJECT FOUND" );
        }
    }
    // Add one line at the last
    for(int i=0;i<elementof(lines);i++) {
        if( lines[i]->isEmpty() ) {
            lines[i]->setString( WHITE, "BACK" );
            break;
        }
    }
}

void RankingWindow::updateCursor() {
    cursor->setLoc( lines[cursor_at]->loc + Vec2(-16,8) );
}
void RankingWindow::moveCursor( DIR d) {
    g_cursormove_sound->play();
    if(d==DIR_DOWN) {
        cursor_at ++;
        if( cursor_at >= elementof(lines) ) cursor_at=0;
        if( lines[cursor_at]->isEmpty() ) cursor_at = 0;
    } else if(d==DIR_UP) {
        cursor_at --;
        if( cursor_at < 0 ) {
            cursor_at = findFinalLine(lines, elementof(lines) ); //elementof(lines)-1;
            if( cursor_at < 0 ) {
                cursor_at = 0;
            }
        }
    }

    
    updateCursor();
}
void RankingWindow::selectAtCursor() {
    if( lines[cursor_at]->isEqual( "NO",2 )
        || lines[cursor_at]->isEqual( "CANNOT", 6 )
        || lines[cursor_at]->isEmpty() ) return;
    g_craft_sound->play();
    hide();
    if( lines[cursor_at]->isEqual("BACK",4) ) {
        g_ranktypewin->show();
        return;
    }
    
    g_projinfowin->setProjectId( project_id_cache[cursor_at] );
    g_projinfowin->previous_window = this;    
    g_projinfowin->show();
}

//////////////////
FriendWindow::FriendWindow() : Window( PREPARATION_GRID_WIDTH, PREPARATION_GRID_HEIGHT, B_ATLAS_WINDOW_FRAME_BASE, WINDOW_ALPHA ), cursor_at(0) {
    setLoc( PREPARATION_WINDOW_LOC );    
    Vec2 base = getTitleLoc();
    
    header = new CharGridTextBox(16);
    header->setString( WHITE, "FRIEND LIST" );
    header->setLoc( base );
    g_hud_layer->insertProp(header);

    assert(SSPROTO_SHARE_MAX % 16 == 0 );
    
    for(int i=0;i<SSPROTO_SHARE_MAX;i++) {
        int x = i % 8, y = i / 8;
        
        Vec2 line_at = base + Vec2(60 + x * 100, y*-30 - 60);
        
        faces[i] = new FacePanel();
        faces[i]->setScl(32);
        faces[i]->setLoc(line_at);
        g_hud_layer->insertProp(faces[i]);
            
        names[i] = new CharGridTextBox(16);
        names[i]->setLoc(line_at + Vec2(20,0));
        names[i]->setString(WHITE, "NOT SET");
        names[i]->setScl(8);
        g_hud_layer->insertProp(names[i]);
        
    }

    back_tb = new CharGridTextBox(4);
    back_tb->setLoc( base + Vec2(60,-640));
    back_tb->setString( WHITE, "BACK" );
    g_hud_layer->insertProp(back_tb);

    cursor = new BlinkCursor( DIR_RIGHT );
    g_hud_layer->insertProp(cursor);
    
    update();

}
void FriendWindow::cancel() {
    hide();
    g_titlewin->show();
    g_runstate = RS_MAIN_MENU;
}
void FriendWindow::toggle( bool vis ) {
    setVisible(vis);
    back_tb->setVisible(vis);
    header->setVisible(vis);
    for(int i=0;i<elementof(names);i++) {
        names[i]->setVisible(vis);
        faces[i]->setVisible(vis);
    }
    cursor->setVisible(vis);
    if(vis) {
        cursor_at = 0;
        update();
    }
}
void FriendWindow::update() {
    if( isDBNetworkActive() ) {
        dbLoadFollowings();
        dbLoadFollowers();

        memset( friends, 0, sizeof(friends) );
        int friend_num = getAllFriendContacts( friends, elementof(friends) );

        print("FRIENd num:%d", friend_num );
        for(int i=0;i<elementof(names);i++) {
            if( i < friend_num ) {
                names[i]->setString( WHITE, friends[i].nickname );
                faces[i]->update( friends[i].face_index, friends[i].hair_index );                
            } else {
                names[i]->setString( WHITE, "---" );
                faces[i]->setVisible(false);
            }
        }
    }
    updateCursor();
}
void FriendWindow::updateCursor() {
    if( cursor_at < SSPROTO_SHARE_MAX ) {
        cursor->setLoc( faces[cursor_at]->loc + Vec2(-30,0) );
    } else if(cursor_at == SSPROTO_SHARE_MAX ){
        cursor->setLoc( back_tb->loc + Vec2(-30,0));
    }
}
void FriendWindow::moveCursor(DIR d) {
    g_cursormove_sound->play();
    bool cursor_out = ( cursor_at >= SSPROTO_SHARE_MAX );

    // left-top is 0,0 right-bottom is 7,15. 8x16=128 friends
    assert( SSPROTO_SHARE_MAX == 128 );
    int cursor_x = cursor_at % 8, cursor_y = cursor_at / 8;

    if( cursor_out ) {
        switch(d) {
        case DIR_DOWN: cursor_x = cursor_y = 0; break;
        case DIR_UP: cursor_x = 0; cursor_y = 15; break;
        default: break;
        }
        cursor_at = cursor_x + cursor_y*8;
    } else {
        switch(d) {
        case DIR_DOWN: cursor_y ++; break;
        case DIR_UP: cursor_y --; break;
        case DIR_RIGHT:
            cursor_x ++;
            if( cursor_x >= 8 ) cursor_x = 0;
            break;
        case DIR_LEFT:
            cursor_x --;
            if( cursor_x < 0 ) cursor_x = 8;
            break;
        default:break;
        }
        print("cursor:%d,%d",cursor_x, cursor_y);
        if( cursor_y < 0 ) {
            cursor_at = SSPROTO_SHARE_MAX;
        } else if( cursor_y >= 16 ) {
            cursor_at = SSPROTO_SHARE_MAX;            
        } else {
            cursor_at = cursor_x + cursor_y*8;
        }
    }

    print("FriendWindow: cursor_at:%d",cursor_at);
    updateCursor();
}
void unfriendCallback( bool positive ) {
    print("unfriendCallback: positive:%d cursor_at:%d",positive, g_friendwin->cursor_at );
    if(positive) {
        Contact *ct = & g_friendwin->friends[ g_friendwin->cursor_at ];
        print("unfriending user_id:%d", ct->user_id );
        unfriendUser( ct->user_id );
        g_friendwin->update();
    }
}
void FriendWindow::selectAtCursor() {
    print("FriendWindow::selectAtCursor: cur:%d",cursor_at);
    if( cursor_at < SSPROTO_SHARE_MAX ) {
        if( friends[cursor_at].user_id > 0 ) {
            g_msgwin->clear();
            PCDump pcd;
            bool res = dbLoadUserFile( friends[cursor_at].user_id, "pc", (char*)&pcd, sizeof(pcd) );
            long long score = 0;
            if(res) score = pcd.score;
            char numstr[32];
            shorterInteger(numstr,sizeof(numstr),score);
            g_msgwin->writeLine( WHITE, Format("UNFRIEND %s ?", friends[cursor_at].nickname).buf );
            g_msgwin->writeLine( WHITE, "" );
            g_msgwin->writeLine( WHITE, Format("SP: %s", numstr).buf );            
            g_msgwin->setCloseCallback( unfriendCallback );
            g_msgwin->setWithCancel(true);
            g_msgwin->toggleDelay(false);
            g_msgwin->show();
            g_craft_sound->play();    
        } else {
            g_cant_sound->play();
        }
    } else if( cursor_at == SSPROTO_SHARE_MAX ){
        g_craft_sound->play();            
        hide();
        g_titlewin->show();
    }
}

/////////////////////////

CreditWindow::CreditWindow() : Window( PREPARATION_GRID_WIDTH, PREPARATION_GRID_HEIGHT, B_ATLAS_WINDOW_FRAME_BASE, WINDOW_ALPHA ), cur_page(0) {
    for(int i=0;i<elementof(lines);i++) lines[i] = NULL;
}
void CreditWindow::toggle( bool vis ) {
    setVisible(vis);
    if(vis) {
        g_runstate = RS_SELECTING_PROJECT;
        setTitleScreenVisible(true,false);
        update();
    } else {
        clear();
    }
}
void CreditWindow::clear() {
    for(int i=0;i<elementof(lines);i++) {
        if(lines[i]) {
            lines[i]->to_clean = true;
            lines[i] = NULL;
        }
    }
}
void CreditWindow::cancel() {
    progressPage();
}
void CreditWindow::selectAtCursor() {
    progressPage();
}
void CreditWindow::progressPage() {
    cur_page ++;
    if( cur_page > 1 ) {
        cur_page = 0;
        hide();
        g_titlewin->show();
        g_runstate = RS_MAIN_MENU;
    } else {
        update();
    }
}

// 2 pages
void CreditWindow::update() {
    print("CreditWindow::update: p:%d", cur_page );
    clear();
    
    char page_data[2][16][32] = {
        {
            "    Designed & Programmed by",
            "         Kengo Nakajima",
            "",
            "       Project Manager",
            "           Alex Tait",
            "",
            "Senior Graphic R&D Programmer",
            "    Jean-Sebastien Bourdon",
            "",
            "           Artists",
            "         Mike Winder",
            "         Sean Dunkley",
            "",
            "           Sound",
            "      Jean-Marc Pereira",
        },
        {
            "             QA",
            "    Jean-Francois Carrier",
            "        Moe El-kaaki",
            "        SEbastien Doe",
            "       Dominic Knowles",
            "       Steve Rousseau",
            "",
            "       Special Thanks",
            "         Yoichi Wada",
            "         Jacob Navok",
            "       Tetsuji Iwasaki",
        },
    };
    for(int i=0;i<elementof(lines);i++) {
        lines[i] = new CharGridTextBox(32);
        lines[i]->setString( WHITE, page_data[cur_page][i] );
        lines[i]->setLoc( Vec2(-260,200-i*32) );
        g_hud_layer->insertProp(lines[i]);
        print("str:%s", page_data[cur_page][i] );
    }
}

//////////////////
DebugWindow::DebugWindow() : MenuWindow( "DEBUG MENU", WINDOW_ALPHA ) {
    setMenuEntry( 0, WHITE, "WALK-SPEED-UP" );
    setMenuEntry( 1, WHITE, "INFINITY-ENERGY" );
    setMenuEntry( 2, WHITE, "UNLOCK ALL ITEMS" );
    setMenuEntry( 3, WHITE, "ADD ITEMS" );
    setMenuEntry( 4, WHITE, "PROGRESS MILESTONE BY 1" );
    setMenuEntry( 5, WHITE, "PROGRESS MILESTONE BY 30" );
    setMenuEntry( 6, WHITE, "UNCOVER NEARBY CORE" );
    setMenuEntry( 7, WHITE, "WARP TO EAST BY 100" );
    setMenuEntry( 8, WHITE, "WARP TO NORTH BY 100" );
}
void DebugWindow::onEntrySelected(int ind) {
    switch(ind) {
    case 0:
        g_pc_walk_speed_accel = 2;
        break;
    case 1:
        g_enable_auto_charge = true;
        break;
    case 2:
        ResearchState::unlockAll();
        break;
    case 3:
        g_pc->clearItems();
        g_pc->incItem( ITT_SHIELD, 1, 0, false );
        g_pc->incItem( ITT_ACCELERATOR, 1, 0, false );
        g_pc->incItem( ITT_BATTERY1, 1, 0, false );        
        

        g_pc->incItem( ITT_BEAMGUN4, 1, PC_SUIT_NUM, false );
        g_pc->incItem( ITT_HEAL_BLASTER, 50, PC_SUIT_NUM, false );
        g_pc->incItem( ITT_DEBRI_SOIL, 50, PC_SUIT_NUM, false );
        g_pc->incItem( ITT_BRICK_PANEL, 50, PC_SUIT_NUM, false );
        g_pc->incItem( ITT_BRICK_PANEL, 50, PC_SUIT_NUM, false );
        g_pc->incItem( ITT_BRICK_PANEL, 50, PC_SUIT_NUM, false );
        g_pc->incItem( ITT_BRICK_PANEL, 50, PC_SUIT_NUM, false );
        g_pc->incItem( ITT_BRICK_PANEL, 50, PC_SUIT_NUM, false );                        
        
#if 0
        g_pc->incItem( ITT_APPLE,10, PC_SUIT_NUM, false );        
    
        g_pc->incItem( ITT_BLASTER, 50, PC_SUIT_NUM, false );

        g_pc->incItem( ITT_DEBRI_ROCK, 10, PC_SUIT_NUM, false );
        g_pc->incItem( ITT_POLE, 50, PC_SUIT_NUM, false  );

#endif
#if 1
        g_pc->incItem( ITT_SHOVEL, 1, PC_SUIT_NUM, false );    
        g_pc->incItem( ITT_BLASTER, 50, PC_SUIT_NUM, false );
        g_pc->incItem( ITT_REACTOR, 5, PC_SUIT_NUM, false );
        g_pc->incItem( ITT_POLE, 50, PC_SUIT_NUM, false  );

        g_pc->incItem( ITT_BEAMGUN1, 1, PC_SUIT_NUM, false );        
        g_pc->incItem( ITT_PORTAL, 5, PC_SUIT_NUM, false  );    
        g_pc->incItem( ITT_TURRET, 5, PC_SUIT_NUM, false );
        g_pc->incItem( ITT_EXCHANGE, 5, PC_SUIT_NUM,  false );
        g_pc->incItem( ITT_CABLE, 50, PC_SUIT_NUM,false );
        g_pc->incItem( ITT_HP_POTION, 10, PC_SUIT_NUM, false );
#endif
    
#if 0    

        g_pc->incItem( ITT_DARK_MATTER_PARTICLE, 1, PC_SUIT_NUM );    
        g_pc->incItem( ITT_HYPER_PARTICLE, 1, PC_SUIT_NUM );
#endif
    
#if 0    
        g_pc->incItem( ITT_WEED_SEED, 10 );
        g_pc->incItem( ITT_TREE_SEED, 10 );
        g_pc->incItem( ITT_MICROBE, 10 );
#endif
    
#if 0
        g_pc->incItem( ITT_FENCE, 20 );    
        g_pc->incItem( ITT_REACTOR, 5 );
        g_pc->incItem( ITT_TURRET, 5 );    

        g_pc->incItem( ITT_EXCHANGE, 5 );
        g_pc->incItem( ITT_CABLE, 20 );
        g_pc->incItem( ITT_DEBRI_ROCK, 20 );
#endif

#if 0
        g_pc->incItem( ITT_CABLE, 50, PC_SUIT_NUM, false );
        g_pc->incItem( ITT_FENCE, 50, PC_SUIT_NUM, false );    
        g_pc->incItem( ITT_DEBRI_SOIL, 50, PC_SUIT_NUM, false );
        g_pc->incItem( ITT_BEAMGUN2, 1, PC_SUIT_NUM, false );
        g_pc->incItem( ITT_BEAMGUN3, 1, PC_SUIT_NUM, false );
        g_pc->incItem( ITT_BEAMGUN4, 1, PC_SUIT_NUM, false );
        g_pc->incItem( ITT_ARTIFACT, 50, PC_SUIT_NUM, false );
        g_pc->incItem( ITT_DEBRI_SOIL, 50, PC_SUIT_NUM, false );
        g_pc->incItem( ITT_DEBRI_RAREMETALORE, 50, PC_SUIT_NUM, false );
        g_pc->incItem( ITT_DEBRI_IRONORE, 50, PC_SUIT_NUM, false );
#endif
        break;
    case 4:
        {
            g_fld->debugClearFlag();
            int cnt = dbSaveFlagCands();
            hudUpdateMilestone(cnt);
            hudMilestoneMessage( g_pc->nickname, cnt, true );
            dbSaveMilestoneProgressLog( g_current_project_id );
            if( cnt == MILESTONE_MAX ) hudCongratulateProjectIsOver();
        }
        break;
    case 5:
        {
            for(int i=0;i<30;i++) g_fld->debugClearFlag();
            int cnt = dbSaveFlagCands();
            hudUpdateMilestone(cnt);
            hudMilestoneMessage( g_pc->nickname, cnt, true );
        }
        break;
    case 6:
        for(int x=-20;x<20;x++) {
            for(int y=-20;y<20;y++){
                Cell *c = g_fld->get( g_pc->loc + Vec2(x*PPC,y*PPC) );
                if(c && c->bt == BT_CORE_COVER ) c->bt = BT_AIR;
            }
        }
        break;
    case 7:
        g_pc->loc.x += 100*PPC;
        break;
    case 8:
        g_pc->loc.y += 100*PPC;
        break;
    default:
        break;
    }
        
}
void DebugWindow::cancel() {
    hide();
}

void hudShowWarningMessage( const char *msg ) {
    g_msgwin->clear();
    g_msgwin->writeLine( WHITE, msg );
    g_msgwin->setCloseCallback(NULL);
    g_msgwin->setWithCancel(false);
    g_msgwin->show();
    g_msgwin->toggleDelay(false);
}

int g_abandon_index=-1;

void abandon_callback( bool positive ) {
    if( g_invwin->getVisible() && positive && g_abandon_index >= 0 ) {
        print("abandon! selected_at:%d", g_abandon_index );
        g_pc->abandonItem( g_abandon_index );
        g_abandon_sound->play();
    }
    g_abandon_index = -1;
}
void hudShowConfirmAbandonMessage( int ind_to_abandon ) {
    if( g_pc->items[ind_to_abandon].isEmpty() ) return;
    g_windowopen_sound->play();
    g_msgwin->clear();

    int num_beamguns = g_pc->countBeamguns();
    ItemConf *itc = g_pc->items[ind_to_abandon].conf;
    bool is_beamgun = (itc->itt == ITT_BEAMGUN1) || (itc->itt == ITT_BEAMGUN2) || (itc->itt == ITT_BEAMGUN3) || (itc->itt == ITT_BEAMGUN4);
    if( num_beamguns <= 1 && is_beamgun ) {
        g_cant_sound->play();
        g_msgwin->setWithCancel(false);
        g_msgwin->writeLine( WHITE, "CAN'T ABANDON THIS BEAMGUN!" );
        g_msgwin->show();        
    } else {
        g_abandon_index = ind_to_abandon;
        if( strlen( itc->name ) < 14 ) {
            Format fmt( "SURE TO ABANDON %s ?", itc->name );
            g_msgwin->writeLine( WHITE, fmt.buf );                    
        } else {
            Format fmt( "SURE TO ABANDON" );
            g_msgwin->writeLine( WHITE, fmt.buf );
            Format fmt2( "%s ?", itc->name );
            g_msgwin->writeLine( WHITE, fmt2.buf );        
        }
        g_msgwin->setCloseCallback( abandon_callback );
        g_msgwin->setWithCancel(true);
        g_msgwin->show();
        g_msgwin->toggleDelay(false);
    }
}
ITEMTYPE g_recovery_item_type;

void recovery_item_callback( bool positive ) {
    print("recovery_item_callback: %d", positive );
    if( positive ) {
        switch(g_recovery_item_type) {
        case ITT_BATTERY1: g_pc->incItem( ITT_BATTERY1, 1, 0, false ); break;
        case ITT_BEAMGUN1: g_pc->incItem( ITT_BEAMGUN1, 1, PC_SUIT_NUM, false ); break;
        default:
            assert(false);
            break;
        }
        g_pc->recovery_count_left --;
        g_pc->last_recovery_at_sec = (unsigned int)now();
    }
}
void hudShowConfirmRecoveryMessage( ITEMTYPE itt ) {
    g_windowopen_sound->play();
    g_msgwin->clear();
    ItemConf *itc = ItemConf::getItemConf(itt);
    assert(itc);
    Format fmt( "DO YOU RECEIVE %s ?", itc->name );
    g_msgwin->writeLine( WHITE, fmt.buf );
    g_msgwin->writeLine( WHITE, "" );
    Format fmt2( "RECOVERY COUNT : %d LEFT", g_pc->recovery_count_left );
    g_msgwin->writeLine( WHITE, fmt2.buf );    
    g_msgwin->setWithCancel(true);
    g_msgwin->setCloseCallback( recovery_item_callback );
    g_msgwin->show();
    g_msgwin->toggleDelay(true);
    g_recovery_item_type = itt;
}
