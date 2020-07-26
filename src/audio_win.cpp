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

#include "audio.h"
#include <stdio.h>
#include <stdint.h>

#include <xaudio2.h>

typedef struct
{
    IXAudio2SourceVoice** sourceVoices;
    XAUDIO2_BUFFER buffer;
    uint8_t numberOfSources;
} ZGAudioSource;

static ZGAudioSource *gActiveMusicSource;
static ZGAudioSource gMenuMusicSource;
static ZGAudioSource gGameMusicSource;

static ZGAudioSource gMenuEffectSource;
static ZGAudioSource gShootingEffectSource;
static ZGAudioSource gTileFallingEffectSource;
static ZGAudioSource gDieingStoneEffectSource;

//---------
// FindChunk() and ReadChunkData() are provided from sample code:
// https://docs.microsoft.com/en-us/windows/win32/xaudio2/how-to--load-audio-data-files-in-xaudio2

// Assuming little endian
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'

static HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition)
{
    HRESULT hr = S_OK;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
        return HRESULT_FROM_WIN32(GetLastError());

    DWORD dwChunkType;
    DWORD dwChunkDataSize;
    DWORD dwRIFFDataSize = 0;
    DWORD dwFileType;
    DWORD bytesRead = 0;
    DWORD dwOffset = 0;

    while (hr == S_OK)
    {
        DWORD dwRead;
        if (0 == ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());

        if (0 == ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL))
            hr = HRESULT_FROM_WIN32(GetLastError());

        switch (dwChunkType)
        {
        case fourccRIFF:
            dwRIFFDataSize = dwChunkDataSize;
            dwChunkDataSize = 4;
            if (0 == ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL))
                hr = HRESULT_FROM_WIN32(GetLastError());
            break;

        default:
            if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT))
                return HRESULT_FROM_WIN32(GetLastError());
        }

        dwOffset += sizeof(DWORD) * 2;

        if (dwChunkType == fourcc)
        {
            dwChunkSize = dwChunkDataSize;
            dwChunkDataPosition = dwOffset;
            return S_OK;
        }

        dwOffset += dwChunkDataSize;

        if (bytesRead >= dwRIFFDataSize) return S_FALSE;

    }

    return S_OK;
}

static HRESULT ReadChunkData(HANDLE hFile, void* buffer, DWORD buffersize, DWORD bufferoffset)
{
    HRESULT hr = S_OK;
    if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN))
        return HRESULT_FROM_WIN32(GetLastError());
    DWORD dwRead;
    if (0 == ReadFile(hFile, buffer, buffersize, &dwRead, NULL))
        hr = HRESULT_FROM_WIN32(GetLastError());
    return hr;
}

//---------

