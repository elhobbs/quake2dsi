/*
Copyright (C) 1996-1997 Id Software, Inc.

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
// snd_null.c -- include this instead of all the other snd_* files to have
// no sound code whatsoever

//#define SND_PRINTF(args...) printf(args)
#define SND_PRINTF(args...)
//#define USE_SOUND

#include "../client/client.h"
#include "../client/snd_loc.h"

#include "ds.h"
#include "../quake_ipc.h"
//
//cvar_t bgmvolume = {"bgmvolume", "1", true};
//cvar_t volume = {"volume", "0.7", true};

cvar_t		*s_volume;

//#define MAX_STATICS	64
//
//unsigned int num_statics = 0;
//
//struct staticsounds
//{
//	sfx_t *sfx;
//	float volume;
//	float attenuation;
//	vec3_t origin;
//	int is_playing;
//	int is_audible;
//	int ds_panning;
//	int ds_volume;
//	int ds_channel;
//} statics[MAX_STATICS];
//
//struct staticsounds *uncached_statics;

bool sound_initialised = false;
bool swapped_out = false;

void ipc_block_ready_9to7(void);
bool ipc_test_ready_9to7(void);
void ipc_set_ready_9to7(void);
void ipc_block_ready_7to9(void);

vec3_t listener_origin, listener_right;

void S_Play(void)
{
#ifdef USE_SOUND
	static int hash=345;
	int 	i;
	char name[256];
	sfx_t	*sfx;
	
	if (swapped_out)
		Sys_Error("playing sound while sound is swapped out! (%d)\n", __LINE__);
	
//	printf("S_Play\n");
	
	i = 1;
	while (i<Cmd_Argc())
	{
		if (!strrchr(Cmd_Argv(i), '.'))
		{
			strcpy(name, Cmd_Argv(i));
			strcat(name, ".wav");
		}
		else
			strcpy(name, Cmd_Argv(i));
		sfx = S_RegisterSound(name);
		S_StartSound(NULL, cl.playernum+1, 0, sfx, 1.0, 1.0, 0);
		i++;
	}
#endif
}
 
void printw(char *str);

void S_Init (void)
{
#ifdef USE_SOUND
	fifo_msg msg;
	Com_Printf("\nDS Sound Initialization\n");
	
	if (swapped_out)
		Sys_Error("playing sound while sound is swapped out! (%d)\n", __LINE__);

	if (COM_CheckParm("-nosound"))
	{
		Com_Printf("...sound disabled\n");
		return;
	}
		
	SND_PRINTF("ARM9: S_Init\n");
	//ipc_block_ready_9to7();
	
//	*((unsigned int *)quake_ipc_9to7_buf) = 0x0;
//	*((unsigned int *)quake_ipc_9to7_buf) = 0x20000;

	extern unsigned int sound_memory;
	
	if (!sound_memory)
		Sys_Error("Couldn\'t allocate sound memory\n");
	
	((unsigned int *)quake_ipc_9to7_buf)[0] = sound_memory;
	((unsigned int *)quake_ipc_9to7_buf)[1] = SOUND_L1_SIZE;

	Com_Printf("\t%d kb sound cache\n", ((unsigned int *)quake_ipc_9to7_buf)[1] >> 10);
	printw("sound  init...");
	
	//quake_ipc_9to7->message_type = kS_Init;
	//ipc_set_ready_9to7();
	msg.type = kS_Init;
	fifoSendDatamsg(FIFO_9to7,sizeof(msg),(u8 *)&msg);
	while(!fifoCheckValue32(FIFO_9to7));
	int ret = (int)fifoGetValue32(FIFO_9to7);
	printw("sound enabled");
	
	//ipc_block_ready_9to7();
	
	sound_initialised = true;
	Com_Printf("...sound enabled\n");
	
	s_volume = Cvar_Get ("s_volume", "0.7", CVAR_ARCHIVE);
	Cmd_AddCommand("play", S_Play);
	
//	uncached_statics = (struct staticsounds *)((unsigned int)statics | 0x400000);
	
	printw("sound enabled");
	S_StopAllSounds ();
//	while(1);
#endif
}

void S_AmbientOff (void)
{
#ifdef USE_SOUND
	if (swapped_out)
		Sys_Error("playing sound while sound is swapped out! (%d)\n", __LINE__);
		
	SND_PRINTF("ARM9: S_AmbientOff\n");
	fifo_msg msg;
	msg.type = kS_AmbientOff;
	fifoSendDatamsg(FIFO_9to7,sizeof(msg),(u8 *)&msg);
	while(!fifoCheckValue32(FIFO_9to7));
	int ret = (int)fifoGetValue32(FIFO_9to7);
	//ipc_block_ready_9to7();
	
	//quake_ipc_9to7->message_type = kS_AmbientOff;
	//ipc_set_ready_9to7();
	
	//ipc_block_ready_9to7();
#endif
}

void S_AmbientOn (void)
{
#ifdef USE_SOUND

	if (swapped_out)
		Sys_Error("playing sound while sound is swapped out! (%d)\n", __LINE__);
	SND_PRINTF("ARM9: S_AmbientOn\n");
	fifo_msg msg;
	msg.type = kS_AmbientOn;
	fifoSendDatamsg(FIFO_9to7,sizeof(msg),(u8 *)&msg);
	//ipc_block_ready_9to7();
	
	//quake_ipc_9to7->message_type = kS_AmbientOn;
	//ipc_set_ready_9to7();
	
	//ipc_block_ready_9to7();
#endif
}

void S_Shutdown (void)
{
#ifdef USE_SOUND

	if (swapped_out)
		Sys_Error("playing sound while sound is swapped out! (%d)\n", __LINE__);
		
	if (!sound_initialised)
		return;
		
	SND_PRINTF("ARM9: S_Shutdown\n");
	fifo_msg msg;
	msg.type = kS_Shutdown;
	fifoSendDatamsg(FIFO_9to7,sizeof(msg),(u8 *)&msg);
	while(!fifoCheckValue32(FIFO_9to7));
	int ret = (int)fifoGetValue32(FIFO_9to7);

	/*ipc_block_ready_9to7();
	
	quake_ipc_9to7->message_type = kS_Shutdown;
	ipc_set_ready_9to7();
	
	ipc_block_ready_9to7();*/
