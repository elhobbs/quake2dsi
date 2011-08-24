#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <nds.h>

#include "null/ds.h"
#include "memory.h"

#define MAX_FILE_HANDLES 10
#define MAX_FAST_HANDLES 10
#define HANDLE_REAL 1
#define HANDLE_FAST 2

struct ds_file regular_file_handles[MAX_FILE_HANDLES];
struct fast_file fast_file_handles[MAX_FAST_HANDLES];

void init_dsfile_handles(void)
{
	memset(regular_file_handles, 0, sizeof(struct ds_file) * MAX_FILE_HANDLES);
	memset(fast_file_handles, 0, sizeof(struct fast_file) * MAX_FAST_HANDLES);
}

void *get_dsfile_handle(int type)
{
	switch (type)
	{
		case HANDLE_REAL:
		{
			int count;
			for (count = 0; count < MAX_FILE_HANDLES; count++)
				if (regular_file_handles[count].file == NULL)
					return &regular_file_handles[count];

			printf("error: no free (real) handles\n");
			return NULL;
		}
		case HANDLE_FAST:
		{
			int count;
			for (count = 0; count < MAX_FAST_HANDLES; count++)
				if (fast_file_handles[count].file == NULL)
					return &fast_file_handles[count];

			printf("error: no free (fast) handles\n");
			return NULL;
		}
		default:
			printf("error: asked for file handle type \'%d\'\n", type);
			break;
	}
	
	return NULL;
}

void return_dsfile_handle(void *handle)
{
	int count;
	for (count = 0; count < MAX_FILE_HANDLES; count++)
		if ((unsigned int)handle == (unsigned int)&regular_file_handles[count])
		{
			regular_file_handles[count].file = NULL;
			regular_file_handles[count].filename_hash = 0;
			regular_file_handles[count].length = 0;
			regular_file_handles[count].current_seek_pos = 0;
			regular_file_handles[count].reference_count = 0;
			return;
		}
	
	for (count = 0; count < MAX_FAST_HANDLES; count++)
		if ((unsigned int)handle == (unsigned int)&fast_file_handles[count])
		{
			fast_file_handles[count].file = NULL;
			fast_file_handles[count].seek_pos = 0;
			return;
		}
	
	printf("error: handle corresponding to %08x not found\n", (unsigned int)handle);
}

unsigned int hash_filename(char *filename)
{
	unsigned int total = 0;
	int count;
	for (count = 0; count < strlen(filename); count++)
		total += filename[count];
	
	return total;
}

struct ds_file *_ds_fopen(char *filename, char *mode)
{
//	printf("_ds_fopen for %s...", filename);
	
	disk_mode();
	
	struct ds_file *handle = NULL;
	FILE *fp = fopen(filename, mode);
	
	if (fp)
	{
		handle = (struct ds_file *)get_dsfile_handle(HANDLE_REAL);
		
		if (handle)
		{
//			printf("ok\n");
			handle->file = fp;
			handle->reference_count = 0;
			
			fseek(fp, 0, SEEK_END);
			handle->length = ftell(fp);
			rewind(fp);
			handle->current_seek_pos = ftell(fp);				//necessary?
			
			handle->filename_hash = hash_filename(filename);
		}
//		else
//			printf("failed\n");
	}
//	else
//		printf("failed\n");
	
	ram_mode();
	
	return handle;
}

int _ds_fclose(struct ds_file *fp)
{
	int ret = 1;
	
	disk_mode();
	
	if (fp)
		ret = fclose(fp->file);
	else
		printf("error: trying to _fclose a NULL handle\n");
	
	if (fp->reference_count)
		printf("warning: closing a file with %d outstanding references\n", fp->reference_count);
	
	return_dsfile_handle(fp);
	
	ram_mode();
	
	return ret;
}

int _ds_fseek(struct ds_file *fp, long offset, int whence)
{
	disk_mode();
	
	if (fp == NULL)
		printf("error: trying to _fseek in a NULL file handle\n");
	if (fp->file == NULL)
		printf("error: trying to _fseek with a valid handle but invalid file pointer\n");
	
	int ret = fseek(fp->file, offset, whence);
	
	if (whence == SEEK_SET)
		fp->current_seek_pos = offset;
	else
		fp->current_seek_pos = ftell(fp->file);
	
	ram_mode();
	
	return ret;
}

