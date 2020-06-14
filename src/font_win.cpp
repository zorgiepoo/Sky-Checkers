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
static size_t gSpaceWidth;

const DWRITE_RENDERING_MODE1 RENDERING_MODE = DWRITE_RENDERING_MODE1_NATURAL;
const DWRITE_MEASURING_MODE MEASURING_MODE = DWRITE_MEASURING_MODE_NATURAL;
const DWRITE_GRID_FIT_MODE GRID_FIT_MODE = DWRITE_GRID_FIT_MODE_ENABLED;
const DWRITE_TEXT_ANTIALIAS_MODE ANTIALIAS_MODE = DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE;
const DWRITE_TEXTURE_TYPE TEXTURE_TYPE = DWRITE_TEXTURE_ALIASED_1x1;

#define _WIN_WIDEN(x) L ## x
#define WIN_WIDEN(x) _WIN_WIDEN(x)
#define WIN_FONT_PATH WIN_WIDEN(FONT_PATH)

typedef struct
{
    BYTE* bytes;
    size_t width;
    size_t height;
} GlyphData;

static RECT getTextureBoundsWithGlyphIndices(IDWriteFontFace3* fontFace, IDWriteFactory3* writeFactory, UINT16 *glyphIndices, UINT32 glyphCount, IDWriteGlyphRunAnalysis** outGlyphRunAnalysis)
{
    DWRITE_GLYPH_RUN glyphRun;
    glyphRun.fontFace = fontFace;
    glyphRun.fontEmSize = (float)FONT_POINT_SIZE;
    glyphRun.glyphCount = glyphCount;
    glyphRun.glyphIndices = glyphIndices;
    glyphRun.glyphAdvances = nullptr;
    glyphRun.glyphOffsets = nullptr;
    glyphRun.isSideways = false;

    IDWriteGlyphRunAnalysis* glyphRunAnalysis = nullptr;
    HRESULT glyphRunAnalysisResult = writeFactory->CreateGlyphRunAnalysis(&glyphRun, nullptr, RENDERING_MODE, MEASURING_MODE, GRID_FIT_MODE, ANTIALIAS_MODE, 0.0f, 0.0f, &glyphRunAnalysis);
    if (FAILED(glyphRunAnalysisResult))
    {
        fprintf(stderr, "Error: failed to create glyph run analysis: %d\n", glyphRunAnalysisResult);
        abort();
    }

    RECT textureBounds;
    HRESULT textureBoundsResult = glyphRunAnalysis->GetAlphaTextureBounds(TEXTURE_TYPE, &textureBounds);
    if (FAILED(textureBoundsResult))
    {
        fprintf(stderr, "Error: failed to get texture bounds: %d\n", textureBoundsResult);
        abort();
    }

    if (outGlyphRunAnalysis != nullptr)
    {
        *outGlyphRunAnalysis = glyphRunAnalysis;
    }
    else
    {
        glyphRunAnalysis->Release();
    }

    return textureBounds;
}

static RECT getTextureBounds(IDWriteFontFace3 *fontFace, IDWriteFactory3 *writeFactory, UINT32* codePoints, UINT32 codePointCount, IDWriteGlyphRunAnalysis** outGlyphRunAnalysis)
{
    UINT16* glyphIndices = (UINT16*)calloc(codePointCount, sizeof(*glyphIndices));

    HRESULT glyphIndicesResult = fontFace->GetGlyphIndicesA(codePoints, codePointCount, glyphIndices);
    if (FAILED(glyphIndicesResult))
    {
        fprintf(stderr, "Error: failed to get glyph indices result: %d\n", glyphIndicesResult);
        abort();
    }

    RECT textureBounds = getTextureBoundsWithGlyphIndices(fontFace, writeFactory, glyphIndices, codePointCount, outGlyphRunAnalysis);
    free(glyphIndices);

    return textureBounds;
}

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
    HRESULT fontFaceReferenceResult = writeFactory->CreateFontFaceReference(WIN_FONT_PATH, nullptr, 0, DWRITE_FONT_SIMULATIONS_NONE, &fontFaceReference);
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

    // Compute max bounding rect vertically
    UINT16 glyphCount = fontFace->GetGlyphCount();
    for (UINT16 glyphIndex = 0; glyphIndex < glyphCount; glyphIndex++)
    {
        RECT textureBounds = getTextureBoundsWithGlyphIndices(fontFace, writeFactory, &glyphIndex, 1, nullptr);
        
        if (textureBounds.bottom > gMaxBoundingRect.bottom)
        {
            gMaxBoundingRect.bottom = textureBounds.bottom;
        }

        if (textureBounds.top < gMaxBoundingRect.top)
        {
            gMaxBoundingRect.top = textureBounds.top;
        }
    }

    // Determine width of a space
    // DirectWrite doesn't allow us to render or get the texture bounds of a space glyph just by itself

    UINT32 spacedCodePoints[] = { 'a', ' ', 'a' };
    RECT spacedRect = getTextureBounds(fontFace, writeFactory, spacedCodePoints, sizeof(spacedCodePoints) / sizeof(*spacedCodePoints), nullptr);

    UINT32 nonSpacedCodePoints[] = { 'a', 'a' };
    RECT nonSpacedRect = getTextureBounds(fontFace, writeFactory, nonSpacedCodePoints, sizeof(nonSpacedCodePoints) / sizeof(*nonSpacedCodePoints), nullptr);

    gSpaceWidth = (size_t)abs(spacedRect.right - spacedRect.left) - (size_t)abs(nonSpacedRect.right - nonSpacedRect.left);

    gFontFace = fontFace;
    gWriteFactory = writeFactory;
}

