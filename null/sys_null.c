/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
c
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// sys_null.h -- null system driver to aid porting efforts

#include <stdio.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <ctype.h>

#include "../qcommon/qcommon.h"
#include "errno.h"
#include "glob.h"

#include "ds.h"
#include "../quake_ipc.h"
#include "../cyg-profile.h"

//#include <user_debugger.h>

//#define USE_DEBUGGER
//#define PROFILE_CODE

int	curtime = 0;
unsigned int sys_frame_time;		//for the game...hmmm...

int should_crash = 0;

void Sys_Error (char *error, ...)
{
	va_list		argptr;

	printf ("Sys_Error: ");	
	va_start (argptr,error);
	vprintf (error,argptr);
	va_end (argptr);
	printf ("\n");
	register unsigned int link_register asm ("lr");

void disable_keyb(void);
	disable_keyb();
	
void r_cache_print(int size);

	r_cache_print(0);

	printf("link register is %08x\n", link_register);

#ifdef USE_DEBUGGER
	while (1)
		*(int *)0 = 0;
	exit(1);
#else
	while(1);
#endif
}
#ifndef GAME_HARD_LINKED
void Com_Printf (char *msg, ...)
{
	va_list		argptr;
	char		text[1024];

	va_start (argptr, msg);
	vsprintf (text, msg, argptr);
	va_end (argptr);

	printf ("%s", text);
}

void *game_library = NULL;
void* (*Sys_GetGameAPI_DLL)(void *);

