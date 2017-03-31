#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include "defs.h"

int menu_main(struct state *state){
	struct button playbutton={-5.25f,0.5f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,"PLAY",false};
	struct button aboutbutton={1.0f,0.5f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,"ABOOT",false};
	struct button quitbutton={1.0f,2.2f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,"QUIT",false};
	struct button musicbutton={-5.25f,2.2f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,"MUSIC",false};
	const char *msg="~ BATTLESHIP ~\nProgramming and Art by Josh Winter\nC99 & OpenGLES 2.0\n~ Music ~\n"
	"BurntOutOnlineMedia - Kings Fall (Kingdoms Rise)\nhttp://burntoutonlinemedia.newgrounds.com/\n"
	"~ Font ~\nHigh Tower Text";
	while(process(state->app)){
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_MENUBACKGROUND].object);
		draw(state,&state->menubackground);
		
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTON].object);
		if(buttondraw(state,&playbutton)==BUTTON_ACTIVATE){
			int readytoplay=false;
			if(!menu_name(state,&readytoplay))return false;
			else if(readytoplay)return true;
		}
		if(buttondraw(state,&aboutbutton)==BUTTON_ACTIVATE){
			if(!menu_message(state,msg))return false;
		}
		if(buttondraw(state,&musicbutton)==BUTTON_ACTIVATE){
			stopallsounds(state->soundengine);
			if(state->musicenabled)playsound(state->soundengine,state->aassets.sound+SID_NO,true);
			else playsound(state->soundengine,state->aassets.sound+SID_BATTLESHIP,true);
			state->musicenabled=!state->musicenabled;
		}
		if(buttondraw(state,&quitbutton)==BUTTON_ACTIVATE||state->back){
			ANativeActivity_finish(state->app->activity);
		}
		glBindTexture(GL_TEXTURE_2D,state->font.header->atlas);
		glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
		buttondrawtext(state->font.header,&aboutbutton);
		buttondrawtext(state->font.header,&playbutton);
		buttondrawtext(state->font.header,&quitbutton);
		buttondrawtext(state->font.header,&musicbutton);
		
		eglSwapBuffers(state->display,state->surface);
	}
	return state->running;
}

int menu_name(struct state *state,int *readytoplay){
	struct button arrow[]={
		{-1.5f,-1.5f,BUTTON_ARROW_SIZE,BUTTON_ARROW_SIZE,0.0f,"",false},
		{-1.5f,0.5f,BUTTON_ARROW_SIZE,BUTTON_ARROW_SIZE,PI,"",false},
		{-0.5f,-1.5f,BUTTON_ARROW_SIZE,BUTTON_ARROW_SIZE,0.0f,"",false},
		{-0.5f,0.5f,BUTTON_ARROW_SIZE,BUTTON_ARROW_SIZE,PI,"",false},
		{0.5f,-1.5f,BUTTON_ARROW_SIZE,BUTTON_ARROW_SIZE,0.0f,"",false},
		{0.5f,0.5f,BUTTON_ARROW_SIZE,BUTTON_ARROW_SIZE,PI,"",false}
	};
	struct generic background={-6.25f,-3.75f,12.5f,7.5f,0.0f};
	struct button cancelbutton={0.5f,2.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,"Cancel",false};
	struct button readybutton={-4.5f,2.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,"Ready",false};
	int changed=false;
	while(process(state->app)){
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BACKGROUND].object);
		draw(state,&state->background);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_MESSAGE].object);
		draw(state,&background);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTON].object);
		if(buttondraw(state,&cancelbutton)==BUTTON_ACTIVATE||state->back){
			state->back=false;
			return true;
		}
		if(buttondraw(state,&readybutton)==BUTTON_ACTIVATE){
			if(changed){
				FILE *file=fopen("data/data/joshwinter.battleship/files/battleship.dat","wb");
				if(file){
					fwrite(state->initial,1,3,file);
					fclose(file);
				}
			}
			if(!menu_board(state,readytoplay,false))return false;
			if(*readytoplay)return true;
		}
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTONARROW].object);
		for(int i=0;i<6;++i){
			if(buttondraw(state,arrow+i)==BUTTON_ACTIVATE){
				changed=true;
				switch(i){
					case 0:
						if(--state->initial[0]<'A')state->initial[0]='Z';
						break;
					case 1:
						if(++state->initial[0]>'Z')state->initial[0]='A';
						break;
					case 2:
						if(--state->initial[1]<'A')state->initial[1]='Z';
						break;
					case 3:
						if(++state->initial[1]>'Z')state->initial[1]='A';
						break;
					case 4:
						if(--state->initial[2]<'A')state->initial[2]='Z';
						break;
					case 5:
						if(++state->initial[2]>'Z')state->initial[2]='A';
						break;
				}
			}
		}
		glUniform1f(state->uniform.rot,0.0f);
		glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->font.header->atlas);
		char initial[]={state->initial[0],0};
		drawtextcentered(state->font.header,-1.0f,-state->font.header->fontsize/2.0f,initial);
		initial[0]=state->initial[1];
		drawtextcentered(state->font.header,0.0f,-state->font.header->fontsize/2.0f,initial);
		initial[0]=state->initial[2];
		drawtextcentered(state->font.header,1.0f,-state->font.header->fontsize/2.0f,initial);
		buttondrawtext(state->font.header,&cancelbutton);
		buttondrawtext(state->font.header,&readybutton);
		glBindTexture(GL_TEXTURE_2D,state->font.main->atlas);
		drawtextcentered(state->font.main,0.0f,-3.0f,"Enter your initials");
		eglSwapBuffers(state->display,state->surface);
	}
	return state->running;
}

