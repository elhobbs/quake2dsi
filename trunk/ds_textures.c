#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <nds.h>

#include "null/ds.h"

void Sys_Error (char *error, ...);

unsigned int super_framecount;
unsigned int vram_used;

#define USE_3D

#define TEXTURE_VRAM 512
#define MALLOC_CHUNK_SIZE	256
#define MALLOC_CHUNK_SHIFT	8
#define NUM_MALLOC_CHUNKS	2048

#define VRAM_BASE 0x6800000

unsigned char chunks_in_use[NUM_MALLOC_CHUNKS];
unsigned char *chunk_base;

//#define TEXTURES_HAVE_NAMES
#define TEXTURE_CACHE_IN_VRAM

#define USE_EXTRA_RAM

#ifdef USE_EXTRA_RAM
#undef TEXTURE_CACHE_IN_VRAM
#endif

bool malloc_changed;

void vram_init(void)
{
	malloc_changed = false;
	ds_memset(chunks_in_use, 0, sizeof(chunks_in_use));
	chunk_base = (unsigned char *)VRAM_BASE;
	
	printf("vram initialised\n");
}

//FILE *vramfp = NULL;

void *vram_malloc(unsigned int size)
{
//	if (vramfp == NULL)
//	{
//		vramfp = fopen("vram.txt", "w");
//		if (!vramfp)
//			while(1);
//	}
	
	
	int slots_needed = size >> MALLOC_CHUNK_SHIFT;
	slots_needed += ((size & (MALLOC_CHUNK_SIZE - 1)) != 0);
	
//	printf("needs %d slots for %d bytes\n", slots_needed, size);
//	fprintf(vramfp, "needs %d slots for %d bytes\n", slots_needed, size);

	if ((size < 1) || (size > MALLOC_CHUNK_SIZE * NUM_MALLOC_CHUNKS))
	{
		printf("failed allocating %d bytes\n", size);
		while(1);
	}
	
	int found_slot = -1;
	int count, length_count;
	
	for (count = 0; count < NUM_MALLOC_CHUNKS; count++)
	{
		if (chunks_in_use[count] == 0)
		{
			int length = 0;
			for (length_count = 0; length_count < NUM_MALLOC_CHUNKS - count; length_count++)
				if (chunks_in_use[count + length_count] == 0)
					length++;
				else
					break;
			
			if (length >= slots_needed)
			{
//				printf("found %d slots at %d\n", length, count);
				found_slot = count;
				break;
			}
			else
				count += length;
		}
	}
	
	if (found_slot == -1)
	{
//		printf("didn\'t find a big enough slot\n");
//		fprintf(vramfp, "didn\'t find a big enough slot\n");
		return NULL;
	}
	else
	{
		for (count = 1; count < slots_needed; count++)
			chunks_in_use[count + found_slot] = 255;
		
		if (slots_needed < 16)
			chunks_in_use[found_slot] = slots_needed & 0xf;
		else
		{
			chunks_in_use[found_slot] = (slots_needed & 0xf) | (1 << 7);
			chunks_in_use[found_slot + 1] = ((slots_needed & 0xf0) >> 4) | (1 << 7);
			chunks_in_use[found_slot + 2] = ((slots_needed & 0xf00) >> 8) | (1 << 7);
		}
		
//		printf("found slot at %08x\n", chunk_base + found_slot * MALLOC_CHUNK_SIZE);
//		fprintf(vramfp, "found slot at %08x\n", chunk_base + found_slot * MALLOC_CHUNK_SIZE);
		
		malloc_changed = true;
		return chunk_base + found_slot * MALLOC_CHUNK_SIZE;
	}
}

unsigned int vram_get_free_size(void)
{
	int block_count = 0;
	int count;
	
	for (count = 0; count < NUM_MALLOC_CHUNKS; count++)
		if (chunks_in_use[count] == 0)
			block_count++;
	
	return block_count * MALLOC_CHUNK_SIZE;
}

void vram_free(void *ptr)
{
	unsigned int int_ptr = (unsigned int)ptr;
//	printf("freeing %08x\n", (unsigned int)ptr);
//	fprintf(vramfp, "freeing %08x\n", (unsigned int)ptr);
//	while(1);
	
	if ((int_ptr < (unsigned int)chunk_base) || (int_ptr >= (unsigned int)chunk_base + MALLOC_CHUNK_SIZE * NUM_MALLOC_CHUNKS))
	{
		printf("trying to free an invalid address!\nout of range: %08x\n", int_ptr);
		while(1);
	}
	
	if ((int_ptr & (MALLOC_CHUNK_SIZE - 1)) != 0)
	{
		printf("trying to free an invalid address!\nnot on slot boundary: %08x\n", int_ptr);
		while(1);
	}
	
	int starting_slot = (int_ptr - (unsigned int)chunk_base) >> MALLOC_CHUNK_SHIFT;

	int slots_used = chunks_in_use[starting_slot];
	
	if (slots_used >> 7)
		slots_used = (slots_used & 0x7f)
					| ((chunks_in_use[starting_slot + 1] & 0x7f) << 4)
					| ((chunks_in_use[starting_slot + 2] & 0x7f) << 8);
	
//	printf("freeing %d, used %d slots, %d\n", starting_slot, slots_used, vram_get_free_size() >> MALLOC_CHUNK_SHIFT);
//	fprintf(vramfp, "freeing %d, used %d slots, %d\n", starting_slot, slots_used, vram_get_free_size() >> MALLOC_CHUNK_SHIFT);
	
	if (slots_used == 255)
	{
		printf("trying to free data contents!\n");
		while(1);
	}
	
	if (slots_used == 0)
	{
		printf("double free to %08x\n", ptr);
		while(1);
	}
	
	chunks_in_use[starting_slot] = 0;
	
	int count;
	
	for (count = 1; count < slots_used; count++)
	{
//		if (chunks_in_use[count + starting_slot] != 255)
//		{
//			printf("allocating over used memory!\n");
//			while(1);
//		}
		chunks_in_use[count + starting_slot] = 0;
	}
	
	malloc_changed = true;
}

void copy_into_vram(unsigned int *dest, unsigned char *src, int size, int rows, int cols, int realX)
{
	if ((rows == 0) || (cols == 0))
		swiCopy((uint32*)src, dest , size / 4 | COPY_MODE_WORD);
	else
	{
		//not needed?
//		ds_memset(dest, 0, size);
		
		realX = realX >> 2;
		
		int y;
		for (y = 0; y < rows; y++)
			ds_memcpy(&dest[y * realX], &src[y * cols], cols);
	}
}

bool free_lru(int);

