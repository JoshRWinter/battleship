#include "glesutil.h"
#define NET_PORT 28855
#define NET_PORT_STR "28855"
#define PI 3.14159f
#define LOCAL_TURN 1
#define REMOTE_TURN 2
#define REMOTE_TURN_OFFSET (0.0f)
#define LOCAL_TURN_OFFSET (-9.0f)
#define BOARD_OFFSET (9.0f)
#define VIBRATE_LENGTH 750

#define TID_BACKGROUND 0
#define TID_SLOT 1
#define TID_SUB 2
#define TID_DESTROYER 3
#define TID_CARRIER 4
#define TID_MARK 5
#define TID_BOARD 6
#define TID_SMOKE 7
#define TID_FLASH 8

#define TID_MENUBACKGROUND 0
#define TID_BUTTON 1
#define TID_MESSAGE 2
#define TID_BUTTONSMALL 3
#define TID_BUTTONARROW 4

#define SID_BATTLESHIP 0
#define SID_EXPLOSION 1
#define SID_SPLASH 2
#define SID_NO 3
#define SID_CLICK 4

struct generic{
	float x,y,w,h,rot;
};

struct ship{
	float x,y,w,h,rot;
	int visible,id;
};

#define SMOKE_SIZE 0.5f
struct smoke{
	float x,y,w,h,rot,alpha;
	int direction,water;
	struct smoke *next;
};

#define FLASH_SIZE 1.2f
struct flash{
	float x,y,w,h,rot,alpha,basealpha;
	struct flash *next;
};

#define BUTTON_PRESS 1
#define BUTTON_ACTIVATE 2
#define BUTTON_HEIGHT 1.5f
#define BUTTON_WIDTH 4.25f
#define BUTTON_SMALL_SIZE 1.0f
#define BUTTON_ARROW_SIZE 1.0f
struct button{
	float x,y,w,h,rot;
	char *label;
	int active;
};

#define SHIPTYPE_SUB 1
#define SHIPTYPE_DESTROYER 2
#define SHIPTYPE_CARRIER 3
#define SHIPCOUNT_SUB 2
#define SHIPCOUNT_DESTROYER 2
#define SHIPCOUNT_CARRIER 1
#define SHIPSIZE_SUB 2
#define SHIPSIZE_DESTROYER 3
#define SHIPSIZE_CARRIER 5
#define SLOT_V 6
#define SLOT_H 15
#define SLOT_SIZE 1.0f
#define BOARD_X -7.5f
#define BOARD_Y -4.0f
struct board{
	float x,y;
	char id,slot,marked;
};

struct state{
	int running,back,showmenu,vibrationenabled,musicenabled,soundenabled,sock,turn,fbuttonstate,bytestr;
	int readyfornextturn,connector;
	float offset;
	char connectto[51],initial[4],opponentinitial[4],coordinate[2];
	struct board *activeslot;
	struct board board[SLOT_H][SLOT_V],opponent[SLOT_H][SLOT_V];
	EGLContext context;
	EGLSurface surface;
	EGLDisplay display;
	struct ship ship_sub[SHIPCOUNT_SUB];
	struct ship ship_destroyer[SHIPCOUNT_DESTROYER];
	struct ship ship_carrier[SHIPCOUNT_CARRIER];
	struct ship eship_sub[SHIPCOUNT_SUB];
	struct ship eship_destroyer[SHIPCOUNT_DESTROYER];
	struct ship eship_carrier[SHIPCOUNT_CARRIER];
	unsigned vao,vbo,program;
	struct pack assets,uiassets;
	struct apack aassets;
	struct jni_info jni_info;
	slesenv *soundengine;
	struct android_app *app;
	struct device device;
	struct crosshair pointer[2];
	struct{ftfont *main,*header;}font;
	struct{int vector,size,rot,texcoords,projection,rgba;}uniform;
	struct{float left,right,bottom,top;}rect;
	struct generic background,menubackground,gameboard;
	struct button firebutton;
	struct smoke *smokelist;
	struct flash *flashlist;
};

void cmdproc(struct android_app*,int32_t);
int32_t inputproc(struct android_app*,AInputEvent*);
int process(struct android_app*);
void init_display(struct state*);
void term_display(struct state*);

void newsmoke(struct state*,float,float,int);
struct smoke *deletesmoke(struct state*,struct smoke*,struct smoke*);
void newflash(struct state*,float,float);
struct flash *deleteflash(struct state*,struct flash*,struct flash*);

int core(struct state*);
void render(struct state*);
void init(struct state*);
void reset(struct state*,int);
void draw(struct state*,void*);
int send_data_blocking(int,void*,int);
int recv_data_blocking(int,void*,int);
int send_info(struct state*);
int recv_info(struct state*);
int buttonprocess(struct state*,struct button*);
int buttondraw(struct state*,struct button*);
void buttondrawtext(ftfont*,struct button*);
void place_ships(struct state*);
int menu_main(struct state*);
int menu_message(struct state*,const char*);
int menu_name(struct state*,int*);
int menu_board(struct state*,int*,int);
int menu_rematch(struct state*,int*);
int menu_end(struct state*);
int menu_pause(struct state*);
int menu_connectto(struct state*,int*);
int menu_connect(struct state*,char*,int*);