int menu_board(struct state *state,int *readytoplay,int rematch){
	/*if(!menu_message(state,"Battleship will automatically generate\nrandom boards. "
	"Tap \"Ready\" to accept the board,\nor \"NEW\" to generate a new one.")){
		return false;
	}*/
	struct button cancelbutton={2.5f,2.5f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,"Cancel",false};
	struct button nextbutton={-BUTTON_WIDTH/2.0f,2.5f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,"New",false};
	struct button readybutton={-7.0f,2.5f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,"Ready",false};
	float x=-7.5f,y;
	for(int i=0;i<SLOT_H;++i){
		y=-4.0f;
		for(int j=0;j<SLOT_V;++j){
			state->board[i][j].x=x;
			state->board[i][j].y=y;
			state->board[i][j].id=-1;
			state->board[i][j].marked=false;
			y+=SLOT_SIZE;
			
		}
		x+=SLOT_SIZE;
	}
	// place ships
	place_ships(state);
	while(process(state->app)){
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BACKGROUND].object);
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		draw(state,&state->background);
		
		struct generic slot={.rot=0.0f,.w=SLOT_SIZE,.h=SLOT_SIZE};
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_SLOT].object);
		for(int i=0;i<SLOT_H;++i){
			for(int j=0;j<SLOT_V;++j){
				slot.x=state->board[i][j].x;
				slot.y=state->board[i][j].y;
				draw(state,&slot);
			}
		}
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_SUB].object);
		for(int i=0;i<SHIPCOUNT_SUB;++i)
			draw(state,state->ship_sub+i);
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_DESTROYER].object);
		for(int i=0;i<SHIPCOUNT_DESTROYER;++i)
			draw(state,state->ship_destroyer+i);
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_CARRIER].object);
		for(int i=0;i<SHIPCOUNT_CARRIER;++i)
			draw(state,state->ship_carrier+i);
		
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTON].object);
		if(buttondraw(state,&cancelbutton)==BUTTON_ACTIVATE||state->back){
			state->back=false;
			return true;
		}
		if(buttondraw(state,&readybutton)==BUTTON_ACTIVATE){
			if(rematch){
				*readytoplay=true;
				return true;
			}
			if(!menu_connectto(state,readytoplay))return false;
			if(*readytoplay)return true;
		}
		if(buttondraw(state,&nextbutton)==BUTTON_ACTIVATE){
			place_ships(state);
		}
		glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->font.header->atlas);
		buttondrawtext(state->font.header,&cancelbutton);
		buttondrawtext(state->font.header,&readybutton);
		buttondrawtext(state->font.header,&nextbutton);
		eglSwapBuffers(state->display,state->surface);
	}
	return state->running;
}

