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

// snddma_null.c
// all other sound mixing is portable
#if 1
#include "../client/client.h"
#include "../client/snd_loc.h"

#define QSND_SKID	    2
#define QSND_BUFFER_FRAMES  8192
#define QSND_BUFFER_SIZE    (QSND_BUFFER_FRAMES)
byte dma_buffer[QSND_BUFFER_SIZE];

qboolean SNDDMA_Init(void)
{
	dma.channels = 1;
	dma.samplebits = 8;
	dma.speed = 11025;

	dma.buffer = (byte *)memUncached(dma_buffer);
    dma.samples = sizeof(dma_buffer)/(dma.samplebits/8);
    dma.submission_chunk = 1;

    dma.samplepos = 0;

	soundEnable();
	memset(dma.buffer,0,QSND_BUFFER_SIZE);
	dma.buffer[4] = dma.buffer[5] = 0x7f;	// force a pop for debugging
	
	soundPlaySample(dma_buffer,
		SoundFormat_8Bit,
		QSND_BUFFER_SIZE,
		dma.speed,
		127,
		64,
		true,
		0);

	TIMER_DATA(0) = 0x10000 - (0x1000000 / 11025) * 2;
	TIMER_CR(0) = TIMER_ENABLE | TIMER_DIV_1;
	TIMER_DATA(1) = 0;
	TIMER_CR(1) = TIMER_ENABLE | TIMER_CASCADE | TIMER_DIV_1;
	TIMER_DATA(2) = 0;
	TIMER_CR(2) = TIMER_ENABLE | TIMER_CASCADE | TIMER_DIV_1;

	return true;
}

long long ds_time()
{
	static u16 last;
	static long long t;
	u16 time1 = TIMER1_DATA;
	u16 time = TIMER2_DATA;
	if(time < last) {
		t += (1LL<<32);
	}
	last = time;
	return (t + (time << 16) + time1);
}

/*
return the current sample position (in mono samples, not stereo)
inside the recirculating dma buffer, so the mixing code will know
how many sample are required to fill it up.
*/
int	SNDDMA_GetDMAPos(void)
{
	static long long v;

	v = (ds_time());// - ds_sound_start);
	
	//printf("time: %d\n",(int)v);

	return (((int)v)& (dma.samples-1));
}

void SNDDMA_Shutdown(void)
{
	printf("SNDDMA_Shutdown\n");
}

void SNDDMA_BeginPainting (void)
{
	//printf("SNDDMA_BeginPainting\n");
}

void SNDDMA_Submit(void)
{
	//printf("SNDDMA_Submit\n");
}
#endif