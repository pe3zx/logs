nclude <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <x86intrin.h>

typedef uint64_t (*measure_fn)(uint64_t t1);

struct TTest {
	measure_fn fn;
};

typedef uint64_t (*test_fn)(struct TTest *mm);

#define MAP_CODE_ADDR1	0x198765432
#define MAP_CODE_ADDR2	0x298765432

static uint64_t __attribute__((noinline)) measure1(uint64_t t1)
{
	uint64_t t2;
	t2 = __rdtsc();
	return t2;
}

static uint64_t __attribute__((noinline)) measure2(uint64_t t1)
{
	uint64_t t2;
	t2 = __rdtsc();
	return t2;
}

uint64_t __attribute__((noinline)) test_btb(struct TTest *mm)
{
	uint64_t t1, t2;
	
	_mm_clflush(mm);
	_mm_mfence();
	
	t1 = __rdtsc();
	t2 = mm->fn(t1);
	
	return t2 - t1;
}

int main(int argc, char **argv)
{
	int i;
	struct TTest *mm;
	test_fn tfn1;
	test_fn tfn2;
	measure_fn tfn1_m1;
	measure_fn tfn1_m2;
	measure_fn tfn2_m1;
	measure_fn tfn2_m2;
	
	//int s1 = strtol(argv[1], 0, 0);
	//int s2 = strtol(argv[2], 0, 0);

	mm = (struct TTest*) malloc(sizeof(struct TTest));

	char *codeMap1 = mmap((void*) (MAP_CODE_ADDR1&(~0xfffL)), 4096, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE|MAP_FIXED, -1, 0);
	tfn1 = (test_fn) MAP_CODE_ADDR1;
	tfn1_m1 = (measure_fn) (MAP_CODE_ADDR1 + 0x100);
	tfn1_m2 = (measure_fn) (MAP_CODE_ADDR1 + 0x100 + 0x20);
	memcpy(tfn1, test_btb, 0x100);
	memcpy(tfn1_m1, measure1, 0x20);
	memcpy(tfn1_m2, measure2, 0x20);
	
	char *codeMap2 = mmap((void*) (MAP_CODE_ADDR2&(~0xfffL)), 4096, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE|MAP_FIXED, -1, 0);
	tfn2 = (test_fn) MAP_CODE_ADDR2;
	tfn2_m1 = (measure_fn) (MAP_CODE_ADDR2 + 0x100);
	tfn2_m2 = (measure_fn) (MAP_CODE_ADDR2 + 0x100 + 0x20);
	memcpy(tfn2, test_btb, 0x100);
	memcpy(tfn2_m1, measure1, 0x20);
	memcpy(tfn2_m2, measure2, 0x20);

	printf("fill branch target history by\n");
	printf(" calling pointer to function from fn addr: %p to %p\n", tfn1, tfn1_m1);
	mm->fn = tfn1_m1;
	for (i = 0; i < 30; i++) {
		uint64_t tt = tfn1(mm);
		if (i == 0 || i == 29)
			printf("indirect call 1st loop time: %lu\n", tt);
	}
	printf("\n");
	
	printf("branch target will be predicted incorrectly on a first time when\n");
	printf(" calling pointer to function from fn addr: %p to %p\n", tfn2, tfn2_m2);
	mm->fn = tfn2_m2;
	for (i = 0; i < 30; i++) {
		uint64_t tt = tfn2(mm);
		if (i == 0 || i == 29)
			printf("indirect call 2nd loop time: %lu\n", tt);
	}
	printf("\n");

	printf("fill branch target history (first time is wrong again) by\n");
	printf(" calling pointer to function from fn addr: %p to %p\n", tfn1, tfn1_m1);
	mm->fn = tfn1_m1;
	for (i = 0; i < 30; i++) {
		uint64_t tt = tfn1(mm);
		if (i == 0 || i == 29)
			printf("indirect call 1st loop time: %lu\n", tt);
	}
	printf("\n");
	
	printf("branch target will be predicted correctly since a first time when\n");
	printf(" calling pointer to function from fn addr: %p to %p\n", tfn2, tfn2_m1);
	mm->fn = tfn2_m1;
	for (i = 0; i < 30; i++) {
		uint64_t tt = tfn2(mm);
		if (i == 0 || i == 29)
			printf("indirect call 2nd loop time: %lu\n", tt);
	}
	printf("\n");
	
	return 0;
}