#endif
}

void S_TouchSound (char *sample)
{
#ifdef USE_SOUND
	if (swapped_out)
		Sys_Error("playing sound while sound is swapped out! (%d)\n", __LINE__);
		
	if (!sound_initialised)
		return;
		
	SND_PRINTF("ARM9: S_TouchSound, %s\n", sample);
	//ipc_block_ready_9to7();
	
	ds_memset((char *)quake_ipc_9to7_buf, 0, 100);
	strcpy((char *)quake_ipc_9to7_buf, sample);
	//quake_ipc_9to7->message_type = kS_TouchSound;
	//ipc_set_ready_9to7();
	
	//ipc_block_ready_9to7();
	fifo_msg msg;
	msg.type = kS_TouchSound;
	fifoSendDatamsg(FIFO_9to7,sizeof(msg),(u8 *)&msg);
	while(!fifoCheckValue32(FIFO_9to7));
	int ret = (int)fifoGetValue32(FIFO_9to7);
#endif
}

void S_ClearBuffer (void)
{
#ifdef USE_SOUND
	if (swapped_out)
		Sys_Error("playing sound while sound is swapped out! (%d)\n", __LINE__);
		
	if (!sound_initialised)
		return;
		
	SND_PRINTF("ARM9: S_ClearBuffer\n");
	fifo_msg msg;
	msg.type = kS_ClearBuffer;
	fifoSendDatamsg(FIFO_9to7,sizeof(msg),(u8 *)&msg);
	while(!fifoCheckValue32(FIFO_9to7));
	int ret = (int)fifoGetValue32(FIFO_9to7);

	//ipc_block_ready_9to7();
	
	//quake_ipc_9to7->message_type = kS_ClearBuffer;
	//ipc_set_ready_9to7();
	
	//ipc_block_ready_9to7();
#endif
}