#if 1
void glColorTable( uint8 format, uint32 addr ) {
GFX_PAL_FORMAT = addr>>(4-(format==GL_RGB4));
}
//---------------------------------------------------------------------------------
inline uint32 aalignVal( uint32 val, uint32 to ) {
	return (val & (to-1))? (val & ~(to-1)) + to : val;
}
uint32 nextPBlock = (uint32)0;
//---------------------------------------------------------------------------------
int ndsgetNextPaletteSlot(u16 count, uint8 format) {
//---------------------------------------------------------------------------------
	// ensure the result aligns on a palette block for this format
	uint32 result = aalignVal(nextPBlock, 1<<(4-(format==GL_RGB4)));

	// convert count to bytes and align to next (smallest format) palette block
	count = aalignVal( count<<1, 1<<3 ); 

	// ensure that end is within palette video mem
	if( result+count > 0x10000 )   // VRAM_F - VRAM_E
		return -1;

	nextPBlock = result+count;
	return (int)result;
} 
int ds_tex_parameters(	int sizeX, int sizeY,
						const unsigned int* addr,
						int mode,
						unsigned int param)
{
//---------------------------------------------------------------------------------
	return param | (sizeX << 20) | (sizeY << 23) | (((unsigned int)addr >> 3) & 0xFFFF) | (mode << 26);
}
#else
//from libnds, modified to do resizing on the fly
extern "C" uint32* getNextTextureSlot(int size);
#ifdef R21
extern void glTexParameter(	uint8 sizeX, uint8 sizeY,
						const uint32* addr,
						GL_TEXTURE_TYPE_ENUM mode,
						uint32 param) ;
#else
extern "C" void glTexParameter(uint8 sizeX, uint8 sizeY, uint32* addr, int mode, uint32 param);
#endif
#endif
	
unsigned int g_tex_parms;
unsigned int glTexImage2DQuake(int target, int empty1, int type, int sizeX, int sizeY, int empty2, int param, uint8* texture,
	unsigned int rows, unsigned int cols, unsigned int realY, unsigned int realX) {
//---------------------------------------------------------------------------------
  uint32 size = 0;
  uint32* addr;
  uint32 vramTemp;

  size = 1 << (sizeX + sizeY + 6);
  

  switch (type) {
    case GL_RGB:
    case GL_RGBA:
      size = size << 1;
      break;
    case GL_RGB4:
      size = size >> 2;
      break;
    case GL_RGB16:
      size = size >> 1;
      break;

    default:
      break;
  }
  
//  addr = getNextTextureSlot(size);

	do
	{
//		printf("%d is free\n", vram_get_free_size());
		if ((rows == 0) && (cols == 0))
			addr = (uint32 *)vram_malloc(size);
		else
			addr = (uint32 *)vram_malloc(rows * realX);
		
		if (addr == NULL)
		{
			bool res = free_lru(super_framecount - 1);
			
			if (!res)
				return 0;
		}
	}
	while (addr == NULL);
	
	vram_used = NUM_MALLOC_CHUNKS * MALLOC_CHUNK_SIZE - vram_get_free_size();
	
//	if (texture == NULL);
//	printf("got %08x\n", addr);
  
  if(!addr)
  {
	  g_tex_parms = 0;
  	printf("failed to allocates texture memory\n");
  	return 0;
  }

  // unlock texture memory
  vramTemp = vramSetMainBanks(VRAM_A_LCD,VRAM_B_LCD,VRAM_C_LCD,VRAM_D_LCD);

  if (type == GL_RGB) {
    // We do GL_RGB as GL_RGBA, but we set each alpha bit to 1 during the copy
    u16 * src = (u16*)texture;
    u16 * dest = (u16*)addr;
    
    g_tex_parms = ds_tex_parameters(sizeX, sizeY, addr, GL_RGBA, param);
    
    while (size--) {
      *dest++ = *src | (1 << 15);
      src++;
    }
  } else {
    // For everything else, we do a straight copy
#ifdef R21
    g_tex_parms = ds_tex_parameters(sizeX, sizeY, addr, (GL_TEXTURE_TYPE_ENUM)type, param);
#else
	g_tex_parms = ds_tex_parameters(sizeX, sizeY, addr, type, param);
#endif
    if (texture)
		copy_into_vram((unsigned int *)addr, (unsigned char *)texture, size, rows, cols, realX);
  }

  vramRestoreMainBanks(vramTemp);
  
//  printf("left %08x\n", addr);

  return (unsigned int)addr;
}

void ds_resize_in_place(int sizeX, int sizeY, unsigned char *image)
{
//	for (int count = 0; count < sizeX * sizeY; count += 2)
//	{
//		image[count >> 1] = image[count];
//	}
	
//	for (int y = 0; y < sizeY >> 1; y++)
//		memcpy(&image[y * (sizeX >> 1)], &image[(y * 2) * (sizeX >> 1)], sizeX >> 1);
//	memset(&image[(sizeX * sizeY) >> 1], 0, (sizeX * sizeY) >> 1);
	
	int y;
	
	for (y = 0; y < sizeY >> 1; y++)
		ds_memcpy(&image[y * sizeX], &image[(y * 2) * sizeX], sizeX);
}

int fail_textures = 1;

