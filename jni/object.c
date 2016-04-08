#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include "defs.h"

void newsmoke(struct state *state,float x,float y,int water){
	for(int i=randomint(0,1);i<5;++i){
		struct smoke *smoke=malloc(sizeof(struct smoke));
		smoke->w=SMOKE_SIZE;
		smoke->h=SMOKE_SIZE;
		smoke->x=x-(SMOKE_SIZE/2.0f);
		smoke->y=y-(SMOKE_SIZE/2.0f);
		smoke->rot=randomint(1,359)*(PI/180.0f);
		smoke->alpha=randomint(60,90)/100.0f;
		smoke->direction=onein(2);
		smoke->water=water;
		smoke->next=state->smokelist;
		state->smokelist=smoke;
	}
}
struct smoke *deletesmoke(struct state *state,struct smoke *smoke,struct smoke *prev){
	if(prev!=NULL)prev->next=smoke->next;
	else state->smokelist=smoke->next;
	void *temp=smoke->next;
	free(smoke);
	return temp;
}

void newflash(struct state *state,float x,float y){
	for(int i=0;i<5;++i){
		struct flash *flash=malloc(sizeof(struct flash));
		flash->w=FLASH_SIZE;
		flash->h=FLASH_SIZE;
		flash->x=x-(FLASH_SIZE/2.0f)+(randomint(-40,40)/100.0f);
		flash->y=y-(FLASH_SIZE/2.0f)+(randomint(-40,40)/100.0f);
		flash->rot=randomint(0,359)*(PI/180.0f);
		flash->alpha=0.7f;
		flash->basealpha=0.7f;
		flash->next=state->flashlist;
		state->flashlist=flash;
	}
}
struct flash *deleteflash(struct state *state,struct flash *flash,struct flash *prev){
	if(prev!=NULL)prev->next=flash->next;
	else state->flashlist=flash->next;
	void *temp=flash->next;
	free(flash);
	return temp;
}

void place_ships(struct state *state){
	int sideways,xstart,ystart,id=0;
	// place subs
	for(int i=0;i<SLOT_H;++i){
		for(int j=0;j<SLOT_V;++j){
			state->board[i][j].slot=0;
		}
	}
	for(int k=0;k<SHIPCOUNT_SUB;++k){
		sideways=onein(2);
		xstart=randomint(1,SLOT_H-(sideways?SHIPSIZE_SUB:0))-1;
		ystart=randomint(1,SLOT_V-(sideways?0:SHIPSIZE_SUB))-1;
		state->ship_sub[k].x=(BOARD_X+xstart)-(sideways?0.0f:0.5f);
		state->ship_sub[k].y=(BOARD_Y+ystart)+(sideways?0.0f:0.5f);
		state->ship_sub[k].w=SLOT_SIZE*SHIPSIZE_SUB;
		state->ship_sub[k].h=SLOT_SIZE;
		state->ship_sub[k].rot=sideways?0.0f:PI/2.0f;
		state->ship_sub[k].visible=true;
		state->ship_sub[k].id=id;
		for(int i=0;i<SHIPSIZE_SUB;++i){
			if(state->board[xstart][ystart].slot!=0){
				place_ships(state);
				return;
			}
			state->board[xstart][ystart].slot=SHIPTYPE_SUB;
			state->board[xstart][ystart].id=id;
			if(sideways)++xstart;
			else ++ystart;
		}
		++id;
	}
	// place destroyers
	for(int k=0;k<SHIPCOUNT_DESTROYER;++k){
		sideways=onein(2);
		xstart=randomint(1,SLOT_H-(sideways?SHIPSIZE_DESTROYER:0))-1;
		ystart=randomint(1,SLOT_V-(sideways?0:SHIPSIZE_DESTROYER))-1;
		state->ship_destroyer[k].x=(BOARD_X+xstart)-(sideways?0.0f:1.0f);
		state->ship_destroyer[k].y=(BOARD_Y+ystart)+(sideways?0.0f:1.0f);
		state->ship_destroyer[k].w=SLOT_SIZE*SHIPSIZE_DESTROYER;
		state->ship_destroyer[k].h=SLOT_SIZE;
		state->ship_destroyer[k].rot=sideways?0.0f:PI/2.0f;
		state->ship_destroyer[k].visible=true;
		state->ship_destroyer[k].id=id;
		for(int i=0;i<SHIPSIZE_DESTROYER;++i){
			if(state->board[xstart][ystart].slot!=0){
				place_ships(state);
				return;
			}
			state->board[xstart][ystart].slot=SHIPTYPE_DESTROYER;
			state->board[xstart][ystart].id=id;
			if(sideways)++xstart;
			else ++ystart;
		}
		++id;
	}
	// place carriers
	for(int k=0;k<SHIPCOUNT_CARRIER;++k){
		sideways=onein(2);
		xstart=randomint(1,SLOT_H-(sideways?SHIPSIZE_CARRIER:0))-1;
		ystart=randomint(1,SLOT_V-(sideways?0:SHIPSIZE_CARRIER))-1;
		state->ship_carrier[k].x=(BOARD_X+xstart)-(sideways?0.0f:2.0f);
		state->ship_carrier[k].y=(BOARD_Y+ystart)+(sideways?0.0f:2.0f);
		state->ship_carrier[k].w=SLOT_SIZE*SHIPSIZE_CARRIER;
		state->ship_carrier[k].h=SLOT_SIZE;
		state->ship_carrier[k].rot=sideways?0.0f:PI/2.0f;
		state->ship_carrier[k].visible=true;
		state->ship_carrier[k].id=id;
		for(int i=0;i<SHIPSIZE_CARRIER;++i){
			if(state->board[xstart][ystart].slot!=0){
				place_ships(state);
				return;
			}
			state->board[xstart][ystart].slot=SHIPTYPE_CARRIER;
			state->board[xstart][ystart].id=id;
			if(sideways)++xstart;
			else ++ystart;
		}
		++id;
	}
}
int send_data_blocking(int sock,void *data,int size){
	int bytestr=0,result;
	while(bytestr!=size){
		result=send(sock,data+bytestr,size-bytestr,0);
		if(result==-1){
			if(errno==EWOULDBLOCK)continue;
			return false;
		}
		bytestr+=result;
	}
	return true;
}
int recv_data_blocking(int sock,void *data,int size){
	int bytestr=0,result;
	while(bytestr!=size){
		result=recv(sock,data+bytestr,size-bytestr,0);
		if(result==-1)continue;
		else if(result==0)return false;
		bytestr+=result;
	}
	return true;
}
int send_info(struct state *state){
	if(send_data_blocking(state->sock,state->initial,3)&&
	send_data_blocking(state->sock,state->board,sizeof(struct board)*SLOT_H*SLOT_V)&&
	send_data_blocking(state->sock,state->ship_sub,sizeof(struct ship)*SHIPCOUNT_SUB)&&
	send_data_blocking(state->sock,state->ship_destroyer,sizeof(struct ship)*SHIPCOUNT_DESTROYER)&&
	send_data_blocking(state->sock,state->ship_carrier,sizeof(struct ship)*SHIPCOUNT_CARRIER))return true;
	return false;
}
int recv_info(struct state *state){
	state->opponentinitial[3]=0;
	if(recv_data_blocking(state->sock,state->opponentinitial,3)&&
	recv_data_blocking(state->sock,state->opponent,sizeof(struct board)*SLOT_V*SLOT_H)&&
	recv_data_blocking(state->sock,state->eship_sub,sizeof(struct ship)*SHIPCOUNT_SUB)&&
	recv_data_blocking(state->sock,state->eship_destroyer,sizeof(struct ship)*SHIPCOUNT_DESTROYER)&&
	recv_data_blocking(state->sock,state->eship_carrier,sizeof(struct ship)*SHIPCOUNT_CARRIER))return true;
	return false;
}

