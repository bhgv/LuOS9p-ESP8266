
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include <sys/mutex.h>


#include "lua.h"
#include "lauxlib.h"
#include "lmem.h"

#include "modules.h"

#include "ssd1306_2/ssd1306.h"
#include "pcf8574/pcf8575.h"
#include "pca9685/pca9685.h"



#define ADDR   SSD1306_I2C_ADDR_0

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

#define NO_SLEEP_DELAY  100

#define SLEEP_PWM_CH	6


#define NO_MSG 		0
#define MSG_GO_TOP 	1
#define MSG_EXIT 	2


#define _ok 1<<8
#define _lft 1<<9
#define _rgt 1<<10
#define _up 1<<11
#define _dwn 1<<12
#define _lpg 1<<13
#define _rpg 1<<14
#define _ekey 1<<15


extern uint8_t ssd1306_buffer[]; 
extern font_info_t *font;
extern font_face_t font_face;

extern QueueHandle_t cbqueue;

static char menu;
static char m_cur_pos = 1;

static char menu_cur=-1;
static char menu_cur_len = 0;

static int no_sleep = 0;

static char msg=NO_MSG;

static char gui_fnt=0;
#define GUI_FNT gui_fnt

static void draw_menu( lua_State *L, int menu_cur){
	font_info_t *fnt_bk=font;
	font = font_builtin_fonts[GUI_FNT];
//printf("draw_menu fnt %x\n", font);
    int m_str_step = (font->height + 4);
//#define m_scr_lns (int)(64/m_str_step)
    int i, yc;
    int y = 0; 
    int bg = 1;
//	menu_cur_len = lua_rawlen(L, menu_cur);
	char* s;
	int n = lua_gettop( L);

/*
    if(menu_cur_len > 6 && m_cur_pos >= 6/2){
		if( m_cur_pos >menu_cur_len - 6/2 ){
		    bg = menu_cur_len - 6 + 1;
		}else{
		    bg = m_cur_pos - 6/2 + 1;
		}
    }
*/
    if(menu_cur_len*m_str_step > 64-m_str_step && m_cur_pos*m_str_step >= (64/2)/*-m_str_step*/){
		if( m_cur_pos*m_str_step >menu_cur_len*m_str_step - 64/2 ){
		    bg = (int)((menu_cur_len*m_str_step - 64)/m_str_step) +1;
		}else{
		    bg =(int)(( m_cur_pos*m_str_step - 64/2)/m_str_step) + 1;
		}
    }
    for( i = bg; i<=menu_cur_len; i++ ){
		lua_rawgeti(L, menu_cur, i);
		if( i == m_cur_pos ){
			ssd1306_draw_hline(ADDR, ssd1306_buffer, 5, y+m_str_step, 128-10, 1);
			ssd1306_fill_rectangle(ADDR, ssd1306_buffer, 2, y+3, 4, 4, 1 );
		}
		lua_pushliteral(L,"name");
		lua_rawget( L, -2);
		if( lua_isstring(L, -1) ){
			s = lua_tostring(L, -1);
			ssd1306_draw_string(ADDR, ssd1306_buffer, font, 11, y+1, s, 1, 0 );
		}
		lua_pop(L, 1);
		
		lua_pushliteral(L,"ind_t");
		lua_rawget( L, -2);
		if( lua_isfunction(L, -1) ){
			usleep(50);
			lua_pushinteger( L, y);
			lua_pushliteral(L,"par");
			lua_rawget( L, -4);
			lua_call(L, 2, 0);
		} else{
			lua_pop(L, 1);
		}
		luaC_fullgc(L, 1);
		usleep(50);
		y = y + m_str_step;
		if( y >= 64 /*-m_str_step*/)
			break;
    }

	font=fnt_bk;
	lua_settop( L, n);
	luaC_fullgc(L, 1);
}

static void draw( lua_State *L){
	int i;
	for(i=0; i<(DISPLAY_WIDTH * DISPLAY_HEIGHT/8); i++)
		ssd1306_buffer[i] = 0;
	draw_menu(L, menu_cur);
//	lua_gc(L, LUA_GCCOLLECT, 0);
	ssd1306_load_frame_buffer(ADDR, ssd1306_buffer);
}

static int new_cur_m( lua_State *L, int m ){
	luaL_dofile(L, lua_tostring(L, m) );
	menu_cur_len = lua_rawlen(L, -1);
	luaC_fullgc(L, 1);
	return lua_gettop(L);
}

static int new_menu( lua_State *L, int m ){
	lua_settop( L, m);
	menu=m; 
	menu_cur=new_cur_m(L, m);
	m_cur_pos=1;
	draw(L);
}

