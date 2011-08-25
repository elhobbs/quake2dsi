#include <nds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

//#include <user_debugger.h>
#include <fat.h>

#include "null/ds.h"
#include "memory.h"
#include "quake_ipc.h"

//#define USE_DEBUGGER
#define USE_EXCEPTION_HANDLER
#define USE_3D
//#define IPC_IN_TIMER
//#define USE_SOUND
//#define OVERRIDE_SPEED

extern void quake2_main (int argc, char **argv);
int ndsgetNextPaletteSlot(u16 count, uint8 format);

char *get_build_time(void);
char *get_build_date(void);

unsigned int sound_memory = 0;
void *backup_sound_memory = NULL;
int update_count = 0;

extern "C" void debug_print_stub(char* string)
{
	printf(string);
}

volatile unsigned int blanks = 0;
volatile unsigned int hblanks = 0;

//unsigned char *frame_buffer_base;
//int frame_buffer_width;
//int frame_buffer_height;

unsigned char *new_palette = NULL;
int palette_address = 0;

void drawKeyboard(int tileBase, int mapBase);
void draw_input(unsigned short *);
void set_keyb_show_mode(int mode);

extern bool use_osk;
extern int show_mode;

void enable_keyb(void)
{
	use_osk = true;
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE | DISPLAY_BG1_ACTIVE);
	REG_BLDALPHA_SUB = (15 << 8) | 31;
	bgShow(4);
}

void disable_keyb(void)
{
	use_osk = false;
	videoSetModeSub(MODE_0_2D | DISPLAY_BG1_ACTIVE);
	bgHide(4);
}

void toggle_keyb(void)
{
	if (use_osk)
	{
		if (show_mode == 0) {
			set_keyb_show_mode(1);
			enable_keyb();
		} else
		{
			set_keyb_show_mode(0);
			disable_keyb();
		}
	}
	else
		enable_keyb();
}

void vblank_handler(void) __attribute__ ((no_instrument_function)) __attribute__((section(".itcm"), long_call));

void vblank_handler(void)
{
	blanks++;
	update_count++;
//	hblanks += 262;
	
	if (blanks & 0x1)
		*(u16*)SCREEN_BASE_BLOCK_SUB(31) = 0xf058;
	else
		*(u16*)SCREEN_BASE_BLOCK_SUB(31) = 0xf02b;
	
	if (new_palette != NULL)
	{
		//so the interrupts don't fuck up when debugging
		unsigned char *temp_palette = new_palette;
		new_palette = NULL;
		
		ds_loadpalette(temp_palette);
	}
	
	if (update_count > 20)
	{
		drawKeyboard(2, 10);
		update_count = 0;
	}

	if (new_palette != NULL)
	{
		ds_loadpalette(new_palette);
		new_palette = NULL;
	}
}

void get_pen_pos(short *px, short *py)
{
	touchPosition touchXY;
	touchRead(&touchXY);
	*px = touchXY.px;
	*py = touchXY.py;
}


void hblank_handler(void) __attribute__ ((no_instrument_function)) __attribute__((section(".itcm"), long_call));
void hblank_handler(void)
{
	hblanks++;
//	sys_frame_time = (unsigned int)((float)hblanks / 15.72f);
}

void ds_schedule_loadpalette(unsigned char *palette)
{
	new_palette = palette;
}

#ifdef ARM9
void glColorTable( uint8 format, uint32 addr ) {
GFX_PAL_FORMAT = addr>>(4-(format==GL_RGB4));
}
#endif

void ds_loadpalette(unsigned char *palette)
{
	unsigned short temp_palette[256];
	unsigned short *dest = (unsigned short *)BG_PALETTE_SUB;
	
	int count;
	for (count = 0; count < 256; count++)
		temp_palette[count] = RGB8(palette[count * 4 + 0], palette[count * 4 + 1], palette[count * 4 + 2]) | (1 << 15);
	
	memcpy(BG_PALETTE_SUB, temp_palette, 255 * 2);
	dest[255] = RGB8(0, 0, 0) | (1 << 15);
	
	if (palette_address == 0)
	{
		int addr = ndsgetNextPaletteSlot(256, GL_RGB256);
		
		if( addr>=0 )
		{
			vramSetBankG(VRAM_G_LCD);
	 		swiCopy( temp_palette, &VRAM_G[addr>>1] , 256 / 2 | COPY_MODE_WORD);
		 	vramSetBankG(VRAM_G_TEX_PALETTE);
		}
		
		if (addr == -1)
		{
			printf("couldn\'t load palette!");
			*(int *)0 = 0;
		}
		palette_address = addr;
	}
	else
	{
		vramSetBankG(VRAM_G_LCD);
 		swiCopy( temp_palette, &VRAM_G[palette_address>>1] , 256 / 2 | COPY_MODE_WORD);
	 	vramSetBankG(VRAM_G_TEX_PALETTE);
	}
		
	glColorTable(GL_RGB256, palette_address);
}