int ds_teximage2d(int sizeX, int sizeY, unsigned char* texture, bool transparency, int transparent_colour)
{
//	if (fail_textures)
//		return -1;
#ifdef USE_3D
	int tex_xsize, tex_ysize;
	int resize_xsize, resize_ysize;
	int do_resize = 0;
	
//	for (int y = 0; y < sizeY >> 1; y++)
//		for (int x = 0; x < sizeX >> 1; x++)
//			texture[y * (sizeX >> 1) + x] = texture[y * 2 * sizeX + x * 2];
//			
//	sizeX = sizeX >> 1;
//	sizeY = sizeY >> 1;
//	
//	memset(texture + sizeX * sizeY, 0, sizeX * sizeY * 3);

//	for (int count = 0; count < (sizeX * sizeY) >> 1; count++)
//		texture[count] = texture[count * 2];
//			
//	sizeX = sizeX >> 1;
	
	if (texture)
		if (transparency)
		{
			int count;
			
			for (count = 0; count < sizeX * sizeY; count++)
					if (texture[count] == transparent_colour)
						texture[count] = 0;
		}
	
	if ((sizeX == 0) || (sizeY == 0))
	{
		printf("zero size texture!\n");
		return -1;
	}
	if ((sizeX > 1024) || (sizeY > 1024))
	{
		printf("texture is too big! %dx%d\n", sizeX, sizeY);
		return -1;
	}
	
	tex_xsize = tex_size(sizeX);
	tex_ysize = tex_size(sizeY);
	resize_xsize = sizeX;
	resize_ysize = sizeY;
	
	if ((tex_xsize == -1) || (tex_ysize == -1))
	{
		//we're gonna have to enlarge it as it's not a normal size
		if (tex_xsize == -1)
			resize_xsize = next_size_up(sizeX);
		if (tex_ysize == -1)
			resize_ysize = next_size_up(sizeY);
//return -1;
//		if (resize_xsize * resize_ysize > 256 * 256)
//			return -1;
//		else
		{
//			int y;
//			memset((void *)resize, 0, resize_xsize * resize_ysize);
//			
//			for (y = 0; y < sizeY; y++)
//				memcpy(&resize[y * resize_xsize], &texture[y * sizeX], sizeX);
			
			do_resize = 1;
		}
	}
	
//	vram_used += resize_xsize * resize_ysize;
		
//#ifdef USE_DEBUGGER
//	char text_buf[100];
//	sprintf(text_buf, "%.2fk VRAM used (%.2fk), %.2fk total\n",
//		(float)(sizeX * sizeY) / 1024, (float)(resize_xsize * resize_ysize) / 1024, (float)vram_used / 1024);
////	DEBUG_MSG(text_buf);
//	printf(text_buf);
//#endif
	tex_xsize = tex_size(resize_xsize);
	tex_ysize = tex_size(resize_ysize);
	
	if (!do_resize)
		return glTexImage2DQuake(0, 0, GL_RGB256, tex_xsize, tex_ysize, 0, TEXGEN_TEXCOORD | GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T | (transparency ? GL_TEXTURE_COLOR0_TRANSPARENT : 0), texture, 0, 0, 0, 0) - 1;
//		return glTexImage2D(0, 0, GL_RGB256, tex_xsize, tex_ysize, 0, TEXGEN_TEXCOORD | GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T, texture) - 1;
	else
	{
//		printf("using resized version, %dx%d->%dx%d\n", sizeX, sizeY, resize_xsize, resize_ysize);
		return glTexImage2DQuake(0, 0, GL_RGB256, tex_xsize, tex_ysize, 0, TEXGEN_TEXCOORD | GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T | (transparency ? GL_TEXTURE_COLOR0_TRANSPARENT : 0), texture, sizeY, sizeX, resize_ysize, resize_xsize) - 1;
	}
#endif
	return 0;
}

#define TEXTURE_VALID			1
#define TEXTURE_BSP				2
#define TEXTURE_IS_LOADED		4
#define TEXTURE_NEEDS_LOADING	8
#define TEXTURE_NEVER_UNLOAD	16
#define TEXTURE_IS_TRANSPARENT	32

#define TEXTURE_VALID_MASK			0xfffe
#define TEXTURE_BSP_MASK			0xfffd
#define TEXTURE_IS_LOADED_MASK		0xfffb
#define TEXTURE_NEEDS_LOADING_MASK	0xfff7
#define TEXTURE_NEVER_UNLOAD_MASK	0xffef

struct managed_texture
{
	unsigned short handle;
	unsigned int addr;
#ifdef TEXTURES_HAVE_NAMES
	char name[35];
#else
	unsigned int name_hash;
#endif
//	bool valid;
//	bool bsp;
//	
//	bool is_loaded;
//	bool needs_loading;
//	bool never_unload;
	unsigned short mask_data;
	
	unsigned int last_used;
	void *address;
	
//	int loaded_rows;
	
	unsigned short fp;
	unsigned int seek_pos;
	
	unsigned short sizeX, sizeY;
	unsigned short half_size;

//	unsigned short transparency;
	unsigned short transparent_colour;
};
#ifdef TEXTURE_CACHE_IN_VRAM
struct managed_texture *texture_cache = NULL;
#else
struct managed_texture texture_cache[MAX_MANAGED_TEXTURES];
#endif

int num_managed_textures = 0;

void init_textures(void)
{
	int count;
#ifdef TEXTURE_CACHE_IN_VRAM
	vramSetBankG(VRAM_G_LCD);
	texture_cache = (struct managed_texture *)0x6894000;
	
	if (sizeof(struct managed_texture) * MAX_MANAGED_TEXTURES > 16384)
		Sys_Error("texture cache will not fit in memory! size %d\n", sizeof(struct managed_texture) * MAX_MANAGED_TEXTURES);
#endif
	num_managed_textures = 0;
//	memset(texture_cache, 0, sizeof(managed_texture) * MAX_MANAGED_TEXTURES);
	
	unsigned short *ptr = (unsigned short *)texture_cache;
	for (count = 0; count < (sizeof(struct managed_texture) * MAX_MANAGED_TEXTURES) >> 1; count++)
		ptr[count] = 0;
	
	printf("texture system initialised\n");
}

int register_texture(bool bsp, char *name, int fp, int seek, int sizeX, int sizeY, bool transparency, int transparent_colour, int half_width, int half_height)
{
	int count;
//	printf("registering a texture\n");
	//test to see if it's already been loaded once
#ifdef TEXTURES_HAVE_NAMES
	if (!bsp)
		for (count = 0; count < num_managed_textures; count++)
			if (texture_cache[count].mask_data & TEXTURE_VALID)
				if (strncmp(name, texture_cache[count].name, 15) == 0)
					return count;
#else
	unsigned int hash = 0;
	
//	for (count = 0; count < strlen(name); count++)
//		hash += name[count];
//	
//	for (count = 0; count < num_managed_textures; count++)
//		if (texture_cache[count].mask_data & TEXTURE_VALID)
//			if (hash == texture_cache[count].name_hash)
//				return count;
	
	if (!bsp)
	{
		int count;
		for (count = 0; count < num_managed_textures; count++)
			if ((texture_cache[count].fp == fp) && (texture_cache[count].seek_pos == seek))
				return count;
	}
#endif	
//	printf("28 is %s\n", texture_cache[28].name);
//	printf("29 is %s\n", texture_cache[29].name);

//	if (fail_textures)
//		return -1;

	if (fp < 0)
		return -1;
		
	if ((sizeX == 0) || (sizeY == 0))
	{
		printf("texture size is zero!\n");
		return -1;
	} 
	
	int id = num_managed_textures;
	
	for (count = 0; (count < num_managed_textures) && (count < MAX_MANAGED_TEXTURES); count++)
		if (!(texture_cache[count].mask_data & TEXTURE_VALID))
		{
//			printf("replacing %d\n", count);
			id = count;
			break;
		}
	
	if (id == num_managed_textures)
		num_managed_textures++;
	
	if (id >= MAX_MANAGED_TEXTURES)
	{
		printf("ran out of texture handles!\n");
		printf("trying to make a texture for %s\n", name);
		*(int *)0 = 0;
		while(1);
	}
	
	struct managed_texture *t = &texture_cache[id];
	
//	t->handle = ds_gentexture();
	t->handle = id;
#ifdef TEXTURES_HAVE_NAMES
	ds_memset(t->name, 0, 15);
	
	for (count = 0; count < 14; count++)
		t->name[count] = name[count];
	
	t->name[14] = 0;
#else
	t->name_hash = hash;
#endif

	t->mask_data = 0;
	
	if (bsp)
		t->mask_data |= TEXTURE_BSP;
	t->mask_data |= TEXTURE_VALID;
	
//	t->bsp = bsp;
//	t->valid = true;
//	t->is_loaded = false;
//	t->needs_loading = false;
//	t->never_unload = false;
	t->last_used = 0;
	t->fp = fp;
	t->seek_pos = seek;
	t->sizeX = sizeX;
	t->sizeY = sizeY;
//	t->transparency = transparency;

	if (transparency)
		t->mask_data |= TEXTURE_IS_TRANSPARENT;

	t->transparent_colour = transparent_colour;
	t->half_size = (half_width << 1) | half_height;
	t->address = NULL;
	
//	printf("bound %d: %s, size %d\n", num_managed_textures - 1, name, sizeX * sizeY);
	
	return id;
}

