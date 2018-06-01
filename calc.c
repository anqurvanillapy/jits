/**
 *  A JIT calculator w/o operator precedence.  Check out this blog:
 *  http://nullprogram.com/blog/2015/03/19/.
 *
 *  Usage:
 *
 *  $ make; ./calc
 *  > 42 - 2 * 2
 *  80
 */

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <assert.h>

#define PAGESIZ 4096

typedef long exprfn_t(long);

typedef struct codeseg_t {
	uint8_t code[PAGESIZ - sizeof(uint64_t)];
	uint64_t count;
} codeseg_t;

void
codeseg_ins(codeseg_t* buf, int size, uint64_t ins)
{
	int i;
	for (i = size - 1; i >= 0; --i) {
		buf->code[buf->count++] = (ins >> (i << 3)) & 0xff;
	}
}

void
codeseg_imm(codeseg_t* buf, int size, const void* v)
{
	memcpy(buf->code + buf->count, v, size);
	buf->count += size;
}

int
main()
{
	int rc;

	int prot = PROT_READ | PROT_WRITE;
	int flags = MAP_ANONYMOUS | MAP_PRIVATE;
	codeseg_t* buf = (codeseg_t*)mmap(NULL, PAGESIZ, prot, flags, -1, 0);
	assert(buf);

	printf("> ");
	long initval;
	scanf("%ld", &initval);

	codeseg_ins(buf, 3, 0x4889f8); // mov %rdi, %rax
	int c;
	while ( (c = fgetc(stdin)) != '\n' && c != EOF) {
		if (c == ' ') {
			continue;
		}

		char opt = c;
		long opd;
		scanf("%ld", &opd);
		codeseg_ins(buf, 2, 0x48bf); // movq opd, %rdi
		codeseg_imm(buf, 8, &opd);
		switch (opt) {
		case '+':
			codeseg_ins(buf, 3, 0x4801f8); // add %rdi, %rax
			break;
		case '-':
			codeseg_ins(buf, 3, 0x4829f8); // sub %rdi, %rax
			break;
		case '*':
			codeseg_ins(buf, 4, 0x480fafc7); // imul %rdi, %rax
			break;
		case '/':
			codeseg_ins(buf, 3, 0x4831d2); // xor %rdx, %rdx
			codeseg_ins(buf, 3, 0x48f7ff); // idiv %rdi
			break;
		}
	}
	codeseg_ins(buf, 1, 0xc3); // retq
	rc = mprotect(buf, PAGESIZ, PROT_READ | PROT_EXEC);
	assert(rc >= 0);

	exprfn_t* fn = (exprfn_t*)buf->code;
	printf("%lu\n", fn(initval));

	rc = munmap(buf, PAGESIZ);
	assert(rc >= 0);

	return 0;
}
