#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android_native_app_glue.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include "defs.h"

int core(struct state *state){
	char message[90];
	if(state->showmenu){
		if(!menu_main(state))return false;
		state->showmenu=false;
		for(int i=0;i<SHIPCOUNT_SUB;++i)
			state->eship_sub[i].visible=false;
		for(int i=0;i<SHIPCOUNT_DESTROYER;++i)
			state->eship_destroyer[i].visible=false;
		for(int i=0;i<SHIPCOUNT_CARRIER;++i)
			state->eship_carrier[i].visible=false;
	}
	targetf(&state->offset,0.6f,state->turn==LOCAL_TURN?REMOTE_TURN_OFFSET:LOCAL_TURN_OFFSET);
	if(state->turn==LOCAL_TURN){
		for(int i=0;i<SLOT_H;++i){
			for(int j=0;j<SLOT_V;++j){
				if(state->pointer[0].active&&state->pointer[0].x>state->opponent[i][j].x&&state->pointer[0].x<state->opponent[i][j].x+SLOT_SIZE&&
				state->pointer[0].y>state->opponent[i][j].y&&state->pointer[0].y<state->opponent[i][j].y+SLOT_SIZE&&!state->opponent[i][j].marked&&!state->readyfornextturn){
					state->activeslot=&state->opponent[i][j];
				}
			}
		}
		if(state->activeslot){
			state->fbuttonstate=buttonprocess(state,&state->firebutton);
			if(state->fbuttonstate==BUTTON_ACTIVATE){
				if(!state->activeslot->marked){
					state->activeslot->marked=true;
					int stop=false;
					int water;
					for(int i=0;i<SLOT_H;++i){
						for(int j=0;j<SLOT_V;++j){
							if(&state->opponent[i][j]==state->activeslot){
								stop=true;
								state->coordinate[0]=i;
								state->coordinate[1]=j;
								break;
							}
						}
						if(stop)break;
					}
					if(!send_data_blocking(state->sock,state->coordinate,2)){
						reset(state,true);
						if(!menu_message(state,"Connection error"))return false;
						return core(state);
					}
					if(state->activeslot->slot==0){
						water=true;
						if(state->soundenabled)playsound(state->soundengine,state->aassets.sound+SID_SPLASH,false);
					}
					else{
						if(state->vibrationenabled)vibratedevice(&state->jni_info,VIBRATE_LENGTH);
						if(state->soundenabled)playsound(state->soundengine,state->aassets.sound+SID_EXPLOSION,false);
						newflash(state,state->activeslot->x+(SLOT_SIZE/2.0f),state->activeslot->y+(SLOT_SIZE/2.0f));
						water=false;
						//if(!menu_message(state,"HIT"))return false;
					}
					newsmoke(state,state->activeslot->x+(SLOT_SIZE/2.0f),state->activeslot->y+(SLOT_SIZE/2.0f),water);
					state->readyfornextturn=true;
					state->activeslot=NULL;
				}
			}
		}
	}
	else{
		int result;
		result=recv(state->sock,state->coordinate+state->bytestr,2-state->bytestr,0);
		if(result==0){
			reset(state,true);
			if(!menu_message(state,"Connection error"))return false;
			return core(state);
		}
		if(result>0)state->bytestr+=result;
		if(state->bytestr==2){
			state->bytestr=0;
			int water;
			state->board[state->coordinate[0]][state->coordinate[1]].marked=true;
			if(state->board[state->coordinate[0]][state->coordinate[1]].slot==0){
				//if(!menu_message(state,"MISS"))return false;
				if(state->soundenabled)playsound(state->soundengine,state->aassets.sound+SID_SPLASH,false);
				water=true;
			}
			else{
				water=false;
				if(state->vibrationenabled)vibratedevice(&state->jni_info,VIBRATE_LENGTH);
				if(state->soundenabled)playsound(state->soundengine,state->aassets.sound+SID_EXPLOSION,false);
				newflash(state,state->board[state->coordinate[0]][state->coordinate[1]].x+(SLOT_SIZE/2.0f),state->board[state->coordinate[0]][state->coordinate[1]].y+(SLOT_SIZE/2.0f));
			}
			newsmoke(state,state->board[state->coordinate[0]][state->coordinate[1]].x+(SLOT_SIZE/2.0f),state->board[state->coordinate[0]][state->coordinate[1]].y+(SLOT_SIZE/2.0f),water);
			state->readyfornextturn=true;
		}
	}
	if(state->readyfornextturn&&!state->smokelist){
		state->readyfornextturn=false;
		if(state->turn==LOCAL_TURN){
			struct board *activeslot=&state->opponent[state->coordinate[0]][state->coordinate[1]];
			if(activeslot->slot!=0){ // ship destroy check for local turn
				int stop=false;
				for(int i=0;i<SLOT_H;++i){
					for(int j=0;j<SLOT_V;++j){
						if(state->opponent[i][j].slot!=0&&!state->opponent[i][j].marked){
							stop=true;
							break;
						}
					}
					if(stop)break;
				}
				if(!stop){ // local win
					sprintf(message,"VICTORY\n\nYou have bested %s!",state->opponentinitial);
					if(!menu_message(state,message))return false;
					int connector=state->connector;
					if(!menu_end(state))return false;
					if(connector!=state->connector){ // rematch
						reset(state,false);
						return core(state);
					}
					reset(state,true);
					return core(state);
				}
				stop=false;
				for(int i=0;i<SLOT_H;++i){
					for(int j=0;j<SLOT_V;++j){
						if(state->opponent[i][j].slot!=0&&!state->opponent[i][j].marked&&state->opponent[i][j].id==activeslot->id){
							stop=true;
							break;
						}
					}
					if(stop)break;
				}
				if(!stop){
					const char *shipname;
					if(activeslot->slot==SHIPTYPE_SUB)shipname="You destroyed one of %s's Submarines!";
					else if(activeslot->slot==SHIPTYPE_DESTROYER)shipname="You sank one of %s's Destroyers!";
					else if(activeslot->slot==SHIPTYPE_CARRIER)shipname="You sank %s's Carrier!";
					sprintf(message,shipname,state->opponentinitial);
					if(!menu_message(state,message))return false;
					if(activeslot->slot==SHIPTYPE_SUB){
						for(int i=0;i<SHIPCOUNT_SUB;++i){
							if(activeslot->id==state->eship_sub[i].id){
								state->eship_sub[i].visible=true;
								break;
							}
						}
					}
					if(activeslot->slot==SHIPTYPE_DESTROYER){
						for(int i=0;i<SHIPCOUNT_DESTROYER;++i){
							if(activeslot->id==state->eship_destroyer[i].id){
								state->eship_destroyer[i].visible=true;
								break;
							}
						}
					}
					if(activeslot->slot==SHIPTYPE_CARRIER){
						for(int i=0;i<SHIPCOUNT_CARRIER;++i){
							if(activeslot->id==state->eship_carrier[i].id){
								state->eship_carrier[i].visible=true;
								break;
							}
						}
					}
				
				}
			}
			state->turn=REMOTE_TURN;
		}
		else{
			struct board *activeslot=&state->board[state->coordinate[0]][state->coordinate[1]];
			if(activeslot->slot!=0){ // ship destroy check for remote
				int stop=false;
				for(int i=0;i<SLOT_H;++i){
					for(int j=0;j<SLOT_V;++j){
						if(state->board[i][j].slot!=0&&!state->board[i][j].marked){
							stop=true;
							break;
						}
					}
					if(stop)break;
				}
				if(!stop){ // remote win
					sprintf(message,"DEFEAT\n\n%s has bested you!",state->opponentinitial);
					if(!menu_message(state,message))return false;
					int connector=state->connector;
					if(!menu_end(state))return false;
					if(connector!=state->connector){
						reset(state,false);
						return core(state);
					}
					reset(state,true);
					return core(state);
				}
				stop=false;
				for(int i=0;i<SLOT_H;++i){
					for(int j=0;j<SLOT_V;++j){
						if(state->board[i][j].slot!=0&&!state->board[i][j].marked&&state->board[i][j].id==activeslot->id){
							stop=true;
							break;
						}
					}
					if(stop)break;
				}
				if(!stop){
					const char *shipname;
					if(activeslot->slot==SHIPTYPE_SUB)shipname="%s destroyed one of your Submarines!";
					else if(activeslot->slot==SHIPTYPE_DESTROYER)shipname="%s sank one of your Destroyers!";
					else if(activeslot->slot==SHIPTYPE_CARRIER)shipname="%s sank your Carrier!";
					sprintf(message,shipname,state->opponentinitial);
					if(!menu_message(state,message))return false;
					if(activeslot->slot==SHIPTYPE_SUB){
						for(int i=0;i<SHIPCOUNT_SUB;++i){
							if(activeslot->id==state->ship_sub[i].id){
								state->ship_sub[i].visible=true;
								break;
							}
						}
					}
					if(activeslot->slot==SHIPTYPE_DESTROYER){
						for(int i=0;i<SHIPCOUNT_DESTROYER;++i){
							if(activeslot->id==state->ship_destroyer[i].id){
								state->ship_destroyer[i].visible=true;
								break;
							}
						}
					}
					if(activeslot->slot==SHIPTYPE_CARRIER){
						for(int i=0;i<SHIPCOUNT_CARRIER;++i){
							if(activeslot->id==state->ship_carrier[i].id){
								state->ship_carrier[i].visible=true;
								break;
							}
						}
					}
				}
			}
			state->turn=LOCAL_TURN;
		}
	}
	
	for(struct smoke *smoke=state->smokelist,*prevsmoke=NULL;smoke!=NULL;){
		if((smoke->alpha-=randomint(4,9)/1000.0f)<=0.0f){
			smoke=deletesmoke(state,smoke,prevsmoke);
			continue;
		}
		float increment=randomint(1,3)/100.0f;
		smoke->w+=increment;
		smoke->h=smoke->w;
		smoke->x+=(-increment/2.0f)+randomint(-5,5)/100.0f;
		smoke->y+=(-increment/2.0f)+randomint(-5,5)/100.0f;
		increment=randomint(1,3)/100.0f;
		smoke->rot+=smoke->direction?increment:-increment;
		prevsmoke=smoke;
		smoke=smoke->next;
	}
	
	for(struct flash *flash=state->flashlist,*prevflash=NULL;flash!=NULL;){
		if(flash->alpha<=0.0f){
			flash=deleteflash(state,flash,prevflash);
			continue;
		}
		flash->basealpha-=0.01f;
		flash->alpha=flash->basealpha+(randomint(-5,5)/100.0f);
		prevflash=flash;
		flash=flash->next;
	}
	if(state->back){
		state->back=false;
		if(!menu_pause(state))return false;
	}
	
	return true;
}