int menu_end(struct state *state){
	struct button okbutton={0.5f,2.75f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,"OK",false};
	struct button rematchbutton={-4.5,2.75f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,"Rematch",false};
	state->gameboard.y=BOARD_Y;
	int buttontimer=90;
	char message[51];
	while(process(state->app)){
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BACKGROUND].object);
		draw(state,&state->background);
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BOARD].object);
		draw(state,&state->gameboard);
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_SUB].object);
		for(int i=0;i<SHIPCOUNT_SUB;++i){
			draw(state,&state->eship_sub[i]);
		}
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_DESTROYER].object);
		for(int i=0;i<SHIPCOUNT_DESTROYER;++i){
			draw(state,&state->eship_destroyer[i]);
		}
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_CARRIER].object);
		for(int i=0;i<SHIPCOUNT_CARRIER;++i){
			draw(state,&state->eship_carrier[i]);
		}
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_MARK].object);
		
		int colorred=false;
		glUniform4f(state->uniform.rgba,0.0f,0.5f,1.0f,1.0f);
		struct generic mark={.w=SLOT_SIZE,.h=SLOT_SIZE,.rot=0};
		for(int i=0;i<SLOT_H;++i){
			for(int j=0;j<SLOT_V;++j){
				if(state->opponent[i][j].marked){
					if(state->opponent[i][j].slot==0){
						if(colorred){
							glUniform4f(state->uniform.rgba,0.0f,0.5f,1.0f,1.0f);
							colorred=false;
						}
					}
					else{
						if(!colorred){
							glUniform4f(state->uniform.rgba,1.0f,0.0f,0.0f,1.0f);
							colorred=true;
						}
					}
					mark.x=state->opponent[i][j].x;
					mark.y=state->opponent[i][j].y;
					draw(state,&mark);
				}
			}
		}
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTON].object);
		if(buttondraw(state,&okbutton)==BUTTON_ACTIVATE||state->back){
			state->back=false;
			return true;
		}
		if(buttondraw(state,&rematchbutton)==BUTTON_ACTIVATE){
			int readytoplay=false;
			if(!menu_board(state,&readytoplay,true))return false;
			if(!readytoplay)return true;
			readytoplay=false;
			menu_rematch(state,&readytoplay);
			if(!readytoplay){
				sprintf(message,"%s doesn't want to play anymore.",state->opponentinitial);
				return menu_message(state,message);
			}
			return true;
		}
		
		glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->font.header->atlas);
		buttondrawtext(state->font.header,&okbutton);
		buttondrawtext(state->font.header,&rematchbutton);
		
		glBindTexture(GL_TEXTURE_2D,state->font.main->atlas);
		char message[50];
		sprintf(message,"%s's board",state->opponentinitial);
		drawtextcentered(state->font.main,0.0f,2.2f,message);
		eglSwapBuffers(state->display,state->surface);
		if(buttontimer>0)--buttontimer;
	}
	return state->running;
}

int menu_rematch(struct state *state,int *readytoplay){
	struct generic background={-6.25f,-3.75f,12.5f,7.5f,0.0f};
	glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BACKGROUND].object);
	draw(state,&state->background);
	glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_MESSAGE].object);
	draw(state,&background);
	glBindTexture(GL_TEXTURE_2D,state->font.main->atlas);
	glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
	drawtextcentered(state->font.main,0.0f,-state->font.main->fontsize/2.0f,"Starting new game...");
	eglSwapBuffers(state->display,state->surface);
	if(state->connector){
		if(!recv_info(state)||!send_info(state)){
			return true;
		}
		state->turn=REMOTE_TURN;
		state->offset=LOCAL_TURN_OFFSET;
	}
	else{
		if(!send_info(state)||!recv_info(state)){
			return true;
		}
		state->turn=LOCAL_TURN;
		state->offset=LOCAL_TURN_OFFSET;
	}
	state->connector=!state->connector;
	for(int i=0;i<SHIPCOUNT_SUB;++i)
		state->eship_sub[i].visible=false;
	for(int i=0;i<SHIPCOUNT_DESTROYER;++i)
		state->eship_destroyer[i].visible=false;
	for(int i=0;i<SHIPCOUNT_CARRIER;++i)
		state->eship_carrier[i].visible=false;
	*readytoplay=true;
	return true;
}

