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

/*
 * Using the Mersenne Twister Random number generator
 * http://www.qbrundage.com/michaelb/pubs/essays/random_number_generation
 * This code is licensed as "Public Domain" (mt_init(), mt_random())
 */
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

static void *create8BitPixelDataWithAlpha(SDL_Surface *surface)
{
	uint8_t bytesPerPixel = surface->format->BytesPerPixel;
	const uint8_t newBytesPerPixel = 4;
	
	uint8_t *pixelData = calloc(surface->h, newBytesPerPixel * sizeof(uint8_t) * surface->w);
	if (pixelData == NULL)
	{
		fprintf(stderr, "Error: failed to allocate pixel data\n");
		abort();
	}
	
	uint8_t *surfacePixelData = surface->pixels;
	for (uint32_t pixelIndex = 0; pixelIndex < (uint32_t)(surface->w * surface->h); pixelIndex++)
	{
		memcpy(pixelData + newBytesPerPixel * pixelIndex, surfacePixelData + bytesPerPixel * pixelIndex, bytesPerPixel);
		pixelData[newBytesPerPixel * pixelIndex + (newBytesPerPixel - 1)] = 0xFF;
	}
	
	return pixelData;
}

TextureObject surfaceToTexture(Renderer *renderer, SDL_Surface *surface)
{
	void *pixelData = NULL;
	SDL_bool sourceMissingAlpha = (surface->format->BytesPerPixel == 3);
	if (sourceMissingAlpha)
	{
		pixelData = create8BitPixelDataWithAlpha(surface);
	}
	else
	{
		pixelData = surface->pixels;
	}
	
	// Create texture from texture image data
	TextureObject texture = textureFromPixelData(renderer, pixelData, surface->w, surface->h);
	
	if (sourceMissingAlpha)
	{
		free(pixelData);
	}
	
	return texture;
}

TextureObject loadTexture(Renderer *renderer, const char *filePath)
{
	SDL_Surface *texImage = SDL_LoadBMP(filePath);
	if (texImage == NULL)
	{
		fprintf(stderr, "Couldn't load texture: %s\n", filePath);
		SDL_Quit();
	}
	
	TextureObject texture = surfaceToTexture(renderer, texImage);
	
	SDL_FreeSurface(texImage);
	
	return texture;
}