void ds_reinit_console(unsigned char *font)
{
	printf("Changing console font\n");
	unsigned short *ptr = (u16*)CHAR_BASE_BLOCK_SUB(0);
	extern unsigned short *d_8to16table;
 
	int new_char, count;
	
	for (new_char = 0; new_char < 256; new_char++)
	{
		int row = new_char >> 4;
		int col = new_char & 15;
		unsigned char *source = font + (row << 10) + (col << 3);
		
		for (count = 16 * new_char; count < 17 * new_char; count += 2)
		{
			unsigned short new_line = 0;
		
			ptr[count * 2] = source[0] | (source[1] << 8);
			ptr[count * 2 + 1] = source[2] | (source[3] << 8);
			ptr[count * 2 + 2] = source[4] | (source[5] << 8);
			ptr[count * 2 + 3] = source[6] | (source[7] << 8);
			
			source += 128;
		}

		BGCTRL_SUB[1] = (u16)(BG_MAP_BASE(9) | BG_TILE_BASE(0) | BgSize_T_256x256 | BG_COLOR_256);
 

		
//		printf("char %d\n", new_char);
//		
//		int tempo;
//		for (tempo = 0; tempo < 1000000; tempo++);
	}
}

void *ds_get_some_vram(void)
{
	vramSetBankE(VRAM_E_LCD);
	vramSetBankF(VRAM_F_LCD);
	
	return (void *)0x6880000;
}

void ds_time(int *year, int *month, int *day, int *hour, int *minute, int *second)
{
	time_t unixTime = time(0);
	struct tm* timeStruct = gmtime((const time_t *)&unixTime);

	*year = timeStruct->tm_year;
	*month = timeStruct->tm_mon;
	*day = timeStruct->tm_mday;
	
	if (timeStruct->tm_hour >= 52)
		*hour = timeStruct->tm_hour - 52 + 12;
	else
		*hour = timeStruct->tm_hour;
	
	*minute = timeStruct->tm_min;
	*second = timeStruct->tm_sec;
}

void handle_ipc2(fifo_msg *msg);

void fifo_DataHandler(int bytes, void *user_data) {
//---------------------------------------------------------------------------------
	fifo_msg msg;

	fifoGetDatamsg(FIFO_7to9, bytes, (u8*)&msg);
	handle_ipc2(&msg);
}
#ifdef ARM9
extern bool __dsimode;
#endif

