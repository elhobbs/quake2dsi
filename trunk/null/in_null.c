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

void IN_Commands (void)
{
	unsigned int down = keysHeld();
	unsigned int pressed = keysDown();
	unsigned int up = keysUp();
	
//	printf("down up %d %d\n", down, up);
	
	if (down || up)
	{
		if ((down & 1024) == 1024)	//X
			Key_Event(K_PAD_X, true, Sys_Milliseconds());
		if ((up & 1024) == 1024)	//X
			Key_Event(K_PAD_X, false, Sys_Milliseconds());
		
		if ((down & 2048) == 2048)	//Y
			Key_Event(K_PAD_Y, true, Sys_Milliseconds());
		if ((up & 2048) == 2048)	//Y
			Key_Event(K_PAD_Y, false, Sys_Milliseconds());
		
		if ((down & 1) == 1)	//A
			Key_Event(K_PAD_A, true, Sys_Milliseconds());
//		{
//			screen_shotting = 1;
//		}
		if ((up & 1) == 1)	//A
			Key_Event(K_PAD_A, false, Sys_Milliseconds());
		
		if ((down & 2) == 2)	//B
			Key_Event(K_PAD_B, true, Sys_Milliseconds());
		if ((up & 2) == 2)	//B
//			use_arm7_bsp = !use_arm7_bsp;
//			while(1);
//			doing_texture_loads = !doing_texture_loads;
			Key_Event(K_PAD_B, false, Sys_Milliseconds());
//			r_drawworld->value = !r_drawworld->value;
		
		if ((pressed & 4) == 4)		//select
			Key_Event(K_ENTER, true, Sys_Milliseconds());
		if ((up & 4) == 4)		//select
			Key_Event(K_ENTER, false, Sys_Milliseconds());
			
		if ((pressed & 8) == 8)		//start
			Key_Event(K_ESCAPE, true, Sys_Milliseconds());
		if ((up & 8) == 8)		//start
			Key_Event(K_ESCAPE, false, Sys_Milliseconds());
		
		if ((down & 16) == 16)		//right
			Key_Event(K_RIGHTARROW, true, Sys_Milliseconds());
		if ((up & 16) == 16)		//right
			Key_Event(K_RIGHTARROW, false, Sys_Milliseconds());
		
		if ((down & 32) == 32)		//left
			Key_Event(K_LEFTARROW, true, Sys_Milliseconds());
		if ((up & 32) == 32)		//left
			Key_Event(K_LEFTARROW, false, Sys_Milliseconds());
		
		if ((down & 64) == 64)		//up
			Key_Event(K_UPARROW, true, Sys_Milliseconds());
		if ((up & 64) == 64)		//up
			Key_Event(K_UPARROW, false, Sys_Milliseconds());
		
		if ((down & 128) == 128)	//down
			Key_Event(K_DOWNARROW, true, Sys_Milliseconds());
		if ((up & 128) == 128)		//down
			Key_Event(K_DOWNARROW, false, Sys_Milliseconds());
			
		if ((down & 256) == 256)	//R
			Key_Event(K_PAD_RIGHT, true, Sys_Milliseconds());
		if ((up & 256) == 256)		//R
			Key_Event(K_PAD_RIGHT, false, Sys_Milliseconds());
			
		if ((down & 512) == 512)	//L
			Key_Event(K_PAD_LEFT, true, Sys_Milliseconds());
		if ((up & 512) == 512)		//L
			Key_Event(K_PAD_LEFT, false, Sys_Milliseconds());
	}
	
	if (use_osk)
	{
		int vkey_press = addKeyboardEvents();
		
		if (vkey_press >= 0)
		{
			Key_Event(vkey_press, true, Sys_Milliseconds());
			Key_Event(vkey_press, false, Sys_Milliseconds());
		}
	}
	
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