int _ds_fread(void *buf, int size, int count, struct ds_file *fp, unsigned int *bytes_advanced)
{
	if (fp == NULL)
		printf("error: trying to _fread in a NULL file handle\n");
	if (fp->file == NULL)
		printf("error: trying to _fread with a valid handle but invalid file pointer\n");
	
	int to_read = count * size;
	unsigned char real_buf[4096];
	int read = 0;
	
//	int start_hblanks = hblanks;
	
	while(to_read)
	{
		int min_move = to_read < 4096 ? to_read : 4096;
		
		disk_mode();
		int this_read = fread(real_buf, 1, min_move, fp->file);
		
		if (this_read == 0)
		{
			ram_mode();
			
			to_read = 0;
			fp->current_seek_pos = _ds_ftell(fp);
		}
		else
		{
			ram_mode();
			
			ds_memcpy(buf, real_buf, this_read);
			buf = (void *)((unsigned int)buf + this_read);
			to_read -= this_read;
			read += this_read;
		}
	}
	
//	fp->current_seek_pos = _ds_ftell(fp);
	
//	int total_hblanks = hblanks - start_hblanks;
//	printf("took %d hblanks to move %d bytes (read)\n", total_hblanks, count * size);
	
	if (bytes_advanced)
		*bytes_advanced = read > 0 ? read : 0;
	
	if (read == 0)
		return 0;
	else
	{
		fp->current_seek_pos += read;
		return read / size;
	}
	
//	return fread (buf, size, count, fp);
}

int _ds_fwrite(void *buf, int size, int count, struct ds_file *fp, unsigned int *bytes_advanced)
{
	if (fp == NULL)
		printf("error: trying to _fwrite in a NULL file handle\n");
	if (fp->file == NULL)
		printf("error: trying to _fwrite with a valid handle but invalid file pointer\n");
	if ((buf >= get_memory_base()) &&
		((unsigned int)buf < (unsigned int)get_memory_base() + (32 << 20)))		//bit of a hack
		printf("error: trying to _fwrite with a buffer that\'s in EXRAM (%08x)\n", buf);
	
	disk_mode();
	
	int ret = fwrite(buf, size, count, fp->file);
	
	ram_mode();
	
	if (bytes_advanced)
		*bytes_advanced = (ret * size);
	
//	fp->current_seek_pos = ftell(fp->file);
	fp->current_seek_pos += (ret * size);
	return ret;
}

long _ds_ftell(struct ds_file *fp)
{
	disk_mode();
	
	if (fp == NULL)
		printf("error: trying to _ftell in a NULL file handle\n");
	if (fp->file == NULL)
		printf("error: trying to _ftell with a valid handle but invalid file pointer\n");
		
	long result =  ftell(fp->file);
	
	ram_mode();
	
	return result;
}

FILE *ds_fopen(char *filename, char *mode)				//we couldn't have an fopen that just differed on return type
{
	return (FILE *)ds_fopenf(filename, mode);
}

struct fast_file *ds_fopenf(char *filename, char *mode)
{
	struct ds_file *fp = NULL;
	struct fast_file *handle = NULL;
	
//	printf("ds_fopen for %s\n", filename);
	
	unsigned int filename_hash = hash_filename(filename);
	int count;
	for (count = 0; count < MAX_FILE_HANDLES; count++)
		if (filename_hash == regular_file_handles[count].filename_hash)
		{
			fp = &regular_file_handles[count];
			
//			printf("found open handle\n");
			
			if (fp->file == NULL)
				printf("error: found an open file match but the file pointer seems to be NULL\n");
			
			break;
		}
	
	if (fp == NULL)
		fp = _ds_fopen(filename, mode);
	
	if (fp)
	{
		fp->reference_count++;
		
		handle = (struct fast_file *)get_dsfile_handle(HANDLE_FAST);
		
		if (handle)
		{
			handle->file = fp;
			handle->seek_pos = 0;
		}
	}
	
	return handle;
}

int ds_remove(char *filename)
{
	if (filename)
		return _ds_remove(filename);
	else
		printf("trying to delete a null filename...\n");
	
	return -1;
}

