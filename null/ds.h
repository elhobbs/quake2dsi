#ifndef DS_H_
#define DS_H_

//#define R21   

//#define GAME_HARD_LINKED

#include <stdio.h>

#define MAX_MANAGED_TEXTURES 800
#define DS_MIP_LEVEL 1
#define DS_GEOM_SCALE 512
#define DS_MINI_GEOM_SCALE 8
#define DS_MINI_GEOM_SHIFT 3
#define DS_SCALE_FUNK 1

#define WITH_BLOCKING_TEXTURE_LOADS

#define HBLANK_START_LOAD 150
#define HBLANK_START_LATE_LOAD 200
#define HBLANK_END_LOAD 205

#define SOUND_L1_SIZE (256 * 1024)
#define MAX_SFX 554				//that's the amount of sound files + 1 in the sound directory

void print_top_four_blocks(void);
void print_top_four_blocks_kb(void);
int count_largest_block_kb(void);
int count_largest_extra_block_kb(void);
int count_largest_extra_block_mb(void);
int count_largest_block(void);

void ram_mode(void);
void disk_mode(void);
void check_mode(void);

void *ds_get_some_vram(void);

int set_gui_loading(void);
int set_gui_not_loading(void);
int set_sky_loading(void);
int set_sky_not_loading(void);

void get_pen_pos(short *px, short *py);
void ds_time(int *year, int *month, int *day, int *hour, int *minute, int *second);

#ifdef _WIN32
__forceinline void byte_write(void *ptr, unsigned char value) {
	*((unsigned char *)ptr) = value;
}
#define ITCM_CALL
#endif

#ifdef ARM9
#define ITCM_CALL __attribute__((section(".itcm"), long_call))
#endif

//void ipc_block_ready_9to7(void) ITCM_CALL;
//bool ipc_test_ready_9to7(void) ITCM_CALL;
//void ipc_set_ready_9to7(void) ITCM_CALL;
//void ipc_block_ready_7to9(void) ITCM_CALL;

void handle_ipc(void) ITCM_CALL;
unsigned char *S_LoadSoundFile(char *name, unsigned int *file_len) ITCM_CALL;
void S_FreeSoundFile(void *data) ITCM_CALL;
void S_FreeMarkedSounds(void);

#define MEM_MAIN 1
#define MEM_XTRA 2
void ds_set_malloc_base(int base);
void *ds_malloc(unsigned int size);
void *ds_realloc(void *ptr, size_t size);
void ds_free(void *ptr);


void ds_memset(void *addr, unsigned char value, unsigned int length) ITCM_CALL;
void ds_memset_basic(void *addr, unsigned char value, unsigned int length) ITCM_CALL;
void ds_memcpy(void *dest, void *source, unsigned int length) ITCM_CALL;
void ds_memcpy_basic(void *dest, void *source, unsigned int length) ITCM_CALL;
//void byte_write(void *addr, unsigned char value) __attribute__ ((no_instrument_function));
void ds_strcpy (char *dest, char *src) ITCM_CALL;

struct ds_file
{
	FILE *file;
	unsigned int filename_hash;
	
	unsigned int current_seek_pos;
	unsigned int length;
	int reference_count;
};

struct fast_file
{
	struct ds_file *file;
	unsigned int seek_pos;
};

//wrappers for the DS
struct fast_file *ds_fopenf(char *filename, char *mode);
int ds_fclose(struct fast_file *fp);
int ds_fseek(struct fast_file *fp, long offset, int whence);
int ds_fread(void *buf, int size, int count, struct fast_file *fp);
int ds_fwrite(void *buf, int size, int count, struct fast_file *fp);
long ds_ftell(struct fast_file *fp);
int ds_remove(char *filename);

//now the overloaded versions (they don't really return FILE *s)
FILE *ds_fopen(char *filename, char *mode);
int ds_fclose(FILE *fp);
int ds_fseek(FILE *fp, long offset, int whence);
int ds_fread(void *buf, int size, int count, FILE *fp);
int ds_fwrite(void *buf, int size, int count, FILE *fp);
long ds_ftell(FILE *fp);

//now the actual functions
struct ds_file *_ds_fopen(char *filename, char *mode);
int _ds_fclose(struct ds_file *fp);
int _ds_fseek(struct ds_file *fp, long offset, int whence);
int _ds_fread(void *buf, int size, int count, struct ds_file *fp, unsigned int *bytes_advanced);
int _ds_fwrite(void *buf, int size, int count, struct ds_file *fp, unsigned int *bytes_advanced);
long _ds_ftell(struct ds_file *fp);
int _ds_remove(char *filename);

void init_dsfile_handles(void);
void *get_dsfile_handle(int type);
void return_dsfile_handle(void *handle);
unsigned int hash_filename(char *filename);

void ds_begin_render(void);
void ds_end_render(void);
void ds_reset_render(void);

void init_textures(void);
void vram_init(void);
void *vram_malloc(unsigned int size)  ITCM_CALL;
unsigned int vram_get_free_size(void) ITCM_CALL;
void vram_free(void *ptr) ITCM_CALL;
void copy_into_vram(unsigned int *dest, unsigned char *src, int size, int rows, int cols, int realX) ITCM_CALL;
int tex_size(int size) ITCM_CALL;
void ds_lock_vram(void) ITCM_CALL;
void ds_unlock_vram(void) ITCM_CALL;
int next_size_up(int size) ITCM_CALL;
bool bind_texture(int id) ITCM_CALL;
bool free_lru(int min_time) ITCM_CALL;
void load_textures(void) ITCM_CALL;
bool load_texture(int id) ITCM_CALL;
void *get_texture_address(int id) ITCM_CALL;
void get_texture_sizes(int handle, int *w, int *h, int *big_w, int *big_h);
void ds_resize_in_place(int sizeX, int sizeY, unsigned char *image) ITCM_CALL;
int ds_teximage2d(int sizeX, int sizeY, unsigned char* texture, bool transparency, int transparent_colour) ITCM_CALL;
void dump_texture_state(void);
void verify_textures(void);