static int sub_menu( lua_State *L, int m ){
    int i=new_cur_m(L, m);
	lua_replace(L,menu_cur);
	luaC_fullgc(L, 1);

	m_cur_pos = 1;
	draw(L);
}

static int top_menu( lua_State *L){
	msg=MSG_GO_TOP;
	//new_menu(L, menu);
	//lua_gc(L, LUA_GCCOLLECT, 0);
	return 0;
}

static int exit_menu( lua_State *L){
	msg=MSG_EXIT;
	return 0;
}

static int set_gui_fnt( lua_State *L){
	gui_fnt=luaL_checkinteger( L, 1);
	return 0;
}

static int menu_up( lua_State *L){
	m_cur_pos=m_cur_pos-1;
	if( m_cur_pos<1 ){
	    m_cur_pos=1; 
	}else{
	    draw(L);
	}
	return m_cur_pos;
}

static int menu_down( lua_State *L){
	m_cur_pos=m_cur_pos+1;
	if( m_cur_pos>menu_cur_len ){
	    m_cur_pos=menu_cur_len; 
	}else{
	    draw(L);
	}
	return m_cur_pos;
}

/*
local menu_pos=function(pos)
	local old = m_cur_pos;
	if pos then
	    m_cur_pos = pos;
	    draw(draw_menu);
	end
	return old;
end
*/

static void gui_screen_lightup( lua_State *L){
	if(no_sleep <= 0){
		ssd1306_display_on(ADDR, true);
		ssd1306_set_contrast(ADDR, 0x9f) ;

		pca9685_set_pwm_value(PCA9685_ADDR_BASE, SLEEP_PWM_CH, 4095 - 100);
	}
	no_sleep = NO_SLEEP_DELAY;
}

static void gui_controller( lua_State *L){
	int n = lua_gettop( L);
	int b = pcf8575_port_read(PCF8575_DEFAULT_ADDRESS);
	int i=m_cur_pos;
	int cur_ln; //=menu_cur[i];
	int m=0; //cur_ln.menu
	int act=0; //cur_ln.act
	int par=0; //cur_ln.par

//	int menu_cur_len = lua_rawlen(L, menu_cur);
	lua_rawgeti( L, menu_cur, i);
	cur_ln = lua_gettop(L);

	lua_pushliteral(L,"menu");
	lua_rawget( L, cur_ln);
	if(!lua_isnoneornil(L, -1)){
 		m = lua_gettop(L);
	}else{
		lua_pop(L, 1);
	}

	lua_pushliteral(L,"act");
	lua_rawget( L, cur_ln);
	if(!lua_isnoneornil(L, -1)){
	 	act = lua_gettop(L);
	}else{
		lua_pop(L, 1);
	}

	lua_pushliteral(L,"par");
	lua_rawget( L, cur_ln);
	if(!lua_isnoneornil(L, -1)){
	 	par = lua_gettop(L);
	}else{
		lua_pop(L, 1);
	}

  if((b & _ok) == 0 ){
  	gui_screen_lightup(L);
    if( i == menu_cur_len ){
		lua_settop( L, menu);
		new_menu(L, menu);
    }else if( m != 0 ){
		lua_settop( L, m);
		sub_menu(L, m);
    }else if( act != 0 /*and act.ok ~= nil*/ ){
//	    act.ok(par);
		lua_pushliteral(L,"ok");
		lua_rawget( L, act);
		if(lua_isfunction(L, -1)){
			usleep(50);
			lua_pushvalue(L, par);
			lua_call(L, 1, 0);
		}else{
			lua_pop(L, 1);
		}
		luaC_fullgc(L, 1);
		usleep(50);
    }
  }
  else if((b & _lft) == 0 ){
  	gui_screen_lightup(L);
    if( act != 0 /*and act.lft ~= nil*/ ){
//	  act.lft(par);
	  lua_pushliteral(L,"lft");
	  lua_rawget( L, act);
	  if(lua_isfunction(L, -1)){
	  	usleep(50);
		lua_pushvalue(L, par);
		lua_call(L, 1, 0);
	  }else{
		lua_pop(L, 1);
	  }
	  luaC_fullgc(L, 1);
	  usleep(50);
	  draw(L);
    }else if( m != 0 ){
	  sub_menu(L, m);
    }
  }
  else if((b & _rgt) == 0 ){
  	gui_screen_lightup(L);
    if( act != 0 /*and act.rgt ~= nil*/ ){
//	  act.rgt(par);
	  lua_pushliteral(L,"rgt");
	  lua_rawget( L, act);
	  if(lua_isfunction(L, -1)){
	  	usleep(50);
		lua_pushvalue(L, par);
		lua_call(L, 1, 0);
	  }else{
		lua_pop(L, 1);
	  }
	  luaC_fullgc(L, 1);
	  usleep(50);
	  draw(L);
    }else if( m != 0 ){
	  sub_menu(L, m);
    }
  }
  else if((b & _up) == 0 ){
  	gui_screen_lightup(L);
    menu_up(L);
  }
  else if((b & _dwn) == 0 ){
  	gui_screen_lightup(L);
    menu_down(L);
  }
  else if((b & _lpg) == 0 ){
  	gui_screen_lightup(L);
    if( act != 0 /*and act.lpg != 0*/ ){
//	  act.lpg(par);
	  lua_pushliteral(L,"lpg");
	  lua_rawget( L, act);
	  if(lua_isfunction(L, -1)){
	  	usleep(50);
		lua_pushvalue(L, par);
		lua_call(L, 1, 0);
	  }else{
		lua_pop(L, 1);
	  }
	  luaC_fullgc(L, 1);
	  usleep(50);
	  draw(L);
    }else if( m != 0 ){
	  sub_menu(L, m);
    }
  }
  else if((b & _rpg) == 0 ){
  	gui_screen_lightup(L);
    if( act != 0 /*and act.rpg != 0*/ ){
//	  act.rpg(par);
	  lua_pushliteral(L,"rpg");
	  lua_rawget( L, act);
	  if(lua_isfunction(L, -1)){
	  	usleep(50);
		lua_pushvalue(L, par);
		lua_call(L, 1, 0);
	  }else{
		lua_pop(L, 1);
	  }
	  luaC_fullgc(L, 1);
	  usleep(50);
	  draw(L);
    }else if( m != 0 ){
	  sub_menu(L, m);
    }
  }
  else if((b & _ekey) == 0 ){
  }
	lua_settop( L, n);
}


