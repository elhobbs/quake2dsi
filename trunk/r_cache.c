#include "ref_nds/r_local.h"
#include "r_cache.h"
#include <nds/arm9/input.h>

byte *r_model_cache = 0;
int r_model_cache_total = 8.5*1024*1024;
int r_model_cache_used = 0;
int r_model_cache_temp = 0;
int r_cache_count = 0;
int r_cache_max = 0;

void disable_keyb(void);
void waitforit() {
	while((keysCurrent()&KEY_A) == 0);
	while((keysCurrent()&KEY_A) != 0);
}

void print_top_four_blocks_kb(void);
void r_cache_print(int size) {
	printf("tot: %d fre: %d\n",r_model_cache_total,r_model_cache_total-r_model_cache_used-r_model_cache_temp);
	printf("use: %d tmp: %d\n",r_model_cache_used,r_model_cache_temp);
	printf("max: %d cbt: %d\n",r_cache_max,r_cache_count);
	printf("wnt: %d\n",size);
	print_top_four_blocks_kb();
}

void r_cache_print_f(void) {
	r_cache_print(0);
}

int r_rache_is_empty = 0;
void r_cache_clear() {
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
	r_model_cache = (byte *)malloc(r_model_cache_total);
	if(r_model_cache == 0) {
		printf("ERROR: r_model_cache == 0\n");
		disable_keyb();
		while(1);
	}
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
	size = ((size + 3) & (~3));
	if(r_model_cache_used + r_model_cache_temp + size > r_model_cache_total) {
		register unsigned int lr_r asm ("lr");
		unsigned int lr = lr_r;

		printf("ERROR: r_cache_alloc %08X\n",lr);
		r_cache_print(size);
		disable_keyb();
		while(1);
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
		register unsigned int lr_r asm ("lr");
		unsigned int lr = lr_r;

		printf("ERROR: r_cache_alloc_temp %08X\n",lr);
		r_cache_print(size);
		disable_keyb();
		while(1);
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
