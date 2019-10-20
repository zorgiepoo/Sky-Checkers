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

#include "font.h"

#import <CoreText/CoreText.h>
#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

static CTFontRef gFontRef;
static CGColorSpaceRef gRGBColorSpace;

void initFont(void)
{
	NSURL *fileURL = [NSURL fileURLWithPath:@FONT_PATH];
	NSArray *fontDescriptors = (NSArray *)CFBridgingRelease(CTFontManagerCreateFontDescriptorsFromURL((CFURLRef)fileURL));
	assert(fontDescriptors.count > 0);
	
	gFontRef = CTFontCreateWithFontDescriptor((CTFontDescriptorRef)fontDescriptors[0], (CGFloat)FONT_POINT_SIZE, NULL);
	
	gRGBColorSpace = CGColorSpaceCreateDeviceRGB();
}

// Info on how to grab raw pixel data from drawn text using CoreText - https://stackoverflow.com/a/41798782/871119
TextData createTextData(const char *string)
{
	NSMutableAttributedString *text = [[NSMutableAttributedString alloc] initWithString:@(string)];
	NSRange textRange = NSMakeRange(0, text.length);

	[text beginEditing];
	[text addAttribute:(NSString *)kCTFontAttributeName value:(__bridge id)gFontRef range:textRange];
	[text addAttribute:(NSString *)kCTForegroundColorFromContextAttributeName value:@YES range:textRange];
	[text endEditing];

	CTFramesetterRef framesetter = CTFramesetterCreateWithAttributedString((CFAttributedStringRef)text);

	CGSize constraints = CGSizeMake(CGFLOAT_MAX , CGFLOAT_MAX);
	CFRange cfTextRange = CFRangeMake(textRange.location, textRange.length);
	CGSize frameSize = CTFramesetterSuggestFrameSizeWithConstraints(framesetter, cfTextRange, NULL, constraints, NULL);

	CGPathRef path = CGPathCreateWithRect(CGRectMake(0.0, 0.0, frameSize.width, frameSize.height), NULL);
	CTFrameRef frame = CTFramesetterCreateFrame(framesetter, cfTextRange, path, NULL);
	
	CGBitmapInfo bitmapInfo = (CGBitmapInfo)kCGImageAlphaPremultipliedLast;

	CGContextRef context = CGBitmapContextCreate(NULL, (size_t)ceil(frameSize.width), (size_t)ceil(frameSize.height), 8, (size_t)ceil(frameSize.width) * 4, gRGBColorSpace, bitmapInfo);

	CGContextSetRGBFillColor(context, 1.0, 1.0, 1.0, 1.0);
	CTFrameDraw(frame, context);

	return (TextData)context;
}

void releaseTextData(TextData textData)
{
	CGContextRelease((CGContextRef)textData);
}

void *getTextDataPixels(TextData textData)
{
	return CGBitmapContextGetData((CGContextRef)textData);
}

int32_t getTextDataWidth(TextData textData)
{
	return (int32_t)CGBitmapContextGetWidth((CGContextRef)textData);
}

int32_t getTextDataHeight(TextData textData)
{
	return (int32_t)CGBitmapContextGetHeight((CGContextRef)textData);
}

PixelFormat getPixelFormat(TextData textData)
{
	return PIXEL_FORMAT_RGBA32;
}
