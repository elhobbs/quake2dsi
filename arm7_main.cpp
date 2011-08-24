#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <dswifi7.h>

#include "quake_ipc.h"
#include "snd_7.h"

#include "null/ds.h"

volatile unsigned int do_mp3 = 0;
volatile unsigned int decoder_stopped = 1;
volatile char track_name[100];

//#ifdef R21
#define REG_EXEMEMSTAT REG_EXMEMSTAT
//#endif

//#define USE_MP3
//#define USE_WIFI
//#define WIFI_ON_DEMAND
//#define USE_ADHOC

//#define IPC_IN_HBLANK
//#define IPC_IN_VBLANK
//#define IPC_IN_TIMER
//#define IPC_IN_MAIN_THREAD
#define IPC_FIFO

#ifdef USE_ADHOC
#include "libwifi/include/smi_startup_viaipc.h"
#include "libwifi/include/messagequeue.h"
#include "libwifi/include/wifi_hal.h"
#endif

//---------------------------------------------------------------------------------
void startSound(int sampleRate, const void* data, u32 bytes, u8 channel, u8 vol,  u8 pan, u8 format, bool looping) {
//---------------------------------------------------------------------------------
	SCHANNEL_TIMER(channel)  = SOUND_FREQ(sampleRate);
	SCHANNEL_SOURCE(channel) = (u32)data;
	SCHANNEL_LENGTH(channel) = bytes >> 2 ;
	SCHANNEL_CR(channel)     = SCHANNEL_ENABLE | (looping ? SOUND_REPEAT : SOUND_ONE_SHOT) | SOUND_VOL(vol) | SOUND_PAN(pan) | (format==1?SOUND_FORMAT_8BIT:SOUND_FORMAT_16BIT);
}

void ds_adjustchannel(int channel, int vol, int pan)
{
	SCHANNEL_CR(channel)     = SCHANNEL_ENABLE | SOUND_ONE_SHOT | SOUND_VOL(vol) | SOUND_PAN(pan) | (SOUND_FORMAT_8BIT);
}


//---------------------------------------------------------------------------------
s32 getFreeSoundChannel() {
//---------------------------------------------------------------------------------
	int i;
	for (i=0; i<16; i++) {
		if ( (SCHANNEL_CR(i) & SCHANNEL_ENABLE) == 0 ) return i;
	}
	return -1;
}

void update_channel_status(int channel, int enabled);
bool is_channel_enabled(int c)
{
	return (SCHANNEL_CR(c) & SCHANNEL_ENABLE);
}


void ds_playsound_on_channel(void *data, int length, int samplerate, int channel, int vol, int panning)
{
//	ARM7_PRINTF("using channel %d\n", channel);
	
	if (channel != -1)
		startSound(samplerate, data, length, channel, vol, panning, 1, false);
}


extern "C" void ds_playsound(void *data, int length, int samplerate)
{
	s32 channel = getFreeSoundChannel();
//	ARM7_PRINTF("using channel %d\n", channel);
	
	if (channel != -1)
		startSound(samplerate, data, length, channel, 127, 64, 0, false);
}

extern "C" int ds_playsound_looping(void *data, int length, int samplerate, int *channels, int num_channels)
{
	/*for (int count = 0; count < num_channels; count++)
	{
		s32 channel = getFreeSoundChannel();
		
		if (channel == -1)
			return 0;
		
		channels[count] = channel;
	}*/
	
	channels[0] = 15;
		
//	ARM7_PRINTF("using channel %d\n", channel);
	
	for (int count = 0; count < num_channels; count++)
		startSound(samplerate, data, length, channels[count], 127, 64, 0, true);
	
	return num_channels;
}

extern "C" void ds_print_ram_stat(void)
{
	ARM7_PRINT("RAM priority is\n");
	ARM7_PRINT_NUMBER(REG_EXEMEMSTAT);
	ARM7_PRINT("\n");
}

extern "C" void ds_enable_interrupts(int enabled)
{
	REG_IME = enabled;
}

extern "C" void ds_dma_copy_async(void *dest, void *source, unsigned int size)
{
//	dmaCopyWordsAsynch(0, source, dest, size);
	memcpy(dest, source, size);
}

