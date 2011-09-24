#ifdef ARM9
#include <nds.h>
//#include <user_debugger.h>
#endif

#include "null/ds.h"
#include "memory.h"

#ifdef ARM9
#include "quake_ipc.h"
#endif

#ifdef R21
void glReset(void) {	
	//---------------------------------------------------------------------------------
	
	while (GFX_STATUS & (1<<27)); // wait till gfx engine is not busy
	
	// Clear the FIFO
	GFX_STATUS |= (1<<29);
	
	// Clear overflows for list memory
	GFX_CONTROL = ((1<<12) | (1<<13)) | GL_TEXTURE_2D;
	glResetMatrixStack();
	
	GFX_TEX_FORMAT = 0;
	GFX_POLY_FORMAT = 0;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}
#endif

//float left_move = 0;
//float up_move = 0;
//float view_fov = 50;
//float x_scale = 160;
//float y_scale = 160;
//float z_scale = 80;
//float z_move = 0;

float oleft_move = 0;
float oup_move = 0;
float oview_fov = 37;
float ox_scale = 210;
float oy_scale = 210;
float oz_scale = 1;
float oz_move = 0;

float left_move = 0;
float up_move = 0;
float view_fov = 37;
float x_scale = 20;
float y_scale = 20;
float z_scale = 20;
float z_move = 0;

bool use_wireframe = false;
int base_polyid = 0;

/* from a thread in gbadev */

#define POLY_FOG  (1<<15) 

#define GL_FOG_ALPHA    (1<<6) 
#define GL_FOG          (1<<7) 
#define GL_FOG_SHIFT(n)   ((n)<<8) 

#define GL_RDLINE_UNDERFLOW   (1<<12) 
#define GL_VERTEX_OVERFLOW    (1<<13) 

#ifdef ARM9
void glFogDepth(uint16 depth) { GFX_FOG_OFFSET = depth; }
#else
#define  glFogDepth(_x)
#endif

int use_fog = 1;
int fog_depth = 0x7ea0;
int fog_shift = 6;

unsigned char fog_red = 15;
unsigned char fog_green = 15;
unsigned char fog_blue = 24;

void ds_set_fog(int shift, int depth)
{
	fog_shift = shift;
	fog_depth = depth;
}

extern unsigned short current_bound;

void ds_reset_render(void)
{
	extern unsigned int super_framecount;
#ifdef ARM9
	glReset();
#endif
	super_framecount++;
}

void reset_triz(void);
void next_triz(void);

void ds_begin_render(void)
{
#ifdef ARM9
	glMatrixMode(GL_PROJECTION);
	
	ds_ortho();
//	ds_perspective();
	
	glMaterialf(GL_AMBIENT, RGB15(0,0,0));
	glMaterialf(GL_DIFFUSE, RGB15(0,0,0));
	glMaterialf(GL_SPECULAR, BIT(15) | RGB15(0,0,0));
	glMaterialf(GL_EMISSION, RGB15(0,0,0));

	glMaterialShinyness();
	
	glFogColor(fog_red >> 2, fog_green >> 2, fog_blue >> 2, 31);
	int i;
	for (i = 0; i < 32; i++) {
		GFX_FOG_TABLE[i] = i << 2; 
	}
	
//	ds_polyfmt(0, 31, 0);
	glEnable(GL_BLEND);
	
//	glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK | BIT(12) | BIT(13));
	glPolyFmt(POLY_ALPHA(31) | POLY_FOG | POLY_ID(0) | POLY_CULL_NONE | BIT(12) | BIT(13));
	
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	if (use_fog)
	{
		glFogDepth(fog_depth);
		glEnable((1 << 7) | (GL_FOG_SHIFT(fog_shift)));
	}
	
	glTranslatef(oleft_move, oup_move, oz_move);
	glScalef(ox_scale, oy_scale, oz_scale); 
	
	glEnable(GL_ANTIALIAS);
#endif
	
	reset_triz();
	
	current_bound = -1;
}