void render(struct state *state){
	char message[90];
	glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
	glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BACKGROUND].object);
	draw(state,&state->background);
	
	// draw game boards
	glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_BOARD].object);
	if(state->offset!=LOCAL_TURN_OFFSET){ //remote board
		state->gameboard.y=state->rect.top+state->offset+0.5f;
		draw(state,&state->gameboard);
	}
	if(state->offset!=REMOTE_TURN_OFFSET){ // local board
		state->gameboard.y=state->offset+BOARD_OFFSET+state->rect.top+0.5f;
		draw(state,&state->gameboard);
	}
	//draw subs
	glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_SUB].object);
	struct generic ship={.w=2.0f,.h=1.0f};
	if(state->offset!=LOCAL_TURN_OFFSET){ // remote subs
		for(int i=0;i<SHIPCOUNT_SUB;++i){
			if(!state->eship_sub[i].visible)continue;
			ship.x=state->eship_sub[i].x;
			ship.y=state->eship_sub[i].y+state->offset;
			ship.rot=state->eship_sub[i].rot;
			draw(state,&ship);
		}
	}
	if(state->offset!=REMOTE_TURN_OFFSET){ //local subs
		for(int i=0;i<SHIPCOUNT_SUB;++i){
			ship.x=state->ship_sub[i].x;
			ship.y=state->ship_sub[i].y+BOARD_OFFSET+state->offset;
			ship.rot=state->ship_sub[i].rot;
			draw(state,&ship);
		}
	}
	//draw destroyers
	glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_DESTROYER].object);
	ship.w=SHIPSIZE_DESTROYER;
	if(state->offset!=LOCAL_TURN_OFFSET){ // remote destroyers
		for(int i=0;i<SHIPCOUNT_DESTROYER;++i){
			if(!state->eship_destroyer[i].visible)continue;
			ship.x=state->eship_destroyer[i].x;
			ship.y=state->eship_destroyer[i].y+state->offset;
			ship.rot=state->eship_destroyer[i].rot;
			draw(state,&ship);
		}
	}
	if(state->offset!=REMOTE_TURN_OFFSET){ //local destroyers
		for(int i=0;i<SHIPCOUNT_DESTROYER;++i){
			ship.x=state->ship_destroyer[i].x;
			ship.y=state->ship_destroyer[i].y+BOARD_OFFSET+state->offset;
			ship.rot=state->ship_destroyer[i].rot;
			draw(state,&ship);
		}
	}
	//draw carriers
	glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_CARRIER].object);
	ship.w=SHIPSIZE_CARRIER;
	if(state->offset!=LOCAL_TURN_OFFSET){ // remote carriers
		for(int i=0;i<SHIPCOUNT_CARRIER;++i){
			if(!state->eship_carrier[i].visible)continue;
			ship.x=state->eship_carrier[i].x;
			ship.y=state->eship_carrier[i].y+state->offset;
			ship.rot=state->eship_carrier[i].rot;
			draw(state,&ship);
		}
	}
	if(state->offset!=REMOTE_TURN_OFFSET){ //local carriers
		for(int i=0;i<SHIPCOUNT_CARRIER;++i){
			ship.x=state->ship_carrier[i].x;
			ship.y=state->ship_carrier[i].y+BOARD_OFFSET+state->offset;
			ship.rot=state->ship_carrier[i].rot;
			draw(state,&ship);
		}
	}
	glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_MARK].object);
	if(state->turn==LOCAL_TURN){
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
					mark.y=state->opponent[i][j].y+state->offset;
					draw(state,&mark);
				}
			}
		}
	}
	if(state->turn==REMOTE_TURN){
		int colorred=false;
		glUniform4f(state->uniform.rgba,0.0f,0.5f,1.0f,1.0f);
		struct generic mark={.w=SLOT_SIZE,.h=SLOT_SIZE,.rot=0};
		for(int i=0;i<SLOT_H;++i){
			for(int j=0;j<SLOT_V;++j){
				if(state->board[i][j].marked){
					if(state->board[i][j].slot==0){
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
					mark.x=state->board[i][j].x;
					mark.y=state->board[i][j].y+BOARD_OFFSET+state->offset;
					draw(state,&mark);
				}
			}
		}
	}
	
	if(state->flashlist)glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_FLASH].object);
	for(struct flash *flash=state->flashlist;flash!=NULL;flash=flash->next){
		glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,flash->alpha);
		draw(state,flash);
	}
	if(state->smokelist)glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_SMOKE].object);
	for(struct smoke *smoke=state->smokelist;smoke!=NULL;smoke=smoke->next){
		if(smoke->water)
			glUniform4f(state->uniform.rgba,0.0f,1.0f,1.0f,smoke->alpha/1.35f);
		else
			glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,smoke->alpha);
		draw(state,smoke);
	}
	
	glUniform1f(state->uniform.rot,0.0f);
	if(state->activeslot){
		if(state->offset==REMOTE_TURN_OFFSET){
			struct generic slot={state->activeslot->x,state->activeslot->y,SLOT_SIZE,SLOT_SIZE,0.0f};
			glUniform4f(state->uniform.rgba,1.75f,1.75f,1.75f,1.0f);
			glBindTexture(GL_TEXTURE_2D,state->assets.texture[TID_SLOT].object);
			draw(state,&slot);
		}
		glBindTexture(GL_TEXTURE_2D,state->uiassets.texture[TID_BUTTON].object);
		if(state->fbuttonstate)glUniform4f(state->uniform.rgba,0.7f,0.7f,0.7f,1.0f);
		else glUniform4f(state->uniform.rgba,1.0f,1.0f,1.0f,1.0f);
		draw(state,&state->firebutton);
		glBindTexture(GL_TEXTURE_2D,state->font.header->atlas);
		glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
		buttondrawtext(state->font.header,&state->firebutton);
	}
	else glUniform4f(state->uniform.rgba,0.0f,0.0f,0.0f,1.0f);
	glBindTexture(GL_TEXTURE_2D,state->font.main->atlas);
	if(state->offset!=LOCAL_TURN_OFFSET){
		sprintf(message,"~ %s's board ~\n",state->opponentinitial);
		if(!state->activeslot){
			strcat(message,"Tap a slot to enter fire coordinates");
		}
		drawtextcentered(state->font.main,0.0f,2.2f+state->offset,message);
	}
	if(state->offset!=REMOTE_TURN_OFFSET){
		sprintf(message,"~ Your board ~\nWaiting for %s to make a move...",state->opponentinitial);
		drawtextcentered(state->font.main,0.0f,2.2f+BOARD_OFFSET+state->offset,message);
	}
}