////extern volatile int timer_hit;
//extern "C" void ds_start_loop_timer(unsigned int frequency, unsigned int multiplier, unsigned int num_samples, void (*function)(void), void (*function2)(void))
//{
////	ARM7_PRINT("frequency is\n");
////	ARM7_PRINT_NUMBER(frequency);
////	ARM7_PRINT("\n");
//	irqSet(IRQ_TIMER1, function);
//	irqSet(IRQ_TIMER2, function2);
//	unsigned short timer_value = 0xffff - ((32 * 1024 * 1024) / frequency) * multiplier + 1;
////	unsigned short timer_value = 0;
//	
//	TIMER_DATA(1) = timer_value;
//	TIMER_DATA(2) = timer_value;
//	
//	
////	ARM7_PRINT("timer set to\n");
////	ARM7_PRINT_NUMBER(timer_value);
////	ARM7_PRINT("\n");
////	
////	ARM7_PRINT("timer will fire \n");
////	ARM7_PRINT_NUMBER(num_samples / multiplier);
////	ARM7_PRINT("\ntimes before buffer finishes\n");
////	while(1);
//	TIMER_CR(1) = TIMER_ENABLE | TIMER_DIV_1 | TIMER_IRQ_REQ;
//	TIMER_CR(2) = TIMER_ENABLE | TIMER_DIV_1 | TIMER_IRQ_REQ;
//	
////	while(1)
////	{
////		ARM7_PRINT_NUMBER(timer_hit);
////		ARM7_PRINT("\n");
////	}
//}

//extern volatile int timer_hit;
extern "C" void ds_start_loop_timer(unsigned int frequency, unsigned int multiplier, unsigned int num_samples, void (*function)(void), void (*function2)(void))
{
	unsigned short timer_value1 = 0xffff - ((32 * 1024 * 1024) / frequency) * multiplier + 1;
	unsigned short timer_value2 = 0;
	unsigned short timer_value3 = 0;
	
	TIMER_DATA(1) = timer_value1;
	TIMER_DATA(2) = timer_value2;
	TIMER_DATA(3) = timer_value3;

	TIMER_CR(1) = TIMER_ENABLE | TIMER_DIV_1;
	TIMER_CR(2) = TIMER_ENABLE | TIMER_CASCADE;
	TIMER_CR(3) = TIMER_ENABLE | TIMER_CASCADE;
}

extern "C" void ds_stop_loop_timer(void)
{
	TIMER_CR(1) = 0;
	TIMER_CR(2) = 0;
	TIMER_CR(3) = 0;
}

extern "C" unsigned short ds_get_loop_timer(void)
{
	return TIMER_DATA(2);
}

extern "C" unsigned int ds_get_clock_time(void)
{
	if (((TIMER_DATA(3) << 16) | TIMER_DATA(2)) < 12)
		return 0;
	else
		return ((TIMER_DATA(3) << 16) | TIMER_DATA(2)) - 12;
}

extern "C" void ds_stopsound(int channel)
{
	SCHANNEL_CR(channel) = 0;
}

void stopAllSounds(void)
{
	int count;
	
	for (count = 0; count < 16; count++)
		SCHANNEL_CR(count) = 0;
}

int vcount;
touchPosition first,tempPos;

//---------------------------------------------------------------------------------
void VcountHandler() {
	inputGetAndSend();
}

volatile unsigned int hblanks = 0;

//extern "C" void handle_ipc(void);
unsigned int mp3_ipc = 0;

//volatile bool in_hblank = false;

void hblank_handler(void)
{
//	if (in_hblank == false)
//		in_hblank = true;
//	else
//		return;
	
	hblanks++;
#ifdef IPC_IN_HBLANK
//	volatile unsigned int saved_regIE = REG_IE;
	
//	if (do_mp3)
//	{
//		REG_IE = IRQ_HBLANK;
//		REG_IME = 1;
//	}	
	
	if (mp3_ipc == 0)
		handle_ipc();
	
//	if (do_mp3)
//	{
//		REG_IME = 0;
//		REG_IE = saved_regIE;
//	}
#endif
//	in_hblank = false;
}



int compute_jump(int address, int jump_to)
{
	int bl_inst = 0xEA000000;
	int offset = jump_to - address;
	
	offset = offset >> 2;
	offset = offset - 2;
	offset = offset & 0xFFFFFF;
	
	int new_opcode = bl_inst | offset;
	
	return new_opcode;
}
//
//unsigned short compute_jump_thumb(int address, int jump_to)
//{
//	unsigned short bl_inst = 0xE000;
//	int offset = jump_to - address;
//	
//	offset = offset >> 1;
//	offset = offset - 1;
//	offset = offset & 2047;
//	
//	unsigned short new_opcode = bl_inst | offset;
//	
//	return new_opcode;
//}

