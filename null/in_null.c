/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// in_null.c -- for systems without a mouse
#include "../client/client.h"

#include "ds.h"

cvar_t	*in_mouse;
cvar_t	*in_joystick;

cvar_t	*ds_screen_clears;

short last_px = 128;
short last_py = 96;

int screen_shotting = 0;
unsigned short *screen_shot_addr = NULL;
bool use_arm7_bsp = false;
bool use_osk = false;

int addKeyboardEvents(void);

/*
void IN_Init (void)
{
}

void IN_Shutdown (void)
{
}

void IN_Commands (void)
{
}*/

void IN_Frame (void)
{
}

//void IN_Move (usercmd_t *cmd)
//{
//}
#if 1
touchPosition	g_lastTouch  = { 0,0,0,0 };
touchPosition	g_currentTouch = { 0,0,0,0 };

void IN_Move (usercmd_t *cmd)
{
	static u32 keys_last;
	u32 keys = keysCurrent();
	int dx,dy;

	//new touch
	if ((keys_last & KEY_TOUCH) == 0 && (keys & KEY_TOUCH) != 0) {
		touchRead(&g_lastTouch);
		g_lastTouch.px <<= 7;
		g_lastTouch.py <<= 7;
	} else if((keys_last & keys & KEY_TOUCH) != 0) {
		touchRead(&g_currentTouch);
		// let's use some fixed point magic to improve touch smoothing accuracy
		g_currentTouch.px <<= 7;
		g_currentTouch.py <<= 7;

		dx = (g_currentTouch.px - g_lastTouch.px) >> 6;
		dy = (g_currentTouch.py - g_lastTouch.py) >> 6;

		if (m_pitch->value > 0)
			cl.viewangles[PITCH] += ((dy * 2) * sensitivity->value / 11);
		else
			cl.viewangles[PITCH] -= ((dy * 2) * sensitivity->value / 11);

		cl.viewangles[YAW] -= ((dx * 2) * sensitivity->value / 11);

		// some simple averaging / smoothing through weightened (.5 + .5) accumulation
		g_lastTouch.px = (g_lastTouch.px + g_currentTouch.px) / 2;
		g_lastTouch.py = (g_lastTouch.py + g_currentTouch.py) / 2;
	}
	keys_last = keys;
}

#else
void IN_Move (usercmd_t *cmd)
{
	short px, py;
	static unsigned int last;
	unsigned int down = keysCurrent();


	if(down & KEY_TOUCH) {
		get_pen_pos(&px, &py);
		if(last & KEY_TOUCH) {
			if (m_pitch->value > 0)
				cl.viewangles[PITCH] += (((py - last_py) * 2) * sensitivity->value / 11);
			else
				cl.viewangles[PITCH] -= (((py - last_py) * 2) * sensitivity->value / 11);

			cl.viewangles[YAW] -= (((px - last_px) * 2) * sensitivity->value / 11);
		}
		last_px = px;
		last_py = py;
	}
	last = down;
}
#endif
void IN_Activate (qboolean active)
{
}

void IN_ActivateMouse (void)
{
}

void IN_DeactivateMouse (void)
{
}

void debug_msg(char *);
void get_pen_pos(short *px, short *py);

void IN_Init (void)
{
	in_mouse = Cvar_Get ("in_mouse", "1", CVAR_ARCHIVE);
    in_joystick = Cvar_Get ("in_joystick", "0", CVAR_ARCHIVE);
    
    ds_screen_clears = Cvar_Get("ds_screen_clears", "1", CVAR_ARCHIVE);
}

void IN_Shutdown (void)
{
}

extern bool ipc_disabled;
extern int doing_texture_loads;
extern cvar_t *r_drawworld;

u32 nds_keys[] = {
K_PAD_A,
K_PAD_B,
K_ENTER,//SELECT
K_ESCAPE,//START
K_RIGHTARROW,
K_LEFTARROW,
K_UPARROW,
K_DOWNARROW,
K_PAD_RIGHT,
K_PAD_LEFT,
K_PAD_X,
K_PAD_Y};

void IN_Buttons() {
	static u32 keys_last;
	u32 key_mask=1;
	u32 i;
	u32 keys = keysCurrent();
	
//	printf("down up %d %d\n", down, up);
	for(i=0;i<12;i++,key_mask<<=1) {
		if( (keys & key_mask) && !(keys_last & key_mask)) {
			//iprintf("pressed start\n");
			Key_Event (nds_keys[i], true,Sys_Milliseconds());
		} else if( !(keys & key_mask) && (keys_last & key_mask)) {
			//iprintf("released start\n");
			Key_Event (nds_keys[i], false,Sys_Milliseconds());
		}
	}
	keys_last = keys;
}

void IN_osk() {
	if (use_osk)
	{
		static int vkey_last = -1;
		int vkey_press = addKeyboardEvents();

		//key down
		if(vkey_last >= 0 && vkey_press == -1) {
			Key_Event(vkey_last, false, Sys_Milliseconds());
			vkey_last = -1;
		}
		
		//key up
		if (vkey_last = -1 && vkey_press >= 0)
		{
			Key_Event(vkey_press, true, Sys_Milliseconds());
			vkey_last = vkey_press;
		}
	}
}

void IN_Commands (void)
{	
	IN_osk();

	
//	if (screen_shotting)
//	{
//		screen_shotting++;
//		screen_shot_addr = ds_screen_capture();
//	}
//	if (screen_shotting > 5)
//	{
//		char filename[20];
//		sprintf(filename, "/screenshot%02d.raw", screen_shotting - 5);
//		FILE *file = fopen(filename, "wb"); 
//		if(file) 
//		{ 
//			fwrite((void*)screen_shot_addr, //memory location of the screen buffer - this might only apply to VRAM bank D, I'm not sure tbh 
//					2, //each pixel is two bytes 
//					256 * 192, //total number of pixels (49152) 
//					file); 
//			fclose(file); 
//		}
//	}
}