int menu_pause(struct state *state){
	struct button backbutton={5.0f,-3.5f,BUTTON_SMALL_SIZE,BUTTON_SMALL_SIZE,0.0f,"X",false};
	struct button vibbutton={-4.5f,1.75f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,"Vibration",false};
	struct button musicbutton={-4.5f,0.15f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,"Music",false};
	struct button soundsbutton={0.5f,0.15f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,"Sounds",false};
	struct button quitbutton={0.5f,1.75f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,"Quit",false};
	struct generic background={-6.25f,-3.75f,12.5f,7.5f,0.0f};
	char message[100];
	while(process(state->app)){
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BACKGROUND].object);
		draw(state,&state->background);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_MESSAGE].object);
		draw(state,&background);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTONSMALL].object);
		if(buttondraw(state,&backbutton)==BUTTON_ACTIVATE||state->back){
			state->back=false;
			return true;
		}
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTON].object);
		if(buttondraw(state,&vibbutton)==BUTTON_ACTIVATE){
			state->vibrationenabled=!state->vibrationenabled;
		}
		if(buttondraw(state,&musicbutton)==BUTTON_ACTIVATE){
			stopallsounds(state->soundengine);
			if(state->musicenabled)playsound(state->soundengine,state->aassets.sound+SID_NO,true);
			else playsound(state->soundengine,state->aassets.sound+SID_BATTLESHIP,true);
			state->musicenabled=!state->musicenabled;
		}
		if(buttondraw(state,&soundsbutton)==BUTTON_ACTIVATE){
			state->soundenabled=!state->soundenabled;
		}
		if(buttondraw(state,&quitbutton)==BUTTON_ACTIVATE){
			ANativeActivity_finish(state->app->activity);
		}
		glBindTexture(GL_TEXTURE_2D,state->font.header->atlas);
		glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
		buttondrawtext(state->font.header,&backbutton);
		buttondrawtext(state->font.header,&vibbutton);
		buttondrawtext(state->font.header,&musicbutton);
		buttondrawtext(state->font.header,&soundsbutton);
		buttondrawtext(state->font.header,&quitbutton);
		drawtextcentered(state->font.header,0.0f,-3.0f,"Paused");
		glBindTexture(GL_TEXTURE_2D,state->font.main->atlas);
		sprintf(message,"Music is %s\nSounds are %s\nVibration is %s",state->musicenabled?"on":"off",state->soundenabled?"on":"off",state->vibrationenabled?"on":"off");
		drawtextcentered(state->font.main,0.0f,-2.0f,message);
		eglSwapBuffers(state->display,state->surface);
	}
	return state->running;
}