//int fail_after = 0;

int register_texture_deferred(char *name, unsigned char *data, int sizeX, int sizeY, int transparency, int transparent_colour, int half_width, int half_height)
{
	int count;
	
	printf("registering a deferred texture\n");
//	//test to see if it's already been loaded once
#ifdef TEXTURES_HAVE_NAMES
	for (count = 0; count < num_managed_textures; count++)
		if (texture_cache[count].mask_data & TEXTURE_VALID)
			if (strcmp(name, texture_cache[count].name) == 0)
				return count;
#else
	unsigned int hash = 0;
	
	//this only really works if the data doesn't move, in fact it hardly works at all (esp for bsp)
//	for (count = 0; count < strlen(name); count++)
//		hash += name[count];
//	
//	for (count = 0; count < num_managed_textures; count++)
//		if (texture_cache[count].mask_data & TEXTURE_VALID)
//			if (hash == texture_cache[count].name_hash)
//			{
//				printf("found texture %s in slot %d\n", name, count);
//				return count;
//			}
#endif

//	if (fail_textures)
//		return -1;

	if ((sizeX == 0) || (sizeY == 0))
	{
		printf("texture size is zero!\n");
		return -1;
	}
	
	int id = num_managed_textures;
	
	for (count = 0; count < num_managed_textures; count++)
		if ((texture_cache[count].mask_data & TEXTURE_VALID) == 0)
		{
			id = count;
			num_managed_textures--;
			break;
		}
	
	if (id >= MAX_MANAGED_TEXTURES)
	{
		printf("ran out of texture handles!\n");
		printf("trying to make a texture for %s\n", name);
		while(1);
	}
	
	printf("loading %s as %d DEFERRED\n", name, id);
	
	num_managed_textures++;
	
	struct managed_texture *t = &texture_cache[id];
	
//	t->handle = ds_gentexture();
	t->handle = id;
	
#ifdef TEXTURES_HAVE_NAMES
	strncpy(t->name, name, 15);
	t->name[14] = NULL;
#else
	t->name_hash = hash;
#endif

	t->mask_data = 0;
	t->mask_data |= TEXTURE_VALID;
	
//	t->valid = true;
//	t->bsp = false;
//	t->is_loaded = false;
//	t->needs_loading = false;
//	t->never_unload = false;
	t->last_used = 0;
	t->fp = 0;
	t->seek_pos = (unsigned int)data;
	t->sizeX = sizeX;
	t->sizeY = sizeY;
//	t->transparency = transparency;

	if (transparency)
		t->mask_data |= TEXTURE_IS_TRANSPARENT;

	t->transparent_colour = transparent_colour;
	t->half_size = (half_width << 1) | half_height;
	
	t->address = NULL;
	
	printf("bound %d: %s, size %d\n", id, name, sizeX * sizeY);
	
	return id;
}

int register_texture_immediate(char *name, unsigned char *data, int sizeX, int sizeY, int transparency, int transparent_colour)
{
	Sys_Error("registering immediate texture\n");
////	//test to see if it's already been loaded once
////	for (int count = 0; count < num_managed_textures; count++)
////		if (strcmp(name, texture_cache[count].name) == 0)
////			return count;
//
////	if (fail_textures)
////		return -1;
//	
//	int id = num_managed_textures;
//	num_managed_textures++;
//	
//	for (int count = 0; count < num_managed_textures; count++)
//		if (!texture_cache[count].valid)
//		{
//			id = count;
//			num_managed_textures--;
//			break;
//		}
//	
//	if (id >= MAX_MANAGED_TEXTURES)
//	{
//		printf("ran out of texture handles!\n");
//		printf("trying to make a texture for %s\n", name);
//		while(1);
//	}
//	
////	printf("loading %s as %d\n", name, id);
//	
//	num_managed_textures++;
//	
//	struct managed_texture *t = &texture_cache[id];
//	
//	t->handle = ds_gentexture();
//	
//	strncpy(t->name, name, 15);
//	t->name[14] = NULL;
//	
//	t->valid = true;
//	t->bsp = false;
//	t->is_loaded = true;
//	t->needs_loading = false;
//	t->never_unload = true;
//	t->last_used = 0;
//	t->fp = NULL;
//	t->seek_pos = 0;
//	t->sizeX = sizeX;
//	t->sizeY = sizeY;
//	t->transparency = transparency;
//	t->transparent_colour = transparent_colour;
//	t->half_size = 0;
//	
////	printf("trans %d, %d\n", transparency, transparent_colour);
////	printf("size %dx%d\n", sizeX, sizeY);
//
////	if (fail_after > 173)
////	{
////		fclose(vramfp);
////		while(1);
////	}
////		return -1;
//	
//	ds_bindtexture(t->handle);
//	unsigned int addr = ds_teximage2d(sizeX, sizeY, data, transparency, transparent_colour);
//	if (addr == -1)
//		return -1;
//		
////	printf("addr is %08x, to %08x\n", addr, addr + sizeX * sizeY);
//	
////	if (strcmp(name, "lower sky") == 0)
////		while(1);
//	
////	fail_after++;
//		
//	t->address = (void *)addr;
//	
//	return id;
}

void *get_texture_address(int id)
{
	return texture_cache[id].address;
}

//unsigned short dummy[512];
int into_cache = -1;

