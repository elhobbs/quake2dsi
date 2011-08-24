/*
Copyright (C) 1996-1997 Id Software, Inc

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

#include <nds.h>

#include "client/client.h"
#include "client/snd_loc.h"

#include "quake_ipc.h"

#include "null/ds.h"

#define HW_CHANNELS 16
#define PHYS_CHANNELS 16
#define HW_CHANNEL_START 0

#define RESAMPLE_RATE 11025
//#define RESAMPLE_RATE 22050

////700k
//#define MALLOC_CHUNK_SIZE 4096
//#define MALLOC_CHUNK_SHIFT 12
//#define NUM_MALLOC_CHUNKS 164

////512k
//#define MALLOC_CHUNK_SIZE 2048
//#define MALLOC_CHUNK_SHIFT 11
//#define NUM_MALLOC_CHUNKS 234

//256k
#define MALLOC_CHUNK_SIZE 1024
#define MALLOC_CHUNK_SHIFT 10
#define NUM_MALLOC_CHUNKS 213

//128k
//#define MALLOC_CHUNK_SIZE 512
//#define MALLOC_CHUNK_SHIFT 9
//#define NUM_MALLOC_CHUNKS 210


extern "C" void ds_playsound(void *data, int length, int samplerate);
void ds_playsound_on_channel(void *data, int length, int samplerate, int channel, int vol, int panning);

struct channel
{
	short playing;
	sfx_t *sound;
	short volume;
	short panning;
	short looping;
} hw_channels[PHYS_CHANNELS];

struct staticsounds
{
	sfx_t *sfx;
	float volume;
	float attenuation;
	vec3_t origin;
	int is_playing;
	int is_audible;
	int ds_panning;
	int ds_volume;
	int ds_channel;
} *uncached_statics;

//cvar_t bgmvolume = {"bgmvolume", "1", true};
//cvar_t volume = {"volume", "0.7", true};

//#define WRAM_BASE 0x6000000
//#define WRAM_BASE 0x08000000
//#define WRAM_SIZE (256 * 1024)
//#define WRAM_SIZE (32 * 1024 * 1024)

unsigned int wram_base /*= WRAM_BASE*/;
unsigned int wram_size /*= WRAM_SIZE*/;
unsigned int free_base;

unsigned char chunks_in_use[NUM_MALLOC_CHUNKS];
unsigned char *chunk_base;

unsigned char chunks_in_use_backup[NUM_MALLOC_CHUNKS];


//hack
void refresh_channel_status(void);

sfx_t *known_sfx;
unsigned int *touch_times;
int num_sfx;

bool play_ambients = true;
bool precache_sounds = false;

byte	*data_p;
byte 	*iff_end;
byte 	*last_chunk;
byte 	*iff_data;
int 	iff_chunk_len;

void memclear16(void *ptr, unsigned int hwords)
{
	unsigned short *sptr = (unsigned short *)ptr;
	for (int count = 0; count < hwords; count++)
		sptr[count] = 0;
}

void strcpy16(char *dest, char *src)
{
	int length = 0;
	char *ptr = src;
	
	while (*ptr != 0)
	{
		length++;
		ptr++;
	}
	length++;		//for the null
	
	length++;		//for alignment;
	length = length & 0xfffffffe;
	
	short hwords = length >> 1;
	unsigned short *sdest = (unsigned short *)dest;
	unsigned short *ssrc = (unsigned short *)src;
	
	for (int count = 0; count < hwords; count++)
		sdest[count] = ssrc[count];
}

bool malloc_changed = false;

void *vram_malloc(unsigned int size)
{
	int slots_needed = size >> MALLOC_CHUNK_SHIFT;
	slots_needed += ((size & (MALLOC_CHUNK_SIZE - 1)) != 0);
	
//	ARM7_PRINTF("needs %d slots for %d bytes\n", slots_needed, size);

	if ((size < 1) || (size > MALLOC_CHUNK_SIZE * NUM_MALLOC_CHUNKS))
	{
		ARM7_PRINT("failed allocating ");
		ARM7_PRINT_NUMBER(size);
		ARM7_PRINT(" bytes\n");
		ARM7_HALT();
	}
	
	int found_slot = -1;
	
	for (int count = 0; count < NUM_MALLOC_CHUNKS; count++)
	{
		if (chunks_in_use[count] == 0)
		{
			int length = 0;
			for (int length_count = 0; length_count < NUM_MALLOC_CHUNKS - count; length_count++)
				if (chunks_in_use[count + length_count] == 0)
					length++;
				else
					break;
			
			if (length >= slots_needed)
			{
//				ARM7_PRINTF("found %d slots at %d\n", length, count);
				found_slot = count;
				break;
			}
		}
	}
	
	if (found_slot == -1)
	{
//		ARM7_PRINT("didn\'t find a big enough slot\n");
		return NULL;
	}
	else
	{
		for (int count = 1; count < slots_needed; count++)
			chunks_in_use[count + found_slot] = 255;
		
		chunks_in_use[found_slot] = slots_needed;
		
//		ARM7_PRINTF("found slot at %08x\n", chunk_base + found_slot * MALLOC_CHUNK_SIZE);
		
		malloc_changed = true;
		return chunk_base + found_slot * MALLOC_CHUNK_SIZE;
	}
}

