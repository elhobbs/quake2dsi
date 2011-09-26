#include "ref_nds/r_local.h"
#include "r_cache.h"
#ifdef ARM9
#include <nds/arm9/input.h>
#endif
#include <malloc.h>

byte *r_model_cache = 0;
int r_model_cache_total = 8.5*1024*1024;
int r_model_cache_used = 0;
int r_model_cache_temp = 0;
int r_cache_count = 0;
int r_cache_max = 0;

int r_cache_fail;

extern hunk_t ds_snd_cache;
extern hunk_t ds_md2_cache;

void r_cache_set_fail(int mode) {
	r_cache_fail = mode == 0 ? 0 : 1;
}

void disable_keyb(void);
void waitforit() {
#ifdef ARM9
	while((keysCurrent()&KEY_A) == 0);
	while((keysCurrent()&KEY_A) != 0);
#endif
}

void print_top_four_blocks_kb(void);
void r_cache_print(int size) {
	printf("tot: %d fre: %d\n",r_model_cache_total,r_model_cache_total-r_model_cache_used-r_model_cache_temp);
	printf("use: %d tmp: %d\n",r_model_cache_used,r_model_cache_temp);
	printf("max: %d cbt: %d\n",r_cache_max,r_cache_count);
	printf("wnt: %d\n",size);
	print_top_four_blocks_kb();
}

void r_cache_stat(char *str) {
	return;
	printf(str);
	r_cache_print(0);
	printf("press A...\n");
#ifdef ARM9
	while((keysCurrent()&KEY_A) == 0);
	while((keysCurrent()&KEY_A) != 0);
#endif
}
void r_cache_print_f(void) {
	r_cache_print(0);
}

void S_UnloadAllSounds(void);
void S_StopAllSounds(void);

int r_rache_is_empty = 1;
void r_cache_clear() {
	Com_DPrintf("r_cache_clear\n");
#ifdef _ARM9
	while((keysCurrent()&KEY_A) == 0);
	while((keysCurrent()&KEY_A) != 0);
#endif
	S_StopAllSounds();
	S_UnloadAllSounds();
	r_cache_set_fail(1);
	r_rache_is_empty = 1;
	disable_keyb();
	//printf("r_cache_clear\n");
	//while((keysCurrent()&KEY_A) == 0);
	//while((keysCurrent()&KEY_A) != 0);
	//printf("r_cache_clear\n");
	//r_cache_print(0);
	//while((keysCurrent() & KEY_A) == 0);
	//while((keysCurrent() & KEY_A) != 0);
	r_cache_count = 0;
	r_cache_max = 0;
	r_model_cache_used = r_model_cache_temp = 0;
	ds_snd_cache.invalid = 1;
	ds_md2_cache.invalid = 1;

}

void r_cache_valid_ptr(void *p) {
	byte *pp = (byte *)p;

	if(pp < r_model_cache || pp>(r_model_cache+r_model_cache_total)) {
		printf("ERROR: r_cache_valid_ptr\n");
		printf("pp: %08X\n",pp);
		r_cache_print(0);
		disable_keyb();
		while(1);
	}

}

void r_cache_init() {
	r_model_cache = (byte *)malloc(r_model_cache_total+31);
	if(r_model_cache == 0) {
		printf("ERROR: r_model_cache == 0\n");
		disable_keyb();
		while(1);
	}
	r_model_cache = (byte *)(((unsigned int)r_model_cache + 31) & ~31);
	r_cache_clear();
	Cmd_AddCommand( "cache", r_cache_print_f );
}
static int r_model_cache_used_last;
int r_cache_end() {
	return r_model_cache_used - r_model_cache_used_last;
}

byte *r_cache_current(int x) {
	r_model_cache_used_last = r_model_cache_used;
	byte *buf = &r_model_cache[r_model_cache_used];
	return buf;
}

byte* r_cache_alloc(int size) {
	byte *buf;

	//size = ((size + 15) & (~15));
	size = ((size + 31) & (~31));
	if(r_model_cache_used + r_model_cache_temp + size > r_model_cache_total) {
#ifdef ARM9
		register unsigned int lr_r asm ("lr");
		unsigned int lr = lr_r;
#endif
		if(r_cache_fail == 0) {
			return 0;
		}
#ifdef ARM9
		printf("ERROR: r_cache_alloc %08X\n",lr);
#else
		printf("ERROR: r_cache_alloc\n");
#endif
		r_cache_print(size);
		disable_keyb();
		Com_Error (ERR_DROP,"r_cache_alloc failed\n");
	}
	buf = &r_model_cache[r_model_cache_used];
	memset(buf,0,size);
	r_model_cache_used += size;
	r_cache_count++;
	if(r_model_cache_used + r_model_cache_temp > r_cache_max) {
		r_cache_max = r_model_cache_used + r_model_cache_temp;
	}
	return buf;
}

