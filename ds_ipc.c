#include <stdlib.h>
#include <stdio.h>

#include <nds.h>

#include "null/ds.h"
#include "quake_ipc.h"

unsigned char *S_LoadSound(char *name);
extern int com_filesize;
extern volatile unsigned int decoder_stopped;

unsigned int time_to_sleep = 0;

void init_arm7(void)
{
	volatile fifo_msg msg;
	int ret;

	printf("Init ARM7...");
	quake_ipc_9to7_buf = (unsigned char *)malloc(100);
	quake_ipc_7to9_buf = (unsigned char *)malloc(100);
	
	if ((quake_ipc_9to7_buf == NULL) || (quake_ipc_7to9_buf == NULL))
	{
		printf("couldn\'t alloc ARM9<->ARM7 IPC space\n");
		*(int *)0 = 0;
		while(1);
	}

	msg.buf_7to9 = quake_ipc_7to9_buf;
	msg.buf_9to7 = quake_ipc_9to7_buf;
	
	quake_ipc_9to7_buf = (volatile unsigned char *)memUncached((void *)quake_ipc_9to7_buf);//(unsigned char *)((unsigned int)quake_ipc_9to7_buf | 0x400000);
	quake_ipc_7to9_buf = (volatile unsigned char *)memUncached((void *)quake_ipc_7to9_buf);//(unsigned char *)((unsigned int)quake_ipc_7to9_buf | 0x400000);
	
	
	printf("9: 9->7 %08x 7->9 %08x\n", quake_ipc_9to7_buf, quake_ipc_7to9_buf);
	
	//send the init message
#if 1
	msg.type = kInit;
	printf("1\n");
	fifoSendDatamsg(FIFO_9to7, sizeof(msg), (u8*)&msg);
	printf("2\n");
	while(!fifoCheckValue32(FIFO_9to7));
	printf("3\n");

	ret = (int)fifoGetValue32(FIFO_9to7);
	printf("4\n");
#else
	quake_ipc_9to7->message_type = kInit;
	ipc_set_ready_9to7();
	
	printf("9: 9->7 %08x 7->9 %08x\n", quake_ipc_9to7_buf, quake_ipc_7to9_buf);
	
	//the ARM7 will now change the message to say it's ready
	ipc_block_ready_9to7();
	
	printf("9: 7 has begun init\n");
	
	quake_ipc_9to7->message = (unsigned int)quake_ipc_9to7_buf;
	
	//it'll change this when it's read it
	while (quake_ipc_9to7->message != 0xffffffff);
	
	printf("9: 7 has read 9to7\n");
	
	quake_ipc_9to7->message = (unsigned int)quake_ipc_7to9_buf;
	
	//it'll change this when it's read it
	while (quake_ipc_9to7->message == (unsigned int)quake_ipc_7to9_buf);
	
	printf("9: 7 has read 7to9\n");
	
	quake_ipc_9to7->message = 0;
#endif

	printf("ARM7 is initialised\n");
}

#if 0
void ipc_block_ready_9to7(void)
{
	while (quake_ipc_9to7->message == 0xffffffff);
}

bool ipc_test_ready_9to7(void)
{
	return (quake_ipc_9to7->message == 0xffffffff);
}

void ipc_set_ready_9to7(void)
{
	quake_ipc_9to7->message = 0xffffffff;
}

void ipc_block_ready_7to9(void)
{
	while (quake_ipc_7to9->message == 0xffffffff);
}
#endif