#ifdef USE_EXTRA_RAM
unsigned short vram_e_base[64 * 1024];		//128k - but still not big enough
#endif

//int texture_loads = 0;
//int texture_caches = 0;

bool load_texture(int id)
{
	unsigned short dummy[512];
	
//#ifdef USE_EXTRA_RAM
//	void *cached_texture = find_texture(id);
////	void *cached_texture = NULL;
//	
//	if (cached_texture)
//	{
//		into_cache = id;
//		printf("found texture %d\n", id);
//		texture_caches++;
//	}
//	else
//	{
//		texture_loads++;
//		printf("couldn\'t find texture %d\n", id);
//	}
//#endif
	
	int count, y;
	
#ifndef USE_EXTRA_RAM
	unsigned short *vram_e_base = (unsigned short *)0x6880000;
#endif
	struct managed_texture *t = &texture_cache[id];
	
	if ((t->sizeX == 0) || (t->sizeY == 0))
	{
		dump_texture_state();
		while(1);
	}
	
	if (into_cache == -1)
	{
//#ifdef TEXTURES_HAVE_NAMES
//		printf("loading %s into cache\n", t->name);
//#else
//		printf("loading %d into cache\n", id);
//#endif
//		printf("size %dx%d\n", t->sizeX, t->sizeY);

		int rows = t->sizeY;
		int cols = t->sizeX;
		int big_cols = next_size_up(cols);
		
#ifdef USE_EXTRA_RAM
		if (next_size_up(cols) * next_size_up(rows) > 128 * 1024)
#else
		if (next_size_up(cols) * next_size_up(rows) > 64 * 1024)
#endif
		{
			Sys_Error("error: texture %d is too large to load into temporary cache memory:\n\twidth %d/%d, height %d/%d\n\tsize %d bytes\n",
				id, cols, next_size_up(cols), rows, next_size_up(rows), next_size_up(cols) * next_size_up(rows));
		}


//		if (t->fp)
//			Sys_FileSeek(t->fp, t->seek_pos);
		if (t->fp)
			Sys_Error("loading textures from disk is not supported\n");
		
		if (t->half_size & 0x1)
		{
			if (t->mask_data & TEXTURE_IS_TRANSPARENT)
			{
				Sys_Error("half-size transparencies are not supported\n");
//				for (y = 0; y < rows; y++)
//				{
//					Sys_FileRead(t->fp, dummy, cols);
//					
//					unsigned char *src = (unsigned char *)dummy;
//					unsigned char *dest = (unsigned char *)dummy;
//					
//					for (count = 0; count < cols; count++)
//					{
//						unsigned char b1 = src[count];
//						
//						if (b1 == t->transparent_colour)
//							b1 = 0;
//						
//						dest[count] = b1;
//					}
//		
//					unsigned short *ptr = (unsigned short *)&((unsigned char *)vram_e_base)[y * big_cols];
//					for (count = 0; count < (cols >> 1); count++)
//						ptr[count] = dummy[count];
//					
//					Sys_FileRead(t->fp, dummy, cols);
//				}
			}
			else
			{
				unsigned short *sptr = (unsigned short *)t->seek_pos;
				
				for (y = 0; y < rows; y++)
				{
//					Sys_FileRead(t->fp, dummy, cols);
//		
//					unsigned short *ptr = (unsigned short *)&((unsigned char *)vram_e_base)[y * big_cols];
//					for (count = 0; count < (cols >> 1); count++)
//						ptr[count] = dummy[count];
//					
//					Sys_FileRead(t->fp, dummy, cols);
					
					unsigned short *ptr = (unsigned short *)&((unsigned char *)vram_e_base)[y * big_cols];
					
					for (count = 0; count < (cols >> 1); count++)
						ptr[count] = sptr[count];
					
					sptr += cols;			//width * 2
				}
			}
		}
		else
			for (y = 0; y < rows; y++)
			{
				if (t->fp)
				{
//					Sys_FileRead(t->fp, dummy, cols);
//					
////					if (t->transparency)
//					if (t->mask_data & TEXTURE_IS_TRANSPARENT)
//					{
//						unsigned char *src = (unsigned char *)dummy;
//						unsigned char *dest = (unsigned char *)dummy;
//						
//						for (count = 0; count < cols; count++)
//						{
//							unsigned char b1 = src[count];
//							
//							if (b1 == t->transparent_colour)
//								b1 = 0;
//							
//							dest[count] = b1;
//						}
//					}
					Sys_Error("loading textures from disk is not supported\n");
				}
				else
				{
					unsigned char *src = (unsigned char *)(t->seek_pos + y * cols);
					
//					if (t->transparency)
					if (t->mask_data & TEXTURE_IS_TRANSPARENT)
					{
						for (count = 0; count < cols; count += 2)
						{
							unsigned char b1 = src[count];
							unsigned char b2 = src[count + 1];
							
							if (b1 == t->transparent_colour)
								b1 = 0;
							if (b2 == t->transparent_colour)
								b2 = 0;
							
							dummy[count >> 1] = (b2 << 8) | b1;
						}
					}
					else
						ds_memcpy(dummy, src, cols); 
				}

				unsigned short *ptr = (unsigned short *)&((unsigned char *)vram_e_base)[y * big_cols];
				for (count = 0; count < (cols >> 1); count++)
					ptr[count] = dummy[count];
			}
		
		into_cache = id;
		
		DC_FlushRange(vram_e_base, sizeof(big_cols * rows));
	}
	
	if (into_cache == id)
	{
//		printf("copying in texture %d\n", id);
		unsigned short vcount = *(volatile unsigned short *)0x4000006;
#ifdef WITH_BLOCKING_TEXTURE_LOADS
//		if ((vcount < HBLANK_START_LOAD) || (vcount >= HBLANK_END_LOAD))
		if ((vcount < HBLANK_START_LOAD) || (vcount >= HBLANK_START_LATE_LOAD))
		{
//			printf("falling out, vcount %d\n", vcount);
			return true;
		}
#endif
		
//		ds_bindtexture(t->handle);
		//removed glBindTexture(0, t->handle);
		unsigned int addr = ds_teximage2d(t->sizeX, t->sizeY, NULL,
			(t->mask_data & TEXTURE_IS_TRANSPARENT) == TEXTURE_IS_TRANSPARENT,
			t->transparent_colour);
		t->address = (void *)(addr + 1);	//yeah
		t->addr = g_tex_parms;
		
//#ifdef TEXTURES_HAVE_NAMES
//		printf("loading %s from cache, %d\n", t->name, *(volatile unsigned short *)0x4000006);
//#else
//		printf("loading from cache\n");
//#endif
		
//		printf("texture address is %08x\n", t->address);

		if (t->address)
		{
			int rows = t->sizeY;
			int cols = t->sizeX;
			int big_cols = next_size_up(cols);
			
//			unsigned short marker_1 = *(volatile unsigned short *)0x4000006;
			
			ds_unlock_vram();
			
			unsigned short *dest = (unsigned short *)t->address;
			unsigned short *src = vram_e_base;
			
//			if (cached_texture)
//				src = (unsigned short *)cached_texture;
			
//			for (count = 0; count < big_cols * rows; count += 2)
//			{
//				*dest = *src;
//				dest++;
//				src++;
//			}
			dmaCopyHalfWords(0, src, dest, big_cols * rows);
			
//			dest = (unsigned short *)t->address;
//			src = vram_e_base;
			
//			if ((dest[0] != src[0]) /*|| (dest[(big_cols * rows) >> 1] != dest[(big_cols * rows) >> 1])*/)
//				printf("bytes differ\n");
				
			ds_lock_vram();
			
//			unsigned short marker_2 = *(volatile unsigned short *)0x4000006;
			
//			printf("loaded %d b, %d\n", big_cols * rows, marker_2 - marker_1);
			
	//		t->is_loaded = true;
	//		t->needs_loading = false;
			t->mask_data |= TEXTURE_IS_LOADED;
			
//			if (cached_texture == NULL)
//			{
//				printf("adding texture to cache\n");
//				add_texture(id, (void *)vram_e_base, big_cols * rows);
//			}
		}
		t->mask_data = t->mask_data & TEXTURE_NEEDS_LOADING_MASK;
		
		into_cache = -1;
	}
	else
	{
//		printf("%d is in cache\n", into_cache);
		t->mask_data = t->mask_data & TEXTURE_NEEDS_LOADING_MASK;
	}
	return false;
}