void S_StaticSound (sfx_t *sfx, vec3_t origin, float vol, float attenuation)
{
//#ifdef USE_SOUND
//	if (!sound_initialised)
//		return;
//	
//	if (num_statics >= MAX_STATICS)
//		return;
//	
//	uncached_statics[num_statics].attenuation = 1/*attenuation*/;
//	uncached_statics[num_statics].volume = vol;
//	uncached_statics[num_statics].origin[0] = origin[0];
//	uncached_statics[num_statics].origin[1] = origin[1];
//	uncached_statics[num_statics].origin[2] = origin[2];
//	uncached_statics[num_statics].sfx = sfx;
//	uncached_statics[num_statics].is_playing = 0;
//	
//	num_statics++;
////	
////		
//////	SND_PRINTF("ARM9: S_StaticSound, %s (%08x)\n%.1f %.1f %.1f, ",
//////		sfx->name, (unsigned int)sfx, origin[0], origin[1], origin[2], origin[3]);
//////	SND_PRINTF("%.1f, %.1f\n", vol, attenuation);
////	ipc_block_ready_9to7();
////	
////	((unsigned int *)quake_ipc_9to7_buf)[0] = (unsigned int)sfx;
////	
////	float *floats = (float *)quake_ipc_9to7_buf;
////	floats[1] = origin[0];
////	floats[2] = origin[1];
////	floats[3] = origin[2];
////	floats[4] = vol;
////	floats[5] = attenuation;
////	
////	quake_ipc_9to7->message_type = kS_StaticSound;
////	ipc_set_ready_9to7();
////	
////	ipc_block_ready_9to7();
//#endif
}

#define SOUND_NOMINAL_CLIP_DISTANCE_MULT 0.001f

bool spatialise (int entnum, float fvol, float attenuation, vec3_t origin, int *ds_pan, int *ds_vol)
{
	float master_v = fvol * 255.0f;
	
	int leftvol = (int)master_v;
	int rightvol = (int)master_v;
	
//	printf("att %.2f, vol %.2f\n", attenuation, fvol);
//	printf("or %.2f %.2f %.2f\n", origin[0], origin[1], origin[2]);

	if (swapped_out)
		Sys_Error("playing sound while sound is swapped out! (%d)\n", __LINE__);
	
//	if (entnum != cl.viewentity)
	if (origin)
	{
		vec3_t source_vec;
		float dist_mult = attenuation * SOUND_NOMINAL_CLIP_DISTANCE_MULT;
		
		VectorSubtract(origin, listener_origin, source_vec);
		
		float dist = VectorNormalize(source_vec) * dist_mult;
		float dot = DotProduct(listener_right, source_vec);
		
//		printf("dist %.2f dot %.2f\n", dist, dot);
		
		float rscale = 1.0f + dot;
		float lscale = 1.0f - dot;
		
		float scale = (1.0f - dist) * rscale;
		rightvol = (int)(master_v * scale);
		
		if (rightvol < 0)
			rightvol = 0;
		
		scale = (1.0f - dist) * lscale;
		leftvol = (int)(master_v * scale);
		
		if (leftvol < 0)
			leftvol = 0;
			
//		printf("l %d, r %d\n", leftvol, rightvol);
			
		if ((leftvol == 0) && (rightvol == 0))
			return false;
	}
	
	int diff = (leftvol - rightvol) >> 3;
	*ds_pan = 64 - diff;
	
	*ds_vol = (leftvol + rightvol) >> 2;
	
	return true;
}

int num_plays = 0;
void S_StartSound (vec3_t origin, int entnum, int entchannel, struct sfx_s *sfx, float fvol,  float attenuation, float timeofs)
{
#ifdef USE_SOUND
	if (swapped_out)
		Sys_Error("playing sound while sound is swapped out! (%d)\n", __LINE__);
		
	if (!sound_initialised)
		return;
	
	int ds_pan, ds_vol;
	if (!spatialise(entnum, fvol * s_volume->value, attenuation, origin, &ds_pan, &ds_vol))
		return;
	
	num_plays++;
		
	SND_PRINTF("ARM9: S_StartSound, %d %d, %s (%08x)",
		entnum, entchannel, sfx->name, (unsigned int)sfx);
	if (origin)
	{
		SND_PRINTF(", %.1f %.1f %.1f, %.1f, %.1f\n", origin[0], origin[1], origin[2], fvol, attenuation);
	}
	else
		SND_PRINTF("\n");

	//ipc_block_ready_9to7();
	
	((unsigned int *)quake_ipc_9to7_buf)[0] = entnum;
	((unsigned int *)quake_ipc_9to7_buf)[1] = entchannel;
	((unsigned int *)quake_ipc_9to7_buf)[2] = (unsigned int)sfx;
	
	float *floats = (float *)quake_ipc_9to7_buf;
//	floats[3] = origin[0];
//	floats[4] = origin[1];
//	floats[5] = origin[2];
	floats[6] = fvol * s_volume->value;
	floats[7] = attenuation;
	
	((unsigned int *)quake_ipc_9to7_buf)[8] = ds_vol;
	((unsigned int *)quake_ipc_9to7_buf)[9] = ds_pan;
	
	//quake_ipc_9to7->message_type = kS_StartSound;
	//ipc_set_ready_9to7();
	
	//ipc_block_ready_9to7();
	fifo_msg msg;
	msg.type = kS_StartSound;
	fifoSendDatamsg(FIFO_9to7,sizeof(msg),(u8 *)&msg);
	while(!fifoCheckValue32(FIFO_9to7));
	int ret = (int)fifoGetValue32(FIFO_9to7);
#endif
}

