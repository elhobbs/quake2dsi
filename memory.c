#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ARM9
#include <nds.h>
#endif
#include "null/ds.h"
#include "memory.h"
#ifdef ARM9
#include "ram.h"
#endif

#ifdef R21
#define REG_EXEMEMCNT REG_EXMEMCNT
#endif

#ifdef ARM9
extern bool __dsimode;
#define DSI_MEM_BYTES (11.75*1024*1024)
#endif

void ds_set_exram_timings(unsigned int first, unsigned int second)
{
#ifdef ARM9
	REG_EXEMEMCNT = REG_EXEMEMCNT | (first << 2) | (second << 4);
#endif
}

void ds_exram_cache_enable(void)
{
#ifdef ARM9
	return;
	DC_FlushAll();
	
	__asm__ __volatile__ (
		"ldr	r0,=0b01001010\n"
		"mcr	p15, 0, r0, c2, c0, 0\n"
		"mcr	p15, 0, r0, c2, c0, 1\n");
#endif
}

void ds_exram_cache_disable(void)
{
#ifdef ARM9
	return;
	DC_FlushAll();
	
	__asm__ __volatile__ (
		"ldr	r0,=0b01000010\n"
		"mcr	p15, 0, r0, c2, c0, 0\n"
		"mcr	p15, 0, r0, c2, c0, 1\n");
#endif
}

unsigned short *memory_base = 0x0;
unsigned int memory_base_len = 0;
void enable_memory(int type)
{
#ifdef ARM9
	if (memory_base == 0x0)
	{
		if(__dsimode) {
			memory_base = (unsigned short *)malloc(DSI_MEM_BYTES);
		} else {
			if (ram_init((RAM_TYPE)type) == 0)
			{
				printf("\nFailed to detect memory\n\n");
				printf("Verify that you have a slot-2\ncard inserted which contains RAM\n");
				printf("If you think your card should\nhave been detected, try\nunplugging and reconnecting it.\n\n");
				printf("If this does not work, try forcedetection of your card by\nholding down R during game startand then select your card.\n");
				while(1);
			}
			memory_base = (unsigned short *)ram_unlock();
		}
		
	}
		
	ds_exram_cache_enable();
#endif
}

void disable_memory(void)
{
#ifdef ARM9
	ds_exram_cache_disable();
	if(!__dsimode) {
		ram_lock();
	}
#endif
}

int mode = 1;
void ram_mode(void)
{
	if (mode == 1)
	{
#ifdef ARM9
		register unsigned int lr_r asm ("lr");
		unsigned int lr = lr_r;
#else
		unsigned int lr = 0;
#endif
		printf("already in RAM mode, %08x\n", lr);
		while(1);
	}
	mode = 1;
	
	//enable_memory();
}

void disk_mode(void)
{
	if (mode == 0)
	{
#ifdef ARM9
		register unsigned int lr_r asm ("lr");
		unsigned int lr = lr_r;
#else
		unsigned int lr = 0;
#endif
		printf("already in disk mode, %08x\n", lr);
		while(1);
	}
	mode = 0;
#ifdef ARM9
	if(!__dsimode) {
		disable_memory();
	}
#endif
}

void check_mode(void)
{
	if (mode != 0)
	{
		printf("we\'re in the wrong mode for disk access\n");
		while(1);
	}
}

void *get_memory_base(void)
{
	return (void *)memory_base;
}

void *get_memory_base_end(void)
{
	return (void *)(((unsigned char *)memory_base) + memory_base_len);
}

unsigned int get_memory_size(void)
{
#ifdef ARM9
	volatile unsigned short *ptr = (unsigned short *)get_memory_base();
	unsigned int count = 0;
	volatile unsigned short read_back = 0;
	
	if(__dsimode) {
		memory_base_len = DSI_MEM_BYTES;
		return DSI_MEM_BYTES;
	}
	do
	{
		*ptr = 0x1234;
		read_back = *ptr;
		
		count++;
		ptr++;
		
	} while (read_back == 0x1234);

	memory_base_len = (count - 1) * 2;
	
	return (count - 1) * 2;
#else
	return 0;
#endif
}

unsigned int find_memory_size(void)
{
#ifdef ARM9
	volatile unsigned short *ptr = (volatile unsigned short *)get_memory_base();
	unsigned int count = 0;
	volatile unsigned short read_back = 0;
	
	printf("finding extra memory\n");

	if(__dsimode) {
		memory_base_len = DSI_MEM_BYTES;
		return DSI_MEM_BYTES;
	}
	
	do
	{
		if ((count % 2097152) == 0)
			printf("%d.", (count * 2) / 1048576);
			
		*ptr = 0x1234;
		read_back = *ptr;
		count += 8;
		ptr += 8;
		
	} while (read_back == 0x1234);
	
	printf("megs\n");
	
	memory_base_len = (count - 8) * 2;
	return (count - 8) * 2;
#else
	return 0;
#endif
}

void ds_memset(void *addr, unsigned char value, unsigned int length)
{
	int count = length;
	memset(addr,value,length);
	return;
	
//	printf("%.2f kb %d\n", (float)count_largest_block_kb() / 1024, __LINE__);
	
	if ((unsigned int)addr & 0x1)
	{
		byte_write(addr, value);
		count--;
	}
	
	while (count > 1)
	{
		*(unsigned short *)addr = (value << 8) | value;
		
		count -= 2;
		addr = (void *)((unsigned int)addr + 2);
	}
	
	if (count)
		byte_write(addr, value);

//	printf("%.2f kb %d\n", (float)count_largest_block_kb() / 1024, __LINE__);
}

