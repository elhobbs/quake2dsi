/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "r_local.h"
#include "../null/ds.h"


#define	MAX_RIMAGES	512
image_t		r_images[MAX_RIMAGES];
int			numr_images;


/*
===============
R_ImageList_f
===============
*/
void	R_ImageList_f (void)
{
	int		i;
	image_t	*image;
	int		texels;

	ri.Con_Printf (PRINT_ALL, "------------------\n");
	texels = 0;

	for (i=0, image=r_images ; i<numr_images ; i++, image++)
	{
		if (image->registration_sequence <= 0)
			continue;
		texels += image->width*image->height;
		switch (image->type)
		{
		case it_skin:
			ri.Con_Printf (PRINT_ALL, "M");
			break;
		case it_sprite:
			ri.Con_Printf (PRINT_ALL, "S");
			break;
		case it_wall:
			ri.Con_Printf (PRINT_ALL, "W");
			break;
		case it_pic:
			ri.Con_Printf (PRINT_ALL, "P");
			break;
		default:
			ri.Con_Printf (PRINT_ALL, " ");
			break;
		}

		ri.Con_Printf (PRINT_ALL,  " %3i %3i : %s\n",
			image->width, image->height, image->name);
	}
	ri.Con_Printf (PRINT_ALL, "Total texel count: %i\n", texels);
}


/*
=================================================================

PCX LOADING

=================================================================
*/

int count_largest_block_kb(void);
bool sky_loading = false;

void printw(char *);
void Z_Check ();
/*
==============
LoadPCX
==============
*/
void LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height)
{
	byte	*raw;
	pcx_t	*pcx;
	int		x, y;
	int		len;
	int		dataByte, runLength;
	byte	*out, *pix;

	*pic = NULL;

	//
	// load the file
	//
	//printw("ri.FS_LoadFile");
	len = ri.FS_LoadFile (filename, (void **)&raw);
	if (!raw)
	{
		ri.Con_Printf (PRINT_DEVELOPER, "Bad pcx file %s\n", filename);
		return;
	}

	Z_Check ();
	//
	// parse the PCX file
	//
	pcx = (pcx_t *)raw;

    pcx->xmin = LittleShort(pcx->xmin);
    pcx->ymin = LittleShort(pcx->ymin);
    pcx->xmax = LittleShort(pcx->xmax);
    pcx->ymax = LittleShort(pcx->ymax);
    pcx->hres = LittleShort(pcx->hres);
    pcx->vres = LittleShort(pcx->vres);
    pcx->bytes_per_line = LittleShort(pcx->bytes_per_line);
    pcx->palette_type = LittleShort(pcx->palette_type);

	raw = &pcx->data;

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| pcx->xmax >= 640
		|| pcx->ymax >= 480)
	{
		ri.Con_Printf (PRINT_ALL, "Bad pcx file %s\n", filename);
		return;
	}

	//printw("ds_set_malloc_base");
	ds_set_malloc_base(MEM_XTRA);
	if (sky_loading == false)
		out = (byte *)ds_malloc ( (pcx->ymax+1) * (pcx->xmax+1) );
	else
		out = (byte *)ds_malloc ( ((pcx->ymax+1) * (pcx->xmax+1)) >> 2 );
	//printw("ds_set_malloc_base");
	ds_set_malloc_base(MEM_MAIN);
	
	if (out == NULL)
	{
		printf("pcx malloc for %d bytes failed\n", (pcx->ymax+1) * (pcx->xmax+1));
		printf("%.2f kb available\n", (float)count_largest_block_kb() / 1024);
		*(int *)0 = 0;
	}

	*pic = out;

	pix = out;

	//printw("palette");
	if (palette)
	{
		ds_set_malloc_base(MEM_XTRA);
		*palette = (byte *)ds_malloc(768);
		ds_set_malloc_base(MEM_MAIN);
		ds_memcpy (*palette, (byte *)pcx + len - 768, 768);
	}

	if (width)
		*width = pcx->xmax+1;
	if (height)
		*height = pcx->ymax+1;

	if (sky_loading == false)
		for (y=0 ; y<=pcx->ymax ; y++, pix += pcx->xmax+1)
		{
			for (x=0 ; x<=pcx->xmax ; )
			{
				dataByte = *raw++;

				if((dataByte & 0xC0) == 0xC0)
				{
					runLength = dataByte & 0x3F;
					dataByte = *raw++;
				}
				else
					runLength = 1;

				while(runLength-- > 0)
				{
					//				pix[x++] = dataByte;
					byte_write(&pix[x++], dataByte);
				}
			}
		}
	else
	{
		*width = *width >> 2;
		
		for (y=0 ; y<=pcx->ymax ; y++, pix += ((pcx->xmax+1) >> 2))
		{
			int skip = 0;
			for (x=0 ; x<=pcx->xmax ; )
			{
				dataByte = *raw++;

				if((dataByte & 0xC0) == 0xC0)
				{
					runLength = dataByte & 0x3F;
					dataByte = *raw++;
				}
				else
					runLength = 1;

				while(runLength-- > 0)
				{
					//				pix[x++] = dataByte;
					if ((skip & 0x3) == 0)
						byte_write(&pix[x >> 2], dataByte);
					skip++;
					x++;
				}
			}
		}
	}

	if ( raw - (byte *)pcx > len)
	{
		ri.Con_Printf (PRINT_DEVELOPER, "PCX file %s was malformed", filename);
		ds_free (*pic);
		*pic = NULL;
	}

	//register unsigned int sp asm ("sp");
	//printf("sp: %08X\n",sp);
	//printw("ri.FS_FreeFile");
	ri.FS_FreeFile (pcx);
}

