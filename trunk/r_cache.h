#ifndef __r_cache_h__
#define __r_cache_h__

#include <nds/ndstypes.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>

void r_cache_print(int size);
void r_cache_clear();
void r_cache_init();
byte* r_cache_alloc(int size);
byte* r_cache_alloc_temp(int size);
void r_cache_free_temp(void *p);
byte *r_cache_current(int x);
int r_cache_end();
void r_cache_valid_ptr(void *p);

#define Z_Malloc(_x) r_cache_alloc_temp(_x)

#define Hunk_Alloc(_x) r_cache_alloc(_x)

#define Z_Free(_x) r_cache_free_temp(_x)

#define Hunk_Begin(_x) r_cache_current(_x)

#define Hunk_End r_cache_end

#define Hunk_Free(_x)

#endif