static bool createAudioBuffer(const char *path, XAUDIO2_BUFFER &buffer, WAVEFORMATEXTENSIBLE &format, bool looping)
{
    HANDLE fileHandle = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Error: failed to create file handle for audio buffer for %s with error: %d\n", path, HRESULT_FROM_WIN32(GetLastError()));
        return false;
    }

    if (SetFilePointer(fileHandle, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
    {
        fprintf(stderr, "Error: failed to initialize file pointer for audio buffer for %s with error: %d\n", path, HRESULT_FROM_WIN32(GetLastError()));
        return false;
    }

    DWORD dwChunkSize;
    DWORD dwChunkPosition;

    FindChunk(fileHandle, fourccRIFF, dwChunkSize, dwChunkPosition);
    DWORD filetype;
    ReadChunkData(fileHandle, &filetype, sizeof(DWORD), dwChunkPosition);

    if (filetype != fourccWAVE)
    {
        return false;
    }

    FindChunk(fileHandle, fourccFMT, dwChunkSize, dwChunkPosition);
    ReadChunkData(fileHandle, &format, dwChunkSize, dwChunkPosition);

    FindChunk(fileHandle, fourccDATA, dwChunkSize, dwChunkPosition);
    BYTE* pDataBuffer = new BYTE[dwChunkSize];
    ReadChunkData(fileHandle, pDataBuffer, dwChunkSize, dwChunkPosition);

    buffer.AudioBytes = dwChunkSize;  //buffer containing audio data
    buffer.pAudioData = pDataBuffer;  //size of the audio buffer in bytes
    buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer
    buffer.LoopCount = looping ? XAUDIO2_LOOP_INFINITE : 0;

    return true;
}

static void createAudioSource(IXAudio2 *engine, const char* path, uint8_t numberOfSources, bool looping, ZGAudioSource &audioSource)
{
    WAVEFORMATEXTENSIBLE format = { 0 };
    if (!createAudioBuffer(path, audioSource.buffer, format, looping))
    {
        return;
    }

    audioSource.sourceVoices = new IXAudio2SourceVoice*[numberOfSources];
    if (audioSource.sourceVoices == nullptr) return;

    uint8_t sourceIndex;
    for (sourceIndex = 0; sourceIndex < numberOfSources; sourceIndex++)
    {
        HRESULT sourceVoiceResult = engine->CreateSourceVoice(&audioSource.sourceVoices[sourceIndex], (WAVEFORMATEX*)&format, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, nullptr, nullptr);
        if (FAILED(sourceVoiceResult))
        {
            fprintf(stderr, "Failed to create source voice for %s with index %d with error %d\n", path, sourceIndex, sourceVoiceResult);
            break;
        }
    }

    audioSource.numberOfSources = sourceIndex;
}

static void setVolume(ZGAudioSource& audioSource, float volume)
{
    for (uint8_t sourceIndex = 0; sourceIndex < audioSource.numberOfSources; sourceIndex++)
    {
        audioSource.sourceVoices[sourceIndex]->SetVolume(volume);
    }
}

static void startAudio(ZGAudioSource& audioSource)
{
    for (uint8_t sourceIndex = 0; sourceIndex < audioSource.numberOfSources; sourceIndex++)
    {
        audioSource.sourceVoices[sourceIndex]->Start(0, XAUDIO2_COMMIT_NOW);
    }
}

static void stopAudio(ZGAudioSource& audioSource)
{
    for (uint8_t sourceIndex = 0; sourceIndex < audioSource.numberOfSources; sourceIndex++)
    {
        audioSource.sourceVoices[sourceIndex]->Stop();
    }
}

static void flushAudio(ZGAudioSource& audioSource)
{
    for (uint8_t sourceIndex = 0; sourceIndex < audioSource.numberOfSources; sourceIndex++)
    {
        audioSource.sourceVoices[sourceIndex]->FlushSourceBuffers();
    }
}

static void submitAudio(ZGAudioSource& audioSource, uint8_t sourceIndex)
{
    if (sourceIndex >= audioSource.numberOfSources) return;
    HRESULT submitResult = audioSource.sourceVoices[sourceIndex]->SubmitSourceBuffer(&audioSource.buffer);
    if (FAILED(submitResult))
    {
        fprintf(stderr, "Error: failed to submit audio buffer with index %d and error %d\n", sourceIndex, submitResult);
    }
}

extern "C" void initAudio(void)
{
    IXAudio2* engine = nullptr;
    HRESULT audioResult = XAudio2Create(&engine, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(audioResult))
    {
        fprintf(stderr, "Error: Failed to initialize audio engine: %d\n", audioResult);
        return;
    }

    IXAudio2MasteringVoice* masteringVoice = nullptr;
    HRESULT masteringResult = engine->CreateMasteringVoice(&masteringVoice);
    if (FAILED(masteringResult))
    {
        fprintf(stderr, "Error: Failed to create mastering voice: %d\n", masteringResult);
        engine->Release();
        return;
    }

    createAudioSource(engine, "Data\\Audio\\main_menu.wav", 1, true, gMenuMusicSource);
    setVolume(gMenuMusicSource, (float)MUSIC_VOLUME / (float)MAX_VOLUME);

    createAudioSource(engine, "Data\\Audio\\fast-track.wav", 1, true, gGameMusicSource);
    setVolume(gGameMusicSource, (float)MUSIC_VOLUME / (float)MAX_VOLUME);
    
    createAudioSource(engine, "Data\\Audio\\sound6.wav", 1, false, gMenuEffectSource);
    setVolume(gMenuEffectSource, (float)MENU_SOUND_VOLUME / (float)MAX_VOLUME);
    startAudio(gMenuEffectSource);

    createAudioSource(engine, "Data\\Audio\\whoosh.wav", SHOOTING_SOUND_MAX_CHANNEL - SHOOTING_SOUND_MIN_CHANNEL + 1, false, gShootingEffectSource);
    setVolume(gShootingEffectSource, (float)SHOOTING_SOUND_VOLUME / (float)MAX_VOLUME);
    startAudio(gShootingEffectSource);

    createAudioSource(engine, "Data\\Audio\\object_falls.wav", TILE_FALLING_SOUND_MAX_CHANNEL - TILE_FALLING_SOUND_MIN_CHANNEL + 1, false, gTileFallingEffectSource);
    setVolume(gTileFallingEffectSource, (float)TILE_FALLING_SOUND_VOLUME / (float)MAX_VOLUME);
    startAudio(gTileFallingEffectSource);

    createAudioSource(engine, "Data\\Audio\\dieing_stone.wav", DIEING_STONE_SOUND_MAX_CHANNEL - DIEING_STONE_SOUND_MIN_CHANNEL + 1, false, gDieingStoneEffectSource);
    setVolume(gDieingStoneEffectSource, (float)TILE_DIEING_STONE_SOUND_VOLUME / (float)MAX_VOLUME);
    startAudio(gDieingStoneEffectSource);
}

static void playMusic(bool paused)
{
    submitAudio(*gActiveMusicSource, 0);

    if (!paused)
    {
        startAudio(*gActiveMusicSource);
    }
}

extern "C" void playMainMenuMusic(bool paused)
{
    stopMusic();
    gActiveMusicSource = &gMenuMusicSource;
    playMusic(paused);
}

extern "C" void playGameMusic(bool paused)
{
    stopMusic();
    gActiveMusicSource = &gGameMusicSource;
    playMusic(paused);
}

extern "C" void stopMusic(void)
{
    if (gActiveMusicSource == nullptr) return;
    stopAudio(*gActiveMusicSource);
    flushAudio(*gActiveMusicSource);
}

extern "C" void pauseMusic(void)
{
    if (gActiveMusicSource == nullptr) return;
    stopAudio(*gActiveMusicSource);
}

extern "C" void unPauseMusic(void)
{
    if (gActiveMusicSource == nullptr) return;
    startAudio(*gActiveMusicSource);
}

extern "C" void playMenuSound(void)
{
    // Allow the menu sound to be interrupted
    stopAudio(gMenuEffectSource);
    flushAudio(gMenuEffectSource);
    startAudio(gMenuEffectSource);
    submitAudio(gMenuEffectSource, 0);
}

extern "C" void playShootingSound(int soundIndex)
{
    submitAudio(gShootingEffectSource, (uint8_t)soundIndex);
}

extern "C" void playTileFallingSound(void)
{
    static uint8_t currentSourceIndex = 0;
    submitAudio(gTileFallingEffectSource, currentSourceIndex);
    currentSourceIndex = (currentSourceIndex + 1) % gTileFallingEffectSource.numberOfSources;
}

extern "C" void playDieingStoneSound(void)
{
    static uint8_t currentSourceIndex = 0;
    submitAudio(gDieingStoneEffectSource, currentSourceIndex);
    currentSourceIndex = (currentSourceIndex + 1) % gDieingStoneEffectSource.numberOfSources;
}