unsigned int vram_get_free_size(void)
{
	int block_count = 0;
	
	for (int count = 0; count < NUM_MALLOC_CHUNKS; count++)
		if (chunks_in_use[count] == 0)
			block_count++;
	
	return block_count * MALLOC_CHUNK_SIZE;
}

void vram_chunk_breakdown(void)
{
	int start = 0;
	do
	{
		while ((chunks_in_use[start] != 0) && (start < NUM_MALLOC_CHUNKS))
			start++;
			
		if (start == NUM_MALLOC_CHUNKS)
		{
			ARM7_PRINTF("no free space\n");
			break;
		}
		
		int length = 0;
		for (int count = start; count < NUM_MALLOC_CHUNKS; count++)
			if (chunks_in_use[count] == 0)
				length++;
			else
				break;
		
		ARM7_PRINTF("chunk at %d, size %d\n", start, length);
		
		start += length;
	} while (start < NUM_MALLOC_CHUNKS);
}

bool needs_defrag(void)
{
	if (malloc_changed == false)
		return false;
	
	int start = 0;
	while (chunks_in_use[start] != 0)
		start++;
	
	ARM7_PRINTF("starting block is %d\n", start);
	
	vram_chunk_breakdown();
	
	int count;
	for (count = start; count < NUM_MALLOC_CHUNKS; count++)
		if (chunks_in_use[count] != 0)
			break;
	
//	malloc_changed = false;
	if (count == NUM_MALLOC_CHUNKS)
	{
		malloc_changed = false;
		return false;
	}
	else
		return true;
}

void defrag(volatile int *free_time)
{
	while (*free_time == kFreeTime)
	{
		int start = 0;
		while ((chunks_in_use[start] != 0) && (start < NUM_MALLOC_CHUNKS))
			start++;
		
		int end = start;
		while ((chunks_in_use[end] == 0) && (end < NUM_MALLOC_CHUNKS))
			end++;
			
		if (end == NUM_MALLOC_CHUNKS)
		{
			malloc_changed = false;
			break;
		}
		
		int length_to_move = chunks_in_use[end];
		
		int sound = -1;
		for (int count = 0; count < num_sfx; count++)
			if ((unsigned int)known_sfx[count].address == ((unsigned int)chunk_base + end * MALLOC_CHUNK_SIZE))
			{
				sound = count;
				break;
			}
		if (sound == -1)
		{
			ARM7_PRINT("couldn't find sound associated with data!\n");
			ARM7_HALT();
		}
		
		known_sfx[sound].address = chunk_base + start * MALLOC_CHUNK_SIZE;
		
		int distance_to_move = end - start;
		
		if (distance_to_move > 0)
		{
			for (int count = end; count < end + length_to_move; count++)
			{
				chunks_in_use[count - distance_to_move] = chunks_in_use[count];
				chunks_in_use[count] = 0;
				
				memcpy(chunk_base + (count - distance_to_move) * MALLOC_CHUNK_SIZE,
					chunk_base + count * MALLOC_CHUNK_SIZE, MALLOC_CHUNK_SIZE);
			}
		}
	}
	
	vram_chunk_breakdown();
}

int num_drops = 0;
int num_dropped_kb = 0;
void vram_free(void *ptr)
{
	unsigned int int_ptr = (unsigned int)ptr;
//	ARM7_PRINTF("freeing %08x\n", (unsigned int)ptr);
	
	if ((int_ptr < (unsigned int)chunk_base) || (int_ptr >= (unsigned int)chunk_base + MALLOC_CHUNK_SIZE * NUM_MALLOC_CHUNKS))
	{
		ARM7_PRINT("trying to free an invalid address!\nout of range: ");
		ARM7_PRINT_NUMBER(int_ptr);
		ARM7_PRINT("\n");
		ARM7_HALT();
	}
	
	if ((int_ptr & (MALLOC_CHUNK_SIZE - 1)) != 0)
	{
		ARM7_PRINT("trying to free an invalid address!\nnot on slot boundary: ");
		ARM7_PRINT_NUMBER(int_ptr);
		ARM7_PRINT("\n");
		ARM7_HALT();
	}
	
	int starting_slot = (int_ptr - (unsigned int)chunk_base) >> MALLOC_CHUNK_SHIFT;
	int slots_used = chunks_in_use[starting_slot];
	
//	ARM7_PRINTF("freeing %d, used %d slots, %d\n", starting_slot, slots_used, vram_get_free_size() >> MALLOC_CHUNK_SHIFT);
	
	if (slots_used == 255)
	{
		ARM7_PRINT("trying to free data contents!\n");
		ARM7_HALT();
	}
	
	if (slots_used == 0)
	{
		ARM7_PRINT("double free to ");
		ARM7_PRINT_NUMBER(ptr);
		ARM7_PRINT("\n");
		ARM7_HALT();
	}
	
	chunks_in_use[starting_slot] = 0;
	
	for (int count = 1; count < slots_used; count++)
	{
		if (chunks_in_use[count + starting_slot] != 255)
		{
			ARM7_PRINT("allocating over used memory!\n");
			ARM7_HALT();
		}
		chunks_in_use[count + starting_slot] = 0;
	}
	
	malloc_changed = true;
	num_drops++;
	num_dropped_kb += (slots_used * MALLOC_CHUNK_SIZE);
}