/*
=========================================================

TARGA LOADING

=========================================================
*/

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


/*
=============
LoadTGA
=============
*/
void LoadTGA (char *name, byte **pic, int *width, int *height)
{
	int		columns, rows, numPixels;
	byte	*pixbuf;
	int		row, column;
	byte	*buf_p;
	byte	*buffer;
	int		length;
	TargaHeader		targa_header;
	byte			*targa_rgba;
	
	Sys_Error("load targa is not byte-write happy\n");

	*pic = NULL;

	//
	// load the file
	//
	length = ri.FS_LoadFile (name, (void **)&buffer);
	if (!buffer)
	{
		ri.Con_Printf (PRINT_DEVELOPER, "Bad tga file %s\n", name);
		return;
	}

	buf_p = buffer;

	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;
	
	targa_header.colormap_index = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.colormap_length = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.colormap_size = *buf_p++;
	targa_header.x_origin = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.y_origin = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.width = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.height = LittleShort ( *((short *)buf_p) );
	buf_p+=2;
	targa_header.pixel_size = *buf_p++;
	targa_header.attributes = *buf_p++;

	if (targa_header.image_type!=2 
		&& targa_header.image_type!=10) 
		ri.Sys_Error (ERR_DROP, "LoadTGA: Only type 2 and 10 targa RGB images supported\n");

	if (targa_header.colormap_type !=0 
		|| (targa_header.pixel_size!=32 && targa_header.pixel_size!=24))
		ri.Sys_Error (ERR_DROP, "LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;

	if (width)
		*width = columns;
	if (height)
		*height = rows;

	targa_rgba = (byte *)malloc (numPixels*4);
	*pic = targa_rgba;

	if (targa_header.id_length != 0)
		buf_p += targa_header.id_length;  // skip TARGA image comment
	
	if (targa_header.image_type==2) {  // Uncompressed, RGB images
		for(row=rows-1; row>=0; row--) {
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; column++) {
				unsigned char red,green,blue,alphabyte;
				switch (targa_header.pixel_size) {
					case 24:
							
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = 255;
							break;
					case 32:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							alphabyte = *buf_p++;
							*pixbuf++ = red;
							*pixbuf++ = green;
							*pixbuf++ = blue;
							*pixbuf++ = alphabyte;
							break;
				}
			}
		}
	}
	else if (targa_header.image_type==10) {   // Runlength encoded RGB images
		unsigned char red,green,blue,alphabyte,packetHeader,packetSize,j;
		for(row=rows-1; row>=0; row--) {
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; ) {
				packetHeader= *buf_p++;
				packetSize = 1 + (packetHeader & 0x7f);
				if (packetHeader & 0x80) {        // run-length packet
					switch (targa_header.pixel_size) {
						case 24:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								alphabyte = 255;
								break;
						case 32:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								alphabyte = *buf_p++;
								break;
					}
	
					for(j=0;j<packetSize;j++) {
						*pixbuf++=red;
						*pixbuf++=green;
						*pixbuf++=blue;
						*pixbuf++=alphabyte;
						column++;
						if (column==columns) { // run spans across rows
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row*columns*4;
						}
					}
				}
				else {                            // non run-length packet
					for(j=0;j<packetSize;j++) {
						switch (targa_header.pixel_size) {
							case 24:
									blue = *buf_p++;
									green = *buf_p++;
									red = *buf_p++;
									*pixbuf++ = red;
									*pixbuf++ = green;
									*pixbuf++ = blue;
									*pixbuf++ = 255;
									break;
							case 32:
									blue = *buf_p++;
									green = *buf_p++;
									red = *buf_p++;
									alphabyte = *buf_p++;
									*pixbuf++ = red;
									*pixbuf++ = green;
									*pixbuf++ = blue;
									*pixbuf++ = alphabyte;
									break;
						}
						column++;
						if (column==columns) { // pixel packet run spans across rows
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row*columns*4;
						}						
					}
				}
			}
			breakOut:;
		}
	}

	ri.FS_FreeFile (buffer);
}


