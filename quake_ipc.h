#ifndef QUAKE_IPC_H_
#define QUAKE_IPC_H_

#define FIFO_7to9 FIFO_USER_01
#define FIFO_9to7 FIFO_USER_02

typedef struct {
	u32	type;
	volatile unsigned char *buf_9to7;
	volatile unsigned char *buf_7to9;
} fifo_msg;

enum IPCMessageType
{
	kPrintMessage,
	kPrintNumber,
	kPrintFloat,
	kInit,
	kMalloc,
	kMallocResponse,
	kFree,
	kStopAllSounds,
	kPlayMP3,
	kStopMP3,
	kFOpen,
	kFClose,
	kFRead,
	
	kS_Init,
	kS_AmbientOff,
	kS_AmbientOn,
	kS_Shutdown,
	kS_TouchSound,
	kS_ClearBuffer,
	kS_StaticSound,
	kS_StartSound,
	kS_StopSound,
	kS_PrecacheSound,
	kS_ClearPrecache,
	kS_Update,
	kS_StopAllSounds,
	kS_BeginPrecaching,
	kS_EndPrecaching,
	kS_ExtraUpdate,
	kS_LocalSound,
	
	kS_LoadSound,
	kS_LoadSoundResponse,
	
	kHalt,
	kLidHasClosed,
	kLidHasOpened,
	kLidHasClosedResponse,
	kLidHasOpenedResponse,
	
	kPing,
	kPingResponse,
	
	kFreeTime,
	kRunningOut,
	kGetReady,
	
	kStartWifi,
	
	kSetBreakpoint,
	kClearBreakpoint,
	
	kNeedBusControl,
	kHasBusControl,
	
	kFreeMarkedSounds,
	
	kDMATransfer,
	
	kPowerOff,
	
	kBspRender,
};

//struct QuakeIPCRegion
//{
//	unsigned int message;
//	unsigned int message_type;
//};
//volatile struct QuakeIPCRegion *quake_ipc_9to7 = (volatile struct QuakeIPCRegion *)0x2400000;
volatile unsigned char *quake_ipc_9to7_buf = 0;
//volatile struct QuakeIPCRegion *quake_ipc_7to9 = (volatile struct QuakeIPCRegion *)0x2400000 + sizeof(struct QuakeIPCRegion);
volatile unsigned char *quake_ipc_7to9_buf = 0;

//#define WITH_PRINTF

#if 1
#ifdef WITH_PRINTF
#define ARM7_PRINTF(args...) \
{\
	ajshdkahsd
	while(quake_ipc_7to9->message == 0xffffffff);\
	sprintf((char *)quake_ipc_7to9_buf, args);/*memset((char *)quake_ipc_7to9_buf, 0, 100);*/\
	quake_ipc_7to9->message_type = kPrintMessage;\
	quake_ipc_7to9->message = 0xffffffff;\
	while(quake_ipc_7to9->message == 0xffffffff);\
}

#else

#define ARM7_PRINTF(args...)
//#define ARM7_PRINT(string)
//#define ARM7_PRINT_NUMBER(number)

#endif

static void arm7_print(u8* string) {
	fifo_msg msg;
	
	memcpy((void *)quake_ipc_7to9_buf, string, strlen((const char *)string) + 1);\
	
	msg.type = kPrintMessage;
	fifoSendDatamsg(FIFO_7to9,sizeof(msg),(u8 *)&msg);
	while(!fifoCheckValue32(FIFO_7to9));
	int ret = (int)fifoGetValue32(FIFO_7to9);
}

#define ARM7_PRINT(string) \
do {\
	arm7_print((u8*)(string)); \
} while(0);

static void arm7_print_number(u32 number) {
	fifo_msg msg;
	return;
	
	*((unsigned int *)quake_ipc_7to9_buf) = (unsigned int)number;\
	
	msg.type = kPrintNumber;
	fifoSendDatamsg(FIFO_7to9,sizeof(msg),(u8 *)&msg);
	while(!fifoCheckValue32(FIFO_7to9));
	int ret = (int)fifoGetValue32(FIFO_7to9);
}

#define ARM7_PRINT_NUMBER(number) \
{\
	arm7_print_number((unsigned int)(number)); \
}

static void arm7_print_float(float number) {
	fifo_msg msg;
	return;
	
	*((float *)quake_ipc_7to9_buf) = number;\
	
	msg.type = kPrintFloat;
	fifoSendDatamsg(FIFO_7to9,sizeof(msg),(u8 *)&msg);
	while(!fifoCheckValue32(FIFO_7to9));
	int ret = (int)fifoGetValue32(FIFO_7to9);
}

#define ARM7_PRINT_FLOAT(number) \
{\
	arm7_print_float(number); \
}

static void arm7_halt() {
	fifo_msg msg;
	return;
	
	msg.type = kHalt;
	fifoSendDatamsg(FIFO_7to9,sizeof(msg),(u8 *)&msg);
	while(!fifoCheckValue32(FIFO_7to9));
	int ret = (int)fifoGetValue32(FIFO_7to9);
}

#define ARM7_HALT() \
{\
	arm7_halt(); \
	while(1);\
}

#endif

#endif /*QUAKE_IPC_H_*/