int buttonprocess(struct state *state,struct button *button){
	if(state->pointer[0].x>button->x&&state->pointer[0].x<button->x+button->w&&
	state->pointer[0].y>button->y&&state->pointer[0].y<button->y+button->h&&state->pointer[0].active){
		//if(!button->active)playsound(state->soundengine,state->aassets.sound+SID_CLICK,false);
		button->active=true;
		return BUTTON_PRESS;
	}
	else if(state->pointer[0].x>button->x&&state->pointer[0].x<button->x+BUTTON_WIDTH&&
	state->pointer[0].y>button->y&&state->pointer[0].y<button->y+BUTTON_HEIGHT&&!state->pointer[0].active&&button->active){
		button->active=false;
		if(state->soundenabled)playsound(state->soundengine,state->aassets.sound+SID_CLICK,false);
		return BUTTON_ACTIVATE;
	}
	button->active=false;
	return 0;
}
int buttondraw(struct state *state,struct button *button){
	const float COLORCHANGE=0.7f;
	int bstate=buttonprocess(state,button);
	switch(bstate){
		case BUTTON_PRESS:
			glUniform4f(state->uniform.rgba,COLORCHANGE,COLORCHANGE,COLORCHANGE,1.0f);
			break;
		case BUTTON_ACTIVATE:
		case 0:
			glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
			break;
	}
	draw(state,button);
	return bstate;
}
void buttondrawtext(ftfont *fnt,struct button *button){
	drawtextcentered(fnt,button->x+(button->w/2.0f),button->y+(button->h/2.0f)-(fnt->fontsize/2.0f),button->label);
}
void draw(struct state *state,void *vp){
	struct generic *target=vp;
	glUniform4f(state->uniform.texcoords,0.0f,1.0f,0.0f,1.0f);
	glUniform2f(state->uniform.vector,target->x,target->y);
	glUniform2f(state->uniform.size,target->w,target->h);
	glUniform1f(state->uniform.rot,target->rot);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);
}