int menu_connectto(struct state *state,int *readytoplay){
	struct button cancelbutton={0.5f,2.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,"Cancel",false};
	struct button connectbutton={-4.5f,2.0f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,NULL,false};
	struct generic background={-6.25f,-3.75f,12.5f,7.5f,0.0f};
	const int BUF_SIZE=50;
	int index=0;
	char buf[BUF_SIZE+1];
	buf[0]=0;
	struct button addrbutton[19]={
		{-5.5f,-1.7f,BUTTON_SMALL_SIZE,BUTTON_SMALL_SIZE,0.0f,"A",false},
		{-4.3f,-1.7f,BUTTON_SMALL_SIZE,BUTTON_SMALL_SIZE,0.0f,"B",false},
		{-5.5f,-0.5f,BUTTON_SMALL_SIZE,BUTTON_SMALL_SIZE,0.0f,"C",false},
		{-4.3f,-0.5f,BUTTON_SMALL_SIZE,BUTTON_SMALL_SIZE,0.0f,"D",false},
		{-5.5f,0.7f,BUTTON_SMALL_SIZE,BUTTON_SMALL_SIZE,0.0f,"E",false},
		{-4.3f,0.7f,BUTTON_SMALL_SIZE,BUTTON_SMALL_SIZE,0.0f,"F",false},
		{0.8f,-0.5f,BUTTON_SMALL_SIZE,BUTTON_SMALL_SIZE,0.0f,"0",false},
		{-2.8f,-1.7f,BUTTON_SMALL_SIZE,BUTTON_SMALL_SIZE,0.0f,"1",false},
		{-1.6f,-1.7f,BUTTON_SMALL_SIZE,BUTTON_SMALL_SIZE,0.0f,"2",false},
		{-0.4f,-1.7f,BUTTON_SMALL_SIZE,BUTTON_SMALL_SIZE,0.0f,"3",false},
		{-2.8f,-0.5f,BUTTON_SMALL_SIZE,BUTTON_SMALL_SIZE,0.0f,"4",false},
		{-1.6f,-0.5f,BUTTON_SMALL_SIZE,BUTTON_SMALL_SIZE,0.0f,"5",false},
		{-0.4f,-0.5f,BUTTON_SMALL_SIZE,BUTTON_SMALL_SIZE,0.0f,"6",false},
		{-2.8f,0.7f,BUTTON_SMALL_SIZE,BUTTON_SMALL_SIZE,0.0f,"7",false},
		{-1.6f,0.7f,BUTTON_SMALL_SIZE,BUTTON_SMALL_SIZE,0.0f,"8",false},
		{-0.4f,0.7f,BUTTON_SMALL_SIZE,BUTTON_SMALL_SIZE,0.0f,"9",false},
		{2.5f,-1.7f,BUTTON_SMALL_SIZE,BUTTON_SMALL_SIZE,0.0f,".",false},
		{2.5f,-0.5f,BUTTON_SMALL_SIZE,BUTTON_SMALL_SIZE,0.0f,":",false},
		{4.5,-1.7f,BUTTON_SMALL_SIZE,BUTTON_SMALL_SIZE,0.0f,"X",false}
	};
	while(process(state->app)){
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BACKGROUND].object);
		draw(state,&state->background);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_MESSAGE].object);
		draw(state,&background);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTON].object);
		if(buttondraw(state,&cancelbutton)==BUTTON_ACTIVATE||state->back){
			state->back=false;
			return true;
		}
		if(buttondraw(state,&connectbutton)==BUTTON_ACTIVATE){
			if(!menu_connect(state,buf,readytoplay))return false;
			if(*readytoplay)return true;
		}
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTONSMALL].object);
		for(int i=0;i<19;++i){
			if(buttondraw(state,addrbutton+i)==BUTTON_ACTIVATE){
				if(addrbutton[i].label[0]=='X'){
					if(index>0)buf[--index]=0;
				}
				else if(index!=BUF_SIZE&&!(addrbutton[i].label[0]==':'&&strstr(buf,"."))&&!(addrbutton[i].label[0]=='.'&&strstr(buf,":"))){
					buf[index++]=tolower(addrbutton[i].label[0]);
					buf[index]=0;
				}
				//logcat("index: %d",index);
			}
		}
		glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->font.header->atlas);
		buttondrawtext(state->font.header,&cancelbutton);
		connectbutton.label=strlen(buf)?"Ready":"Listen";
		buttondrawtext(state->font.header,&connectbutton);
		glBindTexture(GL_TEXTURE_2D,state->font.main->atlas);
		for(int i=0;i<19;++i){
			buttondrawtext(state->font.main,addrbutton+i);
		}
		drawtextcentered(state->font.main,0.0f,-3.5f,"Enter IP address or leave blank to listen");
		drawtextcentered(state->font.main,0.0f,-2.75f,buf);
		eglSwapBuffers(state->display,state->surface);
	}
	return state->running;
}

