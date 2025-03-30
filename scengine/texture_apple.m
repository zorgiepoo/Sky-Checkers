/*
 MIT License

 Copyright (c) 2024 Mayur Pawashe

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
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