int before_load = 1;
int doing_texture_loads = 1;

void ds_end_render(void)
{
#ifdef ARM9
//	printf("%d polys, %d verts\n", GFX_POLYGON_RAM_USAGE, GFX_VERTEX_RAM_USAGE);
	unsigned short *text_map = (unsigned short *)SCREEN_BASE_BLOCK_SUB(9);
	if (GFX_VERTEX_RAM_USAGE >= 6144)
		text_map[30] = 0x0056;//0xf056;
	else
		text_map[30] = 0x0020;
	
	if (GFX_POLYGON_RAM_USAGE >= 2048)
		text_map[31] = 0x0050;//0xf050;
	else
		text_map[31] = 0x0020;
		
	if (doing_texture_loads)
	{
#ifdef WITH_BLOCKING_TEXTURE_LOADS
		while (*((volatile unsigned short *)0x4000006) < HBLANK_START_LOAD);
#endif
		load_textures();
	}
	
#ifdef R21
	glFlush(0);
#else
	GFX_FLUSH = 0;
#endif
	
#endif
//	GFX_FLUSH = flush_val;
	
//	unload_textures_2();
//	verify_textures();
}

void ds_ortho(void)
{
#ifdef ARM9
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//glOrtho(-110, 110, -80, 80, 1.0f, 0x7fff);
	glOrthof32(floattof32(-110), floattof32(110), floattof32(-80), floattof32(80), -1 << 12, 1 << 12);
	//glOrthof32( 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -1 << 12, 1 << 12 );  // downscale projection matrix
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
#endif
}

void ds_perspective(void)
{
#ifdef ARM9
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
#ifdef R21
	gluPerspective(view_fov * 2, 256.0f / 192.0f, 0.2f, 0x7fff);
#else
	gluPerspective(view_fov, 256.0f / 192.0f, 0.2f, 0x7fff);
#endif
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	
	glPolyFmt(POLY_ALPHA(31) | POLY_FOG | POLY_CULL_BACK | BIT(12) | BIT(13));
	
	glLoadIdentity();
	glTranslatef(left_move, up_move, z_move);
	glScalef(x_scale, y_scale, z_scale);
#endif
}

void ds_polyfmt(int poly_id, int trans, int depth, int culling)
{
#ifdef ARM9
	if (use_wireframe)
	{
		int p = (base_polyid++) & 0x1f;
		glPolyFmt(POLY_ALPHA(trans) | POLY_FOG | culling | POLY_ID(p) | (depth << 14) | BIT(12) | BIT(13));
	}
	else
		glPolyFmt(POLY_ALPHA(trans) | POLY_FOG | culling | POLY_ID(poly_id) | (depth << 14) | BIT(12) | BIT(13));
#endif
}

void ds_vertex3f(float x, float y, float z)
{
#ifdef ARM9
	glVertex3f(x, y, z);
#endif
}

void ds_pushmatrix(void)
{
#ifdef ARM9
	glPushMatrix();
#endif
}

void ds_popmatrix(void)
{
#ifdef ARM9
	glPopMatrix(1);
#endif
}

void ds_translatef(float x, float y, float z)
{
#ifdef ARM9
	glTranslatef(x, y, z);
#endif
}

void ds_scalef(float x, float y, float z)
{
#ifdef ARM9
	glScalef(x, y, z);
#endif
}

void ds_rotateX(float angle)
{
#ifdef ARM9
	glRotateX(angle);
#endif
}

void ds_rotateY(float angle)
{
#ifdef ARM9
	glRotateY(angle);
#endif
}

void ds_rotateZ(float angle)
{
#ifdef ARM9
	glRotateZ(angle);
#endif
}

int ds_gentexture(void)
{
#ifdef ARM9
	int temp_texture;
	if (glGenTextures(1, &temp_texture) == 0)
	{
		printf("ram out of libnds texture handles\n");
		while(1);
	}
	return temp_texture;
#else
	return 0;
#endif
}