//bool load_texture(int id)
//{
//	struct managed_texture *t = &texture_cache[id];
//	
//	printf("loading %s\n", t->name);
//	
//	ds_bindtexture(t->handle);
//	unsigned int addr = ds_teximage2d(t->sizeX, t->sizeY, NULL, t->transparency, t->transparent_colour);
//	t->address = (void *)(addr + 1);	//yeah
//	
//	printf("texture address is %08x\n", t->address);
//	
//	bool outa_time = false;
//	
//	if (t->address)
//	{	
//		ds_unlock_vram();
//		
//		int rows = t->sizeY;
//		int cols = t->sizeX;
//		int big_cols = next_size_up(cols);
//		
////		if (*(volatile unsigned short *)0x4000006 > 212) { return true; }
//		
//		Sys_FileSeek(t->fp, t->seek_pos + cols * t->loaded_rows);
//		
////		if (*(volatile unsigned short *)0x4000006 > 212) { return true; }
//		
//		t->loaded_rows = 0;
//		
//		int read = 0;
//
//		if (t->half_size & 0x1)
//			for (int y = 0; y < rows; y++)
//			{
////				if (*(volatile unsigned short *)0x4000006 > 212) { outa_time = true; t->loaded_rows = y; break; }
//				
//				Sys_FileRead(t->fp, dummy, cols);
//	
//				unsigned short *ptr = (unsigned short *)&((unsigned char *)t->address)[y * big_cols];
//				
////				if (*(volatile unsigned short *)0x4000006 > 212) { outa_time = true; t->loaded_rows = y; break; }
//				
//				for (int count = 0; count < (cols >> 1); count++)
//					ptr[count] = dummy[count];
//				
////				if (*(volatile unsigned short *)0x4000006 > 212) { outa_time = true; t->loaded_rows = y + 1; break; }
//				
//				Sys_FileRead(t->fp, dummy, cols);
//			}
//		else
//			for (int y = 0; y < rows; y++)
//			{
////				if (*(volatile unsigned short *)0x4000006 > 212) { outa_time = true; t->loaded_rows = y; break; }
//				
//				Sys_FileRead(t->fp, dummy, cols);
//				
////				if (*(volatile unsigned short *)0x4000006 > 212) { outa_time = true; t->loaded_rows = y; break; }
//	
//				unsigned short *ptr = (unsigned short *)&((unsigned char *)t->address)[y * big_cols];
//				for (int count = 0; count < (cols >> 1); count++)
//					ptr[count] = dummy[count];
//				
////				if (*(volatile unsigned short *)0x4000006 > 212) { outa_time = true; t->loaded_rows = y + 1; break; }
//			}
//		
////		unsigned short *ptrs = (unsigned short *)&((unsigned char *)t->address)[0];
////		ptrs[0] = 0;
////		ptrs[1] = 0;
//		
//		ds_lock_vram();
//		
//		t->is_loaded = true;
//		
////		printf("loading %s\n", t->name);
//	}
//	
//	t->needs_loading = false;
//	
//	return outa_time;
//}

volatile int texture_critical_section = 0;
int textures_needing_load = 0;

void load_textures(void)
{
	bool loaded_something = false;
//	dump_texture_state();
//	if (super_framecount == 17)
//		printf("this is it!\n");
	if (!texture_critical_section)
	{
//		printf("%d textures need load\n", textures_needing_load);
		if (textures_needing_load)
		{
#ifdef WITH_BLOCKING_TEXTURE_LOADS
//			printf("waiting at %d\n", *((volatile unsigned short *)0x4000006));
			
//			int in_at = *((volatile unsigned short *)0x4000006);
			
			if (*((volatile unsigned short *)0x4000006) > HBLANK_START_LATE_LOAD)
			{
				while (*((volatile unsigned short *)0x4000006) > HBLANK_START_LATE_LOAD);
				while (*((volatile unsigned short *)0x4000006) < HBLANK_START_LOAD);
			}
			
//			printf("loads at %d %d\n", in_at, *((volatile unsigned short *)0x4000006));
#endif
			
//			printf("%d vram free before loading\n", vram_get_free_size());
			texture_critical_section = 1;
			
			int count;
			
			for (count = 0; (count < num_managed_textures) && (count < MAX_MANAGED_TEXTURES); count++)
				if ((texture_cache[count].mask_data & (TEXTURE_NEEDS_LOADING | TEXTURE_VALID)) == (TEXTURE_NEEDS_LOADING | TEXTURE_VALID))
				{
//					printf("loading %d at %d\n", count, *((volatile unsigned short *)0x4000006));
					if (load_texture(count))	//outatime
						break;
//					verify_textures();
				}
			
			loaded_something = true;
			
			texture_critical_section = 0;
//			printf("%d vram free after loading\n", vram_get_free_size());
		}
		textures_needing_load = 0;
	}
	
//	if (loaded_something)
//		verify_textures();
}
//
//void unload_textures_2(void)
//{
//	printf("deleting all textures\n");
//	if (!texture_critical_section)
//	{
//		texture_critical_section = 1;
//		
//		int count;
//		
//		for (count = 0; count < num_managed_textures; count++)
//		{
//			if (texture_cache[count].mask_data & TEXTURE_VALID)
//			{
//				if (texture_cache[count].mask_data& TEXTURE_IS_LOADED)
//				{
//					printf("freeing %s, %dx%d, %08x\n",
//						/*texture_cache[count].name*/"",
//						texture_cache[count].sizeX,
//						texture_cache[count].sizeY,
//						texture_cache[count].address);
//					vram_free(texture_cache[count].address);
//					
//					texture_cache[count].address = NULL;
//					texture_cache[count].mask_data = texture_cache[count].mask_data & TEXTURE_IS_LOADED_MASK;
//					texture_cache[count].last_used = 0;
//				}
//			}
//		}
//
//		texture_critical_section = 0;
//		
//		printf("%d bytes vram free\n", vram_get_free_size());
//		
//		if (vram_get_free_size() < 524288)
//			while(1);
//	}
//	else
//	{
//		Sys_Error("trying to free textures during critical section!\n");
//	}
//}

