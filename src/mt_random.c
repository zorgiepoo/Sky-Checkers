
#include "mt_random.h"
#include <stdlib.h>
#include <time.h>

#define MT_LEN			624

int mt_index;
unsigned long mt_buffer[MT_LEN];

void mt_init(void) {
    srand((unsigned int)time(NULL));
	int i;
    for (i = 0; i < MT_LEN; i++)
        mt_buffer[i] = rand();
    mt_index = 0;
}

#define MT_IA           397
#define MT_IB           (MT_LEN - MT_IA)
#define UPPER_MASK      0x80000000
#define LOWER_MASK      0x7FFFFFFF
#define MATRIX_A        0x9908B0DF
#define TWIST(b,i,j)    ((b)[i] & UPPER_MASK) | ((b)[j] & LOWER_MASK)
#define MAGIC(s)        (((s)&1)*MATRIX_A)

unsigned long mt_random(void) {
    unsigned long * b = mt_buffer;
    int idx = mt_index;
    unsigned long s;
    int i;
	
    if (idx == MT_LEN*sizeof(unsigned long))
    {
        idx = 0;
        i = 0;
        for (; i < MT_IB; i++) {
            s = TWIST(b, i, i+1);
            b[i] = b[i + MT_IA] ^ (s >> 1) ^ MAGIC(s);
        }
        for (; i < MT_LEN-1; i++) {
            s = TWIST(b, i, i+1);
            b[i] = b[i - MT_IB] ^ (s >> 1) ^ MAGIC(s);
        }
        
        s = TWIST(b, MT_LEN-1, 0);
        b[MT_LEN-1] = b[MT_IA-1] ^ (s >> 1) ^ MAGIC(s);
    }
    mt_index = idx + sizeof(unsigned long);
    return *(unsigned long *)((unsigned char *)b + idx);
    /*
	 Matsumoto and Nishimura additionally confound the bits returned to the caller
	 but this doesn't increase the randomness, and slows down the generator by
	 as much as 25%.  So I omit these operations here.
	 
	 r ^= (r >> 11);
	 r ^= (r << 7) & 0x9D2C5680;
	 r ^= (r << 15) & 0xEFC60000;
	 r ^= (r >> 18);
	 */
}
