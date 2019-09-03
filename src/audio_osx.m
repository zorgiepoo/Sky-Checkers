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

#import "audio.h"
#import <AVFoundation/AVFoundation.h>

static AVAudioEngine *gAudioEngine;
static AVAudioPlayerNode *gPlayerMusicNode;
static AVAudioPlayerNode *gPlayerEffectNodes[MAX_CHANNELS];

static AVAudioPCMBuffer *gMainMusicBuffer;
static AVAudioPCMBuffer *gGameMusicBuffer;

static AVAudioPCMBuffer *gMenuSoundBuffer;
static AVAudioPCMBuffer *gShootingSoundBuffer;
static AVAudioPCMBuffer *gTileFallingSoundBuffer;
static AVAudioPCMBuffer *gDieingStoneSoundBuffer;

static AVAudioPCMBuffer *createAudioBuffer(NSString *filePath)
{
	NSURL *fileURL = [NSURL fileURLWithPath:filePath];
	NSError *fileError = nil;
	AVAudioFile *file = [[AVAudioFile alloc] initForReading:fileURL error:&fileError];
	if (file == nil)
	{
		NSLog(@"Failed to initialize audio file with error: %@", fileError);
		return nil;
	}
	
	AVAudioFormat *audioFormat = file.processingFormat;
	AVAudioPCMBuffer *buffer = [[AVAudioPCMBuffer alloc] initWithPCMFormat:audioFormat frameCapacity:(AVAudioFrameCount)file.length];
	
	NSError *bufferError = nil;
	if (![file readIntoBuffer:buffer error:&bufferError])
	{
		NSLog(@"Failed to read file into audio buffer with error: %@", bufferError);
		return nil;
	}
	
	return buffer;
}

