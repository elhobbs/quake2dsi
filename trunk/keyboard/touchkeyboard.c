///* ScummVM - Scumm Interpreter
// * Copyright (C) 2005-2006 Neil Millstone
// * Copyright (C) 2006 The ScummVM project
// *
// * This program is free software; you can redistribute it and/or
// * modify it under the terms of the GNU General Public License
// * as published by the Free Software Foundation; either version 2
// * of the License, or (at your option) any later version.
// *
// * This program is distributed in the hope that it will be useful,
// * but WITHOUT ANY WARRANTY; without even the implied warranty of
// * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// * GNU General Public License for more details.
// *
// * You should have received a copy of the GNU General Public License
// * along with this program; if not, write to the Free Software
// * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
// *
// */
#include <nds.h>

struct key_data {
	char keyNum;
	char x, y;
	int character;
	bool pressed;
};


#define DS_TOUCH_KEYS_AT 60
#define DS_NUM_TOUCH_KEYS 32

#define DS_NUM_KEYS (60 + DS_NUM_TOUCH_KEYS)

//#define DS_SHIFT 0
//#define DS_BACKSPACE 8
//#define DS_RETURN 13
//#define DS_CAPSLOCK 1
//
//
key_data keys[DS_NUM_KEYS] = {
	// Key number		x		y		character
	
	// Numbers
	{28,				3,		0,		'1'},
	{29,				5,		0,		'2'},
	{30,				7,		0,		'3'},
	{31,				9,		0,		'4'},
	{32,				11,		0,		'5'},
	{33,				13,		0,		'6'},
	{34,				15,		0,		'7'},
	{35,				17,		0,		'8'},
	{36,				19,		0,		'9'},
	{27,				21,		0,		'0'},
	{45,				23,		0,		'-'},
	{50,				25,		0,		'='},
	{52,				27,		0,		127},			//backspace

	// Top row
	{'Q'-'A' + 1,		4,		2,		'q'},
	{'W'-'A' + 1,		6,		2,		'w'},
	{'E'-'A' + 1,		8,		2,		'e'},
	{'R'-'A' + 1,		10,		2,		'r'},
	{'T'-'A' + 1,		12,		2,		't'},
	{'Y'-'A' + 1,		14,		2,		'y'},
	{'U'-'A' + 1,		16,		2,		'u'},
	{'I'-'A' + 1,		18,		2,		'i'},
	{'O'-'A' + 1,		20,		2,		'o'},
	{'P'-'A' + 1,		22,		2,		'p'},
	{43,				24,		2,		'('},
	{44,				26,		2,		')'},

	// Middle row
	{55,				3,		4,		'#'},
	{'A'-'A' + 1,		5,		4,		'a'},
	{'S'-'A' + 1,		7,		4,		's'},
	{'D'-'A' + 1,		9,		4,		'd'},
	{'F'-'A' + 1,		11,		4,		'f'},
	{'G'-'A' + 1,		13,		4,		'g'},
	{'H'-'A' + 1,		15,		4,		'h'},
	{'J'-'A' + 1,		17,		4,		'j'},
	{'K'-'A' + 1,		19,		4,		'k'},
	{'L'-'A' + 1,		21,		4,		'l'},
	{42,				23,		4,		';'},
	{41,				25,		4,		'\''},
	{46,				27,		4,		13},			//enter

	// Bottom row
	{51,				4,		6,		127},			//shift
	{'Z'-'A' + 1,		6,		6,		'z'},
	{'X'-'A' + 1,		8,		6,		'x'},
	{'C'-'A' + 1,		10,		6,		'c'},
	{'V'-'A' + 1,		12,		6,		'v'},
	{'B'-'A' + 1,		14,		6,		'b'},
	{'N'-'A' + 1,		16,		6,		'n'},
	{'M'-'A' + 1,		18,		6,		'm'},
	{38,				20,		6,		','},
	{39,				22,		6,		'.'},
	{40,				24,		6,		'/'},

	// Space bar
	{47,				9,		8,		' '},
	{48,				11,		8,		' '},
	{48,				13,		8,		' '},
	{48,				15,		8,		' '},
	{48,				17,		8,		' '},
	{49,				19,		8,		' '},

	// Cursor arrows
	{52,				27,		8,		'#'},
	{54,				29,		8,		'#'},
	{53,				31,		8,		'#'},
	{51,				29,		6,		'#'},
	
	// Close button
	{56,				30,		0,		'#'},
	
	
	//left touch buttons (mapped to aux1->4)
	{47, 				0,		0,		207},
	{49, 				2,		0,		207},
	{47, 				0,		2,		207},
	{49, 				2,		2,		207},
	
	{47, 				0,		6,		208},
	{49, 				2,		6,		208},
	{47, 				0,		8,		208},
	{49, 				2,		8,		208},
	
	{47, 				0,		12,		209},
	{49, 				2,		12,		209},
	{47, 				0,		14,		209},
	{49, 				2,		14,		209},
	
	{47, 				0,		18,		210},
	{49, 				2,		18,		210},
	{47, 				0,		20,		210},
	{49, 				2,		20,		210},
	
	
	//right touch buttons (mapped to aux5->8)
	{47, 				28,		0,		211},
	{49, 				30,		0,		211},
	{47, 				28,		2,		211},
	{49, 				30,		2,		211},
	
	{47, 				28,		6,		212},
	{49, 				30,		6,		212},
	{47, 				28,		8,		212},
	{49, 				30,		8,		212},
	
	{47, 				28,		12,		213},
	{49, 				30,		12,		213},
	{47, 				28,		14,		213},
	{49, 				30,		14,		213},
	
	{47, 				28,		18,		214},
	{49, 				30,		18,		214},
	{47, 				28,		20,		214},
	{49, 				30,		20,		214},
	
};
//
int keyboardX = -2;
int keyboardY = 14;

