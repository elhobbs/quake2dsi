#include "r_cache.h"
#include <nds/arm9/input.h>

byte *r_model_cache = 0;
int r_model_cache_total = 8.5*1024*1024;
int r_model_cache_used = 0;
int r_model_cache_temp = 0;

void print_top_four_blocks_kb(void);

void r_cache_print(int size) {
	printf("tot: %d fre: %d\n",r_model_cache_total,r_model_cache_total-r_model_cache_used-r_model_cache_temp);
	printf("use: %d tmp: %d\n",r_model_cache_used,r_model_cache_temp);
	printf("wnt: %d\n",size);
	print_top_four_blocks_kb();
}

void r_cache_clear() {
	printf("r_cache_clear\n");
	r_cache_print(0);
	while((keysCurrent() & KEY_A) == 0);
	while((keysCurrent() & KEY_A) != 0);
	r_model_cache_used = r_model_cache_temp = 0;
}

void r_cache_init() {
	r_model_cache = (byte *)malloc(r_model_cache_total);
	if(r_model_cache_total == 0) {
		printf("ERROR: r_model_cache_total == 0\n");
		while(1);
	}
	r_cache_clear();
}

byte *r_cache_current(int x) {
	byte *buf = &r_model_cache[r_model_cache_used];
	return buf;
}

byte* r_cache_alloc(int size) {
	byte *buf;

	size = ((size + 15) & (~15));
	if(r_model_cache_used + r_model_cache_temp + size > r_model_cache_total) {
		printf("ERROR: r_cache_alloc\n");
		r_cache_print(size);
		while(1);
	}
	buf = &r_model_cache[r_model_cache_used];
	memset(buf,0,size);
	r_model_cache_used += size;
	return buf;
}

byte* r_cache_alloc_temp(int size) {
	byte *buf;

	size = ((size + 15) & (~15));
	if(r_model_cache_used + r_model_cache_temp + size > r_model_cache_total) {
		printf("ERROR: r_cache_alloc_temp\n");
		r_cache_print(size);
		while(1);
	}
	r_model_cache_temp += size;
	buf = &r_model_cache[r_model_cache_total - r_model_cache_temp];
	memset(buf,0,size);
	return buf;
}

void r_cache_free_temp(void *p) {
	r_model_cache_temp = 0;
}