extern "C" void *new_malloc(int size);
extern "C" int helix_main(int argc, char **argv);
extern "C" void helix_init(void);

int keepalive = 0;
int arm7_initialised = 0;
int helix_initialised = 0;

volatile int free_time = kGetReady;

extern bool low_memory;
void mark_freeable(void);
void free_marked(void);

void S_UpdateStatics(void *statics, int num_statics);

void send_mp3_stop_message(void)
{
	do_mp3 = 0;
	
	fifo_msg msg;
	msg.type = kStopMP3;
	fifoSendDatamsg(FIFO_7to9,sizeof(msg),(u8 *)&msg);
	while(!fifoCheckValue32(FIFO_7to9));
	int ret = (int)fifoGetValue32(FIFO_7to9);

	/*while (quake_ipc_7to9->message == 0xffffffff);
	
	quake_ipc_7to9->message_type = kStopMP3;
	quake_ipc_7to9->message = 0xffffffff;
	
	while (quake_ipc_7to9->message == 0xffffffff);*/
	
	ARM7_PRINT("mp3 stop message sent\n");
}

#ifdef USE_WIFI

// callback to allow wifi library to notify arm9
void arm7_synctoarm9() { // send fifo message
   REG_IPC_FIFO_TX = 0x87654321;
}
// interrupt handler to allow incoming notifications from arm9
void arm7_fifo() { // check incoming fifo messages
   u32 msg = REG_IPC_FIFO_RX;
#ifdef USE_WIFI
   if(msg==0x87654321) Wifi_Sync();
#endif
}

void wifi_go(void)
{
	irqSet(IRQ_WIFI, Wifi_Interrupt); // set up wifi interrupt
	irqEnable(IRQ_WIFI);
	
	{ // sync with arm9 and init wifi
  	u32 fifo_temp;   

	  while(1) { // wait for magic number
    	while(REG_IPC_FIFO_CR&IPC_FIFO_RECV_EMPTY) swiWaitForVBlank();
      fifo_temp=REG_IPC_FIFO_RX;
      if(fifo_temp==0x12345678) break;
   	}
   	while(REG_IPC_FIFO_CR&IPC_FIFO_RECV_EMPTY) swiWaitForVBlank();
   	fifo_temp=REG_IPC_FIFO_RX; // give next value to wifi_init
   	Wifi_Init(fifo_temp);
   	
   	irqSet(IRQ_FIFO_NOT_EMPTY,arm7_fifo); // set up fifo irq
   	irqEnable(IRQ_FIFO_NOT_EMPTY);
   	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_RECV_IRQ;

   	Wifi_SetSyncHandler(arm7_synctoarm9); // allow wifi lib to notify arm9
  } // arm7 wifi init complete
}
#endif

void setup_bsp_render(void *model, void *clipplanes, int frc, int vfrc, byte *areab, int **pfri, int *modelo, unsigned int *head_surf);