//=======================================================

image_t *R_FindFreeImage (void)
{
	image_t		*image;
	int			i;

	// find a free image_t
	for (i=0, image=r_images ; i<numr_images ; i++,image++)
	{
		if (!image->registration_sequence)
			break;
	}
	if (i == numr_images)
	{
		if (numr_images == MAX_RIMAGES)
			ri.Sys_Error (ERR_DROP, "MAX_RIMAGES");
		numr_images++;
	}
	image = &r_images[i];

	return image;
}

/*
================
GL_LoadPic

================
*/

bool gui_loading = false;

int set_gui_loading(void)
{
	gui_loading = true;
	return 0;
}

int set_gui_not_loading(void)
{
	gui_loading = false;
	return 0;
}

int set_sky_loading(void)
{
	sky_loading = true;
	return 0;
}

int set_sky_not_loading(void)
{
	sky_loading = false;
	return 0;
}

void printw(char *str);

image_t *GL_LoadPic (char *name, byte *pic, int width, int height, imagetype_t type)
{
	image_t		*image;
	int			i, c, b;

	//printw("R_FindFreeImage");
	image = R_FindFreeImage ();
	if (strlen(name) >= sizeof(image->name))
		ri.Sys_Error (ERR_DROP, "Draw_LoadPic: \"%s\" is too long", name);
//	strcpy (image->name, name);
	ds_memcpy(image->name, name, strlen(name) + 1);
	image->registration_sequence = registration_sequence;

	image->width = width;
	image->height = height;
//	image->type = type;
	byte_write(&image->type, type);

	c = width*height;
	
	ds_set_malloc_base(MEM_XTRA);
	image->pixels[0] = (byte *)ds_malloc (c);
	ds_set_malloc_base(MEM_MAIN);
	
//	image->transparent = false;
	byte_write(&image->transparent, false);
	for (i=0 ; i<c ; i++)
	{
		b = pic[i];
		if (b == 255)
//			image->transparent = true;
			byte_write(&image->transparent, true);
//		image->pixels[0][i] = b;
		byte_write(&image->pixels[0][i], b);
//		image->pixels[0][i] = 255;
	}
	
//	ds_memcpy(&image->pixels[0][0], &pic[0], c);
//	ds_memset(image->pixels[0], 255, c);
	
	//printw("register_texture_deferred");
	if (gui_loading)
		image->texture_handle = register_texture_deferred(name, image->pixels[0], image->width, image->height, true, 255, 0, 0);
	else
		image->texture_handle = register_texture_deferred(name, image->pixels[0], image->width, image->height >> 1, false/*image->transparent*/, 255, 0, 1);
	
	//printw("image");
	return image;
}

/*
================
R_LoadWal
================
*/
#if 0
image_t *R_LoadWal (char *name)
{
	miptex_t	*mt;
	int			ofs;
	image_t		*image;
	int			size;

	ri.FS_LoadFile (name, (void **)&mt);
	if (!mt)
	{
		ri.Con_Printf (PRINT_ALL, "R_LoadWal: can't load %s\n", name);
		return r_notexture_mip;
	}

	image = R_FindFreeImage ();
	strcpy (image->name, name);
	image->width = LittleLong (mt->width);
	image->height = LittleLong (mt->height);
//	image->type = it_wall;
	byte_write(&image->type, it_wall);
	image->registration_sequence = registration_sequence;

	size = image->width*image->height * (256+64+16+4)/256;
	
	ds_set_malloc_base(MEM_XTRA);
	image->pixels[0] = (byte *)ds_malloc (size);
	ds_set_malloc_base(MEM_MAIN);
	
//	image->pixels[1] = image->pixels[2] = image->pixels[3] = NULL;
	image->pixels[1] = image->pixels[0] + image->width*image->height;
	image->pixels[2] = image->pixels[1] + image->width*image->height/4;
	image->pixels[3] = image->pixels[2] + image->width*image->height/16;

	ofs = LittleLong (mt->offsets[0]);
	ds_memcpy ( image->pixels[0], (byte *)mt + ofs, size);
	
//	int count;
//	for (count = 0; count < image->width * image->height; count++)
//		byte_write(&image->pixels[0][count], ((unsigned char *)((byte *)mt + ofs))[count * 2]);

	ri.FS_FreeFile ((void *)mt);
	
	image->texture_handle = register_texture_deferred(name, image->pixels[DS_MIP_LEVEL], image->width >> DS_MIP_LEVEL, image->height >> DS_MIP_LEVEL, false/*image->transparent*/, 255, 0, 0);

	return image;
}
#else
image_t *R_LoadWal (char *name)
{
	miptex_t	*mt;
	int			ofs;
	image_t		*image;
	int			size;

	ri.FS_LoadFile (name, (void **)&mt);
	if (!mt)
	{
		ri.Con_Printf (PRINT_ALL, "R_LoadWal: can't load %s\n", name);
		return r_notexture_mip;
	}

	image = R_FindFreeImage ();
	strcpy (image->name, name);
	image->width = LittleLong (mt->width);
	image->height = LittleLong (mt->height);
//	image->type = it_wall;
	byte_write(&image->type, it_wall);
	image->registration_sequence = registration_sequence;

	size = (image->width*image->height) >> 2;				//this mip shift values in this function only work for mip 0 and 1
	
	ds_set_malloc_base(MEM_XTRA);
	image->pixels[0] = (byte *)ds_malloc (size);
	ds_set_malloc_base(MEM_MAIN);

	ofs = LittleLong (mt->offsets[DS_MIP_LEVEL]);
	ds_memcpy ( image->pixels[0], (byte *)mt + ofs, size);

	ri.FS_FreeFile ((void *)mt);
	
	image->texture_handle = register_texture_deferred(name, image->pixels[0], image->width >> DS_MIP_LEVEL, image->height >> DS_MIP_LEVEL, false/*image->transparent*/, 255, 0, 0);

	return image;
}
#endif