extern "C" void ds_stopsound(int channel);

void hwc_stopsound(int channel)
{
	if ((channel < 0) || (channel > 15))
	{
		ARM7_PRINT("stopping invalid hardware channel: ");
		ARM7_PRINT_NUMBER(channel);
		ARM7_PRINT("\n");
		ARM7_HALT();
	}
	hw_channels[channel].playing = false;
	
	ds_stopsound(channel);
}

void update_channel_status(int channel, int enabled)
{
//	ARM7_PRINT_NUMBER(enabled);
//	ARM7_PRINT("\n");
	if (channel < PHYS_CHANNELS)
		hw_channels[channel].playing = enabled;
}

int hwc_playsound(sfx_t *sound, void *data, int length, int samplerate, int panning, int volume)
{
	if (data)
	{
		if ((data < (void *)wram_base) || (data >= (void *)(wram_base + wram_size)))
		{
			ARM7_PRINT("playing sound from invalid address\n");
			ARM7_PRINT_NUMBER(data);
			ARM7_HALT();
		}
		
		int channel = -1;
		for (int count = HW_CHANNEL_START; count < HW_CHANNELS; count++)
			if (hw_channels[count].playing == false)
			{
				channel = count;
				break;
			}
		
		int used_channels = 0;
		for (int count = HW_CHANNEL_START; count < HW_CHANNELS; count++)
			if (hw_channels[count].playing == true)
				used_channels++;
		
//		ARM7_PRINTF("chosen channel is %d\n", channel);
//		ARM7_PRINT("chosen channel is\n");
//		ARM7_PRINT_NUMBER(channel);
//		ARM7_PRINT("\n");
		
		if (channel != -1)
		{
//			ARM7_PRINTF("playing %s from %08x, %d\n", sound->name, data, length);
			
			if (length == 0)
			{
				ARM7_PRINT("playing "); ARM7_PRINT(sound->name); ARM7_PRINT("\n");
				ARM7_PRINT_NUMBER(used_channels);
				ARM7_PRINT_NUMBER(data);
				ARM7_PRINT_NUMBER(length);
				ARM7_PRINT("\n");
			}
			
			hw_channels[channel].looping = false;
			hw_channels[channel].panning = panning;
			hw_channels[channel].sound = sound;
			hw_channels[channel].volume = volume;
			
			ds_playsound_on_channel(data, length, samplerate, channel, volume, panning);
			hw_channels[channel].playing = true;
		}
		
		return channel;
	}
	
	return -1;
}

bool is_sound_playing(sfx_t *sound)
{
	for (int count = HW_CHANNEL_START; count < HW_CHANNELS; count++)
		if (hw_channels[count].playing)
			if (hw_channels[count].sound == sound)
				return true;
	
	return false;
}

sfx_t *get_lru(int *lru_index)
{
	unsigned int min = -1;
	sfx_t *id = NULL;
//	
//	for (int count = 0; count < num_sfx; count++)
//		if (!is_sound_playing(&known_sfx[count]))
//			if ((touch_times[count] < min) && (touch_times[count] != 0))
//			{
//				min = touch_times[count];
//				id = &known_sfx[count];
//				*lru_index = count;
//			}
	
	for (int count = 0; count < num_sfx; count++)
//		if (!is_sound_playing(&known_sfx[count]))
			if ((touch_times[count] < min)
				&& ((unsigned int)known_sfx[count].address >= wram_base)
				&& ((unsigned int)known_sfx[count].address < (wram_base + wram_size)))
			{
				min = touch_times[count];
				id = &known_sfx[count];
				*lru_index = count;
			}
	
	return id;
}

int S_GetID(sfx_t *sfx)
{
	for (int count = 0; count < num_sfx; count++)
		if (&known_sfx[count] == sfx)
			return count;
	
	return -1;
}

sfx_t *S_FindName (char *name, int *number)
{
	int		i;
	sfx_t	*sfx;

	if (!name)
	{
		ARM7_PRINT("S_FindName: NULL\n");
		ARM7_HALT();
	}

	if (strlen(name) >= MAX_QPATH)
	{
		ARM7_PRINT ("Sound name too long: ");
		ARM7_PRINT(name);
		ARM7_PRINT("\n");
		ARM7_HALT();
	}

// see if already loaded
	for (i=0 ; i < num_sfx ; i++)
		if (!strcmp(known_sfx[i].name, name))
		{
			*number = i;
			return &known_sfx[i];
		}

	if (num_sfx == MAX_SFX)
	{
		ARM7_PRINT ("S_FindName: out of sfx_t");
		ARM7_HALT();
	}
	
	*number = i;
	sfx = &known_sfx[i];
//	memset(sfx->name, 0, MAX_QPATH);
	memclear16(sfx->name, MAX_QPATH >> 1);
	strcpy16 (sfx->name, name);
	sfx->address = (void *)0xdeadbeef;		//not yet loaded
	sfx->address_l2 = (void *)0xdeadbeef;	//not yet loaded

	num_sfx++;
	
	return sfx;
}