static GlyphData createGlyphTexture(UINT32 codePoint)
{
    IDWriteGlyphRunAnalysis* glyphRunAnalysis = nullptr;
    RECT initialTextureBounds = getTextureBounds(gFontFace, gWriteFactory, &codePoint, 1, &glyphRunAnalysis);

    RECT textureBounds = initialTextureBounds;
    textureBounds.top = gMaxBoundingRect.top;
    textureBounds.bottom = gMaxBoundingRect.bottom;

    const size_t width = (size_t)abs(textureBounds.right - textureBounds.left);
    const size_t height = (size_t)abs(textureBounds.top - textureBounds.bottom);

    const size_t bytesPerPixel = 1;
    const size_t bufferSize = (size_t)(width * bytesPerPixel * height);

    BYTE* alphaBytes = (BYTE*)calloc(1, bufferSize);

    HRESULT alphaTextureResult = glyphRunAnalysis->CreateAlphaTexture(TEXTURE_TYPE, &textureBounds, alphaBytes, (UINT32)bufferSize);
    if (FAILED(alphaTextureResult))
    {
        fprintf(stderr, "Error: failed to create alpha texture: %d\n", alphaTextureResult);
        abort();
    }

    glyphRunAnalysis->Release();

    GlyphData glyphData;
    glyphData.bytes = alphaBytes;
    glyphData.width = width;
    glyphData.height = height;
    return glyphData;
}

extern "C" TextureData createTextData(const char* string)
{
    size_t length = strlen(string);

    GlyphData* glyphsData = (GlyphData*)calloc(length, sizeof(*glyphsData));

    // Get the texture of every individual glyph
    // We don't pass all the code points at once to CreateGlyphRunAnalysis() because there appears to be a bug where the order
    // the glyphs are outputted in are non-deterministic. Instead, we get individual glyphs and stitch them together in one final texture buffer.
    size_t totalWidth = 0;
    size_t height = (size_t)abs(gMaxBoundingRect.top - gMaxBoundingRect.bottom);
    for (size_t stringIndex = 0; stringIndex < length; stringIndex++)
    {
        UINT32 codePoint = (UINT32)string[stringIndex];

        // Spaces are a special case because DirectWrite would give us a 0 width if we only asked for a space, without padding from other glyphs
        // So we use a precomputed space width instead with a zeroed alpha buffer
        if (codePoint == ' ')
        {
            glyphsData[stringIndex].width = gSpaceWidth;
            glyphsData[stringIndex].height = height;
            glyphsData[stringIndex].bytes = (BYTE *)calloc(1, glyphsData[stringIndex].width * glyphsData[stringIndex].height);
        }
        else
        {
            glyphsData[stringIndex] = createGlyphTexture(codePoint);
        }
        totalWidth += glyphsData[stringIndex].width;
    }

    // Stitch all the glyphs together
    const size_t bytesPerPixel = 4;
    const size_t bufferSize = (size_t)(totalWidth * bytesPerPixel * height);
    BYTE* rgbaBytes = (BYTE*)calloc(1, bufferSize);

    size_t accumulatedWidth = 0;
    for (size_t stringIndex = 0; stringIndex < length; stringIndex++)
    {
        for (size_t rowIndex = 0; rowIndex < glyphsData[stringIndex].height; rowIndex++)
        {
            for (size_t columnIndex = 0; columnIndex < glyphsData[stringIndex].width; columnIndex++)
            {
                BYTE alphaValue = glyphsData[stringIndex].bytes[rowIndex * glyphsData[stringIndex].width + columnIndex];

                // Convert grayscale -> RGBA
                BYTE* pixelValues = &rgbaBytes[bytesPerPixel * (rowIndex * totalWidth + accumulatedWidth + columnIndex)];
                pixelValues[0] = alphaValue;
                pixelValues[1] = alphaValue;
                pixelValues[2] = alphaValue;
                pixelValues[3] = alphaValue;
            }
        }

        accumulatedWidth += glyphsData[stringIndex].width;
        free(glyphsData[stringIndex].bytes);
    }

    free(glyphsData);

    TextureData textureData = { 0 };
    textureData.width = (int32_t)totalWidth;
    textureData.height = (int32_t)height;
    textureData.pixelData = (uint8_t*)rgbaBytes;
    textureData.pixelFormat = PIXEL_FORMAT_RGBA32;
    return textureData;
}