int copy_of_kx;
int copy_of_ky;

int show_mode = 0;

//int keyboardX = 0;
//int keyboardY = 0;

bool needs_refresh = true;
bool keys_down = false;

int gmapBase;
int gtileBase;
//
//u16* baseAddress;
//
bool shiftState = false;
bool capsLockState = false;
//
//bool closed;
//
//char autoCompleteWord[NUM_WORDS][32];
//int autoCompleteCount;
//
//int selectedCompletion = -1;
//int charactersEntered = 0;
//
//
//void restoreVRAM(int tileBase, int mapBase, u16* saveSpace) {
///*	for (int r = 0; r < 32 * 32; r++) {
//		((u16 *) SCREEN_BASE_BLOCK_SUB(mapBase))[r] = *saveSpace++;
//	}
//	
//	for (int r = 0; r < 4096; r++) {
//		((u16 *) CHAR_BASE_BLOCK_SUB(tileBase))[r]	= *saveSpace++;
//	}*/
//}
//

extern unsigned char keyboard_raw[];
extern unsigned char keyboard_pal_raw[];



#include <stdlib.h>
#include <stdio.h>

void clear_keyboard(int mapBase)
{
	printf("clearing\n");
	for (int r = 0; r < 32 * 32; r++)
		((u16 *) SCREEN_BASE_BLOCK_SUB(mapBase))[r] = 127;
}

void set_keyb_show_mode(int mode)
{
	show_mode = mode;
	
	if (show_mode == 0)
	{
		keyboardX = copy_of_kx;
		keyboardY = copy_of_ky;
	}
	else
	{
		copy_of_kx = keyboardX;
		copy_of_ky = keyboardY;
		
		keyboardX = 0;
		keyboardY = 0;
	}
	
	needs_refresh = true;
}

bool inited = false;

void drawKeyboard(int tileBase, int mapBase)
{
	if (needs_refresh || keys_down)
	{
		needs_refresh = false;
		keys_down = false;
	}
	else
		return;
		
	printf("drawKeyboard\n");
	gmapBase = mapBase;
	gtileBase = tileBase;
		
	
	for (int r = 0; r < 32 * 32; r++)
		((u16 *) SCREEN_BASE_BLOCK_SUB(mapBase))[r] = 127;
	
	if (!inited)
	{
		for (int r = 0; r < 4096; r++)
			((u16 *) CHAR_BASE_BLOCK_SUB(tileBase))[r] = ((u16 *) (keyboard_raw))[r];
		inited = true;
	}
	
	int run_from, run_to;
	if (show_mode == 0)
	{
		run_from = 0;
		run_to = DS_TOUCH_KEYS_AT;
	}
	else
	{
		run_from = DS_TOUCH_KEYS_AT;
		run_to = DS_NUM_KEYS;
	}

	
	int x = keyboardX;
	int y = keyboardY;
	
	u16* base = ((u16 *) SCREEN_BASE_BLOCK_SUB(mapBase));

	
	for (int r = run_from; r < run_to; r++)
	{
		int pos_x = x + keys[r].x;
		int pos_y = y + keys[r].y;
		
		if ((pos_x < 0) || (pos_x >= 31))
			continue;
		
		base[pos_y * 32 + pos_x] = (keys[r].keyNum * 2);
		base[pos_y * 32 + pos_x + 1] = keys[r].keyNum * 2 + 1;
		
		base[(pos_y + 1) * 32 + pos_x] = 128 + keys[r].keyNum * 2;
		base[(pos_y + 1) * 32 + pos_x + 1] = 128 + keys[r].keyNum * 2 + 1;
		
		keys[r].pressed = false;
	}
}