int dma_channel = 0;
extern "C" void handle_ipc(u32 type)
{
//	if (arm7_initialised)
//	{
//		keepalive++;
//		if (keepalive > 96)
//		{
//			ARM7_PRINT("keepalive\n");
//			keepalive = 0;
//		}
//	}
	
	
	//if (quake_ipc_9to7->message == 0xffffffff)
	//{
//		ARM7_PRINTF("message type %d\n", quake_ipc_9to7->message_type);
		
		switch (type)
		{
			case kPrintMessage:
			{
				ARM7_PRINTF((char *)quake_ipc_9to7_buf);
				break;
			}
			case kStopAllSounds:
			{
				ARM7_PRINT("ARM7: Stopping sounds...");
				stopAllSounds();
				ARM7_PRINT("...done\n");
				
				break;
			}
			case kPlayMP3:
			{
				ARM7_PRINT("arm7 mp3 start msg\ntrack: ");
				memcpy((void *)track_name, (void *)quake_ipc_9to7_buf, 100);
				ARM7_PRINT((char *)track_name);
				ARM7_PRINT("\n");
				do_mp3 = 1;
				
				break;
			}
			case kStopMP3:
			{
				ARM7_PRINT("arm7 mp3 stop msg\n");
				do_mp3 = 0;
				
//				if (decoder_stopped)
//					send_mp3_stop_message();
				
				break;
			}
			
			//sound subsystem
			case kS_Init:
			{
				S_Init7(((unsigned int *)quake_ipc_9to7_buf)[0], ((unsigned int *)quake_ipc_9to7_buf)[1]);
				break;
			}
			case kS_AmbientOff:
			{
				S_AmbientOff7();
				break;
			}
			case kS_AmbientOn:
			{
				S_AmbientOn7();
				break;
			}
			case kS_Shutdown:
			{
				S_Shutdown7();
				break;
			}
			case kS_TouchSound:
			{
				S_TouchSound7((char *)quake_ipc_9to7_buf);
				break;
			}
			case kS_ClearBuffer:
			{
				S_ClearBuffer7();
				break;
			}
			case kS_StaticSound:
			{
				float *floats = (float *)quake_ipc_9to7_buf;
				S_StaticSound7((void *)*(unsigned int *)quake_ipc_9to7_buf,
					&floats[1],
					floats[4],
					floats[5]);
				break;
			}
			case kS_StartSound:
			{
				float *floats = (float *)quake_ipc_9to7_buf;
				S_StartSound7(((unsigned int *)quake_ipc_9to7_buf)[0], ((unsigned int *)quake_ipc_9to7_buf)[1],
					(void *)((unsigned int *)quake_ipc_9to7_buf)[2],
					&floats[3], //floats[6], floats[7],
					((unsigned int *)quake_ipc_9to7_buf)[8], ((unsigned int *)quake_ipc_9to7_buf)[9]);
				break;
			}
			case kS_StopSound:
			{
				S_StopSound7(((unsigned int *)quake_ipc_9to7_buf)[0], ((unsigned int *)quake_ipc_9to7_buf)[1]);
				break;
			}
			case kS_StopAllSounds:
			{
				S_StopAllSounds7(((unsigned int *)quake_ipc_9to7_buf)[0]);
				break;
			}
			case kS_ClearPrecache:
			{
				S_ClearPrecache7();
				break;
			}
			case kS_BeginPrecaching:
			{
				S_BeginPrecaching7();
				break;
			}
			case kS_EndPrecaching:
			{
				S_EndPrecaching7();
				break;
			}
			case kS_PrecacheSound:
			{
				void *pc = S_PrecacheSound7((char *)quake_ipc_9to7_buf);
				*(unsigned int *)quake_ipc_7to9_buf = (unsigned int)pc;
				break;
			}
			case kS_Update:
			{
//				float *floats = (float *)quake_ipc_9to7_buf;
//				S_Update7(&floats[0], &floats[3], &floats[6], &floats[9]);
				S_UpdateStatics((void *)((unsigned int *)quake_ipc_9to7_buf)[12], ((unsigned int *)quake_ipc_9to7_buf)[13]);
				break;
			}
			case kS_ExtraUpdate:
			{
				S_ExtraUpdate7();
				break;
			}
			case kS_LocalSound:
			{
				S_LocalSound7((char *)quake_ipc_9to7_buf);
				break;
			}
			case kFreeTime:
			case kRunningOut:
			case kGetReady:
			{
				//free_time = quake_ipc_9to7->message_type;
//				ARM7_PRINTF("free time is %d\n", quake_ipc_9to7->message_type);
				break;
			}
			case kStartWifi:
			{
#ifdef WIFI_ON_DEMAND
//				ARM7_PRINT("ARM7 Initialising wifi...\n");
				wifi_go();
//				ARM7_PRINTF("ARM7 ...done\n");
#else
				ARM7_PRINT("Wifi has already been initialised\n");
#endif
				break;
			}
			case kDMATransfer:
			{
				unsigned int source = ((unsigned int *)quake_ipc_9to7_buf)[0];
				unsigned int size = ((unsigned int *)quake_ipc_9to7_buf)[1];
				unsigned int dest = ((unsigned int *)quake_ipc_9to7_buf)[2];
				
				while(DMA_CR(dma_channel & 0x3) & DMA_BUSY);

				DMA_SRC(dma_channel & 0x3) = source;
				DMA_DEST(dma_channel & 0x3) = dest;
				DMA_CR(dma_channel & 0x3) = (DMA_ENABLE | DMA_32_BIT  | DMA_DST_FIX | DMA_START_NOW) | size;
				
				while(DMA_CR(dma_channel & 0x3) & DMA_BUSY);
//				ARM7_PRINT("from ");
//				ARM7_PRINT_NUMBER(source);
//				ARM7_PRINT("to ");
//				ARM7_PRINT_NUMBER(dest);
//				ARM7_PRINT("size ");
//				ARM7_PRINT_NUMBER(size);
				
				dma_channel++;
				break;
			}
			case kPowerOff:
			{
				ARM7_PRINT("ARM7: Powering down...\n");
				SerialWaitBusy();

				REG_SPICNT = SPI_ENABLE | SPI_DEVICE_POWER | SPI_BAUD_1MHz | SPI_CONTINUOUS;
				REG_SPIDATA = 0;

				SerialWaitBusy();

				REG_SPICNT = SPI_ENABLE | SPI_DEVICE_POWER | SPI_BAUD_1MHz;
				REG_SPIDATA = 1 << 6;

				break;
			}
			case kBspRender:
			{
//				ARM7_PRINT("ARM7: BSP Render\n");
				
				setup_bsp_render((void *)((unsigned int *)quake_ipc_9to7_buf)[0],
					(void *)((unsigned int *)quake_ipc_9to7_buf)[1],
					((int *)quake_ipc_9to7_buf)[2],
					((int *)quake_ipc_9to7_buf)[3],
					(unsigned char *)((unsigned int *)quake_ipc_9to7_buf)[4],
					(int **)((unsigned int *)quake_ipc_9to7_buf)[5],
					(int *)((unsigned int *)quake_ipc_9to7_buf)[6],
					(unsigned int *)((unsigned int *)quake_ipc_9to7_buf)[7]);
//				ARM7_PRINT("ARM7: BSP Render done\n");
				break;
			}
			//
			default:
			{
				ARM7_PRINT("some other message, ");
				ARM7_PRINT_NUMBER(type);
				ARM7_PRINT("\n");
				break;
			}
		}
		//quake_ipc_9to7->message = 0;
		fifoSendValue32(FIFO_9to7,0);
	//}

low_mem:

	if (low_memory)
	{
		mark_freeable();
		free_marked();
		
		low_memory = false;
	}
}