byte* r_cache_alloc_temp(int size) {
	byte *buf;

	//size = ((size + 15) & (~15));
	size = ((size + 3) & (~3));
	if(r_model_cache_used + r_model_cache_temp + size > r_model_cache_total) {
#ifdef ARM9
		register unsigned int lr_r asm ("lr");
		unsigned int lr = lr_r;
#else
		unsigned int lr = 0;
#endif

		if(r_cache_fail == 0) {
			return 0;
		}
		printf("ERROR: r_cache_alloc_temp %08X\n",lr);
		r_cache_print(size);
		disable_keyb();
		Com_Error (ERR_DROP,"r_cache_alloc_temp failed\n");
	}
	r_model_cache_temp += size;
	buf = &r_model_cache[r_model_cache_total - r_model_cache_temp];
	memset(buf,0,size);
	if(r_model_cache_used + r_model_cache_temp > r_cache_max) {
		r_cache_max = r_model_cache_used + r_model_cache_temp;
	}
	return buf;
}

void r_cache_free_temp(void *p) {
	r_model_cache_temp = 0;
}

#if 0
/*============================================================================


//============================================================================*/

#define GUARDSIZE       4

void DS_CacheCheckGuard (cache_t *cache)
{
	byte    *s;
	int             i;

	s = (byte *)cache->base + cache->size;
	for (i=0 ; i<GUARDSIZE ; i++)
		if (s[i] != (byte)i)
			Sys_Error ("DS_CheckCacheGuard: failed");
}

void DS_CacheClearGuard (cache_t *cache)
{
	byte    *s;
	int             i;
	
	s = (byte *)cache->base + cache->size;
	for (i=0 ; i<GUARDSIZE ; i++)
		s[i] = (byte)i;
}

void DS_CacheInit (cache_t *cache,void *buffer, int size)
{

	//if (!msg_suppress_1)
	//	Con_Printf ("%ik cache\n", size/1024);

	cache->size = size - GUARDSIZE;
	cache->base = (cached_t *)buffer;
	cache->rover = cache->base;
	
	cache->base->next = cache->base;//NULL;
	cache->base->owner = NULL;
	cache->base->size = cache->size;
	
	DS_CacheClearGuard (cache);
}

void DS_CacheReset (cache_t *cache)
{

	cache->rover = cache->base;
	
	cache->base->next = cache->base;//NULL;
	cache->base->owner = NULL;
	cache->base->size = cache->size;
	
	DS_CacheClearGuard (cache);
}

void DS_CachePrint (cache_t *cache)
{
	cached_t     *c;
	int vis,total;
	
	if (!cache->base)
		return;

	total = vis = 0;
	for (c = cache->base ; c ; c = c->next)
	{
		if(c->owner)
			total += c->size;
		if(c->visframe == r_framecount)
			vis += c->size;
		if(c->next < c)
			break;
	}
	
	Com_Printf("vct: %d %d %d\n",vis,total,cache->size);
}

void DS_CacheFlush (cache_t *cache)
{
	cached_t     *c;
	
	if (!cache->base)
		return;

	for (c = cache->base ; c && c != cache->base ; c = c->next)
	{
		if (c->owner)
			*c->owner = NULL;
	}
	
	cache->rover = cache->base;
	cache->base->next = cache->base;//NULL;
	cache->base->owner = NULL;
	cache->base->size = cache->size;
}

cached_t *DS_CacheAlloc (cache_t *cache, int size)
{
	cached_t             *cached,*next;
	
	//size = (int)&((cached_t *)0)->data[size];
	size += sizeof(cached_t);
	size = (size + 15) & ~15;
	//size = (size + 127) & ~127;
	if (size > cache->size) {
		return 0;
		//Sys_Error ("DS_SCAlloc: %i > cache size",size);
	}

	// colect and free surfcache_t blocks until the rover block is large enough
	cached = cache->rover;
	
	while (cached->size < size)
	{
		//check if other is being used this frame
		if(cached->visframe)
		{
			cached->visframe = 0;
			cached = cached->next;
			continue;
		}
		if(cached->next->visframe)
		{
			cached->next->visframe = 0;
			cached = cached->next->next;
			continue;
		}
		//check for wrap
		if(cached->next < cached) {
			cached = cache->base;
			continue;
		}
	//merge next
		if (cached->next->owner)
			*cached->next->owner = NULL;

			
		cached->size += cached->next->size;
		cached->next = cached->next->next;
	}
	
// create a fragment out of any leftovers
	if (cached->size - size >= 64)
	{
		next = (cached_t *)( (byte *)cached + size);
		if(next >= (cached_t *)( (byte *)cache->base + cache->size))
		{
			next = 0;
		}
		next->visframe = 0;
		next->size = cached->size - size;
		next->next = cached->next;
		next->owner = NULL;
		cached->next = next;
		cached->size = size;
	}

	cache->rover = cached->next;
	
	cached->owner = NULL;              // should be set properly after return
	cached->visframe = r_framecount;

	//DS_CacheCheckGuard (cache);   // DEBUG
	return cached;
}