/* QUAKE WAVE LOADING */
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
		if (!strncmp((char *)data_p, name, 4))
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
		ARM7_PRINTF ("0x%x : %s (%d)\n", (int)(data_p - 4), str, iff_chunk_len);
		data_p += (iff_chunk_len + 1) & ~1;
//		while(1);
	} while (data_p < iff_end);
}

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
	
//	for (int ph = 0; ph < 4; ph++)
//	{
//		for (int count = 0; count < 4; count++)
//			ARM7_PRINTF("%02x%02x%02x%02x ", wav[count * 4], wav[count * 4 + 1],
//					wav[count * 4 + 2], wav[count * 4 + 3]);
//		wav += 16;
//		ARM7_PRINT("\n");
//	}
	
//	DumpChunks ();
//	ARM7_HALT();

// find "RIFF" chunk
	FindChunk("RIFF");
	if (!(data_p && !strncmp((char *)data_p+8, "WAVE", 4)))
	{
		ARM7_PRINT("Missing RIFF/WAVE chunks\n");
		return info;
	}

// get "fmt " chunk
	iff_data = data_p + 12;
// DumpChunks ();

	FindChunk("fmt ");
	if (!data_p)
	{
		ARM7_PRINT("Missing fmt chunk\n");
		return info;
	}
	data_p += 8;
	format = GetLittleShort();
	if (format != 1)
	{
		ARM7_PRINT("Microsoft PCM format only\n");
		return info;
	}

	info.channels = GetLittleShort();
//	ARM7_PRINTF("%d channels\n", info.channels);
	info.rate = GetLittleLong();
//	ARM7_PRINTF("%d rate\n", info.rate);
	data_p += 4+2;
	info.width = GetLittleShort() / 8;
//	ARM7_PRINTF("%d width\n", info.width);

// get cue chunk
	FindChunk("cue ");
	if (data_p)
	{
		data_p += 32;
		info.loopstart = GetLittleLong();
//		Con_Printf("loopstart=%d\n", sfx->loopstart);

	// if the next chunk is a LIST chunk, look for a cue length marker
		FindNextChunk ("LIST");
		if (data_p)
		{
			if (!strncmp ((char *)data_p + 28, "mark", 4))
			{	// this is not a proper parse, but it works with cooledit...
				data_p += 24;
				i = GetLittleLong ();	// samples in loop
				info.samples = info.loopstart + i;
//				Con_Printf("looped length: %i\n", i);
			}
		}
	}
	else
		info.loopstart = -1;

// find data chunk
	FindChunk("data");
	if (!data_p)
	{
		ARM7_PRINT("Missing data chunk\n");
		return info;
	}
	
//	ARM7_PRINTF("found data chunk at %08x\n", (unsigned int)data_p);

	data_p += 4;
	samples = GetLittleLong () / info.width;

	if (info.samples)
	{
		if (samples < info.samples)
		{
			ARM7_PRINT("Sound ");
			ARM7_PRINT(name);
			ARM7_PRINT(" has a bad loop length");
			ARM7_HALT();
		}
	}
	else
		info.samples = samples;

	info.dataofs = data_p - wav;
	
	return info;
}

void ResampleSfx (sfxcache_t *sc, int inrate, int inwidth, byte *data)
{
	int		outcount;
	int		srcsample;
	float	stepscale;
	int		i;
	int		sample, samplefrac, fracstep;

	stepscale = (float)inrate / RESAMPLE_RATE;	// this is usually 0.5, 1, or 2

	outcount = (int)((float)sc->length / stepscale);
	sc->length = outcount;
	if (sc->loopstart != -1)
		sc->loopstart = (int)((float)sc->loopstart / stepscale);

	sc->speed = RESAMPLE_RATE;
//	if (loadas8bit.value)
		sc->width = 1;
//	else
//		sc->width = inwidth;
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
		fracstep = (int)(stepscale*256);
		for (i=0 ; i<outcount ; i++)
		{
			srcsample = samplefrac >> 8;
			samplefrac += fracstep;
			if (inwidth == 2)
				sample = /*ShortSwap */( ((short *)data)[srcsample] );
			else
				sample = (int)( (unsigned char)(data[srcsample]) - 128) << 8;
			if (sc->width == 2)
				((short *)sc->data)[i] = sample;
			else
				((signed char *)sc->data)[i] = sample >> 8;
		}
	}
}

/* END QUAKE WAVE LOADING */

int verify_sounds(void)
{
	for (int count = 0; count < num_sfx; count++)
	{
		void *ptr = known_sfx[count].address;
		sfxcache_t *p = (sfxcache_t *)ptr;
		
		if (p->length < 0)
			return count;
	}
	return -1;
}