int _ds_remove(char *filename)
{
	disk_mode();
	
	unsigned int filename_hash = hash_filename(filename);
	int count;
	for (count = 0; count < MAX_FILE_HANDLES; count++)
		if (filename_hash == regular_file_handles[count].filename_hash)
			if (regular_file_handles[count].reference_count)
			{
				printf("trying to delete file \"%s\" which has outstanding references\n");
				
				ram_mode();
				return -1;
			}
	
	int result = remove(filename);
	
	ram_mode();
	
	return result;
}

int ds_fclose(FILE *fp)
{
	return ds_fclose((struct fast_file *)fp);
}

int ds_fclose(struct fast_file *fp)
{
	int ret = 1;
	
	if (fp == NULL)
		printf("error: tried to fclose a NULL handle\n");
	else if (fp->file == NULL)
		printf("error: tried to fclose a valid handle with a NULL file pointer\n");
		
	fp->file->reference_count--;
	
	if (fp->file->reference_count == 0)
		ret = _ds_fclose(fp->file);
	
	return_dsfile_handle(fp);
	
	return ret;
}

int ds_fseek(FILE *fp, long offset, int whence)
{
	return ds_fseek((struct fast_file *)fp, offset, whence);
}

int ds_fseek(struct fast_file *fp, long offset, int whence)
{
	switch (whence)
	{
		case SEEK_SET:
			fp->seek_pos = offset;
			break;
		case SEEK_CUR:
			fp->seek_pos += offset;
			break;
		case SEEK_END:
			fp->seek_pos = fp->file->length + offset;
			break;
		default:
			printf("error: unknown seek type %d\n", whence);
			return 1;
	}
	return 0;
}

int ds_fread(void *buf, int size, int count, FILE *fp)
{
	return ds_fread(buf, size, count, (struct fast_file *)fp);
}

int ds_fread(void *buf, int size, int count, struct fast_file *fp)
{
	int ret = 0;
	
	if (fp == NULL)
		printf("tried to fread a NULL handle\n");
	else if (fp && (fp->file == NULL))
		printf("tried to fread a valid handle with a NULL file pointer\n");
	
	//printf("ds_fread: %d\n",size*count);
	
	if (fp->seek_pos != fp->file->current_seek_pos)
	{
		if (_ds_fseek(fp->file, fp->seek_pos, SEEK_SET) != 0)
			printf("failed seek in fread\n");
//		else
//			printf("seek in fread ok\n");
	}
//	else
//		printf("seek not needed in fread\n");
	
	unsigned int update_by;
	ret = _ds_fread(buf, size, count, fp->file, &update_by);
	
	unsigned int computed_end = fp->seek_pos + update_by;
	fp->seek_pos = computed_end;
	/*fp->seek_pos = _ds_ftell(fp->file);
	
	if (fp->seek_pos != computed_end)
	{
		printf("ftell is %d, com %d\n", fp->seek_pos, computed_end);
		while(1);
	}*/
	
	return ret;
}

int ds_fwrite(void *buf, int size, int count, FILE *fp)
{
	return ds_fwrite(buf, size, count, (struct fast_file *)fp);
}

int ds_fwrite(void *buf, int size, int count, struct fast_file *fp)
{
	int ret = 0;
	
	if (fp == NULL)
		printf("tried to fwrite a NULL handle\n");
	else if (fp && (fp->file == NULL))
		printf("tried to fwrite a valid handle with a NULL file pointer\n");
	
	if (fp->seek_pos != fp->file->current_seek_pos)
	{
		if (_ds_fseek(fp->file, fp->seek_pos, SEEK_SET) != 0)
			printf("failed seek in fwrite\n");
		else
			printf("seek in fwrite ok\n");
	}
//	else
//		printf("seek not needed in fwrite\n");
		
	unsigned int update_by;
	ret = _ds_fwrite(buf, size, count, fp->file, &update_by);
//	fp->seek_pos = _ds_ftell(fp->file);
	unsigned int computed_end = fp->seek_pos + update_by;
	fp->seek_pos = computed_end;
	
	return ret;
}

long ds_ftell(FILE *fp)
{
	return ds_ftell((struct fast_file *)fp);
}

long ds_ftell(struct fast_file *fp)
{
	if (fp == NULL)
		printf("tried to fclose a NULL handle\n");
	else if (fp && (fp->file == NULL))
		printf("tried to fclose a valid handle with a NULL file pointer\n");
	
	return fp->seek_pos;
}