void initAudio(void)
{
	gAudioEngine = [[AVAudioEngine alloc] init];
	
	AVAudioFormat *finalAudioFormat = [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat32 sampleRate:(double)AUDIO_FORMAT_SAMPLE_RATE
	   channels:AUDIO_FORMAT_NUM_CHANNELS interleaved:NO];
	
	gPlayerMusicNode = [[AVAudioPlayerNode alloc] init];
	gPlayerMusicNode.volume = (float)MUSIC_VOLUME / (float)MAX_VOLUME;
	[gAudioEngine attachNode:gPlayerMusicNode];
	
	gPlayerEffectNodes[MENU_SOUND_CHANNEL] = [[AVAudioPlayerNode alloc] init];
	gPlayerEffectNodes[MENU_SOUND_CHANNEL].volume = (float)MENU_SOUND_VOLUME / (float)MAX_VOLUME;
	[gAudioEngine attachNode:gPlayerEffectNodes[MENU_SOUND_CHANNEL]];
	
	for (uint16_t shootingNodeIndex = SHOOTING_SOUND_MIN_CHANNEL; shootingNodeIndex <= SHOOTING_SOUND_MAX_CHANNEL; shootingNodeIndex++)
	{
		gPlayerEffectNodes[shootingNodeIndex] = [[AVAudioPlayerNode alloc] init];
		gPlayerEffectNodes[shootingNodeIndex].volume = (float)SHOOTING_SOUND_VOLUME / (float)MAX_VOLUME;
		
		[gAudioEngine attachNode:gPlayerEffectNodes[shootingNodeIndex]];
	}
	
	for (uint16_t tileFallingNodeIndex = TILE_FALLING_SOUND_MIN_CHANNEL; tileFallingNodeIndex <= TILE_FALLING_SOUND_MAX_CHANNEL; tileFallingNodeIndex++)
	{
		gPlayerEffectNodes[tileFallingNodeIndex] = [[AVAudioPlayerNode alloc] init];
		gPlayerEffectNodes[tileFallingNodeIndex].volume = (float)TILE_FALLING_SOUND_VOLUME / (float)MAX_VOLUME;
		
		[gAudioEngine attachNode:gPlayerEffectNodes[tileFallingNodeIndex]];
	}
	
	for (uint16_t dieingStoneNodeIndex = DIEING_STONE_SOUND_MIN_CHANNEL; dieingStoneNodeIndex <= DIEING_STONE_SOUND_MAX_CHANNEL; dieingStoneNodeIndex++)
	{
		gPlayerEffectNodes[dieingStoneNodeIndex] = [[AVAudioPlayerNode alloc] init];
		gPlayerEffectNodes[dieingStoneNodeIndex].volume = (float)TILE_DIEING_STONE_SOUND_VOLUME / (float)MAX_VOLUME;
		
		[gAudioEngine attachNode:gPlayerEffectNodes[dieingStoneNodeIndex]];
	}
	
	void (^cleanupFailure)(void) = ^{
		gAudioEngine = nil;
		gMainMusicBuffer = nil;
		gGameMusicBuffer = nil;
		gMenuSoundBuffer = nil;
		gShootingSoundBuffer = nil;
		gTileFallingSoundBuffer = nil;
		gDieingStoneSoundBuffer = nil;
		gPlayerMusicNode = nil;
		for (uint16_t nodeIndex = 0; nodeIndex < MAX_CHANNELS; nodeIndex++)
		{
			gPlayerEffectNodes[nodeIndex] = nil;
		}
	};
	
	gMainMusicBuffer = createAudioBuffer(@"Data/Audio/main_menu.wav");
	if (gMainMusicBuffer == nil)
	{
		cleanupFailure();
		return;
	}
	
	gGameMusicBuffer = createAudioBuffer(@"Data/Audio/fast-track.wav");
	if (gGameMusicBuffer == nil)
	{
		cleanupFailure();
		return;
	}
	
	gMenuSoundBuffer = createAudioBuffer(@"Data/Audio/sound6.wav");
	if (gMenuSoundBuffer == nil)
	{
		cleanupFailure();
		return;
	}
	
	gShootingSoundBuffer = createAudioBuffer(@"Data/Audio/whoosh.wav");
	if (gShootingSoundBuffer == nil)
	{
		cleanupFailure();
		return;
	}
	
	gTileFallingSoundBuffer = createAudioBuffer(@"Data/Audio/object_falls.wav");
	if (gTileFallingSoundBuffer == nil)
	{
		cleanupFailure();
		return;
	}
	
	gDieingStoneSoundBuffer = createAudioBuffer(@"Data/Audio/dieing_stone.wav");
	if (gDieingStoneSoundBuffer == nil)
	{
		cleanupFailure();
		return;
	}
	
	AVAudioMixerNode *mainMixerNode = [gAudioEngine mainMixerNode];
	
	[gAudioEngine connect:gPlayerMusicNode to:mainMixerNode format:gMainMusicBuffer.format];
	[gAudioEngine connect:gPlayerEffectNodes[0] to:mainMixerNode format:gMenuSoundBuffer.format];
	
	for (uint16_t shootingNodeIndex = SHOOTING_SOUND_MIN_CHANNEL; shootingNodeIndex <= SHOOTING_SOUND_MAX_CHANNEL; shootingNodeIndex++)
	{
		[gAudioEngine connect:gPlayerEffectNodes[shootingNodeIndex] to:mainMixerNode format:gShootingSoundBuffer.format];
	}
	
	for (uint16_t tileFallingNodeIndex = TILE_FALLING_SOUND_MIN_CHANNEL; tileFallingNodeIndex <= TILE_FALLING_SOUND_MAX_CHANNEL; tileFallingNodeIndex++)
	{
		[gAudioEngine connect:gPlayerEffectNodes[tileFallingNodeIndex] to:mainMixerNode format:gTileFallingSoundBuffer.format];
	}
	
	for (uint16_t dieingStoneNodeIndex = DIEING_STONE_SOUND_MIN_CHANNEL; dieingStoneNodeIndex <= DIEING_STONE_SOUND_MAX_CHANNEL; dieingStoneNodeIndex++)
	{
		[gAudioEngine connect:gPlayerEffectNodes[dieingStoneNodeIndex] to:mainMixerNode format:gDieingStoneSoundBuffer.format];
	}
	
	[gAudioEngine connect:mainMixerNode to:gAudioEngine.outputNode format:finalAudioFormat];
	
	NSError *engineError = nil;
	if (![gAudioEngine startAndReturnError:&engineError])
	{
		NSLog(@"Failed to start audio engine with error: %@", engineError);
		cleanupFailure();
	}
}