void refresh_channel_status(void)
{
	for (int count = 0; count < 16; count++)
		update_channel_status(count, is_channel_enabled(count));
}

//---------------------------------------------------------------------------------
void VblankHandler(void) {
//---------------------------------------------------------------------------------	
#ifdef USE_WIFI
	Wifi_Update(); // update wireless in vblank
#endif
#ifdef IPC_IN_VBLANK
	handle_ipc();
#endif

	if (arm7_initialised)
		refresh_channel_status();
}

//handle the lid event
//void lid(void)
//{
//	REG_IPC_FIFO_TX = 0xabadbabe;
//	for (volatile int wait_count = 0; wait_count < 200000; wait_count++);
//	REG_IPC_FIFO_CR = REG_IPC_FIFO_CR | (1 << 3);
//	
//	ARM7_PRINT("ARM7: restarting interrupts\n");
//	
//	powerON(POWER_SOUND);
//	
//	irqEnable(IRQ_HBLANK);
//	irqEnable(IRQ_VBLANK);
//	irqEnable(IRQ_VCOUNT);
//	
////	irqDisable(IRQ_LID);
//}

bool needs_defrag(void);
void defrag(volatile int *);

//void temp_test(void)
//{
//	__asm volatile (
//		"mov r0, #1		\t\n"
//		"mov r1, #2		\t\n"
//		"mov r2, #3		\t\n"
//		"mov r3, #4		\t\n"
//		"mov r4, #5		\t\n"
//		"mov r5, #6		\t\n"
//		"mov r6, #7		\t\n"
//		"mov r7, #8		\t\n"
//		"mov r8, #9		\t\n"
//		"mov r9, #0xa		\t\n"
//	);
//}

void dummy(void)
{
	ARM7_PRINT_NUMBER(REG_KEYXY);
}

volatile bool exitflag = false;

//---------------------------------------------------------------------------------
void powerButtonCB() {
//---------------------------------------------------------------------------------
	exitflag = true;
}