extern "C" void	*Sys_GetGameAPI (void *parms)
{
	printf("Loading game DLL\n");
	
	if (game_library)
		Com_Error (ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame");
	
	FILE *fp = ds_fopen("gamearm.bin", "rb");
	
	if (fp == NULL)
		Com_Error(ERR_FATAL, "couldn\'t open gamearm.bin\n");
	
	ds_fseek(fp, 0, SEEK_END);
	unsigned int length = ds_ftell(fp);
	ds_fseek(fp, 0, SEEK_SET);
	
	printf("game size is %d bytes\n", length);
	
	ds_set_malloc_base(MEM_XTRA);
//	game_library = ds_malloc(length);
	game_library = ds_malloc(4 * 1024 * 1024);
	ds_memset(game_library, 0, 4 * 1024 * 1024);
	game_library = (void *)(((unsigned int)game_library + 1048575) & 0xfff00000);
	ds_set_malloc_base(MEM_MAIN);
	
	if (game_library == NULL)
		Com_Error(ERR_FATAL, "insufficient memory to load game\n");
	
	ds_fread(game_library, 1, length, fp);
	ds_fclose(fp);
	
	IC_InvalidateAll();
	
	printf("Sys_GetGameAPI is at %08x\n%08x + %08x\n",
			/*(unsigned int)game_library + */*(unsigned int *)game_library,
			(unsigned int)game_library,
			*(unsigned int *)game_library);
	
	Sys_GetGameAPI_DLL = (void *(*)(void *))(/*(unsigned int)game_library + */*(unsigned int *)game_library);
	void *handles = Sys_GetGameAPI_DLL(parms);
	
	printf("got game handles\n");
	
	for (int count = 0; count < 15; count++)
	{
		unsigned int *addr = (unsigned int *)handles + 4 * (count + 1);
		*addr = *addr /*+ (unsigned int)game_library*/;
	}
	
	return handles;
}
#endif

void Sys_Quit (void)
{
	printf("Sys_Quit()\n");
//	exit (0);
	
	//ipc_block_ready_9to7();
	//quake_ipc_9to7->message_type = kPowerOff;
	//ipc_set_ready_9to7();
	fifo_msg msg;
	msg.type = kPowerOff;
	fifoSendDatamsg(FIFO_9to7,sizeof(msg),(u8 *)&msg);
	while(!fifoCheckValue32(FIFO_9to7));
	int ret = (int)fifoGetValue32(FIFO_9to7);
}

void	Sys_UnloadGame (void)
{
}

char *Sys_ConsoleInput (void)
{
	return NULL;
}

void	Sys_ConsoleOutput (char *string)
{
	printf(string);
}



void Sys_SendKeyEvents (void)
{
void IN_Buttons();
void IN_osk();
	IN_Buttons();
	IN_osk();
}

void Sys_AppActivate (void)
{
}

void Sys_CopyProtect (void)
{
}

char *Sys_GetClipboardData( void )
{
	return NULL;
}

byte *membase;
int maxhunksize;
int curhunksize;

int cb_alloc = 0;

void *Hunk_Begin (int maxsize)
{
	maxhunksize = maxsize + sizeof(int);
	curhunksize = 0;
/* 	membase = mmap(0, maxhunksize, PROT_READ|PROT_WRITE,  */
/* 		MAP_PRIVATE, -1, 0); */
/* 	if ((membase == NULL) || (membase == MAP_FAILED)) */

	ds_set_malloc_base(MEM_XTRA);
	membase = (byte *)ds_malloc(maxhunksize);
	ds_set_malloc_base(MEM_MAIN);
	
	if (membase == NULL)
	{
		print_top_four_blocks_kb();
		printf("extra: %.2fMB\n", (float)count_largest_extra_block_mb() / 1048576.0f);
		printf("cb_alloc: %d\n",cb_alloc);
		while(1);
		Com_Error(ERR_FATAL, "unable to virtual allocate %d bytes max %d", maxsize, maxhunksize);
	}
		
//	ds_memset(membase, 0, maxhunksize);

	*((int *)membase) = curhunksize;

	return membase + sizeof(int);
}

void *Hunk_Alloc (int size)
{
	byte *buf;
	
//	printf("allocating %d bytes\n", size);

	// round to cacheline
	size =  (size + 0xf) & 0xfffffff0;
	if (curhunksize + size > maxhunksize)
	{
		register unsigned int lr asm ("lr");
		printf("hunk size is %x bytes, tried adding %d to %d, %d short\nlr is %08x\n",
				maxhunksize, size, curhunksize, (curhunksize + size) - maxhunksize, lr);
		printf("cb_alloc: %d\n",cb_alloc);
		while(1);
		Com_Error(ERR_FATAL, "Hunk_Alloc overflow");
	}
	buf = membase + sizeof(int) + curhunksize;
	curhunksize += size;
	
	ds_memset(buf, 0, size);

	cb_alloc += size;
	
	return buf;
}

int Hunk_End (void)
{
	byte *orig_membase = membase;
	
	if(maxhunksize > (2*1024*1024)) {
		printf("Hunk_End: %d %d\n",curhunksize,maxhunksize);
		while((keysCurrent()&KEY_A) == 0);
		while((keysCurrent()&KEY_A) != 0);
	}

	ds_set_malloc_base(MEM_XTRA);
	membase = (byte *)ds_realloc(membase, (curhunksize + 0xf) & 0xfffffff0);
	ds_set_malloc_base(MEM_MAIN);
	
	if (membase != orig_membase) {
		Sys_Error("uh-oh, not the same memory bases %08x %08x\n", membase, orig_membase);
	}
	
	return curhunksize;
}

void Hunk_Free (void *base)
{
	byte *m;

	if (base) {
		m = ((byte *)base) - sizeof(int);
		ds_free(m);
	}
}

/*
void	*Hunk_Begin (int maxsize)
{
	void * temp =  malloc(maxsize);
	memset(temp, 0, maxsize);
	
	return temp;
}

void	*Hunk_Alloc (int size)
{
	void * temp =  malloc(size);
	memset(temp, 0, size);
	
	return temp;
}

void	Hunk_Free (void *buf)
{
	free(buf);
}

int		Hunk_End (void)
{
	return 0;
}*/

int		Sys_Milliseconds (void)
{
	curtime = (unsigned int)((float)hblanks / 15.72f);
	
	return curtime;
}

//double hblank_ms = 1.0 / 15.7343;
//unsigned int initial_hblank = -1;
//
//int Sys_Milliseconds (void)
//{
//	if (initial_hblank == -1)
//		initial_hblank = hblanks;
//	return (double)(hblanks - initial_hblank) / (262 * 60) * 1000;
//}

void	Sys_Mkdir (char *path)
{
	disk_mode();
	mkdir(path, 777);
	ram_mode();
}

DIR *dir_iterator = NULL;
char	findbase[MAX_OSPATH];
char	findpattern[MAX_OSPATH];
char	findpath[MAX_OSPATH];

char *Sys_FindFirst (char *path, unsigned musthave, unsigned canhave)
{
	struct dirent *pent;
	char *p;
	
//	printf("find first %s\n", path);
	disk_mode();

	if (dir_iterator)
		Sys_Error ("Sys_BeginFind without close");

	memset(findbase, 0, sizeof(findbase));
	memset(findpattern, 0, sizeof(findpattern));
	
	COM_FilePath (path, findbase);

	if ((p = strrchr(path, '/')) != NULL) {
		strcpy(findpattern, p + 1);
	} else
		strcpy(findpattern, "*");

	if (strcmp(findpattern, "*.*") == 0)
		strcpy(findpattern, "*");
	
	for (int count = 0; count < strlen(findbase); count++)
		findbase[count] = tolower(findbase[count]);
	for (int count = 0; count < strlen(findpattern); count++)
		findpattern[count] = tolower(findpattern[count]);
	
//	printf("base: %s\n", findbase);
//	printf("patt: %s\n", findpattern);
	
	if ((dir_iterator = opendir(findbase)) == NULL)
	{
		ram_mode();
		return NULL;
	}
	
	char temp_filename[256];
	//struct stat st;
	                   
	while ((pent = readdir(dir_iterator)) != NULL) {		
		for (int count = 0; count < strlen(pent->d_name); count++)
			temp_filename[count] = tolower(pent->d_name[count]);
		
//		printf("found file %s\n", temp_filename);
		
		if (!*findpattern || glob_match(findpattern, temp_filename)) {
			if (*findpattern)
//				printf("%s matched %s\n", findpattern, temp_filename);
			/*if (CompareAttributes(findbase, d->d_name, musthave, canhave)) */{
				sprintf (findpath, "%s/%s", findbase, temp_filename);
				
				ram_mode();
				return findpath;
			}
		}
	}
	
	ram_mode();
	return NULL;
}

char *Sys_FindNext (unsigned musthave, unsigned canhave)
{
	struct dirent *pent;
	char temp_filename[256];
	struct stat st;
	
//	printf("find next\n");
	
	disk_mode();
	              
	if (dir_iterator == NULL)
	{
//		printf("find next without find first\n");
		
		ram_mode();
		return NULL;
	}
	while ((pent = readdir(dir_iterator)) != NULL) {
		for (int count = 0; count < strlen(pent->d_name); count++)
			temp_filename[count] = tolower(pent->d_name[count]);
		
//		printf("next file is %s\n", temp_filename);
		if (!*findpattern || glob_match(findpattern, temp_filename)) {
			if (*findpattern)
//				printf("%s matched %s\n", findpattern, temp_filename);
			/*if (CompareAttributes(findbase, d->d_name, musthave, canhave)) */{
				sprintf (findpath, "%s/%s", findbase, temp_filename);
				
				ram_mode();
				return findpath;
			}
		}
	}
	
	ram_mode();
	return NULL;
}

void Sys_FindClose (void)
{
//	printf("find close\n");
	
	disk_mode();
	
	if (dir_iterator != NULL)
		closedir(dir_iterator);
	dir_iterator = NULL;
	
	ram_mode();
}

void	Sys_Init (void)
{
}


//=============================================================================
extern unsigned char *frame_buffer;
extern unsigned d_8to24table[256];

void user_debugger_update();
uint32 keysDown(void);
uint32 keysUp(void);
void scanKeys();
void get_pen_pos(short *px, short *py);

bool allocate_statics(void);

extern int rendering_is_go;

void quake2_main (int argc, char **argv)
{
	//printf("%.2f kb %d\n", (float)count_largest_block_kb() / 1024, __LINE__);
//	Z_TagMalloc(1, 0);
//	printf("%.2f kb %d\n", (float)count_largest_block_kb() / 1024, __LINE__);
//	while(1);
	//cygprofile_begin();
//	cygprofile_enable();
	
	if (allocate_statics() == false)
		Sys_Error("could\n't allocate slot-2 RAM for globals\n");
	
	//printf("%.2f kb of heap available\n", (float)count_largest_block_kb() / 1024);
	Qcommon_Init (argc, argv);
	//printf("%.2f kb of heap available\n", (float)count_largest_block_kb() / 1024);

	int prev_millis = Sys_Milliseconds() - 100;
	
//#ifdef PROFILE_CODE
//	cygprofile_end();
//	printf("profiling done\n");
//	while(1);
//#endif
	
	int hit_count = 0;
	while (1)
	{
		scanKeys();
		
#ifdef PROFILE_CODE
//		if (hit_count == 100)
		if (rendering_is_go)
		{
//			printf("STARTING PROFILE\n");
			cygprofile_enable();
		}
#endif
		
		time_t start = hblanks;
		
		sys_frame_time = Sys_Milliseconds();
	
		int curr_millis = Sys_Milliseconds();
		Qcommon_Frame (curr_millis - prev_millis);
		prev_millis = curr_millis;
		
		time_t stop = hblanks;
		
#ifdef PROFILE_CODE
		hit_count++;
		
		if (hit_count == 1000)
		{
			cygprofile_end();
			printf("profiling done\n");
			while(1);
		}
#endif
		
//		extern bool use_arm7_bsp;
//		Com_Printf("%.1f, %.1fms, %d hbs %d\n", 15720.0f / (float)(stop - start), (float)(stop - start) * 0.06361323f, stop - start, use_arm7_bsp);
//		printf("%d main heap\n", count_largest_block_kb());
//		printf("%d extra heap\n", count_largest_extra_block_mb());
//		printf("%d kb VRAM free\n", vram_get_free_size() >> 10);
//		printf("%d hunk\n", maxhunksize - curhunksize);
//
//		print_top_four_blocks_kb();
		
#ifdef USE_DEBUGGER
		user_debugger_update();
#endif
	/*	
		volatile unsigned int down = keysDown();
		volatile unsigned int up = keysUp();

		short px, py;
		get_pen_pos(&px, &py);
	
		printf("down up %d %d %d %d\n", down, up, px, py);*/
		
		/*if (Sys_Milliseconds() > 1000)
		{
			FILE *fp = fopen("/mnt/remote/q2dump.raw", "wb");
			
			unsigned int *out = (unsigned int *)malloc(320 * 240 * 4);
			int x, y;
			
			for (y = 0; y < 240; y++)
				for (x = 0; x < 320; x++)
				{
					unsigned char index;
					
					index = frame_buffer[y * 320 + x];
					out[y * 320 + x] = d_8to24table[index];
				}
			
			fwrite(out, 320 * 240, 4, fp);
			free(out);
			fclose(fp);
			exit(0);
		}*/
	}
}
