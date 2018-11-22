/*
 * Copyright 2010 Mayur Pawashe
 * https://zgcoder.net
 
 * This file is part of skycheckers.
 * skycheckers is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 
 * skycheckers is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with skycheckers.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "utilities.h"
#include <stdarg.h>
#include "renderer.h"

/*
 * Using the Mersenne Twister Random number generator
 * http://www.qbrundage.com/michaelb/pubs/essays/random_number_generation
 * This code is licensed as "Public Domain" (mt_init(), mt_random())
 */
#define MT_LEN			624

int mt_index;
unsigned long mt_buffer[MT_LEN];

void mt_init() {
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

unsigned long mt_random() {
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

/*
 * This function does what printf does (however limited), except that it prints
 * to stderr and appends a '\n' to the string for us. This function was created because
 * of lazyness. It can also print out SDL error messages with just typing %e (no arguements needed).
 *
 * example of what zgPrint() supports: zgPrint("hi %% %c, %i : %c, %f, %s\n, zor! %i", 'g', 453, 'c', 5.2, "foo", 452);
 *
 * And..
 * example of SDL Error message: zgPrint("Something went wrong! ERROR MESSAGE: %e");
 */
void zgPrint(const char *format, ...)
{
	va_list ap;
	char buffer[256];
	int formatIndex;
	int bufferIndex = 0;
	int stringIndex;
	const char *string;
	char newLine = 1;
	
	va_start(ap, format);
	
	for (formatIndex = 0; format[formatIndex] != '\0'; formatIndex++)
	{
		if (format[formatIndex] != '%')
		{
			buffer[bufferIndex] = format[formatIndex];
			bufferIndex++;
			continue;
		}
		
		formatIndex++;
		
		switch (format[formatIndex])
		{
			case 'c':
				buffer[bufferIndex] = va_arg(ap, int);
				break;
			case 'd':
				bufferIndex += sprintf(&buffer[bufferIndex], "%i", va_arg(ap, int)) - 1;
				break;
			case 'i':
				bufferIndex += sprintf(&buffer[bufferIndex], "%i", va_arg(ap, int)) - 1;
				break;
			case '%':
				buffer[bufferIndex] = '%';
				break;
			case 'f':
				bufferIndex += sprintf(&buffer[bufferIndex], "%f", va_arg(ap, double)) - 1;
				break;
			case 's':
				string = va_arg(ap, char *);
				
				for (stringIndex = 0; string[stringIndex] != '\0'; stringIndex++)
				{
					buffer[bufferIndex++] = string[stringIndex];
				}
				
				bufferIndex--;
				break;
			case 'e':
				string = SDL_GetError();
				
				for (stringIndex = 0; string[stringIndex] != '\0'; stringIndex++)
				{
					buffer[bufferIndex++] = string[stringIndex];
				}
				
				bufferIndex--;
				break;
			case 't':
				string = TTF_GetError();
				
				for (stringIndex = 0; string[stringIndex] != '\0'; stringIndex++)
				{
					buffer[bufferIndex++] = string[stringIndex];
				}
					
				bufferIndex--;
				break;
		}
		
		bufferIndex++;
	}
	
	va_end(ap);
	
	// if there's a ^ at the end of the string, then we won't put a newline character at the end of the output.
	if (buffer[bufferIndex - 1] == '^')
	{
		buffer[bufferIndex - 1] = '\0';
		newLine = 0;
	}
	else
	{
		buffer[bufferIndex] = '\0';
	}
	
	fprintf(stderr, "%s", buffer);
	
	if (newLine == 1)
		fprintf(stderr, "\n");
}