void fifo_DataHandler(int bytes, void *user_data) {
//---------------------------------------------------------------------------------
	fifo_msg msg;

	fifoGetDatamsg(FIFO_9to7, bytes, (u8*)&msg);
	switch(msg.type) {
		case kInit:
		{
	//				*(unsigned int *)malloc = compute_jump((unsigned int)malloc, (unsigned int)new_malloc);
	#if 1
			quake_ipc_9to7_buf = msg.buf_9to7;
			quake_ipc_7to9_buf = msg.buf_7to9;
			fifoSendValue32(FIFO_9to7,1);
	#else
			quake_ipc_9to7->message = 0;
				
			while (quake_ipc_9to7->message == 0);
				
			quake_ipc_9to7_buf = (unsigned char *)quake_ipc_9to7->message;
				
			quake_ipc_9to7->message = 0xffffffff;
				
			while (quake_ipc_9to7->message == 0xffffffff);
				
			quake_ipc_7to9_buf = (unsigned char *)quake_ipc_9to7->message;
				
			ARM7_PRINT("done\n");
	//				ARM7_PRINTF("7: 9->7 %08x 7->9 %08x\n", quake_ipc_9to7_buf, quake_ipc_7to9_buf);

			quake_ipc_9to7->message = 0xffffffff;
	#endif
			arm7_initialised = 1;
			break;
				
		}
		default:
			handle_ipc(msg.type);
			break;
	}
}


