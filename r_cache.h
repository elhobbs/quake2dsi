#ifndef __r_cache_h__
#define __r_cache_h__

#ifdef ARM9
#include <nds/ndstypes.h>
#endif

#include <stdio.h>
#include <malloc.h>
#include <string.h>

void r_cache_set_fail(int mode);
void r_cache_print(int size);
void r_cache_clear();
void r_cache_init();
byte* r_cache_alloc(int size);
byte* r_cache_alloc_temp(int size);
void r_cache_free_temp(void *p);
byte *r_cache_current(int x);
int r_cache_end();
void r_cache_valid_ptr(void *p);

extern int r_rache_is_empty;

#define Z_Malloc(_x) r_cache_alloc_temp(_x)

#define Hunk_Alloc(_x) r_cache_alloc(_x)

#define Z_Free(_x) r_cache_free_temp(_x)

#define Hunk_Begin(_x) r_cache_current(_x)

#define Hunk_End r_cache_end

#define Hunk_Free(_x)

typedef struct cache_user_s
{
	void	*data;
} cache_user_t;


//#define CACHENAME_LEN	32
typedef struct cache_system_s
{
	int				size;		// including this header
	cache_user_t		*user;
	//char			name[CACHENAME_LEN];
	struct cache_system_s	*prev, *next;
	struct cache_system_s	*lru_prev, *lru_next;	// for LRU flushing
} cache_system_t;

typedef struct hunk_s {
	byte	*hunk_base;
	int	hunk_size;
	int	invalid;

	int	hunk_low_used;
	int	hunk_high_used;
	cache_system_t	cache_head;
} hunk_t;

void Cache_Flush (hunk_t *hunk);

void *Cache_Check (hunk_t *hunk,void **cc);
// returns the cached data, and moves to the head of the LRU list
// if present, otherwise returns NULL

void Cache_Free (hunk_t *hunk,cache_user_t *c);

void *Cache_Alloc (hunk_t *hunk,cache_user_t *c, int size, const char *name);
// Returns NULL if all purgable data was tossed and there still
// wasn't enough room.

void Cache_Init (hunk_t *hunk,void *buf, int size);

/*
typedef struct cached_s {
	struct cached_s		*next;
	struct cached_s		**owner;	// NULL is an empty chunk of memory
	int					visframe;	// when added
	int					size;		// including header
	//unsigned char		data[4];
}cached_t;

typedef struct cache_s {
	struct cached_s		*base;
	struct cached_s		*rover;	// NULL is an empty chunk of memory
	int					size;		// including header
}cache_t;

void DS_CacheInit(cache_t *cache,void *buffer, int size);
void DS_CacheReset (cache_t *cache);
cached_t *DS_CacheAlloc (cache_t *cache, int size);
void DS_CacheFlush (cache_t *cache);
void DS_CacheCheckGuard (cache_t *cache);
void DS_CachePrint (cache_t *cache);
*/

#endif