int register_texture_deferred(char *name, unsigned char *data, int sizeX, int sizeY, int transparency, int transparent_colour, int half_width, int half_height);
void release_texture(int handle);


unsigned int glTexImage2DQuake(int target, int empty1, int type, int sizeX, int sizeY, int empty2, int param, unsigned char* texture,
	unsigned int rows, unsigned int cols, unsigned int realY, unsigned int realX)
		ITCM_CALL;

void ds_reinit_console(unsigned char *font);
void ds_loadpalette(unsigned char *palette);
void ds_schedule_loadpalette(unsigned char *palette);
void toggle_keyb(void);

void ds_ortho(void);
void ds_perspective(void);
void ds_polyfmt(int poly_id, int trans, int depth, int culling) ITCM_CALL;
void ds_set_fog(int shift, int depth);

void ds_pushmatrix(void) ITCM_CALL;
void ds_popmatrix(void) ITCM_CALL;
void ds_translatef(float x, float y, float z) ITCM_CALL;
void ds_scalef(float x, float y, float z) ITCM_CALL;
void ds_rotateX(float angle) ITCM_CALL;
void ds_rotateY(float angle) ITCM_CALL;
void ds_rotateZ(float angle) ITCM_CALL;

void ds_mulmatf(float m[3][4]) ITCM_CALL;

void ds_vertex3f(float x, float y, float z);

int ds_gentexture(void);

void ds_get_geometry_count(unsigned int *verts, unsigned int *polys);
int ds_boxtest(short x, short y, short z, short height, short width, short depth) ITCM_CALL;
unsigned short *ds_screen_capture(void);
void ds_setup_bsp_render(void *model, void *clipplanes, int frc, int vfrc, unsigned char *areab, int **pfri, int *modelo, unsigned int *head_surf);
#ifdef ARM9
static inline void byte_write(void *ptr, unsigned char value) __attribute__ ((no_instrument_function));

static inline void byte_write(void *ptr, unsigned char value)
{
	unsigned int aligned_addr = ((unsigned int)ptr) & 0xfffffffe;
	unsigned short existing = *(unsigned short *)aligned_addr;
	
	unsigned short result;
	
	unsigned short is_lower = (unsigned int)ptr & 0x1;
	unsigned short value_shift = is_lower << 3;
	
	unsigned short shifted_value = value << value_shift;
	unsigned short existing_masked = existing & (0xff00 >> value_shift);
	
	result = shifted_value | existing_masked;
	
//	if ((((unsigned int)ptr) & 0x1) == 0)
//		result = (existing & 0xff00) | value;
//	else
//		result = (value << 8) | (existing & 0xff);
	
	*(unsigned short *)aligned_addr = result;
}
void init_arm7(void);



#ifndef NDS_INCLUDE
#define NDS_INCLUDE		//to prevent re-defines

#define floattov16(n)         ((short int)((n) * (1 << 12)))

#define GFX_COLOR				(*(volatile unsigned int *) 0x4000480)
#define GFX_TEX_COORD			(*(volatile unsigned int *) 0x04000488)
#define GFX_VERTEX16			(*(volatile unsigned int *) 0x0400048C)

#endif


#define PACK_TEXTURE(u,v)    (((u) << 16) | ((v) & 0xFFFF))

#define DS_COLOUR3B(red, green, blue) GFX_COLOR = (volatile unsigned int)RGB15(red>>3, green>>3, blue>>3)
#define DS_TEXCOORD2T16(x, y) GFX_TEX_COORD = PACK_TEXTURE(x, y)
#define DS_VERTEX3V16(x, y, z) GFX_VERTEX16 = ((y) << 16) | ((x) & 0xFFFF); GFX_VERTEX16 = ((unsigned int)(unsigned short)(z))
#define DS_BEGIN_TRIANGLE() GFX_BEGIN = 0
#define DS_BEGIN_QUAD() GFX_BEGIN = 1
#define DS_BEGIN_TRIANGLE_STRIP() GFX_BEGIN = 2

#endif

#ifdef _WIN32
#define PACK_TEXTURE(u,v)

#define DS_COLOUR3B(red, green, blue)
#define DS_TEXCOORD2T16(x, y)
#define DS_VERTEX3V16(x, y, z)
#define DS_BEGIN_TRIANGLE()
#define DS_BEGIN_QUAD()
#define DS_BEGIN_TRIANGLE_STRIP()

#define POLY_CULL_NONE 0
#define POLY_CULL_FRONT 1
#define POLY_CULL_BACK 2
#define v16 short
#define floattov16(n)        ((v16)((n) * (1 << 12))) /*!< \brief convert float to v16 */
#define u8 unsigned char
#define uint32 unsigned int
#define uint8 unsigned char
#define u16 unsigned short
#endif

extern volatile unsigned int hblanks;
extern volatile unsigned int blanks;

#endif /*DS_H_*/