void S_StopSound (int entnum, int entchannel)
{
#ifdef USE_SOUND
	if (swapped_out)
		Sys_Error("playing sound while sound is swapped out! (%d)\n", __LINE__);
		
	if (!sound_initialised)
		return;
	
	SND_PRINTF("ARM9: S_StopSound, %d %d\n", entnum, entchannel);

	//ipc_block_ready_9to7();
	
	((unsigned int *)quake_ipc_9to7_buf)[0] = entnum;
	((unsigned int *)quake_ipc_9to7_buf)[1] = entchannel;
	
	//quake_ipc_9to7->message_type = kS_StopSound;
	//ipc_set_ready_9to7();
	
	//ipc_block_ready_9to7();
	fifo_msg msg;
	msg.type = kS_StopSound;
	fifoSendDatamsg(FIFO_9to7,sizeof(msg),(u8 *)&msg);
	while(!fifoCheckValue32(FIFO_9to7));
	int ret = (int)fifoGetValue32(FIFO_9to7);
#endif
}

struct sfx_s *S_RegisterSound (char *sample)
{
#ifdef USE_SOUND
	if (swapped_out)
		Sys_Error("playing sound while sound is swapped out! (%d)\n", __LINE__);
		
	if (!sound_initialised)
		return NULL;
	
	SND_PRINTF("ARM9: S_PrecacheSound, %s\n", sample);
	//ipc_block_ready_9to7();
	
	ds_memset((char *)quake_ipc_9to7_buf, 0, 100);
	ds_memcpy((void *)quake_ipc_9to7_buf, sample, 100);
	//quake_ipc_9to7->message_type = kS_PrecacheSound;
	//ipc_set_ready_9to7();
	
	//ipc_block_ready_9to7();
	
	fifo_msg msg;
	msg.type = kS_PrecacheSound;
	fifoSendDatamsg(FIFO_9to7,sizeof(msg),(u8 *)&msg);
	while(!fifoCheckValue32(FIFO_9to7));
	int ret = (int)fifoGetValue32(FIFO_9to7);

	unsigned int retr = ((unsigned int *)quake_ipc_7to9_buf)[0];
//	SND_PRINTF("return is %08x\n", retr);while(1);
	return (sfx_t *)retr;
#else
	return NULL;
#endif
}

void S_ClearPrecache (void)
{
//#ifdef USE_SOUND
//	if (!sound_initialised)
//		return;
//		
//	SND_PRINTF("ARM9: S_ClearPrecache\n");
//	ipc_block_ready_9to7();
//	
//	quake_ipc_9to7->message_type = kS_ClearPrecache;
//	ipc_set_ready_9to7();
//	
//	ipc_block_ready_9to7();
//#endif
}

