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
// snd_mem.c: sound caching
#if 1

#include "client.h"
#include "snd_loc.h"
#include "../r_cache.h"
#include "../null/ds.h"

int			cache_full_cycle;

byte *S_Alloc (int size);
extern hunk_t ds_snd_cache;
extern int		r_framecount;	// so frame counts initialized to 0 don't match

/*
================
ResampleSfx
================
*/
void ResampleSfx (sfx_t *sfx, int inrate, int inwidth, byte *data)
{
	int		outcount;
	int		srcsample;
	float	stepscale;
	int		i;
	int		sample, samplefrac, fracstep;
	sfxcache_t	*sc;
	
	if(sfx->extradata == 0)
		return;
	sc = (sfxcache_t*)sfx->extradata;
	if (!sc)
		return;

	stepscale = (float)inrate / dma.speed;	// this is usually 0.5, 1, or 2

	outcount = sc->length / stepscale;
	sc->length = outcount;
	if (sc->loopstart != -1)
		sc->loopstart = sc->loopstart / stepscale;

	sc->speed = dma.speed;
	//we are forcing 8 bit sound
	//if (s_loadas8bit->value)
		sc->width = 1;
	//else
	//	sc->width = inwidth;
	sc->stereo = 0;

// resample / decimate to the current source rate

	if (stepscale == 1 && inwidth == 1 && sc->width == 1)
	{
// fast special case
		for (i=0 ; i<outcount ; i++)
			((signed char *)sc->data)[i]
			= (int)( (unsigned char)(data[i]) - 128);
	}
	else
	{
// general case
		samplefrac = 0;
		fracstep = stepscale*256;
		for (i=0 ; i<outcount ; i++)
		{
			srcsample = samplefrac >> 8;
			samplefrac += fracstep;
			if (inwidth == 2)
				sample = LittleShort ( ((short *)data)[srcsample] );
			else
				sample = (int)( (unsigned char)(data[srcsample]) - 128) << 8;
			if (sc->width == 2)
				((short *)sc->data)[i] = sample;
			else
				((signed char *)sc->data)[i] = sample >> 8;
		}
	}
}

//=============================================================================

extern int file_from_pak;
extern FILE* g_pack_file;
extern int g_pack_file_pos;
extern int g_pack_file_len;

/*
==============
S_LoadSound
==============
*/
sfxcache_t *S_LoadSound (sfx_t *s)
{
    char	namebuffer[MAX_QPATH];
	byte	*data;
	wavinfo_t	info;
	int		len;
	float	stepscale;
	sfxcache_t	*sc;
	int		size;
	char	*name;
	int		close_file = 0;
	extern int s_in_precache;

	if(r_rache_is_empty) {
		return NULL;
	}

	if (s->name[0] == '*')
		return NULL;

	sc = (sfxcache_t *)Cache_Check (&ds_snd_cache,&(s->extradata));
	if (sc)
		return sc;

	if(s_in_precache == 0) {
		s_in_precache = 0;
	}

//Com_Printf ("S_LoadSound: %x\n", (int)stackbuf);
// load it in
	if (s->truename)
		name = s->truename;
	else
		name = s->name;

	if (name[0] == '#')
		strcpy(namebuffer, &name[1]);
	else
		Com_sprintf (namebuffer, sizeof(namebuffer), "sound/%s", name);

	//printf ("%s\n",namebuffer);

	FILE *h = (FILE *)s->handle;
	size = s->len;
	if(h == 0) {
		close_file = 1;
		//size = FS_LoadFile (namebuffer, (void **)&data);
		size = FS_FOpenFile (namebuffer, &h);
	} else {
		ds_fseek (h, s->pos, SEEK_SET);
	}
	if (!h) {
		Com_DPrintf ("Couldn't load %s\n", namebuffer);
		return NULL;
	}
	r_cache_set_fail(0);
	data = (byte *)Z_Malloc(size);
	if (!data)
	{
		r_cache_set_fail(1);
		if(close_file) {
			ds_fclose (h);
		}
		Com_DPrintf ("Couldn't load %s\n", namebuffer);
		return NULL;
	}
	FS_Read (data, size, h);
	if(close_file) {
		ds_fclose (h);
		s->handle = g_pack_file;
		s->pos = g_pack_file_pos;
		s->len = g_pack_file_len;
	}

	info = GetWavinfo (s->name, data, size);
	if (info.channels != 1)
	{
		r_cache_set_fail(1);
		Com_DPrintf ("%s is a stereo sample\n",s->name);
		//FS_FreeFile (data);
		Z_Free(data);
		return NULL;
	}

	stepscale = (float)info.rate / dma.speed;	
	len = info.samples / stepscale;
	/*if (info.width != 1)
	{
		r_cache_set_fail(1);
		printf ("%s is a 16 bit sample\n",s->name);
		//FS_FreeFile (data);
		Z_Free(data);
		return NULL;
	}*/

	//we are forcing 8 bit sound
	//len = len * info.width * info.channels;

	Cache_Alloc(&ds_snd_cache,(cache_user_t *)&(s->extradata),len + sizeof(sfxcache_t) + 4,s->name);

	/*cached_t *ds = DS_CacheAlloc(&ds_snd_cache,len + sizeof(sfxcache_t) + 4);
	if(!ds) {
		r_cache_set_fail(1);
		//FS_FreeFile (data);
		Z_Free(data);
		return NULL;
	}
	ds->owner = (cached_t **)&s->cache;
	ds->visframe = r_framecount;
	sc = s->cache = (sfxcache_t *)(ds+1);
	*/
	//sc = s->cache = (sfxcache_t *)Hunk_Alloc (len + sizeof(sfxcache_t) + 4);//pad by 4 just to be certain - too lazy to check
	sc = (sfxcache_t *)s->extradata;
	if (!sc)
	{
		r_cache_set_fail(1);
		//FS_FreeFile (data);
		Z_Free(data);
		return NULL;
	}
	r_cache_set_fail(1);
	
	sc->length = info.samples;
	sc->loopstart = info.loopstart;
	sc->speed = info.rate;
	sc->width = info.width;
	sc->stereo = info.channels;

	ResampleSfx (s, sc->speed, sc->width, data + info.dataofs);

	//FS_FreeFile (data);
	Z_Free(data);

	return sc;
}