#ifdef __cplusplus
extern "C" {
#endif 
   
void systemErrorExit(int rc) {
    printf("exit with code %d\n",rc);
    while(1) {
        swiWaitForVBlank();
        if (keysCurrent() & KEY_A) break;
    }
}

#ifdef __cplusplus
};
#endif 
int main(int argc, char **argv)
{
	//powerON(POWER_ALL);
	
	//REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR;
	
#ifdef USE_EXCEPTION_HANDLER
	defaultExceptionHandler();
#endif
  
	// Use the main screen for output
#ifdef USE_3D
	videoSetMode(MODE_0_3D);
	
	vramSetBankA(VRAM_A_TEXTURE);
 	vramSetBankB(VRAM_B_TEXTURE);
 	vramSetBankC(VRAM_C_TEXTURE);
 	vramSetBankD(VRAM_D_TEXTURE);
 	
// 	vramSetBankE(VRAM_E_LCD);		//for texture info (the chunk stuff)
#else
 	videoSetMode(MODE_FB0 | DISPLAY_BG0_ACTIVE);
 	vramSetBankA(VRAM_A_LCD);
#endif
 	
 	REG_BG0CNT = BG_MAP_BASE(31);
 	videoSetModeSub(MODE_0_2D | DISPLAY_BG1_ACTIVE);

	vramSetBankH(VRAM_H_SUB_BG);
	vramSetBankI(VRAM_I_SUB_BG_0x06208000);
	
	//SUB_BG0_CR = BG_MAP_BASE(10) | BgType_Text4bpp | BG_TILE_BASE(2);
	bgInitSub(0,BgType_Text4bpp,BgSize_T_256x256,10,2);
	//videoBgDisableSub(0);
	bgSetPriority(4,0);
	bgSetPriority(5,1);
	//SUB_BG1_CR = BG_MAP_BASE(9) | BG_256_COLOR;
	
	//SUB_BLEND_CR = BLEND_SRC_BG0 | BLEND_DST_BG1 | BLEND_ALPHA;
	//SUB_BLEND_AB = (15 << 8) | 13;
		
	//BG_PALETTE_SUB[0] = RGB15(31,31,31);
	
	consoleInit(0,1,BgType_Text4bpp,BgSize_T_256x256,9, 0,false,true);
	
	disable_keyb();
	
//	lcdSwap();
	
	printf("QUAKE2DS, by Simon Hall\n\t built %s %s\n", get_build_date(), get_build_time());
	printf("Starting up\n");
	
#ifdef USE_WIFI
	printf("\twifi is enabled\n");
#endif
//	printf("\textended RAM is enabled\n\thold R for options\n");
	printf("\textended RAM is enabled\n\n");
	printf("\n");

	printf("Initialising fat...");

	if (fatInitDefault()) //(16, true))
		printf("ok\n");
	else
	{
		printf("failed!\n");
		while(1);
	}
	
#ifdef USE_DEBUGGER
	//start up the wireless, connect to the AP
	set_verbosity(VERBOSE_ERROR /*| VERBOSE_INFO | VERBOSE_TRACE VERBOSE_NONE*/);
	wireless_init(1);
	wireless_connect();

	//connect to the PC stub, initialise the debugger
	debugger_connect_tcp(192, 168, 0, 20);
	debugger_init();					//from here on in you can get the debugger's attention
	
	//this will check for new commands from the debugger - call this repeatedly
	//the first time it is call it will break until you decide to resume
	//you can call this more than once in your program
	user_debugger_update();			//the earlier you call it, the earlier you can pause your program
#endif
	
	irqSet(IRQ_VBLANK, (void (*)())vblank_handler);
	irqEnable(IRQ_VBLANK);
	irqSet(IRQ_HBLANK, (void (*)())hblank_handler);
	irqEnable(IRQ_HBLANK);

	fifoSetDatamsgHandler(FIFO_7to9, fifo_DataHandler, 0);

#ifdef IPC_IN_TIMER
	irqSet(IRQ_TIMER0, (void (*)())handle_ipc);
	TIMER_DATA(0) = TIMER_FREQ_256(20 * 60);
	TIMER_CR(0) = TIMER_ENABLE | TIMER_IRQ_REQ | TIMER_DIV_256;
#endif
	
#ifdef OVERRIDE_SPEED
	ds_set_exram_timings(2, 1);
#endif
	
//	printf("%d main heap\n", count_largest_block_kb());
//	while(1);
	int selected_card = 0;
	if(__dsimode) {
		printf("dsi mode detected\n");
	} else {
		scanKeys();
	//	if ((keysHeld() & KEY_R) == KEY_R)
		{
	#define RAM_MENU_START_AT 9
			printf("\nSelect your RAM card\n");
			printf("\tAuto-detect\n");
			printf("\tSuperCard\n");
			printf("\tM3 Perfect\n");
			printf("\tNDS Memory Expansion Pak\n");
			printf("\tG6 Flash\n");
			printf("\tEZ-Flash\n");
		
			printf("\nSelect RAM speed\n");
			printf("\tSlowest\n");
			printf("\tSlower\n");
			printf("\tSlow\n");
		
			printf("\n\n\tOK\n");
		
			bool setup_ram = false;
			int selected_position = 0;
			int selected_speed = 0;
		
			unsigned short *text_map = (u16*)bgGetMapPtr(5);//(unsigned short *)SCREEN_BASE_BLOCK_SUB(9);
			text_map[32 * (RAM_MENU_START_AT + selected_position)] = '>';
			text_map[32 * (RAM_MENU_START_AT + selected_card) + 1] = 0xf0 | '>';
			text_map[32 * (RAM_MENU_START_AT + 8 + selected_speed) + 1] = 0xf0 | '>';
		
			while (setup_ram == false)
			{
				scanKeys();
				unsigned int down = keysDown();
			
				if (down == 0) {
					//printf("+");
					continue;
				}
			
				if ((down & KEY_DOWN) == KEY_DOWN)
					selected_position++;
				if ((down & KEY_UP) == KEY_UP)
					selected_position--;
			
				if ((down & KEY_SELECT) == KEY_SELECT)
				{
					if (selected_position < 6)
						selected_card = selected_position;
					else if (selected_position < 12)
						selected_speed = selected_position - 8;
					else
					{
						setup_ram = true;
						printf("\x1b[2J");
					
						switch (selected_speed)
						{
						case 0:
							printf("Setting RAM speed to 10, 6\n\n");
							ds_set_exram_timings(0, 0);
							break;
						case 1:
							printf("Setting RAM speed to 8, 4\n\n");
							ds_set_exram_timings(1, 1);
							break;
						case 2:
							printf("Setting RAM speed to 6, 4\n\n");
							ds_set_exram_timings(2, 1);
							break;
						default:
							printf("invalid RAM speed, %d\n", selected_speed);
							while(1);
							break;
						}
						break;
					}
				}
			
				if (selected_position < 0)
					selected_position = 13;
				if (selected_position == 6)
					selected_position = 8;
				if (selected_position == 7)
					selected_position = 5;
				if (selected_position == 11)
					selected_position = 13;
				if (selected_position == 12)
					selected_position = 10;
				if (selected_position >= 14)
					selected_position = 0;

				for (int count = 0; count < 6; count++)
				{
					text_map[32 * (RAM_MENU_START_AT + count)] = 0xf000;
					text_map[32 * (RAM_MENU_START_AT + count) + 1] = 0xf000;
				}
			
				for (int count = 0; count < 3; count++)
				{
					text_map[32 * (RAM_MENU_START_AT + 8 + count)] = 0xf000;
					text_map[32 * (RAM_MENU_START_AT + 8 + count) + 1] = 0xf000;
				}
			
				text_map[32 * (RAM_MENU_START_AT + 13)] = 0xf000;
			
				iprintf ("\x1b[%d;0H%c",RAM_MENU_START_AT+selected_position,'>');
				iprintf ("\x1b[%d;1H%c",RAM_MENU_START_AT+selected_card,'x');
				iprintf ("\x1b[%d;1H%c",RAM_MENU_START_AT+selected_speed+8,'x');
				//text_map[32 * (RAM_MENU_START_AT + selected_position)] = '>';
				//text_map[32 * (RAM_MENU_START_AT + selected_card) + 1] = 0xf0 | '>';
				//text_map[32 * (RAM_MENU_START_AT + 8 + selected_speed) + 1] = 0xf0 | '>';
			}
		}
	}
	printf("enable_memory\n");
	//enable_memory(selected_card);
	//unsigned int memory_size = find_memory_size();
	//ds_malloc_init(get_memory_base(), memory_size);
	
	//printf("\tSlot %d FAT driver\n", 0);//ds_find_dldi_slot());
	
	init_arm7();
	
	int proper_argc = 4;
	char *proper_argv[6];
	char argv0[20], argv1[20], argv2[20], argv3[20], argv4[20], argv5[20];
	
	strcpy(argv0, "/_boot_mp.nds");
	strcpy(argv1, "+set");
	strcpy(argv2, "basedir");
	strcpy(argv3, "/");
	//strcpy(argv4, "+map");
	//strcpy(argv5, "demo1");
	
	proper_argv[0] = argv0;
	proper_argv[1] = argv1;
	proper_argv[2] = argv2;
	proper_argv[3] = argv3;
	proper_argv[4] = argv4;
	proper_argv[5] = argv5;
	
//	frame_buffer_base = VRAM_A;
//	frame_buffer_width = SCREEN_WIDTH;
//	frame_buffer_height = SCREEN_HEIGHT;
	
#ifdef USE_3D
	vram_init();
	init_textures();
	
	printf("init 3D...");
#ifdef R21
	glInit();
	glViewport(0,0,255,191);
	glClearColor(0, 0, 0, 0);
#else
	glViewPort(0,0,255,191);
	glClearColor(0, 0, 0);
#endif

	glClearDepth(0x7FFF);
#endif
	printf("done\n");
	
//	print_top_four_blocks_kb();
	
	printf("init sound...");
	//allocate memory for the sound system
#ifdef USE_SOUND
	void *p = ds_malloc(SOUND_L1_SIZE);
	sound_memory = (unsigned int)memUncached(p);// | 0x400000;
	printf("1");
	
	ds_set_malloc_base(MEM_XTRA);
	printf("2");
	backup_sound_memory = ds_malloc(SOUND_L1_SIZE);
	printf("3");
	ds_set_malloc_base(MEM_MAIN);
	printf("4");
	
	if (sound_memory == 0)
	{
		printf("could\n't allocate sound memory\n");
		while(1);
	}
#else
	sound_memory = 0;
#endif
	printf("done\n");
	
	srand(0);
	printf("entering main loop...\n");
	quake2_main(proper_argc, proper_argv);

	return 0;
}
