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
#include "../ref_soft/r_local.h"
#include "ds.h"

void ds_proxy_reset_render(void)
{
	ds_reset_render();
}

void ds_proxy_begin_render(void)
{
	ds_begin_render();
}

void ds_proxy_end_render(void)
{
	ds_end_render();
}


void		SWimp_BeginFrame( float camera_separation )
{
	ds_proxy_reset_render();
	ds_proxy_begin_render();
}

void		SWimp_EndFrame (void)
{
	ds_proxy_end_render();
}

int			SWimp_Init( void *hInstance, void *wndProc )
{
	return 0;
}

void		SWimp_SetPalette( const unsigned char *palette)
{
#ifdef ARM9
	ds_schedule_loadpalette((byte *)palette);
#else
	ds_loadpalette((byte *)palette);
#endif
}

void		SWimp_Shutdown( void )
{
}

unsigned char *frame_buffer;

extern int count_largest_block_kb(void);

rserr_t		SWimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen )
{
	ri.Vid_GetModeInfo(pwidth, pheight, mode);
	
	SWimp_Shutdown();
	
	vid.width = *pwidth;
	vid.height = *pheight;
	
	ri.Vid_NewWindow(vid.width, vid.height);
	vid.rowbytes = vid.width;
	
//	printf("%.2f kb pre-fb\n", (float)count_largest_block_kb() / 1024);
//
////	ds_set_malloc_base(MEM_XTRA);
//	vid.buffer = Z_Malloc(vid.width * vid.height);
////	ds_set_malloc_base(MEM_MAIN);
//
//	printf("%.2f kb post-fb\n", (float)count_largest_block_kb() / 1024);
	
	vid.buffer = NULL;
	
//	if (vid.buffer == NULL)
//	{
//		printf("not enough memory for framebuffer\n");
//		*(int *)0 = 0;
//	}
//	else
//		printf("framebuffer lives at %08x\n", vid.buffer);
//	
//	ds_memset(vid.buffer, 0, vid.width * vid.height);
	
	frame_buffer = vid.buffer;
	
	return rserr_ok;
}

void		SWimp_AppActivate( qboolean active )
{
}