void init(struct state *state){
	state->showmenu=true;
	state->soundenabled=true;
	state->musicenabled=true;
	state->vibrationenabled=true;
	state->back=false;
	memset(state->pointer,0,sizeof(struct crosshair)*2);
	state->rect.right=8.0f;
	state->rect.left=-8.0f;
	state->rect.bottom=4.5f;
	state->rect.top=-4.5f;
	state->background.w=state->rect.right*2.0f;
	state->background.h=state->rect.bottom*2.0f;
	state->background.x=state->rect.left;
	state->background.y=state->rect.top;
	state->background.rot=0.0f;
	state->menubackground=state->background;
	state->sock=-1;
	FILE *file=fopen("data/data/joshwinter.battleship/files/battleship.dat","rb");
	if(file){
		fread(state->initial,1,3,file);
		fclose(file);
	}
	else{
		state->initial[0]='A';
		state->initial[1]='A';
		state->initial[2]='A';
	}
	state->initial[3]=0;
	state->gameboard.x=BOARD_X;
	state->gameboard.w=SLOT_H*SLOT_SIZE;
	state->gameboard.h=SLOT_V*SLOT_SIZE;
	state->gameboard.rot=0.0f;
	state->firebutton.x=-BUTTON_WIDTH/2.0f;
	state->firebutton.y=2.75f;
	state->firebutton.w=BUTTON_WIDTH;
	state->firebutton.h=BUTTON_HEIGHT;
	state->firebutton.rot=0.0f;
	state->firebutton.label="FIRE";
	state->firebutton.active=false;
	state->smokelist=NULL;
	state->flashlist=NULL;
}
void reset(struct state *state,int discon){
	if(discon){
		if(state->sock!=-1){
			close(state->sock);
			state->sock=-1;
		}
		state->showmenu=true;
	}
	state->fbuttonstate=0;
	state->bytestr=0;
	state->activeslot=NULL;
	state->readyfornextturn=false;
	for(struct smoke *smoke=state->smokelist;smoke!=NULL;smoke=deletesmoke(state,smoke,NULL));
	state->smokelist=NULL;
	for(struct flash *flash=state->flashlist;flash!=NULL;flash=deleteflash(state,flash,NULL));
	state->flashlist=NULL;
}