volatile bool in_loadsound = false;
bool low_memory = false;

sfxcache_t *S_LoadSound (sfx_t *s)
{
	if (in_loadsound)
	{
		ARM7_PRINT("S_LoadSound re-entry!\n");
		ARM7_HALT();
	}
	else
		in_loadsound = 1;
	sfxcache_t *loaded = NULL;
#if 0	
	if ((s->address != NULL) && (s->address != (void *)0xdeadbeef)  && (s->address != (void *)0xbabebabe))
	{
		in_loadsound = false;
		return (sfxcache_t *)s->address;
	}
	
	if ((s->address_l2 != (void *)0xdeadbeef)
			&& (s->address_l2 != (void *)0xbabebabe)
			&& (s->address_l2 != NULL))
	{
		while(quake_ipc_7to9->message == 0xffffffff);
		
		quake_ipc_7to9->message_type = kNeedBusControl;
		memcpy((void *)quake_ipc_7to9_buf, s->name, strlen(s->name) + 1);
		quake_ipc_7to9->message = 0xffffffff;
		
		while (quake_ipc_7to9->message_type != kHasBusControl);
		
		unsigned int size_needed = *((unsigned int *)s->address_l2);
		unsigned char *ptr = (unsigned char *)vram_malloc(size_needed);

		while (ptr == NULL)
		{	
			int index;
			sfx_t *lru = get_lru(&index);

			if (is_sound_playing(lru))
			{
				for (int count = HW_CHANNEL_START; count < HW_CHANNELS; count++)
					if (hw_channels[count].sound == lru)
						hwc_stopsound(count);
			}


			if (lru == NULL)
			{
				//					ARM7_PRINT("no sound is lru!\n");
				ARM7_HALT();
			}

			touch_times[index] = 0;
			if ((lru->address != (void *)0xdeadbeef) && (lru->address != (void *)0xbabebabe))
			{
				vram_free(lru->address);
				lru->address = (void *)0xbabebabe;
			}

			ptr = (unsigned char *)vram_malloc(size_needed);
		}
		
		loaded = (sfxcache_t *)ptr;
		dmaCopyHalfWords(3, (void *)((unsigned int)s->address_l2 + 4), ptr, size_needed);
		s->address = ptr;
		
		if (loaded->length == 0)
		{
			quake_ipc_7to9->message_type = kS_LoadSound;
			while (quake_ipc_7to9->message == 0xffffffff);
	
			ARM7_PRINT("sound "); ARM7_PRINT(s->name); ARM7_PRINT(" length is zero\n");
			ARM7_PRINT_NUMBER(size_needed);
			ARM7_PRINT_NUMBER((unsigned int)s->address_l2 + 4);
			ARM7_PRINT_NUMBER(ptr);
			ARM7_HALT();
		}
	}
	else
	{
		while(quake_ipc_7to9->message == 0xffffffff);

		memcpy((void *)quake_ipc_7to9_buf, s->name, strlen(s->name) + 1);
		quake_ipc_7to9->message_type = kS_LoadSound;

		quake_ipc_7to9->message = 0xffffffff;

		while (quake_ipc_7to9->message_type != kS_LoadSoundResponse);

		unsigned char *data = (unsigned char *)(*(unsigned int *)quake_ipc_7to9_buf);

		int file_size = ((unsigned int *)quake_ipc_7to9_buf)[1];
		void *address_l2 = (void *)(((unsigned int *)quake_ipc_7to9_buf)[2]);

		unsigned char *ptr = NULL;
		int len = 0;

		if (data != NULL)
		{
			wavinfo_t info = GetWavinfo (s->name, data, file_size);
			float	stepscale;

			if (info.channels != 1)
			{
				quake_ipc_7to9->message_type = kS_LoadSound;
				while (quake_ipc_7to9->message == 0xffffffff);
	
				ARM7_PRINT(s->name);
				ARM7_PRINT(" is a stereo sample (");
				ARM7_PRINT_NUMBER(info.channels);
				ARM7_PRINT(" \n");
			}

			stepscale = (float)info.rate / RESAMPLE_RATE;	
			len = (int)((float)info.samples / stepscale);
			//		len = len * info.width * info.channels;
			len = len * info.channels;			//8-bit samples

			len += 0x14;		//for the header

			if (len < MALLOC_CHUNK_SIZE * NUM_MALLOC_CHUNKS)
			{
				ptr = (unsigned char *)vram_malloc(len);

				while (ptr == NULL)
				{	
					int index;
					sfx_t *lru = get_lru(&index);

					if (is_sound_playing(lru))
					{
						for (int count = HW_CHANNEL_START; count < HW_CHANNELS; count++)
							if (hw_channels[count].sound == lru)
								hwc_stopsound(count);
					}


					if (lru == NULL)
					{
						//					ARM7_PRINT("no sound is lru!\n");
						ARM7_HALT();
					}

					touch_times[index] = 0;
					if ((lru->address != (void *)0xdeadbeef) && (lru->address != (void *)0xbabebabe))
					{
						vram_free(lru->address);
						lru->address = (void *)0xbabebabe;
					}

					ptr = (unsigned char *)vram_malloc(len);
				}

				loaded = (sfxcache_t *)ptr;

				loaded->length = info.samples;
				loaded->loopstart = info.loopstart;
				loaded->speed = info.rate;
				loaded->width = info.width;
				loaded->stereo = info.channels;

				s->address = (void *)loaded;
				s->address_l2 = address_l2;

				ResampleSfx(loaded, loaded->speed, loaded->width, data + info.dataofs);

				if (address_l2)
				{
					dmaCopyHalfWords(3, s->address, (void *)((unsigned int)s->address_l2 + 4), len);
					*((unsigned int *)s->address_l2) = len;			//how much sound data + header there is
				}
				else
					low_memory = true;
			}
			((unsigned int *)quake_ipc_7to9_buf)[2] = len + 4;		//+4 for the size
		}
	}
	
	((unsigned int *)quake_ipc_7to9_buf)[0] = num_drops;
	((unsigned int *)quake_ipc_7to9_buf)[1] = num_dropped_kb >> 10;
	
	quake_ipc_7to9->message_type = kS_LoadSound;
	while (quake_ipc_7to9->message == 0xffffffff);
#endif
	in_loadsound = false;
	return loaded;
}
 