#endif

/*
===============================================================================

CACHE MEMORY

===============================================================================
*/


static cache_system_t *Cache_TryAlloc (hunk_t *hunk,int size, qboolean nobottom);


/*
===========
Cache_Move
===========
*/
static void Cache_Move (hunk_t *hunk, cache_system_t *c)
{
	cache_system_t		*new_cs;

// we are clearing up space at the bottom, so only allocate it late
	new_cs = (cache_system_t *)Cache_TryAlloc (hunk,c->size, true);
	if (new_cs)
	{
	//	Con_Printf ("cache_move ok\n");
		memcpy (new_cs+1, c+1, c->size - sizeof(cache_system_t));
		new_cs->user = c->user;
		//memcpy (new_cs->name, c->name, sizeof(new_cs->name));
		Cache_Free (hunk,c->user);
		new_cs->user->data = (void *)(new_cs + 1);
	}
	else
	{
	//	Con_Printf ("cache_move failed\n");
		Cache_Free (hunk,c->user);	// tough luck...
	}
}

/*
============
Cache_FreeLow

Throw things out until the hunk can be expanded to the given point
============
*/
static void Cache_FreeLow (hunk_t *hunk,int new_low_hunk)
{
	cache_system_t	*c;

	while (1)
	{
		c = hunk->cache_head.next;
		if (c == &(hunk->cache_head))
			return;		// nothing in cache at all
		if ((byte *)c >= hunk->hunk_base + new_low_hunk)
			return;		// there is space to grow the hunk
		Cache_Move ( hunk,c );	// reclaim the space
	}
}

/*
============
Cache_FreeHigh

Throw things out until the hunk can be expanded to the given point
============
*/
static void Cache_FreeHigh (hunk_t *hunk,int new_high_hunk)
{
	cache_system_t	*c, *prev;

	prev = NULL;
	while (1)
	{
		c = hunk->cache_head.prev;
		if (c == &(hunk->cache_head))
			return;		// nothing in cache at all
		if ( (byte *)c + c->size <= hunk->hunk_base + hunk->hunk_size - new_high_hunk)
			return;		// there is space to grow the hunk
		if (c == prev)
			Cache_Free (hunk,c->user);	// didn't move out of the way
		else
		{
			Cache_Move (hunk,c);	// try to move it
			prev = c;
		}
	}
}

static void Cache_UnlinkLRU (cache_system_t *cs)
{
	if (!cs->lru_next || !cs->lru_prev)
		Sys_Error ("%s: NULL link", "Cache_UnlinkLRU");

	cs->lru_next->lru_prev = cs->lru_prev;
	cs->lru_prev->lru_next = cs->lru_next;

	cs->lru_prev = cs->lru_next = NULL;
}

static void Cache_MakeLRU (hunk_t *hunk,cache_system_t *cs)
{
	if (cs->lru_next || cs->lru_prev)
		Sys_Error ("%s: active link", "Cache_MakeLRU");

	hunk->cache_head.lru_next->lru_prev = cs;
	cs->lru_next = hunk->cache_head.lru_next;
	cs->lru_prev = &(hunk->cache_head);
	hunk->cache_head.lru_next = cs;
}