bool arm7_ping = false;
bool ipc_disabled = false;
unsigned int num_kb_loads = 0;
extern volatile int texture_critical_section;
void printw(char *str);
void handle_ipc2(fifo_msg *msg)
{
//	if (files_locked())
//		return;
	
	if (ipc_disabled)
		return;
	
	if (texture_critical_section)
		return;
	
	//printf("msg %d\n",msg->type);
	//if (quake_ipc_7to9->message == 0xffffffff)
	//{
		switch (msg->type)
		{
			case kPrintMessage:
			{
				printf("ARM7 M: %02x %02x %02x %02x\n%16s\n",
					quake_ipc_7to9_buf[0],
					quake_ipc_7to9_buf[1],
					quake_ipc_7to9_buf[2],
					quake_ipc_7to9_buf[3],
					quake_ipc_7to9_buf);
				//printw("");
				break;
			}
			case kPrintNumber:
			{
				printf("ARM7 N: %08x\n", *(unsigned int *)quake_ipc_7to9_buf);
				break;
			}
			case kPrintFloat:
			{
				printf("ARM7 F: %d", (int)*(volatile float *)quake_ipc_7to9_buf);
//				do
//				{
//					scanKeys();
//				} while (keysHeld());
				break;
			}
			case kMalloc:
			{
				printf("ARM7 malloc req %db...", *(volatile unsigned int *)quake_ipc_7to9_buf);
				volatile unsigned int addr = (volatile unsigned int)malloc(*(volatile unsigned int *)quake_ipc_7to9_buf);
				
				if (addr == 0)
					printf("\nARM7 MALLOC FAILED!\n");
				
				//addr |= 0x400000;
				
//				printf("9 says %08x\n", addr);
				
				//*(volatile unsigned int *)(msg->buf_7to9) = addr;
				fifoSendAddress(FIFO_7to9,(void *)addr);
				//quake_ipc_7to9->message_type = kMallocResponse;
				
//				printf("9 says %08x\n", addr);
				
				break;
			}
			case kFree:
			{
				printf("ARM7 free req on %08x\n", *(volatile unsigned int *)quake_ipc_7to9_buf);
				void *ptr = (void *)*(volatile unsigned int *)quake_ipc_7to9_buf;
				free(ptr);
				break;
			}
			case kFOpen:
			{
//				file_lock();
				//this ain't gonna work for the exram build
				printf("ARM7 fopen: %s\n",quake_ipc_7to9_buf);
				FILE *fp = fopen((char *)quake_ipc_7to9_buf, "rb");
				
				if (fp)
					printf("...ok\n");
				else
				{
					printf("...failed to open\n", quake_ipc_7to9_buf);
					printf("message type is %08x\ndata block is\n%08x\n%08x\n%08x\n",
						msg->type,
						*(unsigned int *)quake_ipc_7to9_buf,
						*((unsigned int *)quake_ipc_7to9_buf + 1),
						*((unsigned int *)quake_ipc_7to9_buf + 2));
					*(int *)0 = 0;
					while(1);
				}
				
				printf("9: fp is %08x\n", (unsigned int)fp);
				
				//*(volatile unsigned int *)quake_ipc_9to7_buf = (volatile unsigned int)fp;
				fifoSendAddress(FIFO_7to9,(void *)fp);
				
//				file_unlock();
				
				break;
			}
			case kFClose:
			{
//				file_lock();
				
				printf("ARM7 fclose: %08x\n", *(volatile unsigned int *)quake_ipc_7to9_buf);
				unsigned int fp = *(volatile unsigned int *)quake_ipc_7to9_buf;
				fclose((FILE *)fp);
				
//				file_unlock();
				
				break;
			}
			case kFRead:
			{
//				file_lock();
				
				unsigned int buf = ((volatile unsigned int *)quake_ipc_7to9_buf)[0];
				unsigned int size = ((volatile unsigned int *)quake_ipc_7to9_buf)[1];
				unsigned int num = ((volatile unsigned int *)quake_ipc_7to9_buf)[2];
				unsigned int fp = ((volatile unsigned int *)quake_ipc_7to9_buf)[3];
				
				printf("ARM7 fread(%08x %d %d %08x)\n",
						buf, size, num, fp);
				
				unsigned int result = fread((unsigned char *)buf, size, num, (FILE *)fp);
				//*(volatile unsigned int *)quake_ipc_9to7_buf = result;
				fifoSendValue32(FIFO_7to9,result);
				
				if (result != num)
					printf("9: too short\n");
				
//				file_unlock();

				break;
			}
			case kStopMP3:
			{
				printf("received decoder stop message\n");
//				decoder_stopped = 1;
				break;
			}
			case kS_LoadSound:
			{
//				printf("ARM9: S_LoadSound, %s\n", (char *)quake_ipc_7to9_buf);
				unsigned int file_len;
				unsigned char *data = S_LoadSoundFile((char *)quake_ipc_7to9_buf, &file_len);
				
				if ((int)file_len > 0)
					num_kb_loads += file_len;
				
				if (data == NULL)
					printf("failed to open sound file %s\n", (char *)quake_ipc_7to9_buf);
				extern int num_loads, num_plays;
//				printf("%d l (%d), %d p\n", num_loads, num_kb_loads >> 10, num_plays);
				
				((unsigned int *)quake_ipc_7to9_buf)[0] = (unsigned int)data;	//yeah, I know
				((unsigned int *)quake_ipc_7to9_buf)[1] = file_len;
				
				ds_set_malloc_base(MEM_XTRA);

				unsigned int address_l2 = ((unsigned int *)quake_ipc_7to9_buf)[2] =
					(int)file_len > 0 ? (unsigned int)ds_malloc(file_len) : 0;
				
//				printf("ARM9: sound data is in %08x, size %d\n", (unsigned int)data, file_len);
//				printf("ARM9: read size as %d bytes\n", ((unsigned int *)data)[1] + 8);
				
				DC_FlushRange(data, file_len);
				
				sysSetCartOwner(BUS_OWNER_ARM7);				//ARM9 can't access the memory in this time
				//quake_ipc_7to9->message_type = kS_LoadSoundResponse;
				
				//while (quake_ipc_7to9->message_type == kS_LoadSoundResponse);
				fifoSendValue32(FIFO_7to9,0);
				while(!fifoCheckValue32(FIFO_7to9));
				int ret = (int)fifoGetValue32(FIFO_7to9);


				
				sysSetCartOwner(BUS_OWNER_ARM9);
				
//				printf("%d d (%dkb)\n",
//						((unsigned int *)quake_ipc_7to9_buf)[0],
//						((unsigned int *)quake_ipc_7to9_buf)[1]);
				
				if (address_l2)
				{
//					printf("shrinking %08x from %d->%d\n", address_l2, file_len, ((unsigned int *)quake_ipc_7to9_buf)[2]);
					ds_realloc((void *)address_l2, ((unsigned int *)quake_ipc_7to9_buf)[2]);
				}
				
				ds_set_malloc_base(MEM_MAIN);
				
				S_FreeSoundFile(data);
				break;
			}
			case kHalt:
			{
				printf("ARM7 IS HALTING!\n");
//				*(int *)0 = 0;
				while(1);
				break;
			}
			case kLidHasClosed:
			{
				printf("lid has closed\n");
				time_to_sleep = 1;
				break;
			}
			case kPing:
			{
				printf("ping\n");
				arm7_ping = !arm7_ping;
				unsigned short *text_map = (unsigned short *)SCREEN_BASE_BLOCK_SUB(9);
		
				if (arm7_ping)
					text_map[0] = 0xf058;
				else
					text_map[0] = 0xf02b;
					
				break;
			}
			case kNeedBusControl:
			{
//				printf("switching bus control to ARM7\n");
				
//				if (((char *)quake_ipc_7to9_buf)[0])
//					printf("msg: %s\n", (char *)quake_ipc_7to9_buf);
				
				sysSetCartOwner(BUS_OWNER_ARM7);
				//quake_ipc_7to9->message_type = kHasBusControl;
				
				//while (quake_ipc_7to9->message_type == kHasBusControl);
				fifoSendValue32(FIFO_7to9,0);
				while(!fifoCheckValue32(FIFO_7to9));
				int ret = (int)fifoGetValue32(FIFO_7to9);
				
//				printf("switching bus control back\n");
				sysSetCartOwner(BUS_OWNER_ARM9);
				
				break;
			}
			case kFreeMarkedSounds:
			{
				S_FreeMarkedSounds();
				break;
			}
		}

		fifoSendValue32(FIFO_7to9,0);
		//quake_ipc_7to9->message = 0;
	//}
}
