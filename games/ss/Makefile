CCDK=../..
BACKEND=$(CCDK)/backend
VCE=$(CCDK)/vce

EXTERNALS=$(CCDK)/externals
MOYAI=$(EXTERNALS)/moyai
MOYAILIBS=$(MOYAI)/libmoyaicl.a $(MOYAI)/freetype-2.4.10/objs/.libs/libfreetype.a $(MOYAI)/bzip2-1.0.6/libbz2.a $(MOYAI)/zlib-1.2.7/libz.a $(MOYAI)/libftgl.a $(MOYAI)/libsoil.a $(MOYAI)/libsnappy.a

GLFWLIB=$(MOYAI)/glfw-2.7.7/lib/cocoa/libglfw.a
VCELIB=$(VCE)/libvce.a
LIBFLAGS=-framework OpenGL -framework Cocoa -framework IOKit  -framework CoreFoundation  $(MOYAILIBS) $(MOYAI)/fmod/api/lib/libfmodex.dylib $(GLFWLIB) -lev -lssl -lcrypto $(VCELIB) -Wl,-stack_size -Wl,1000000





#CFLAGS= -I/usr/x11/include  -g -Wall -I./moyai -O0
CFLAGS= -O0 -g -Wall -I. -I../pdn -I.. -I$(MOYAI)/glfw-2.7.7/include/ -Irapidjson/include -DUSE_OPENGL -I$(VCE) -I$(BACKEND) -I$(EXTERNALS)



GAMESRCS=$(BACKEND)/ssproto_cli.cpp net.cpp main.cpp enemy.cpp util.cpp pwgrid.cpp item.cpp hud.cpp effect.cpp pc.cpp char.cpp noise.cpp field.cpp mapview.cpp

GAMEOBJS=$(GAMESRCS:.cpp=.o)

OUT=ss

all : $(OUT)

$(OUT) : $(GAMEOBJS) $(MOYAILIBS) $(GWFLLIB) $(VCELIB)
	g++ $(CFLAGS) $(LIBFLAGS) $(GAMEOBJS)  -o $(OUT)


%.o : %.cpp
	g++ -c $(CFLAGS) $< -o $@


clean:
	rm -f *.o deps.make $(OUT)

depend: 
	$(CC) $(CFLAGS) -MM $(GAMESRCS) > deps.make

ss2:
	./ss &
	sleep 1
	./ss &

clearuser:
	redis-cli del user_id_gen
	redis-cli del user_id_h
	find $(BACKEND)/datadir | grep pc | xargs rm

clearpj:
	redis-cli del pj_by_owner_h
	redis-cli del pj_info_h
	redis-cli del project_id_gen
	redis-cli del project_status_h
	redis-cli del playtime_h
	redis-cli del ms_progress_z
	redis-cli del all_clear_z
	find $(BACKEND)/datadir | grep field | xargs rm -f
	find $(BACKEND)/datadir | grep powersystem | xargs rm -f
	find $(BACKEND)/datadir | grep deposit | xargs rm -f
	find $(BACKEND)/datadir | grep research | xargs rm -f
	find $(BACKEND)/datadir | grep reveal | xargs rm -f
	find $(BACKEND)/datadir | grep envs | xargs rm -f
	find $(BACKEND)/datadir | grep unfriendlog | xargs rm -f
	find $(BACKEND)/datadir | grep _image_ | xargs rm -f
	find $(BACKEND)/datadir | grep _counter_ | xargs rm -f
	find $(BACKEND)/datadir | grep _sharedprojects | xargs rm -f

clearcontact:
	find $(BACKEND)/datadir | grep followers | xargs rm -f
	find $(BACKEND)/datadir | grep followings | xargs rm -f

cleardb: clearpj clearuser clearcontact


-include deps.make