void S_Init7 (unsigned int base, unsigned int size)
{
	//ARM7_PRINT("ARM7: S_Init\n");
	
//	free_base = (WRAM_BASE + base);
//	wram_base = (WRAM_BASE + base);
//	wram_size = WRAM_SIZE - base;
#if 0
	free_base = base;
	wram_base = base;
	wram_size = size;
	
	//ARM7_PRINTF("WRAM from %08x to %08x\n", wram_base, wram_base + wram_size);
	
	/*
FIXME not sure what this is suppoed to do??????
	volatile unsigned short *ptr = (volatile unsigned short *)wram_base;
	for (int count = 0; count < wram_size >> 1; count++)
	{
		ptr[count] = 0x1234;
		if (ptr[count] != 0x1234)
		{
			ARM7_PRINT("WRAM failed r/w test!\n");
			ARM7_HALT();
		}
	}*/
	
	num_sfx = 0;
	
	known_sfx = (sfx_t *)free_base;
	free_base += sizeof(sfx_t) * MAX_SFX;
	
	touch_times = (unsigned int *)free_base;
	free_base += sizeof(unsigned int) * MAX_SFX;
	
//	chunks_in_use = (unsigned char *)free_base;
//	free_base += sizeof(unsigned char) * NUM_MALLOC_CHUNKS;
	
	free_base = ((free_base + MALLOC_CHUNK_SIZE) >> MALLOC_CHUNK_SHIFT) << MALLOC_CHUNK_SHIFT;
	chunk_base = (unsigned char *)free_base;
	free_base += MALLOC_CHUNK_SIZE * NUM_MALLOC_CHUNKS;
	
	if (0) //free_base > wram_size + wram_base)
	{
		ARM7_PRINT("we\'re out of WRAM!\nfree_base is ");
		ARM7_PRINT_NUMBER(free_base);
		ARM7_PRINT("\n");
		ARM7_PRINT("we\'re over by\n");
		ARM7_PRINT_NUMBER(free_base - (wram_size + wram_base));
		ARM7_PRINT("\nbytes\n");
		ARM7_PRINT_NUMBER(wram_size);
		ARM7_PRINT_NUMBER(wram_base);
		ARM7_HALT();
	}
	
	memset(hw_channels, 0, sizeof(struct channel) * HW_CHANNELS);
//	memset(known_sfx, 0, sizeof(sfx_t) * MAX_SFX);
	memclear16(known_sfx, (sizeof(sfx_t) * MAX_SFX) >> 1);
//	memset(touch_times, 0, sizeof(unsigned int) * MAX_SFX);
	memclear16(touch_times, (sizeof(unsigned int) * MAX_SFX) >> 1);
	memset(chunks_in_use, 0, sizeof(unsigned char) * NUM_MALLOC_CHUNKS);
//	memset(chunk_base, 0, MALLOC_CHUNK_SIZE * NUM_MALLOC_CHUNKS);
	memclear16(chunk_base, (MALLOC_CHUNK_SIZE * NUM_MALLOC_CHUNKS) >> 1);
	memset(chunks_in_use_backup, 0, sizeof(unsigned char) * NUM_MALLOC_CHUNKS);
	
	//ARM7_PRINT("sfx at ");
	//ARM7_PRINT_NUMBER(known_sfx);
	//ARM7_PRINT("data at ");
	//ARM7_PRINT_NUMBER(chunk_base);
	//ARM7_PRINT("");
#endif
}

void S_AmbientOff7 (void)
{
	ARM7_PRINT("ARM7: S_AmbientOff\n");
	play_ambients = false;
}