/*
===============================================================================

WAV loading

===============================================================================
*/


byte	*data_p;
byte 	*iff_end;
byte 	*last_chunk;
byte 	*iff_data;
int 	iff_chunk_len;


short GetLittleShort(void)
{
	short val = 0;
	val = *data_p;
	val = val + (*(data_p+1)<<8);
	data_p += 2;
	return val;
}

int GetLittleLong(void)
{
	int val = 0;
	val = *data_p;
	val = val + (*(data_p+1)<<8);
	val = val + (*(data_p+2)<<16);
	val = val + (*(data_p+3)<<24);
	data_p += 4;
	return val;
}

void FindNextChunk(char *name)
{
	while (1)
	{
		data_p=last_chunk;

		if (data_p >= iff_end)
		{	// didn't find the chunk
			data_p = NULL;
			return;
		}
		
		data_p += 4;
		iff_chunk_len = GetLittleLong();
		if (iff_chunk_len < 0)
		{
			data_p = NULL;
			return;
		}
//		if (iff_chunk_len > 1024*1024)
//			Sys_Error ("FindNextChunk: %i length is past the 1 meg sanity limit", iff_chunk_len);
		data_p -= 8;
		last_chunk = data_p + 8 + ( (iff_chunk_len + 1) & ~1 );
		if (!strncmp((const char *)data_p, name, 4))
			return;
	}
}

void FindChunk(char *name)
{
	last_chunk = iff_data;
	FindNextChunk (name);
}


void DumpChunks(void)
{
	char	str[5];
	
	str[4] = 0;
	data_p=iff_data;
	do
	{
		memcpy (str, data_p, 4);
		data_p += 4;
		iff_chunk_len = GetLittleLong();
		Com_Printf ("0x%x : %s (%d)\n", (int)(data_p - 4), str, iff_chunk_len);
		data_p += (iff_chunk_len + 1) & ~1;
	} while (data_p < iff_end);
}

/*
============
GetWavinfo
============
*/
wavinfo_t GetWavinfo (char *name, byte *wav, int wavlength)
{
	wavinfo_t	info;
	int     i;
	int     format;
	int		samples;

	memset (&info, 0, sizeof(info));

	if (!wav)
		return info;
		
	iff_data = wav;
	iff_end = wav + wavlength;

// find "RIFF" chunk
	FindChunk("RIFF");
	if (!(data_p && !strncmp((const char *)data_p+8, "WAVE", 4)))
	{
		Com_Printf("Missing RIFF/WAVE chunks\n");
		return info;
	}

// get "fmt " chunk
	iff_data = data_p + 12;
// DumpChunks ();

	FindChunk("fmt ");
	if (!data_p)
	{
		Com_Printf("Missing fmt chunk\n");
		return info;
	}
	data_p += 8;
	format = GetLittleShort();
	if (format != 1)
	{
		Com_Printf("Microsoft PCM format only\n");
		return info;
	}

	info.channels = GetLittleShort();
	info.rate = GetLittleLong();
	data_p += 4+2;
	info.width = GetLittleShort() / 8;

// get cue chunk
	FindChunk("cue ");
	if (data_p)
	{
		data_p += 32;
		info.loopstart = GetLittleLong();
//		Com_Printf("loopstart=%d\n", sfx->loopstart);

	// if the next chunk is a LIST chunk, look for a cue length marker
		FindNextChunk ("LIST");
		if (data_p)
		{
			if (!strncmp ((const char *)data_p + 28, "mark", 4))
			{	// this is not a proper parse, but it works with cooledit...
				data_p += 24;
				i = GetLittleLong ();	// samples in loop
				info.samples = info.loopstart + i;
//				Com_Printf("looped length: %i\n", i);
			}
		}
	}
	else
		info.loopstart = -1;

// find data chunk
	FindChunk("data");
	if (!data_p)
	{
		Com_Printf("Missing data chunk\n");
		return info;
	}

	data_p += 4;
	samples = GetLittleLong () / info.width;

	if (info.samples)
	{
		if (samples < info.samples)
			Com_Error (ERR_DROP, "Sound %s has a bad loop length", name);
	}
	else
		info.samples = samples;

	info.dataofs = data_p - wav;
	
	return info;
}

#endif
