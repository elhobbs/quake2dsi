#ifndef MEMORY_H_
#define MEMORY_H_

void enable_memory(int type = 0);
void disable_memory(void);
void *get_memory_base(void);
void *get_memory_base_end(void);
unsigned int get_memory_size(void);
unsigned int find_memory_size(void);
bool is_exram(void *);

void ds_malloc_init(void *base, unsigned int size);
void *ds_malloc(size_t size);
void ds_free(void *ptr);

void ds_set_exram_timings(unsigned int first, unsigned int second);

#endif /*MEMORY_H_*/