void S_Update (vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up)
{
#ifdef USE_SOUND
	if (swapped_out)
		Sys_Error("playing sound while sound is swapped out! (%d)\n", __LINE__);
		
	if (!sound_initialised)
		return;
	
	VectorCopy(origin, listener_origin);
	VectorCopy(v_right, listener_right);
	return;
//	SND_PRINTF("ARM9: S_Update\n");
//	SND_PRINTF("%.1f %.1f %.1f\n", origin[0], origin[1], origin[2]);
//	SND_PRINTF("%.1f %.1f %.1f\n", v_forward[0], v_forward[1], v_forward[2]);
//	SND_PRINTF("%.1f %.1f %.1f\n", v_right[0], v_right[1], v_right[2]);
//	SND_PRINTF("%.1f %.1f %.1f\n", v_up[0], v_up[1], v_up[2]);

//	int num_audible = 0;
//	for (int count = 0; count < num_statics; count++)
//	{
//		int ds_vol, ds_pan;
//		bool audible = spatialise(0, uncached_statics[count].volume, uncached_statics[count].attenuation,
//			uncached_statics[count].origin, &ds_pan, &ds_vol);
//		
//		uncached_statics[count].ds_panning = ds_pan;
//		uncached_statics[count].ds_volume = ds_vol;	
//		uncached_statics[count].is_audible = audible;
//		num_audible += audible;
//	}
//	
//	ipc_block_ready_9to7();
//	
//	float *floats = (float *)quake_ipc_9to7_buf;
//	floats[0] = origin[0];
//	floats[1] = origin[1];
//	floats[2] = origin[2];
//	
//	floats[3] = v_forward[0];
//	floats[4] = v_forward[1];
//	floats[5] = v_forward[2];
//	
//	floats[6] = v_right[0];
//	floats[7] = v_right[1];
//	floats[8] = v_right[2];
//	
//	floats[9] = v_up[0];
//	floats[10] = v_up[1];
//	floats[11] = v_up[2];
//	
//	printf("%d of %d statics are audible\n", num_audible, num_statics);
//	
////	printf("uncached statics at %08x\n", (unsigned int)uncached_statics);
//	
//	((unsigned int *)quake_ipc_9to7_buf)[12] = (unsigned int)uncached_statics;
//	((unsigned int *)quake_ipc_9to7_buf)[13] = num_statics;
//	
//	quake_ipc_9to7->message_type = kS_Update;
//	ipc_set_ready_9to7();
//	
//	ipc_block_ready_9to7();
#endif
}

void S_StopAllSounds (void)
{
#ifdef USE_SOUND
	if (swapped_out)
		Sys_Error("playing sound while sound is swapped out! (%d)\n", __LINE__);
		
	if (!sound_initialised)
		return;
	
//	num_statics = 0;
	
	SND_PRINTF("ARM9: S_StopAllSounds\n");

	//ipc_block_ready_9to7();
	
	//quake_ipc_9to7->message_type = kS_StopAllSounds;
	//ipc_set_ready_9to7();
	
	//ipc_block_ready_9to7();
	fifo_msg msg;
	msg.type = kS_StopAllSounds;
	fifoSendDatamsg(FIFO_9to7,sizeof(msg),(u8 *)&msg);
	while(!fifoCheckValue32(FIFO_9to7));
	int ret = (int)fifoGetValue32(FIFO_9to7);
#endif
}

void S_BeginRegistration (void)
{
#ifdef USE_SOUND
	if (swapped_out)
	{
		register unsigned int lr_r asm ("lr");
		volatile unsigned int lr = lr_r;
		Sys_Error("playing sound while sound is swapped out! (%d, %08x)\n", __LINE__, lr);
	}
		
	if (!sound_initialised)
		return;
	
	SND_PRINTF("ARM9: S_BeginPrecaching\n");
//	ipc_block_ready_9to7();
//	
//	quake_ipc_9to7->message_type = kS_BeginPrecaching;
//	ipc_set_ready_9to7();
//	
//	ipc_block_ready_9to7();
	
	extern sfx_t *sound_memory;
	
	for (int count = 0; count < MAX_SFX; count++)
	{
		sfx_t *s = &sound_memory[count];
		
		if ((s->address_l2 != NULL) && (s->address_l2 != (void *)0xdeadbeef)  && (s->address_l2 != (void *)0xbabebabe))
		{
			SND_PRINTF("freeing L2 for %s\n", s->name);
			ds_free(s->address_l2);
			
			s->address_l2 = (void *)0xdeadbeef;
		}
	}
#endif
}

void S_EndRegistration (void)
{
//#ifdef USE_SOUND
//	if (!sound_initialised)
//		return;
//	
//	SND_PRINTF("ARM9: S_EndPrecaching\n");
//	ipc_block_ready_9to7();
//	
//	quake_ipc_9to7->message_type = kS_EndPrecaching;
//	ipc_set_ready_9to7();
//	
//	ipc_block_ready_9to7();
//#endif
}

void S_ExtraUpdate (void)
{
//#ifdef USE_SOUND
////	SND_PRINTF("ARM9: S_ExtraUpdate\n");
//	ipc_block_ready_9to7();
//	
//	quake_ipc_9to7->message_type = kS_ExtraUpdate;
//	ipc_set_ready_9to7();
//	
//	ipc_block_ready_9to7();
//#endif
}

