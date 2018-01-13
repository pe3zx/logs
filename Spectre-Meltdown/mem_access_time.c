#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <x86intrin.h>

static inline void __attribute__((always_inline)) maccess(void *ptr)
{
	asm volatile("movb (%0), %%al\n" : : "r"(ptr) : "rax");
}

//#define HAVE_RDTSCP 1
static uint64_t __attribute__((noinline)) measure_atime(void *addr)
{
	register uint64_t time1, time2;

#if HAVE_RDTSCP
	int junk;
	time1 = __rdtscp(&junk);
	maccess(addr);
	time2 = __rdtscp(&junk);
#else
	_mm_mfence();
	time1 = __rdtsc();
	maccess(addr);
	_mm_mfence();
	time2 = __rdtsc();
#endif

	return time2 - time1;
}

int main()
{
	int i;
	uint64_t t;
	int *val = malloc(sizeof(int));
	
	*val = 10;
	_mm_clflush(val);
	
	// load from memory
	t = measure_atime(val);
	printf("load from memory: %lu\n", t);
	
	// load from cache
	for (i = 0; i < 4; i++) {
		t = measure_atime(val);
		printf("load from cache: %lu\n", t);
	}

	// load from memory
	for (i = 0; i < 4; i++) {
		_mm_clflush(val);
		t = measure_atime(val);
		printf("load from memory: %lu\n", t);
	}
	
	// load from cache
	for (i = 0; i < 4; i++) {
		t = measure_atime(val);
		printf("load from cache: %lu\n", t);
	}

	return 0;
}