//---------------------------------------------------------------------------------
int main(int argc, char ** argv) {
//---------------------------------------------------------------------------------
	readUserSettings();
	
	irqInit();
	initClockIRQ();
	fifoInit();
	
	SetYtrigger(80);
#ifdef USE_WIFI
	installWifiFIFO();
#endif
	//installSoundFIFO();
	installSystemFIFO();

 	//TIMER_CR(0) = TIMER_ENABLE | TIMER_IRQ_REQ | TIMER_DIV_1;

	vcount = 80;
	irqSet(IRQ_VBLANK, VblankHandler);
	irqSet(IRQ_VCOUNT, VcountHandler);
	irqSet(IRQ_HBLANK, hblank_handler);

	irqEnable(IRQ_VBLANK | IRQ_VCOUNT | IRQ_HBLANK);
	
	setPowerButtonCB(powerButtonCB);   
	
	fifoSetDatamsgHandler(FIFO_9to7, fifo_DataHandler, 0);

//	irqSet(IRQ_LID, lid);
//	irqEnable(IRQ_LID);
	
#ifdef IPC_IN_TIMER
	irqSet(IRQ_TIMER0, handle_ipc);
	TIMER_DATA(0) = TIMER_FREQ_256(20 * 60);
 	TIMER_CR(0) = TIMER_ENABLE | TIMER_IRQ_REQ | TIMER_DIV_256;
#endif
	
#ifdef USE_WIFI
#ifndef WIFI_ON_DEMAND
	wifi_go();
#endif
#endif

#ifdef USE_ADHOC
	IPC_Init() ;
	LWIFI_Init() ;
#endif
	
	// Keep the ARM7 idle
	while (!exitflag)
	{
		//ARM7_PRINT("arm7 ");
		if ( 0 == (REG_KEYINPUT & (KEY_SELECT | KEY_START | KEY_L | KEY_R))) {
			exitflag = true;
		}
#ifdef USE_ADHOC
		LWIFI_IPC_UpdateNOIRQ() ;
#endif
		if (do_mp3 != 0)
		{
#ifdef USE_MP3
			ds_enable_interrupts(0);
			if (helix_initialised == 0)
			{
				ARM7_PRINT("beginning helix init\n");
				helix_init();
				ARM7_PRINT("helix init done\n");
				
				helix_initialised = 1;
			}
			
			char helix_argv1[100];
			char helix_argv2[100];
			memset(helix_argv1, 0, 100);
			memset(helix_argv2, 0, 100);
			strcpy(helix_argv1, "/jones.mp3");
//			sprintf(helix_argv1, "/quake soundtrack/0%d-AudioTrack 0%d.mp3", mp3_track_no, mp3_track_no);
//			strcpy(helix_argv2, "/jones.pcm");
			
			char *proper_helix_argv[3];
			proper_helix_argv[0] = NULL;
			proper_helix_argv[1] = helix_argv1;
//			proper_helix_argv[1] = (char *)track_name;
			proper_helix_argv[2] = helix_argv2;
			
			ARM7_PRINT("Starting Helix...\n");
			decoder_stopped = 0;
			
			ds_enable_interrupts(1);		
			helix_main(3, proper_helix_argv);
			ds_enable_interrupts(0);
			
			decoder_stopped = 1;
			ARM7_PRINT("out of helix\n");
//			ARM7_HALT();
			send_mp3_stop_message();
			
			ds_enable_interrupts(1);
#endif
		}

//		if (REG_KEYXY & (1 << 7))
//		{
//			while (quake_ipc_7to9->message == 0xffffffff);
//			
//			quake_ipc_7to9->message_type = kLidHasClosed;
//			quake_ipc_7to9->message = 0xffffffff;
//			
//			while (quake_ipc_7to9->message == 0xffffffff);
//			
//			ARM7_PRINT("ARM7: waiting...\n");
//			while (REG_IPC_FIFO_CR&IPC_FIFO_RECV_EMPTY);
//			ARM7_PRINT("ARM7: disabling interrupts\n");
//			
//			irqDisable(IRQ_HBLANK);
//			irqDisable(IRQ_VBLANK);
//			irqDisable(IRQ_VCOUNT);
//			
////			irqEnable(IRQ_LID);
//			
//			powerOFF(POWER_SOUND);
//			
//			swiWaitForVBlank();			//wake up when the lid re-enables the interrupt
//		}
		
		if (arm7_initialised && ((hblanks & 0x3f) == 0))
		{
			arm7_initialised = 1;
			//ARM7_PRINT("ping");
			//while (quake_ipc_7to9->message == 0xffffffff);
			
			//quake_ipc_7to9->message_type = kPing;
			//quake_ipc_7to9->message = 0xffffffff;
			//while (quake_ipc_7to9->message == 0xffffffff);
		}
			
//		int total = 0;
//		for (int count = 0 ; count < 16; count++)
//			if (!is_channel_enabled(count))
//				total++;
//		if (free_time == kFreeTime)
//			ARM7_PRINTF("%d channels free\n", total);
		
//		if (free_time == kFreeTime)
//			if (needs_defrag())
//			{
//				ARM7_PRINT("NEEDS DEFRAG\n");
//				defrag(&free_time);
//			}
		
//		swiWaitForVBlank();

#ifdef IPC_IN_MAIN_THREAD
		//handle_ipc();
#endif
//		if (arm7_initialised == true)
//		{
////			temp_test();
//			
//			arm7_initialised = false;
//			ARM7_PRINT("installing exception handler\n");
//			install();
//			register unsigned int sp asm ("13");
//			ARM7_PRINT("done\nlet\'s try and crash it\n");
//			/*ARM7_PRINT_NUMBER(sp);*/
//			
////			asm("LDR r12,=MyReg2\n"
////				"STMIA r12,{r0-r15}\n"
////					:
////					:
////					: "r12");
//			
////			ARM7_PRINT_NUMBER(MyReg2[12]);
//			
////			for (int count = 0; count < 16; count++)
////				ARM7_PRINT_NUMBER(MyReg2[count]);
////			ARM7_PRINT("\n");
////			while(1);
////			asm (".word 0xffffffff");
////			asm (".word 0x0");
//			
//			
////			Exception();
//	
////			asm (".word 0xe6000010");
//			
////			asm("LDR r12,=MyReg2\n"
////				"STMIA r12,{r0-r15}\n"
////					:
////					:
////					: "r12");
////			
////			for (int count = 0; count < 8; count++)
////			{
////				ARM7_PRINT_NUMBER(MyReg[count + 8]);
////				ARM7_PRINT_NUMBER(MyReg2[count + 8]);
////			}
//			
//			
////			asm ("mcr 15,0,r0,c0,c0,0");
////			asm ("bkpt");
////			*(int *)0 = 0;
////			asm ("b debugger_hit");
////			debugger_hit();
////			debugger_handle_breakpoint(0,0,0);
//			
////			asm (".word 0xe6000010");
////			asm (".short 0xe801");
//			ARM7_PRINT("shouldn\'t make it here\n");
//			ARM7_PRINT_NUMBER(ProperCPSR);
//			while(1);
//		}
		swiIntrWait(1, 1);
	}
}