void verify_textures(void)
{
	if (/*!texture_critical_section && */(into_cache == -1))
	{
		/*texture_critical_section = 1;*/
		
		int count;
		
		for (count = 0; (count < num_managed_textures) && (count < MAX_MANAGED_TEXTURES); count++)
			if (texture_cache[count].mask_data & TEXTURE_VALID)
				if (texture_cache[count].mask_data & TEXTURE_IS_LOADED)
				{
//					int bytes;
					int different = 0;
					
					ds_unlock_vram();
					
//					for (bytes = 0; bytes < (texture_cache[count].sizeX * texture_cache[count].sizeY); bytes++)
//						if (((unsigned char *)texture_cache[count].seek_pos)[bytes] != ((unsigned char *)texture_cache[count].address)[bytes])
//							different++;
					
					int x, y;
					
					for (y = 0; y < texture_cache[count].sizeY; y++)
					{
						unsigned char *ptr = (unsigned char *)texture_cache[count].address;
						ptr = ptr + y * next_size_up(texture_cache[count].sizeX);
						
						for (x = 0; x < texture_cache[count].sizeX; x++)
						{
							if (((unsigned char *)texture_cache[count].seek_pos)[y * texture_cache[count].sizeX + x] != ptr[x])
								different++;
						}
					}
					
					ds_lock_vram();
					
					if (different)
						printf("tex %d %dx%d, %.2f%%\n",
								count,
								texture_cache[count].sizeX,
								texture_cache[count].sizeY,
								(float)different / (texture_cache[count].sizeX * texture_cache[count].sizeY) * 100);
				}
		
		/*texture_critical_section = 0;*/
	}
}

void unload_textures(void)
{
//	dump_texture_state();
	
	if (!texture_critical_section)
	{
		texture_critical_section = 1;
		into_cache = -1;
		
		int count;
		
		for (count = 0; (count < num_managed_textures) && (count < MAX_MANAGED_TEXTURES); count++)
		{
			if (/*(texture_cache[count].mask_data & TEXTURE_BSP) && */(texture_cache[count].mask_data & TEXTURE_VALID))
			{
				if (texture_cache[count].mask_data & TEXTURE_IS_LOADED)
				{
//					printf("freeing %s, %dx%d, %08x\n",
//						texture_cache[count].name,
//						texture_cache[count].sizeX,
//						texture_cache[count].sizeY,
//						texture_cache[count].address);
					vram_free(texture_cache[count].address);
					texture_cache[count].address = NULL;
				}
//				texture_cache[count].valid = false;
				//invalidate bsp textures - gui textures are loaded once at the beginning of the game and never again
				if (texture_cache[count].mask_data & TEXTURE_BSP)
				{
					texture_cache[count].mask_data = texture_cache[count].mask_data & TEXTURE_VALID_MASK;
					texture_cache[count].mask_data = texture_cache[count].mask_data & TEXTURE_BSP_MASK;
					
//					remove_texture(count);
				}
					
				texture_cache[count].mask_data = texture_cache[count].mask_data & TEXTURE_IS_LOADED_MASK;
				texture_cache[count].last_used = 0;
			}
		}
		
//		dump_texture_state();
		
		printf("leaked %d bytes of VRAM\n", TEXTURE_VRAM * 1024 - vram_get_free_size());
		
//		if (512 * 1024 - vram_get_free_size() != 0)
//		{
//			fprintf(vramfp, "\n");
//			for (int count = 0; count < NUM_MALLOC_CHUNKS; count++)
//				fprintf(vramfp, "%d: %d\n", count, chunks_in_use[count]);
//			fclose(vramfp);
//			while(1);
//		}

		texture_critical_section = 0;
		
//		unload_textures_2();
	}
	else
	{
		Sys_Error("trying to free textures during critical section!\n");
	}
}

int find_texture_by_name(char *name)
{
#ifdef TEXTURES_HAVE_NAMES
	int count;
	
	for (count = 0; count < num_managed_textures; count++)
		if (strcmp(texture_cache[count].name, name) == 0)
			return count;
#endif
	
	return -1;
}

int find_texture_by_addr(void *addr)
{
	int count;
	
	for (count = 0; (count < num_managed_textures) && (count < MAX_MANAGED_TEXTURES); count++)
		if ((unsigned int)texture_cache[count].seek_pos == (unsigned int)addr) 
			return count;
	
	return -1;
}

bool free_lru(int min_time)
{
	int lru = -1;
	unsigned int lru_time = -1;
	
//	printf("free lru\n");
	
	int count;
	
	for (count = 0; (count < num_managed_textures) && (count < MAX_MANAGED_TEXTURES); count++)
//		if ((texture_cache[count].last_used < lru_time)
//			&& (texture_cache[count].last_used < min_time)
//			&& texture_cache[count].is_loaded
//			&& !texture_cache[count].never_unload
//			&& texture_cache[count].valid)
		if ((texture_cache[count].last_used < lru_time)
//			&& (texture_cache[count].last_used < min_time)
			&& (texture_cache[count].mask_data & TEXTURE_IS_LOADED)
			&& !(texture_cache[count].mask_data & TEXTURE_NEVER_UNLOAD)
			&& (texture_cache[count].mask_data & TEXTURE_VALID))
		{
			lru_time = texture_cache[count].last_used;
			lru = count;
		}
	
//	printf("lru is %d\n", lru);
//	printf("freeing %s\n", texture_cache[lru].name);
	
	if (lru == -1)
	{
//		printf("no more textures to free!\n");
		return false;
	}
	
	texture_cache[lru].mask_data = texture_cache[lru].mask_data & TEXTURE_IS_LOADED_MASK;
//	texture_cache[lru].is_loaded = false;
	texture_cache[lru].last_used = 0;
	
	vram_free(texture_cache[lru].address);
	texture_cache[lru].address = NULL;
	
	return true;
}

