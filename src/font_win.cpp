/*
 * Copyright 2020 Mayur Pawashe
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
#include <stdio.h>
#include <dwrite_3.h>

static IDWriteFontFace3* gFontFace;
static IDWriteFactory3* gWriteFactory;
static RECT gMaxBoundingRect;

const DWRITE_RENDERING_MODE1 RENDERING_MODE = DWRITE_RENDERING_MODE1_NATURAL;
const DWRITE_MEASURING_MODE MEASURING_MODE = DWRITE_MEASURING_MODE_NATURAL;
const DWRITE_GRID_FIT_MODE GRID_FIT_MODE = DWRITE_GRID_FIT_MODE_ENABLED;
const DWRITE_TEXT_ANTIALIAS_MODE ANTIALIAS_MODE = DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE;
const DWRITE_TEXTURE_TYPE TEXTURE_TYPE = DWRITE_TEXTURE_ALIASED_1x1;

extern "C" void initFont(void)
{
    IDWriteFactory3* writeFactory = nullptr;
    HRESULT factoryResult = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory3), reinterpret_cast<IUnknown**>(&writeFactory));
    if (FAILED(factoryResult))
    {
        fprintf(stderr, "Error: failed to create DWrite factory: %d\n", factoryResult);
        abort();
    }

    IDWriteFontFaceReference* fontFaceReference = nullptr;
    const WCHAR* filePath = L"Data\\Fonts\\typelib.dat";
    HRESULT fontFaceReferenceResult = writeFactory->CreateFontFaceReference(filePath, nullptr, 0, DWRITE_FONT_SIMULATIONS_NONE, &fontFaceReference);
    if (FAILED(fontFaceReferenceResult))
    {
        fprintf(stderr, "Error: failed to create font face reference: %d\n", fontFaceReferenceResult);
        abort();
    }

    IDWriteFontFace3* fontFace = nullptr;
    HRESULT fontFaceResult = fontFaceReference->CreateFontFace(&fontFace);
    if (FAILED(fontFaceResult))
    {
        fprintf(stderr, "Error: failed to create font face: %d\n", fontFaceResult);
        abort();
    }

    UINT16 glyphCount = fontFace->GetGlyphCount();
    for (UINT16 glyphIndex = 0; glyphIndex < glyphCount; glyphIndex++)
    {
        IDWriteGlyphRunAnalysis* glyphRunAnalysis = nullptr;

        DWRITE_GLYPH_RUN glyphRun;
        glyphRun.fontFace = fontFace;
        glyphRun.fontEmSize = (float)FONT_POINT_SIZE;

        glyphRun.glyphCount = 1;
        glyphRun.glyphIndices = &glyphIndex;
        glyphRun.glyphAdvances = nullptr;
        glyphRun.glyphOffsets = nullptr;
        glyphRun.isSideways = false;

        HRESULT glyphRunAnalysisResult = writeFactory->CreateGlyphRunAnalysis(&glyphRun, nullptr, RENDERING_MODE, MEASURING_MODE, GRID_FIT_MODE, ANTIALIAS_MODE, 0.0f, 0.0f, &glyphRunAnalysis);
        if (FAILED(glyphRunAnalysisResult))
        {
            fprintf(stderr, "Error: failed to create glyph run analysis: %d\n", glyphRunAnalysisResult);
            continue;
        }
        
        RECT textureBounds;
        HRESULT textureBoundsResult = glyphRunAnalysis->GetAlphaTextureBounds(TEXTURE_TYPE, &textureBounds);
        if (FAILED(textureBoundsResult))
        {
            fprintf(stderr, "Error: failed to get texture bounds: %d\n", textureBoundsResult);
            continue;
        }
        
        if (textureBounds.bottom > gMaxBoundingRect.bottom)
        {
            gMaxBoundingRect.bottom = textureBounds.bottom;
        }

        if (textureBounds.top < gMaxBoundingRect.top)
        {
            gMaxBoundingRect.top = textureBounds.top;
        }
    }

    gFontFace = fontFace;
    gWriteFactory = writeFactory;
}

extern "C" TextureData createTextData(const char* string)
{
    size_t length = strlen(string);
    UINT32* codePoints = (UINT32 *)calloc(length, sizeof(*codePoints));
    for (size_t stringIndex = 0; stringIndex < length; stringIndex++)
    {
        codePoints[stringIndex] = (UINT32)string[stringIndex];
    }
    
    UINT16* glyphIndices = (UINT16 *)calloc(length, sizeof(*glyphIndices));

    HRESULT glyphIndicesResult = gFontFace->GetGlyphIndicesA(codePoints, (UINT32)length, glyphIndices);
    if (FAILED(glyphIndicesResult))
    {
        fprintf(stderr, "Error: failed to get glyph indices result: %d\n", glyphIndicesResult);
        abort();
    }

    free(codePoints);

    IDWriteGlyphRunAnalysis* glyphRunAnalysis = nullptr;

    DWRITE_GLYPH_RUN glyphRun;
    glyphRun.fontFace = gFontFace;
    glyphRun.fontEmSize = (float)FONT_POINT_SIZE;

    glyphRun.glyphCount = (UINT32)length;
    glyphRun.glyphIndices = glyphIndices;
    glyphRun.glyphAdvances = nullptr;
    glyphRun.glyphOffsets = nullptr;
    glyphRun.isSideways = false;

    HRESULT glyphRunAnalysisResult = gWriteFactory->CreateGlyphRunAnalysis(&glyphRun, nullptr, RENDERING_MODE, MEASURING_MODE, GRID_FIT_MODE, ANTIALIAS_MODE, 0.0f, 0.0f, &glyphRunAnalysis);
    if (FAILED(glyphRunAnalysisResult))
    {
        fprintf(stderr, "Error: failed to create glyph run analysis: %d\n", glyphRunAnalysisResult);
        abort();
    }
    
    free(glyphIndices);
    
    RECT initialTextureBounds;
    HRESULT textureBoundsResult = glyphRunAnalysis->GetAlphaTextureBounds(TEXTURE_TYPE, &initialTextureBounds);
    if (FAILED(textureBoundsResult))
    {
        fprintf(stderr, "Error: failed to get texture bounds: %d\n", textureBoundsResult);
        abort();
    }

    RECT textureBounds = initialTextureBounds;
    textureBounds.top = gMaxBoundingRect.top;
    textureBounds.bottom = gMaxBoundingRect.bottom;

    const size_t width = (size_t)abs(textureBounds.right - textureBounds.left);
    const size_t height = (size_t)abs(textureBounds.top - textureBounds.bottom);
    const size_t bytesPerPixel = 1;
    const size_t bufferSize = (size_t)(width * bytesPerPixel * height);
    
    BYTE* alphaBytes = (BYTE *)calloc(1, bufferSize);

    HRESULT alphaTextureResult = glyphRunAnalysis->CreateAlphaTexture(TEXTURE_TYPE, &textureBounds, alphaBytes, (UINT32)bufferSize);
    if (FAILED(alphaTextureResult))
    {
        fprintf(stderr, "Error: failed to create alpha texture: %d\n", alphaTextureResult);
        abort();
    }

    glyphRunAnalysis->Release();

    const size_t newBytesPerPixel = 4;
    const size_t newBufferSize = (size_t)(width * newBytesPerPixel * height);
    BYTE* rgbaBytes = (BYTE *)calloc(1, newBufferSize);

    for (size_t pixelIndex = 0; pixelIndex < width * height; pixelIndex++)
    {
        BYTE pixelValue = alphaBytes[pixelIndex];
        rgbaBytes[newBytesPerPixel * pixelIndex] = pixelValue;
        rgbaBytes[newBytesPerPixel * pixelIndex + 1] = pixelValue;
        rgbaBytes[newBytesPerPixel * pixelIndex + 2] = pixelValue;
        rgbaBytes[newBytesPerPixel * pixelIndex + 3] = pixelValue;
    }

    free(alphaBytes);

    TextureData textureData = { 0 };
    textureData.width = (int32_t)width;
    textureData.height = (int32_t)height;
    textureData.context = nullptr;
    textureData.pixelData = (uint8_t *)rgbaBytes;
    textureData.pixelFormat = PIXEL_FORMAT_RGBA32;

    return textureData;
}