void printw(char *);

/*
===============
R_FindImage

Finds or loads the given image
===============
*/
image_t	*R_FindImage (char *name, imagetype_t type)
{
	image_t	*image;
	int		i, len;
	byte	*pic, *palette;
	int		width, height;

	if (!name)
		return NULL;	// ri.Sys_Error (ERR_DROP, "R_FindImage: NULL name");
	len = strlen(name);
	if (len<5)
		return NULL;	// ri.Sys_Error (ERR_DROP, "R_FindImage: bad name: %s", name);
	//printw("look for it");
	// look for it
	for (i=0, image=r_images ; i<numr_images ; i++,image++)
	{
		if (!strcmp(name, image->name))
		{
			image->registration_sequence = registration_sequence;
			return image;
		}
	}

	//
	// load the pic from disk
	//
	pic = NULL;
	palette = NULL;
	//printw("strcmp");
	if (!strcmp(name+len-4, ".pcx"))
	{
		//printw("LoadPCX");
		LoadPCX (name, &pic, &palette, &width, &height);
		if (!pic)
			return NULL;	// ri.Sys_Error (ERR_DROP, "R_FindImage: can't load %s", name);
		//printw("GL_LoadPic");
		image = GL_LoadPic (name, pic, width, height, type);
	}
	else if (!strcmp(name+len-4, ".wal"))
	{
		//printw("R_LoadWal");
		image = R_LoadWal (name);
	}
	else if (!strcmp(name+len-4, ".tga"))
		return NULL;	// ri.Sys_Error (ERR_DROP, "R_FindImage: can't load %s in software renderer", name);
	else
		return NULL;	// ri.Sys_Error (ERR_DROP, "R_FindImage: bad extension on: %s", name);

	//printf("%08X %08X\n",pic,palette);
	//printw("ds_free");
	if (pic)
		ds_free(pic);
	if (palette)
		ds_free(palette);

	//printw("R_FindImage done");
	return image;
}



/*
===============
R_RegisterSkin
===============
*/
struct image_s *R_RegisterSkin (char *name)
{
	return R_FindImage (name, it_skin);
}


/*
================
R_FreeUnusedImages

Any image that was not touched on this registration sequence
will be freed.
================
*/
void R_FreeUnusedImages (void)
{
	int		i;
	image_t	*image;

	for (i=0, image=r_images ; i<numr_images ; i++, image++)
	{
		if (image->registration_sequence == registration_sequence)
		{
			Com_PageInMemory ((byte *)image->pixels[0], image->width*image->height);
			continue;		// used this sequence
		}
		if (!image->registration_sequence)
			continue;		// free texture
		if (image->type == it_pic)
			continue;		// don't free pics
		// free it
		ds_free (image->pixels[0]);	// the other mip levels just follow
		release_texture(image->texture_handle);
		ds_memset (image, 0, sizeof(*image));
	}
}



/*
===============
R_InitImages
===============
*/
void	R_InitImages (void)
{
	registration_sequence = 1;
}

/*
===============
R_ShutdownImages
===============
*/
void	R_ShutdownImages (void)
{
	int		i;
	image_t	*image;

	for (i=0, image=r_images ; i<numr_images ; i++, image++)
	{
		if (strstr(image->name, "conchars"))
			continue;
		
		if (!image->registration_sequence)
			continue;		// free texture
		// free it
		ds_free (image->pixels[0]);	// the other mip levels just follow
		release_texture(image->texture_handle);
		ds_memset (image, 0, sizeof(*image));
	}
}