void ds_memset_basic(void *addr, unsigned char value, unsigned int length)
{
	int count;
	for (count = 0; count < length; count++)
	{
		byte_write(addr, value);
		addr = (void *)((unsigned int)addr + 1);
	}
}

void ds_memcpy_basic(void *dest, void *source, unsigned int length)
{
	int count;
	for (count = 0; count < length; count++)
	{
		byte_write(dest, *(unsigned char *)source);
		
		dest = (void *)((unsigned int)dest + 1);
		source = (void *)((unsigned int)source + 1);
	}
}

void ds_memcpy(void *dest, void *source, unsigned int length)
{
//	unsigned int dest_orig = (unsigned int)dest;
//	unsigned int source_orig = (unsigned int)source;
	unsigned int count = length;

//	register unsigned int lr_r asm ("lr");
//	volatile unsigned int lr = lr_r;
//	printf("ds_memcpy from %08x to %08x, length %d, lr %08x\n", source, dest, length, lr);
	
	if ((unsigned int)source & 0x1)
		ds_memcpy_basic(dest, source, length);
	else
	{
		if ((unsigned int)dest & 0x1)
		{
			byte_write(dest, *(unsigned char *)source);
			
			count--;
			dest = (void *)((unsigned int)dest + 1);
			source = (void *)((unsigned int)source + 1);
		}
		
		while (count > 1)
		{
			*(unsigned short *)dest = *(unsigned short *)source;
			
			count -= 2;
			dest = (void *)((unsigned int)dest + 2);
			source = (void *)((unsigned int)source + 2);
		}
		
		if (count)
		{
			byte_write(dest, *(unsigned char *)source);
			
	//		count--;
	//		(unsigned int)dest++;
	//		(unsigned int)source++;
		}
	}
	
//	for (count = 0; count < length; count++)
//	{
//		if (*(unsigned char *)dest_orig != *(unsigned char *)source_orig)
//		{
//			printf("they\'re different\n");
//			printf("source is %08x, dest %08x, curr %d, length %d\n",
//				source_orig, dest_orig, count, length);
//			while(1);
//		}
//			
//		(unsigned int)dest_orig++;
//		(unsigned int)source_orig++;
//	}
}

int count_largest_block(void)
{
	int largest = 0;
	void *ptr = NULL;
	
	do
	{
		if (ptr != NULL)
		{
			free(ptr);
			ptr = NULL;
			largest++;
		}
			
		ptr = malloc(largest + 1);
		
	} while(ptr != NULL);
	
	return largest;
}

int count_largest_block_kb(void)
{
	int largest = 0;
	void *ptr = NULL;
	
	do
	{
		if (ptr != NULL)
		{
			free(ptr);
			ptr = NULL;
			largest += 1024;
		}
			
		ptr = malloc(largest + 1024);
		
	} while(ptr != NULL);
	
	return largest;
}

int count_largest_extra_block_kb(void)
{
	int largest = 0;
	void *ptr = NULL;
	
	ds_set_malloc_base(MEM_XTRA);
	
	do
	{
		if (ptr != NULL)
		{
			ds_free(ptr);
			ptr = NULL;
			largest += 1024;
		}
			
		ptr = ds_malloc(largest + 1024);
		
	} while(ptr != NULL);
	
	ds_set_malloc_base(MEM_MAIN);
	
	return largest;
}

int count_largest_extra_block_mb(void)
{
	int largest = 0;
	void *ptr = NULL;
	
	ds_set_malloc_base(MEM_XTRA);
	
	do
	{
		if (ptr != NULL)
		{
			ds_free(ptr);
			ptr = NULL;
			largest += (1 << 18);
		}
			
		ptr = ds_malloc(largest + (1 << 18));
		
	} while(ptr != NULL);
	
	ds_set_malloc_base(MEM_MAIN);
	
	return largest;
}

void print_top_four_blocks_kb(void)
{
#ifdef ARM9
	void *l1, *l2, *l3, *l4;
	
	printf("big free blocks, kb:\n");
	int size = count_largest_block_kb();
	l1 = malloc(size);
	printf("1: %.2f\n", (float)size / 1024);
	
	size = count_largest_block_kb();
	l2 = malloc(size);
	printf("2: %.2f\n", (float)size / 1024);
	
	size = count_largest_block_kb();
	l3 = malloc(size);
	printf("3: %.2f\n", (float)size / 1024);
	
	size = count_largest_block_kb();
	l4 = malloc(size);
	printf("4: %.2f\n", (float)size / 1024);
	
	free(l1);
	free(l2);
	free(l3);
	free(l4);
	
	printf("done\n");
#endif
}

void print_top_four_blocks(void)
{
	void *l1, *l2, *l3, *l4;
	
	printf("big free blocks, kb:\n");
	int size = count_largest_block();
	l1 = malloc(size);
	printf("1: %.2f\n", (float)size / 1024);
	
	size = count_largest_block();
	l2 = malloc(size);
	printf("2: %.2f\n", (float)size / 1024);
	
	size = count_largest_block();
	l3 = malloc(size);
	printf("3: %.2f\n", (float)size / 1024);
	
	size = count_largest_block();
	l4 = malloc(size);
	printf("4: %.2f\n", (float)size / 1024);
	
	free(l1);
	free(l2);
	free(l3);
	free(l4);
	
	printf("done\n");
}

void ds_strcpy (char *dest, char *src)
{
	while (*src)
	{
		byte_write(dest, *src);
		
		dest++;
		src++;
	}
	byte_write(dest, 0);
	dest++;
}

bool is_exram(void *p)
{
	if ((p >= get_memory_base()) && ((unsigned int)p < (unsigned int)get_memory_base_end()))
		return true;
	else
		return false;
}