/*
============
Cache_TryAlloc

Looks for a free block of memory between the high and low hunk marks
Size should already include the header and padding
============
*/
static cache_system_t *Cache_TryAlloc (hunk_t *hunk,int size, qboolean nobottom)
{
	cache_system_t	*cs, *new_cs;

// is the cache completely empty?

	if (!nobottom && hunk->cache_head.prev == &(hunk->cache_head))
	{
		if (hunk->hunk_size - hunk->hunk_high_used - hunk->hunk_low_used < size)
			Sys_Error ("%s: out of hunk memory (failed to allocate %i bytes)", "Cache_TryAlloc", size);

		new_cs = (cache_system_t *) (hunk->hunk_base + hunk->hunk_low_used);
		memset (new_cs, 0, sizeof(*new_cs));
		new_cs->size = size;

		hunk->cache_head.prev = hunk->cache_head.next = new_cs;
		new_cs->prev = new_cs->next = &(hunk->cache_head);

		Cache_MakeLRU (hunk,new_cs);
		return new_cs;
	}

// search from the bottom up for space

	new_cs = (cache_system_t *) (hunk->hunk_base + hunk->hunk_low_used);
	cs = hunk->cache_head.next;

	do
	{
		if (!nobottom || cs != hunk->cache_head.next)
		{
			if ((byte *)cs - (byte *)new_cs >= size)
			{	// found space
				memset (new_cs, 0, sizeof(*new_cs));
				new_cs->size = size;

				new_cs->next = cs;
				new_cs->prev = cs->prev;
				cs->prev->next = new_cs;
				cs->prev = new_cs;

				Cache_MakeLRU (hunk,new_cs);

				return new_cs;
			}
		}

	// continue looking
		new_cs = (cache_system_t *)((byte *)cs + cs->size);
		cs = cs->next;

	} while (cs != &(hunk->cache_head));

// try to allocate one at the very end
	if (hunk->hunk_base + hunk->hunk_size - hunk->hunk_high_used - (byte *)new_cs >= size)
	{
		memset (new_cs, 0, sizeof(*new_cs));
		new_cs->size = size;

		new_cs->next = &(hunk->cache_head);
		new_cs->prev = hunk->cache_head.prev;
		hunk->cache_head.prev->next = new_cs;
		hunk->cache_head.prev = new_cs;

		Cache_MakeLRU (hunk,new_cs);

		return new_cs;
	}

	return NULL;		// couldn't allocate
}

/*
============
Cache_Flush

Throw everything out, so new data will be demand cached
============
*/
void Cache_Flush (hunk_t *hunk)
{
	while (hunk->cache_head.next != &(hunk->cache_head))
		Cache_Free ( hunk, hunk->cache_head.next->user );	// reclaim the space
}


/*
============
Cache_Report

============
*/
void Cache_Report (hunk_t *hunk)
{
	Com_Printf ("%4.1f megabyte data cache\n", (hunk->hunk_size - hunk->hunk_high_used - hunk->hunk_low_used) / (float)(1024*1024) );
}

/*
============
Cache_Init

============
*/
void Cache_Init (hunk_t *hunk,void *buf,int size)
{
	hunk->invalid = 0;
	hunk->hunk_base = (byte *) buf;
	hunk->hunk_size = size;
	hunk->hunk_low_used = 0;
	hunk->hunk_high_used = 0;
	hunk->cache_head.next = hunk->cache_head.prev = &(hunk->cache_head);
	hunk->cache_head.lru_next = hunk->cache_head.lru_prev = &(hunk->cache_head);
}

/*
==============
Cache_Free

Frees the memory and removes it from the LRU list
==============
*/
void Cache_Free (hunk_t *hunk,cache_user_t *c)
{
	cache_system_t	*cs;

	if (!c->data)
		Sys_Error ("%s: not allocated", "Cache_Free");

	cs = ((cache_system_t *)c->data) - 1;

	cs->prev->next = cs->next;
	cs->next->prev = cs->prev;
	cs->next = cs->prev = NULL;

	c->data = NULL;

	Cache_UnlinkLRU (cs);
}


/*
==============
Cache_Check
==============
*/
void *Cache_Check (hunk_t *hunk,void **cc)
{
	cache_user_t *c = (cache_user_t *)cc;
	cache_system_t	*cs;

	if (!c->data)
		return NULL;

	cs = ((cache_system_t *)c->data) - 1;

// move to head of LRU
	Cache_UnlinkLRU ( cs);
	Cache_MakeLRU (hunk,cs);

	return c->data;
}


/*
==============
Cache_Alloc
==============
*/
void *Cache_Alloc (hunk_t *hunk,cache_user_t *c, int size, const char *name)
{
	cache_system_t	*cs;
	extern int s_in_precache;

	if(r_rache_is_empty || hunk->invalid) {
		return 0;
	}

	if (c->data)
		Sys_Error ("%s: %s is already allocated", "Cache_Alloc", name);

	if (size <= 0)
		Sys_Error ("%s: bad size %i for %s", "Cache_Alloc", size, name);

	size = (size + sizeof(cache_system_t) + 15) & ~15;

// find memory for it
	while (1)
	{
		cs = Cache_TryAlloc (hunk, size, false);
		if (cs)
		{
			//q_strlcpy (cs->name, name, CACHENAME_LEN);
			c->data = (void *)(cs + 1);
			cs->user = c;
			break;
		}
		if(s_in_precache) {
			return 0;
		}

	// free the least recently used cahedat
		if (hunk->cache_head.lru_prev == &(hunk->cache_head))	// not enough memory at all
			Sys_Error ("%s: out of memory", "Cache_Alloc");
		Cache_Free (hunk, hunk->cache_head.lru_prev->user );
	}

	return Cache_Check (hunk,(void **)c);
}