void S_AmbientOn7 (void)
{
	ARM7_PRINT("ARM7: S_AmbientOn\n");
	play_ambients = true;
}

void S_Shutdown7 (void)
{
	ARM7_PRINT("ARM7: S_Shutdown\n");
	
	S_StopAllSounds();
	
	memset(hw_channels, 0, sizeof(struct channel) * HW_CHANNELS);
	num_sfx = 0;
}

void S_TouchSound7 (char *sample)
{
//	ARM7_PRINT("ARM7: S_TouchSound, ");
//	ARM7_PRINT(sample);
//	ARM7_PRINT("\n");
	
	int id;
	
	sfx_t *sfx = S_FindName(sample, &id);
	if (sfx)
	{
		touch_times[id] = hblanks;
	}
}

void S_ClearBuffer7 (void)
{
//	ARM7_PRINT("ARM7: S_ClearBuffer\n");
}

void S_StaticSound7 (void *sfx, float *origin, float vol, float attenuation)
{
//	S_StaticSound((sfx_t *)sfx, origin, vol, attenuation);
}

void S_StaticSound (sfx_t *sfx, vec3_t origin, float vol, float attenuation)
{
//	ARM7_PRINTF("ARM7: S_StaticSound, %s (%08x)\n%.1f %.1f %.1f, ",
//		sfx->name, (unsigned int)sfx, origin[0], origin[1], origin[2], origin[3]);
//	ARM7_PRINTF("%.1f, %.1f\n",	vol, attenuation);
//	S_StartSound(0, 0, sfx, NULL, 127, 64);
	ARM7_PRINTF("static sound %s\n", sfx->name);
}

void S_StartSound7 (int entnum, int entchannel, void *sfx, float *origin, float fvol,  float attenuation)
{
	S_StartSound(origin, entnum, entchannel, (sfx_t *)sfx, fvol, attenuation, 0);
}

volatile bool in_startsound = false;
int last_played_channel = -1;
void S_StartSound (vec3_t origin, int entnum, int entchannel, struct sfx_s *sfx, float fvol,  float attenuation, float timeofs)
{
	if (in_startsound)
	{
		ARM7_PRINTF("S_StartSound re-entry!\n");
		ARM7_HALT();
	}
	in_startsound = 1;
	
	int id = S_GetID(sfx);
	touch_times[id] = hblanks;
	
	if (S_LoadSound(sfx))
	{
//	ARM7_PRINT("ARM7: S_StartSound\n");
//	ARM7_PRINTF("ARM7: S_StartSound, ");
//	ARM7_PRINTF("id is %d\n", id);
//	ARM7_PRINTF("%d %d, ", entnum, entchannel);
//	ARM7_PRINTF("%s (%08x), data %08x ", sfx->name, (unsigned int)sfx, (unsigned int)sfx->address);
//	ARM7_PRINTF("%.1f %.1f %.1f, %.1f, %.1f\n", origin[0], origin[1], origin[2], fvol, attenuation);

//	ARM7_PRINTF("volume is %.2f, attenuation %.2f\n", fvol, attenuation);
	
	void *ptr = sfx->address;
	sfxcache_t *p = (sfxcache_t *)ptr;
	
//	ARM7_PRINTF("%d\n", p->speed);
	
//	ds_playsound(&p->data[0], p->length, p->speed);

//	if ((strstr(sfx->name, "lock4") == NULL))
		last_played_channel = hwc_playsound(sfx, &p->data[0], p->length, p->speed, (int)attenuation, (int)fvol);
//	else
//		ARM7_PRINT("skipping %s\n");
		
//	ARM7_PRINT("ARM7: S_StartSound - out\n");
	}
	
	in_startsound = false;
}

void S_StopSound7 (int entnum, int entchannel)
{
	ARM7_PRINTF("ARM7: S_StopSound, %d %d\n", entnum, entchannel);
}

void *S_PrecacheSound7 (char *sample)
{
	return S_RegisterSound(sample);
}

struct sfx_s *S_RegisterSound (char *sample)
{
	ARM7_PRINTF("ARM7: S_PrecacheSound, %s\n", sample);
	
	int id;
	sfx_t *sfx = S_FindName(sample, &id);
	touch_times[id] = hblanks;
	
	if (precache_sounds)
	{
		S_LoadSound(sfx);
		ARM7_PRINTF("%d bytes free\n", vram_get_free_size());
	}

	
	return sfx;
}

void S_ClearPrecache7 (void)
{
	ARM7_PRINT("ARM7: S_ClearPrecache\n");
}

void S_Update7 (float *origin, float *v_forward, float *v_right, float *v_up)
{
	S_Update(origin, v_forward, v_right, v_up);
}