void setKeyHighlight(int key, bool highlight) {
	u16* base = ((u16 *) SCREEN_BASE_BLOCK_SUB(gmapBase));

	if (highlight) {
		base[(keyboardY + keys[key].y) * 32 + keyboardX + keys[key].x] |= 0x1000;
		base[(keyboardY + keys[key].y) * 32 + keyboardX + keys[key].x + 1] |= 0x1000;
		base[(keyboardY + keys[key].y + 1) * 32 + keyboardX + keys[key].x] |= 0x1000;
		base[(keyboardY + keys[key].y + 1) * 32 + keyboardX + keys[key].x + 1] |= 0x1000;
	} else {
		base[(keyboardY + keys[key].y) * 32 + keyboardX + keys[key].x] &= ~0x1000;
		base[(keyboardY + keys[key].y) * 32 + keyboardX + keys[key].x + 1] &= ~0x1000;
		base[(keyboardY + keys[key].y + 1) * 32 + keyboardX + keys[key].x] &= ~0x1000;
		base[(keyboardY + keys[key].y + 1) * 32 + keyboardX + keys[key].x + 1] &= ~0x1000;
	}
}

void get_pen_pos(short *px, short *py);

extern bool use_osk;
void disable_keyb(void);

int addKeyboardEvents()
{
	if ((keysDown() & (1 << 12)) == (1 << 12))
	{
		short x, y;
		get_pen_pos(&x, &y);
		
		int tx = (x >> 3);
		int ty = (y >> 3);

		if (ty >= 12)
		{
			int current = -1;

			if (tx < 12) {
				current = (ty - 12) / 2;
			} else {
				current = 6 + (ty - 12) / 2;
			}
		}

		tx -= keyboardX;
		ty -= keyboardY;
		
//		printf("x=%d y=%d\n", tx, ty);
		
		for (int r = 0; r < DS_NUM_KEYS; r++) {
			if (( (tx >= keys[r].x) && (tx <= keys[r].x + 1)) && 
				   (ty >= keys[r].y) && (ty <= keys[r].y + 1))
			{
				
				needs_refresh = true;
				
				keys_down = true;
				
				if (show_mode == 0)
				{
					if (r >= DS_TOUCH_KEYS_AT)
						break;
					
					setKeyHighlight(r, true);

					switch (keys[r].keyNum)
					{
						case 45:
							if (shiftState)
							{
								shiftState = false;
								return '_';
							}
							else
								return '-';
						case 51:
							if (keys[r].character != 127)
								keyboardY -= 2;
							else
								shiftState = !shiftState;
							return -1;
						case 54:
							keyboardY += 2;
							return -1;
						case 52:
							if (keys[r].character != 127)
							{
								keyboardX--;
//								printf("keyboard is now at %d %d\n", keyboardX, keyboardY);
							}
							else
							{
								needs_refresh = false;
								return 127;			//backspace
							}
							return -1;
						case 53:
							keyboardX++;
//							printf("keyboard is now at %d %d\n", keyboardX, keyboardY);
							return -1;
						case 56:
							use_osk = false;
							disable_keyb();
							return -1;
						default:
							if (shiftState)
							{
								shiftState = false;
								if ((keys[r].character >= 'a') && (keys[r].character <= 'z'))
									return keys[r].character - 32;
								else if ((keys[r].character >= '0') && (keys[r].character <= '9'))
								{
									switch (keys[r].character)
									{
									case '1':
										return '!';
									case '2':
										return '\"';
									case '3':
										return '#';
									case '4':
										return '$';
									case '5':
										return '%';
									case '6':
										return '^';
									case '7':
										return '&';
									case '8':
										return '*';
									case '9':
										return '(';
									case '0':
										return ')';
									default:
										printf("err...received dozy key %x/%c\n", keys[r].character, keys[r].character);
									}
								}
								else if (keys[r].character == '-')
									return '_';
								else if (keys[r].character == '=')
									return '+';
								else
									return keys[r].character;
							}
							else
								return keys[r].character;
					}
				}
				else
					switch (keys[r].keyNum)
					{
					case 47:
					case 49:
							setKeyHighlight(r, true);
							return keys[r].character;
					default:
						break;
					}
			}
		}
	}
	
	return -1;
	
//	if (DS::getPenReleased()) {
//		for (int r = 0; r < DS_NUM_KEYS; r++) {
//			if (keys[r].pressed) {
//				DS::setKeyHighlight(r, false);
//				keys[r].pressed = false;
//			}
//		}
//	}
}
//
//}
//
