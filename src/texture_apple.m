/*
* Copyright 2019 Mayur Pawashe
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

#import <ImageIO/ImageIO.h>
#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

#import "texture.h"
#import "quit.h"

TextureData loadTextureData(const char *filePath)
{
	NSURL *fileURL = [NSURL fileURLWithPath:@(filePath)];
	NSDictionary *options = @{};
	
	CGImageSourceRef imageSource = CGImageSourceCreateWithURL((CFURLRef)fileURL, (CFDictionaryRef)options);
	
	if (imageSource == NULL)
	{
		fprintf(stderr, "Couldn't create image source for texture: %s\n", filePath);
		ZGQuit();
	}
	
	CGImageRef image = CGImageSourceCreateImageAtIndex(imageSource, 0, NULL);
	
	CFRelease(imageSource);
	
	if (image == NULL)
	{
		fprintf(stderr, "Couldn't create image for texture: %s\n", filePath);
		ZGQuit();
	}
	
	size_t width = CGImageGetWidth(image);
	size_t height = CGImageGetHeight(image);
	size_t bytesPerRow = CGImageGetBytesPerRow(image);
	size_t bytesPerPixel = bytesPerRow / width;
	assert(bytesPerPixel == 4);
	
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
	uint8_t *pixelData = calloc(height * width * bytesPerPixel, sizeof(uint8_t));
	assert(pixelData != NULL);
	
	CGBitmapInfo bitmapInfo = (CGBitmapInfo)kCGImageAlphaPremultipliedLast;
	size_t bitsPerComponent = 8;
	
	CGContextRef context = CGBitmapContextCreate(pixelData, width, height, bitsPerComponent, bytesPerRow, colorSpace, bitmapInfo);
	assert(context != NULL);
	
	CGColorSpaceRelease(colorSpace);
	
	CGContextSetRGBFillColor(context, 1.0, 1.0, 1.0, 1.0);
	CGContextDrawImage(context, CGRectMake(0.0, 0.0, (CGFloat)width, (CGFloat)height), image);
	CGContextRelease(context);
	CFRelease(image);
	
	TextureData textureData = {.pixelData = pixelData, .width = (int32_t)width, .height = (int32_t)height, .pixelFormat = PIXEL_FORMAT_RGBA32};
	
	return textureData;
}

void freeTextureData(TextureData textureData)
{
	if (textureData.context != NULL)
	{
		CGContextRelease(textureData.context);
	}
	else
	{
		free(textureData.pixelData);
	}
}