extern int (*cb_httpd)(lua_State *L);

static void _cb_task (lua_State *L/*, int n, int cnt*/ ) { 
	  uint32_t v;
//	  int i = cnt;
	  while(1) {
		  if(!portIN_ISR()){
//			  if(cnt > 0) i--;
//			  mtx_lock(&tloop_mtx);
			  if(xQueueReceive(cbqueue, &v, 1024)) {
				  if(v > 0) { 
					gui_controller(L);
					switch(msg){
						case MSG_GO_TOP:
							msg=NO_MSG;
							new_menu(L, menu);
							break;
						case MSG_EXIT:
							msg=NO_MSG;
							//lua_gc(L, LUA_GCCOLLECT, 0);
							luaC_fullgc(L, 1);
							return;
							break;
					}
					lua_gc(L, LUA_GCCOLLECT, 0);

				  	if(no_sleep > 0){
						//ssd1306_display_on(ADDR, true);
						//ssd1306_set_contrast(ADDR, 0x9f) ;
				  		no_sleep--;
					}else if(0x9f+no_sleep > 0){
				  		no_sleep--;
						ssd1306_set_contrast(ADDR, 0x9f+no_sleep) ;
				  	}else{
						static uint16_t blnk = 0;
						char ch;
						ssd1306_display_on(ADDR, false);
						if(blnk & (1<<9))
							blnk=0;
						//if(blnk & (1<<7))
						//	ch=5;
						//else
							ch=SLEEP_PWM_CH;
						
						if(blnk & (1<<8))
							pca9685_set_pwm_value(PCA9685_ADDR_BASE, ch, 
								4095 - ((1<<8)-1 -(blnk&((1<<8)-1))));
						else
							pca9685_set_pwm_value(PCA9685_ADDR_BASE, ch, 
								4095 - (blnk&((1<<8)-1)));

						blnk+=1<<5;
							
				  	}
				  }

				  if(cb_httpd != NULL){
				  	cb_httpd(L);
				  }
			  }
//			  mtx_unlock(&tloop_mtx);
		  }
	  }
  }

static int main_loop( lua_State *L ){
	//char *m;
	if(lua_isstring(L,1)){
		gui_screen_lightup(L);
		new_menu(L, 1);
		_cb_task(L);
	}
	return 0;
}


static const LUA_REG_TYPE lgui_map[] = {
  { LSTRKEY( "run" ), 		LFUNCVAL( main_loop) },
  { LSTRKEY( "setFont" ), 	LFUNCVAL( set_gui_fnt) },
  { LSTRKEY( "gotop" ),		LFUNCVAL( top_menu) },
  { LSTRKEY( "exit" ), 		LFUNCVAL(exit_menu) },
  { LNILKEY, LNILVAL }
};


int luaopen_gui( lua_State *L ) {
  return 0;
}


MODULE_REGISTER_MAPPED(GUI, gui, lgui_map, luaopen_gui);