void ds_get_geometry_count(unsigned int *verts, unsigned int *polys)
{
#ifdef ARM9
	if (verts)
		*verts = GFX_VERTEX_RAM_USAGE;
	if (polys)
		*polys = GFX_POLYGON_RAM_USAGE;
#endif
}

int ds_boxtest(short x, short y, short z, short height, short width, short depth)
{
#ifdef ARM9
	GFX_BOX_TEST = VERTEX_PACK(x, y);
	GFX_BOX_TEST = VERTEX_PACK(z, height);
	GFX_BOX_TEST = VERTEX_PACK(width, depth);

	while(GFX_STATUS & BIT(0));

	return (GFX_STATUS & BIT(1));
#else
	return 0;
#endif
}

void ds_mulmatf(float m[3][4])
{
#ifdef ARM9
	m4x4 mat_fp;
	int x, y;
	
	for (y = 0; y < 16; y++)
		mat_fp.m[y] = 0;
	
	for (y = 0; y < 4; y++)
		mat_fp.m[y * 4 + y] = 1 << 12;
	
	for (x = 0; x < 3; x++)
	{
		mat_fp.m[0 + x] = (int)(m[x][0] * (1 << 12));
		mat_fp.m[4 + x] = (int)(m[x][1] * (1 << 12));
		mat_fp.m[8 + x] = (int)(m[x][2] * (1 << 12));
//		mat_fp.m[12 + x] = m[x][3] * (1 << 12);
	}
	
//	for (y = 0; y < 4; y++)
//		printf("%.2f %.2f %.2f %.2f\n",
//				(float)mat_fp.m[y * 4 ] / 4096,
//				(float)mat_fp.m[y * 4 + 1] / 4096,
//				(float)mat_fp.m[y * 4 + 2] / 4096,
//				(float)mat_fp.m[y * 4 + 3] / 4096);
	glMultMatrix4x4(&mat_fp);
#endif
}

#ifndef R21
unsigned short *ds_screen_capture(void)
{
#ifdef ARM9
	vramSetBankD(VRAM_D_LCD); 

	DISP_CAPTURE = DCAP_ENABLE | //enables display capturing 
	DCAP_BANK(3) | //which vram bank to use - 0=A, 1=B, 2=C, 3=D 
	DCAP_SIZE(3) | //3 = full size 256 x 192 
	DCAP_SRC(1); //capture source - not sure what this does
	
	return (unsigned short *)0x6860000;
#else
	return 0;
#endif
}
#endif

#if 0
void ds_setup_bsp_render(void *model, void *clipplanes, int frc, int vfrc, byte *areab, int **pfri, int *modelo, unsigned int *head_surf)
{
#ifdef ARM9
	fifo_msg msg;
	//ipc_block_ready_9to7();
	
	//quake_ipc_9to7->message_type = kBspRender;
	msg.type = kBspRender;
	msg.buf_9to7;
	((unsigned int *)quake_ipc_9to7_buf)[0] = (unsigned int)model;
	((unsigned int *)quake_ipc_9to7_buf)[1] = (unsigned int)clipplanes;
	((unsigned int *)quake_ipc_9to7_buf)[2] = (unsigned int)frc;
	((unsigned int *)quake_ipc_9to7_buf)[3] = (unsigned int)vfrc;
	((unsigned int *)quake_ipc_9to7_buf)[4] = (unsigned int)areab;
	((unsigned int *)quake_ipc_9to7_buf)[5] = (unsigned int)pfri;
	((unsigned int *)quake_ipc_9to7_buf)[6] = (unsigned int)modelo;
	((unsigned int *)quake_ipc_9to7_buf)[7] = (unsigned int)head_surf;
	
	sysSetCartOwner(BUS_OWNER_ARM7);
	//ipc_set_ready_9to7();
	//ipc_block_ready_9to7();
	sysSetCartOwner(BUS_OWNER_ARM9);
#endif
}
#endif