static void playMusicBuffer(SDL_bool paused, AVAudioPCMBuffer *buffer)
{
	if (paused)
	{
		[gPlayerMusicNode stop];
	}
	
	[gPlayerMusicNode scheduleBuffer:buffer atTime:nil options:(AVAudioPlayerNodeBufferLoops | AVAudioPlayerNodeBufferInterrupts) completionHandler:nil];
	
	if (!paused)
	{
		[gPlayerMusicNode play];
	}
}

void playMainMenuMusic(SDL_bool paused)
{
	if (gAudioEngine == nil) return;

	playMusicBuffer(paused, gMainMusicBuffer);
}

void playGameMusic(SDL_bool paused)
{
	if (gAudioEngine == nil) return;

	playMusicBuffer(paused, gGameMusicBuffer);
}

void stopMusic(void)
{
	[gPlayerMusicNode stop];
}

void pauseMusic(void)
{
	[gPlayerMusicNode pause];
}

void unPauseMusic(void)
{
	[gPlayerMusicNode play];
}

void playMenuSound(void)
{
	[gPlayerEffectNodes[MENU_SOUND_CHANNEL] scheduleBuffer:gMenuSoundBuffer atTime:nil options:AVAudioPlayerNodeBufferInterrupts completionHandler:nil];
	
	[gPlayerEffectNodes[MENU_SOUND_CHANNEL] play];
}

void playShootingSound(int soundIndex)
{
	if (gAudioEngine == nil) return;
	
	int nodeIndex = soundIndex + SHOOTING_SOUND_MIN_CHANNEL;
	
	[gPlayerEffectNodes[nodeIndex] scheduleBuffer:gShootingSoundBuffer atTime:nil options:AVAudioPlayerNodeBufferInterrupts completionHandler:nil];
	
	[gPlayerEffectNodes[nodeIndex] play];
}

void playTileFallingSound(void)
{
	if (gAudioEngine == nil) return;
	
	static int currentSoundIndex = 0;
	
	int currentNodeIndex = currentSoundIndex + TILE_FALLING_SOUND_MIN_CHANNEL;
	
	[gPlayerEffectNodes[currentNodeIndex] scheduleBuffer:gTileFallingSoundBuffer atTime:nil options:AVAudioPlayerNodeBufferInterrupts completionHandler:nil];
	
	[gPlayerEffectNodes[currentNodeIndex] play];
	
	currentSoundIndex = (currentSoundIndex + 1) % (TILE_FALLING_SOUND_MAX_CHANNEL - TILE_FALLING_SOUND_MIN_CHANNEL + 1);
}

void playDieingStoneSound(void)
{
	if (gAudioEngine == nil) return;
	
	static int currentSoundIndex = 0;
	
	int currentNodeIndex = currentSoundIndex + DIEING_STONE_SOUND_MIN_CHANNEL;
	
	[gPlayerEffectNodes[currentNodeIndex] scheduleBuffer:gDieingStoneSoundBuffer atTime:nil options:AVAudioPlayerNodeBufferInterrupts completionHandler:nil];
	
	[gPlayerEffectNodes[currentNodeIndex] play];
	
	currentSoundIndex = (currentSoundIndex + 1) % (DIEING_STONE_SOUND_MAX_CHANNEL - DIEING_STONE_SOUND_MIN_CHANNEL + 1);
}