void S_Update (vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up)
{
//	ARM7_PRINT("ARM7: S_Update\n");
//	ARM7_PRINTF("%.1f %.1f %.1f\n", origin[0], origin[1], origin[2]);
//	ARM7_PRINTF("%.1f %.1f %.1f\n", v_forward[0], v_forward[1], v_forward[2]);
//	ARM7_PRINTF("%.1f %.1f %.1f\n", v_right[0], v_right[1], v_right[2]);
//	ARM7_PRINTF("%.1f %.1f %.1f\n", v_up[0], v_up[1], v_up[2]);
	memcpy(chunks_in_use_backup, chunks_in_use, NUM_MALLOC_CHUNKS);
	
	for (int count = 0; count < num_sfx; count++)
		if (((unsigned int)known_sfx[count].address != 0xdeadbeef)
			&& ((unsigned int)known_sfx[count].address != 0xbabebabe)
			&& ((unsigned int)known_sfx[count].address != 0x0))
			vram_free(known_sfx[count].address);
	
	unsigned int free_size = vram_get_free_size();
	
	if (free_size != MALLOC_CHUNK_SIZE * NUM_MALLOC_CHUNKS)
	{
		ARM7_PRINTF("couldn\'t free VRAM, got %d instead\n", free_size >> MALLOC_CHUNK_SHIFT);
		ARM7_HALT();
	}
	else
	{
		ARM7_PRINTF("%dk available\n", free_size >> MALLOC_CHUNK_SHIFT);
		memcpy(chunks_in_use, chunks_in_use_backup, NUM_MALLOC_CHUNKS);
	}
}

void ds_adjustchannel(int channel, int vol, int pan);

void S_UpdateStatics(void *statics, int num_statics)
{
////	ARM7_PRINTF("updating %d statics from %08x\n", num_statics, (unsigned int)statics);
//	uncached_statics = (struct staticsounds *)statics;
//	
//	for (int count = 0; count < num_statics; count++)
//	{
//		if (uncached_statics[count].is_playing)
//			if (!hw_channels[uncached_statics[count].ds_channel].playing)
//				uncached_statics[count].is_playing = false;
//					
//		if (uncached_statics[count].is_audible)
//		{
////			ARM7_PRINTF("st %s\n", uncached_statics[count].sfx->name);
//			if (uncached_statics[count].is_playing)
//			{
////				hw_channels[uncached_statics[count].ds_channel].panning = uncached_statics[count].ds_panning;
////				hw_channels[uncached_statics[count].ds_channel].volume = uncached_statics[count].ds_volume;
//				
//				ds_adjustchannel(uncached_statics[count].ds_channel,
//					hw_channels[uncached_statics[count].ds_channel].volume,
//					hw_channels[uncached_statics[count].ds_channel].panning);
//			}
//			else
//			{
//				sfx_t *sfx = uncached_statics[count].sfx;
//				S_StartSound(0, 0, sfx, NULL, uncached_statics[count].ds_volume, uncached_statics[count].ds_panning);
//				
//				if (last_played_channel != -1)
//				{
//					uncached_statics[count].is_playing = true;
//					uncached_statics[count].ds_channel = last_played_channel;
//				}
//			}
//		}
//	}
}

void S_StopAllSounds7 (bool clear)
{
	S_StopAllSounds();
}

void S_StopAllSounds (void)
{
	ARM7_PRINTF("ARM7: S_StopAllSounds, %d\n");
	
	for (int count = HW_CHANNEL_START; count < HW_CHANNELS; count++)
		if (hw_channels[count].playing)
			hwc_stopsound(count);
}

void S_BeginPrecaching7 (void)
{
	ARM7_PRINT("ARM7: S_BeginPrecaching\n");
}

void S_EndPrecaching7 (void)
{
	ARM7_PRINT("ARM7: S_EndPrecaching\n");
}

void S_ExtraUpdate7 (void)
{
//	ARM7_PRINT("ARM7: S_ExtraUpdate\n");
}

void S_LocalSound7 (char *s)
{
//	ARM7_PRINT("ARM7: S_LocalSound, ");
//	ARM7_PRINT(s);
//	ARM7_PRINT("\n");
	
	sfx_t *sfx;
	sfx = S_RegisterSound(s);
	S_StartSound(NULL, 0, 0, sfx, 127, 64, 0);
}

void mark_freeable(void)
{
	ARM7_PRINT("ARM7: marking freeable sounds\n");
	
	for (int count = 0; count < MAX_SFX; count++)
		if ((known_sfx[count].address == NULL)							//if it's not in local memory
				|| (known_sfx[count].address == (void *)0xdeadbeef)
				|| (known_sfx[count].address == (void *)0xbabebabe))
		{
			if ((known_sfx[count].address_l2 != (void *)0xdeadbeef)					//but it's in main memory
					&& (known_sfx[count].address_l2 != (void *)0xbabebabe)
					&& (known_sfx[count].address_l2 != NULL))
			{
				known_sfx[count].address_l2 = (void *)((unsigned int)known_sfx[count].address_l2 | (1 << 31));
			}
		}
}

void free_marked(void)
{
	ARM7_PRINT("ARM7: freeable marked sounds\n");
	/*
	while(quake_ipc_7to9->message == 0xffffffff);
	
	quake_ipc_7to9->message_type = kFreeMarkedSounds;
	quake_ipc_7to9->message = 0xffffffff;
	
	while (quake_ipc_7to9->message == 0xffffffff);*/
}