void S_StartLocalSound (char *s)
{
#ifdef USE_SOUND
	if (swapped_out)
		Sys_Error("playing sound while sound is swapped out! (%d)\n", __LINE__);
		
	if (!sound_initialised)
		return;
	
//	SND_PRINTF("ARM9: S_LocalSound, %s\n", s);
	//ipc_block_ready_9to7();
	
	ds_memset((char *)quake_ipc_9to7_buf, 0, 100);
	strcpy((char *)quake_ipc_9to7_buf, s);
	//quake_ipc_9to7->message_type = kS_LocalSound;
	//ipc_set_ready_9to7();
	
	//ipc_block_ready_9to7();
	fifo_msg msg;
	msg.type = kS_LocalSound;
	fifoSendDatamsg(FIFO_9to7,sizeof(msg),(u8 *)&msg);
	while(!fifoCheckValue32(FIFO_9to7));
	int ret = (int)fifoGetValue32(FIFO_9to7);
#endif
}

void ds_dcacheflush(void);
extern unsigned int force_load_size;
extern unsigned int force_load_seek;
extern int uncached_address;

int num_loads = 0;
unsigned char *S_LoadSoundFile(char *name, unsigned int *file_len)
{
#ifdef USE_SOUND
	if (swapped_out)
		Sys_Error("playing sound while sound is swapped out! (%d)\n", __LINE__);
		
	char filename[256];
	unsigned char *data;
	
	ds_memset(filename, 0, 256);
	
	if (name[0] == '*')
	{
		char name_copy[256];
		strcpy(name_copy, name + 1);
		sprintf(name, "player/%s/%s", "male", name_copy);
	}
	
	strcpy(filename, "sound/");
	strcat(filename, name);
	
//	printf("!!!FORCE %d %d\n", force_load_seek, force_load_size);
//	uncached_address = 0x400000;
//	data = COM_LoadTempFile(filename);
	*file_len = FS_LoadFile(filename, (void **)&data);
	
//	for (int count = 0; count < 4; count++)
//		printf("%02x%02x%02x%02x ", data[count * 4 + 0], data[count * 4 + 1], data[count * 4 + 2], data[count * 4 + 3]);
//	while(1);
//	uncached_address = 0;
	
	if (data == NULL)
		printf("Couldn\'t load %s\n", filename);
	
//	ds_dcacheflush();

	num_loads++;
//	printf("%d LOADS NEEDED\n", num_loads);
	
	return data;
#endif
}

void S_FreeSoundFile(void *data)
{
#ifdef USE_SOUND
	if (swapped_out)
		Sys_Error("playing sound while sound is swapped out! (%d)\n", __LINE__);
		
	if (data)
		FS_FreeFile(data);
#endif
}

void S_FreeMarkedSounds(void)
{
#ifdef USE_SOUND
	if (swapped_out)
		Sys_Error("playing sound while sound is swapped out! (%d)\n", __LINE__);
		
	printf("ARM9: freeable marked sounds\n");
	
	extern sfx_t *sound_memory;
	
	for (int count = 0; count < MAX_SFX; count++)
	{
		sfx_t *s = &sound_memory[count];
		
		if ((s->address == NULL)							//if it's not in local memory
				|| (s->address == (void *)0xdeadbeef)
				|| (s->address == (void *)0xbabebabe))
		{
			if ((s->address_l2 != (void *)0xdeadbeef)					//but it's in main memory
					&& (s->address_l2 != (void *)0xbabebabe)
					&& (s->address_l2 != NULL))
			{
				SND_PRINTF("freeing %08x\n", ((unsigned int)s->address_l2 & 0x7fffffff));
				ds_free((void *)((unsigned int)s->address_l2 & 0x7fffffff));
				s->address_l2 = NULL;
			}
		}
	}
#endif
}

void ds_swap_sound(bool out)
{
	extern void *backup_sound_memory;
	extern unsigned int sound_memory;
	
	if (out)
	{
		S_StopAllSounds();
		ds_memcpy(backup_sound_memory, (void *)sound_memory, SOUND_L1_SIZE);
	}
	else
		ds_memcpy((void *)sound_memory, backup_sound_memory, SOUND_L1_SIZE);
		
	swapped_out = out;
}
