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
// vid_null.c -- null video driver to aid porting efforts
// this assumes that one of the refs is statically linked to the executable

#include "../client/client.h"
#include "../client/qmenu.h"

viddef_t	viddef;				// global video state

refexport_t	re;

refexport_t GetRefAPI (refimport_t rimp);

static menuframework_s  s_software_menu;

/*
==========================================================================

DIRECT LINK GLUE

==========================================================================
*/

#define	MAXPRINTMSG	4096
void VID_Printf (int print_level, char *fmt, ...)
{
        va_list		argptr;
        char		msg[MAXPRINTMSG];

        va_start (argptr,fmt);
        vsprintf (msg,fmt,argptr);
        va_end (argptr);

        if (print_level == PRINT_ALL)
                Com_Printf ("%s", msg);
        else
                Com_DPrintf ("%s", msg);
}

void VID_Error (int err_level, char *fmt, ...)
{
        va_list		argptr;
        char		msg[MAXPRINTMSG];

        va_start (argptr,fmt);
        vsprintf (msg,fmt,argptr);
        va_end (argptr);

		Com_Error (err_level, "%s", msg);
}

void VID_NewWindow (int width, int height)
{
        viddef.width = width;
        viddef.height = height;
}

/*
** VID_GetModeInfo
*/
typedef struct vidmode_s
{
    const char *description;
    int         width, height;
    int         mode;
} vidmode_t;

vidmode_t vid_modes[] =
{
    { "Mode 0: 256x192",   256, 192,   0 },
};
#define VID_NUM_MODES ( sizeof( vid_modes ) / sizeof( vid_modes[0] ) )

qboolean VID_GetModeInfo( int *width, int *height, int mode )
{
    if ( mode < 0 || mode >= VID_NUM_MODES )
        return false;

    *width  = vid_modes[mode].width;
    *height = vid_modes[mode].height;

    return true;
}

void printw(char *str);

void	VID_Init (void)
{
    refimport_t	ri;

    viddef.width = 256;
    viddef.height = 192;

    ri.Cmd_AddCommand = Cmd_AddCommand;
    ri.Cmd_RemoveCommand = Cmd_RemoveCommand;
    ri.Cmd_Argc = Cmd_Argc;
    ri.Cmd_Argv = Cmd_Argv;
    ri.Cmd_ExecuteText = Cbuf_ExecuteText;
    ri.Con_Printf = VID_Printf;
    ri.Sys_Error = VID_Error;
    ri.FS_LoadFile = FS_LoadFile;
    ri.FS_FreeFile = FS_FreeFile;
    ri.FS_Gamedir = FS_Gamedir;
	ri.Vid_NewWindow = VID_NewWindow;
    ri.Cvar_Get = Cvar_Get;
    ri.Cvar_Set = Cvar_Set;
    ri.Cvar_SetValue = Cvar_SetValue;
    ri.Vid_GetModeInfo = VID_GetModeInfo;

	printw("GetRefAPI");
    re = GetRefAPI(ri);

    if (re.api_version != API_VERSION)
        Com_Error (ERR_FATAL, "Re has incompatible api_version");
    
	printw((char *)(re.Init == 0 ? "re.Init error" : "re.Init found"));
        // call the init function
    if (re.Init (NULL, NULL) == -1)
		Com_Error (ERR_FATAL, "Couldn't start refresh");
}

void	VID_Shutdown (void)
{
    if (re.Shutdown)
    {
    	printf("DS video shutting down\n");
	    re.Shutdown ();
	    printf("...done\n");
    }
}

void	VID_CheckChanges (void)
{
}

void	VID_MenuInit (void)
{
}

void	VID_MenuDraw (void)
{
}

const char *VID_MenuKey(int key)
{
//	static const char *sound = "misc/menu1.wav";
//	menuframework_s *m = s_software_menu;
//
//	switch ( key )
//	{
//	case K_ESCAPE:
////		ApplyChanges( 0 );
//		M_ForceMenuOff();
//		return NULL;
//	case K_KP_UPARROW:
//	case K_UPARROW:
//		m->cursor--;
//		Menu_AdjustCursor( m, -1 );
//		break;
//	case K_KP_DOWNARROW:
//	case K_DOWNARROW:
//		m->cursor++;
//		Menu_AdjustCursor( m, 1 );
//		break;
//	case K_KP_LEFTARROW:
//	case K_LEFTARROW:
//		Menu_SlideItem( m, -1 );
//		break;
//	case K_KP_RIGHTARROW:
//	case K_RIGHTARROW:
//		Menu_SlideItem( m, 1 );
//		break;
//	case K_KP_ENTER:
//	case K_ENTER:
//		if ( !Menu_SelectItem( m ) )
////			ApplyChanges( NULL );
//			M_ForceMenuOff();
//		break;
//	}
//
//	return sound;
	
	switch (key)
	{
	case K_ESCAPE:
		M_ForceMenuOff();
	default:
		break;
	}
	return NULL;
}