int menu_connect(struct state *state,char *buf,int *readytoplay){
	struct generic background={-6.25f,-3.75f,12.5f,7.5f,0.0f};
	struct button cancelbutton={-BUTTON_WIDTH/2.0f,1.8f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,"Cancel",false};
	char msg_connect[100];
	if(strlen(buf)){ // connecting
		struct addrinfo *ai=NULL,hints;
		memset(&hints,0,sizeof(struct addrinfo));
		hints.ai_family=AF_UNSPEC;
		hints.ai_socktype=SOCK_STREAM;
		hints.ai_protocol=IPPROTO_TCP;
		if(getaddrinfo(buf,NET_PORT_STR,&hints,&ai)){
			sprintf(msg_connect,"Unable to resolve address\n%s\nMake sure that it is a valid IPv4 or IPv6 address.",buf);
			return menu_message(state,msg_connect);
		}
		getnameinfo(ai->ai_addr,ai->ai_addrlen,state->connectto,51,NULL,0,NI_NUMERICHOST);
		char tempstring[51];
		strcpy(tempstring,state->connectto);
		if(strcasestr(state->connectto,"::ffff:"))
			strcpy(state->connectto,tempstring+7);
		state->sock=socket(ai->ai_family,SOCK_STREAM,IPPROTO_TCP);
		if(state->sock==-1)logcat("Unable to create socket");
		fcntl(state->sock,F_SETFL,fcntl(state->sock,F_GETFL,0)|O_NONBLOCK);
		int starting_time=time(NULL);
		int connect_result=1;
		while(time(NULL)-starting_time<5&&connect_result){
			connect_result=connect(state->sock,ai->ai_addr,ai->ai_addrlen);
			usleep(1000);
		}
		if(connect_result){
			freeaddrinfo(ai);
			sprintf(msg_connect,"Unable to connect to\n%s\nCheck your internet connection.",state->connectto);
			return menu_message(state,msg_connect);
		}
		freeaddrinfo(ai);
		if(!send_info(state)||!recv_info(state)){
			reset(state,true);
			return menu_message(state,"Connection error");
		}
		*readytoplay=true;
		state->connector=true;
		state->turn=LOCAL_TURN;
		state->offset=REMOTE_TURN_OFFSET;
		sprintf(msg_connect,"Connected to\n%s @ %s",state->opponentinitial,state->connectto);
		if(state->soundenabled)playsound(state->soundengine,state->aassets.sound+SID_BELL,false);
		return menu_message(state,msg_connect);
	}
	struct sockaddr_in6 scanaddr,sockaddr;
	int scan;
	int sockaddrlen;
	int reusesocket=true;
	memset(&scanaddr,0,sizeof(struct sockaddr_in6));
	scanaddr.sin6_family=AF_INET6;
	scanaddr.sin6_port=htons(NET_PORT);
	scanaddr.sin6_addr=in6addr_any;
	scan=socket(AF_INET6,SOCK_STREAM,IPPROTO_TCP);
	setsockopt(scan,SOL_SOCKET,SO_REUSEADDR,&reusesocket,sizeof(int));
	if(scan==-1)logcat("Unable to create listener socket");
	if(bind(scan,(struct sockaddr*)&scanaddr,sizeof(struct sockaddr_in6))){
		return menu_message(state,"Unable to bind to port "NET_PORT_STR"\nIf you continue to experience "
		"this problem,\nkill and restart the process");
	}
	fcntl(scan,F_SETFL,O_NONBLOCK);
	listen(scan,1);
	sockaddrlen=sizeof(struct sockaddr_in6);
	while(process(state->app)){
		state->sock=accept(scan,(struct sockaddr*)&sockaddr,&sockaddrlen);
		if(state->sock!=-1){
			fcntl(state->sock,F_SETFL,O_NONBLOCK);
			getnameinfo((struct sockaddr*)&sockaddr,sockaddrlen,state->connectto,51,NULL,0,NI_NUMERICHOST);
			char tempstring[51];
			strcpy(tempstring,state->connectto);
			if(strcasestr(state->connectto,"::ffff:"))
				strcpy(state->connectto,tempstring+7);
			close(scan);
			if(!recv_info(state)||!send_info(state)){
				reset(state,true);
				return menu_message(state,"Connection error");
			}
			*readytoplay=true;
			state->turn=REMOTE_TURN;
			state->connector=false;
			state->offset=LOCAL_TURN_OFFSET;
			sprintf(msg_connect,"Connected to\n%s @ %s",state->opponentinitial,state->connectto);
			if(state->soundenabled)playsound(state->soundengine,state->aassets.sound+SID_BELL,false);
			return menu_message(state,msg_connect);
		}
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BACKGROUND].object);
		draw(state,&state->background);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_MESSAGE].object);
		draw(state,&background);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTON].object);
		if(buttondraw(state,&cancelbutton)==BUTTON_ACTIVATE||state->back){
			state->back=false;
			close(scan);
			return true;
		}
		glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->font.header->atlas);
		buttondrawtext(state->font.header,&cancelbutton);
		glBindTexture(GL_TEXTURE_2D,state->font.main->atlas);
		sprintf(msg_connect,"Listening for connections on port "NET_PORT_STR"...");
		drawtextcentered(state->font.main,0.0f,-2.0f,msg_connect);
		eglSwapBuffers(state->display,state->surface);
	}
	return state->running;
}

int menu_message(struct state *state,const char *msg){
	struct generic background={-6.25f,-3.75f,12.5f,7.5f,0.0f};
	struct button okbutton={-BUTTON_WIDTH/2.0f,1.8f,BUTTON_WIDTH,BUTTON_HEIGHT,0.0f,"OK",false};
	while(process(state->app)){
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BACKGROUND].object);
		draw(state,&state->background);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_MESSAGE].object);
		draw(state,&background);
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTON].object);
		if(buttondraw(state,&okbutton)==BUTTON_ACTIVATE||state->back){
			state->back=false;
			return true;
		}
		glBindTexture(GL_TEXTURE_2D,state->font.header->atlas);
		glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
		buttondrawtext(state->font.header,&okbutton);
		glBindTexture(GL_TEXTURE_2D,state->font.main->atlas);
		drawtextcentered(state->font.main,0.0f,-3.0f,msg);
		eglSwapBuffers(state->display,state->surface);
	}
	return state->running;
}