void dump_texture_state(void)
{
		FILE *fp;
		char filename[50];
		sprintf(filename, "/textures_%d.txt", super_framecount);
		fp = fopen(filename, "w");
		
		fprintf(fp, "hblanks is %d, blanks is %d, sfc is %d\n", hblanks, blanks, super_framecount);
		fprintf(fp, "%d managed textures\n", num_managed_textures);
		fprintf(fp, "%d bytes free\n", vram_get_free_size());
		
		int count;
		
		for (count = 0; count < num_managed_textures; count++)
			fprintf(fp, "%d: name %s, bsp %d, valid %d, loaded %d, needs loading %d, never unload %d, address %08x, seek pos %d, fp %08x, lru %d, size %dx%d\n",
				count,
#ifdef TEXTURES_HAVE_NAMES
				texture_cache[count].name,
#else
				"no name",
#endif
				(texture_cache[count].mask_data & TEXTURE_BSP) > 0,
				(texture_cache[count].mask_data & TEXTURE_VALID) > 0,
				(texture_cache[count].mask_data & TEXTURE_IS_LOADED) > 0,
				(texture_cache[count].mask_data & TEXTURE_NEEDS_LOADING) > 0,
				(texture_cache[count].mask_data & TEXTURE_NEVER_UNLOAD) > 0,
				texture_cache[count].address,
				texture_cache[count].seek_pos,
				texture_cache[count].fp,
				texture_cache[count].last_used,
				texture_cache[count].sizeX, texture_cache[count].sizeY);
		
		fclose(fp);
		printf("dumped texture state\n");
		
//		unload_textures_2();

//		while(1);	
}

unsigned short current_bound = -1;

bool bind_texture(int id)
{		
	struct managed_texture *t = &texture_cache[id];
	bool bound = false;
	
	if (id >= num_managed_textures)
	{
		register unsigned int lr_r asm("lr");
		volatile unsigned int lr = lr_r;
	
		printf("trying to bind unknown texture! (%d)\n", id);
		Sys_Error("called from %08x\n", lr);
	}
	
//	if ((t->fp == NULL) && (t->never_unload == false))
//		printf("binding deferred %d\n", id);
	
	if (id >= 0)
	{
//		printf("trying to bind %d (%d), %d bytes free\n", id, t->handle, vram_get_free_size());
		
//		if (t->is_loaded)
		if (t->mask_data & TEXTURE_IS_LOADED)
		{
//			ds_bindtexture(t->handle);
			t->last_used = super_framecount;
			bound = true;
			
			if (current_bound != t->handle)
			{
				//removed glBindTexture(0, t->handle);
				GFX_TEX_FORMAT = t->addr;
				current_bound = t->handle;
			}	
		}
		else
		{
//			ds_bindtexture(0);
//			t->needs_loading = true;
			t->mask_data |= TEXTURE_NEEDS_LOADING;
			
			textures_needing_load++;
			
			if (current_bound != 0)
			{
				//removed glBindTexture(0, 0);
				current_bound = 0;
				GFX_TEX_FORMAT = 0;
			}
		}
	}
	else if (current_bound != 0)
	{
		current_bound = 0;
		GFX_TEX_FORMAT = 0;
		//removed glBindTexture(0, 0);
//		ds_bindtexture(0);
	}
	
	return bound;
}

void release_texture(int handle)
{
	if (handle >= num_managed_textures)
	{
		printf("trying to release texture handle %d, yet there are only %d textures registered\n",
				handle, num_managed_textures);
		*(int *)0 = 0;
		while(1);
	}
	
	if (texture_cache[handle].mask_data & TEXTURE_VALID)
	{
//		printf("releasing texture handle %d\n", handle);
			
		if (texture_cache[handle].mask_data & TEXTURE_IS_LOADED)
		{
			vram_free(texture_cache[handle].address);
			texture_cache[handle].address = NULL;
		}
		
		if (into_cache == handle)
			into_cache = -1;

		texture_cache[handle].mask_data = 0;
		texture_cache[handle].last_used = 0;
		texture_cache[handle].address = 0x0;
		texture_cache[handle].seek_pos = 0;
		texture_cache[handle].name_hash = 0;
	}
	else
	{
		printf("trying to release texture handle %d, but it was never in use\n", handle);
		while(1);
	}
}

int next_size_up(int size)
{
	if (size > 512)
		return 1024;
	if (size > 256)
		return 512;
	if (size > 128)
		return 256;
	if (size > 64)
		return 128;
	if (size > 32)
		return 64;
	if (size > 16)
		return 32;
	if (size > 8)
		return 16;
	if (size > 0)
		return 8;
	return 0;
}

uint32 vram_banks;

void ds_unlock_vram(void)
{
	vram_banks = vramSetMainBanks(VRAM_A_LCD,VRAM_B_LCD,VRAM_C_LCD,VRAM_D_LCD);
}

void ds_lock_vram(void)
{
	vramRestoreMainBanks(vram_banks);
}

int tex_size(int size)
{
	switch (size)
	{
		case 8:
			size = TEXTURE_SIZE_8;
			break;
		case 16:
			size = TEXTURE_SIZE_16;
			break;
		case 32:
			size = TEXTURE_SIZE_32;
			break;
		case 64:
			size = TEXTURE_SIZE_64;
			break;
		case 128:
			size = TEXTURE_SIZE_128;
			break;
		case 256:
			size = TEXTURE_SIZE_256;
			break;
		case 512:
			size = TEXTURE_SIZE_512;
			break;
		case 1024:
			size = TEXTURE_SIZE_1024;
			break;
		default:
//			printf("invalid texture size, %d\n", size);
			size = -1;
			break;
	}
	return size;
}

void get_texture_sizes(int handle, int *w, int *h, int *big_w, int *big_h)
{
	if (handle >= MAX_MANAGED_TEXTURES)
		Sys_Error("error: getting texture size for non-existant texture, %d\n", handle);
	
	if (w)
		*w = texture_cache[handle].sizeX;
	if (h)
		*h = texture_cache[handle].sizeY;
	
	if (big_w)
		*big_w = next_size_up(texture_cache[handle].sizeX);
	if (big_h)
		*big_h = next_size_up(texture_cache[handle].sizeY);
}
