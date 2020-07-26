<!-- This document is licensed under CC BY-SA 3.0: https://creativecommons.org/licenses/by-sa/3.0/ -->
# Sky Checkers

![alt text](https://zgcoder.net/software/skycheckers/images/playing-thumb.png?1 "A screen-shot of Sky Checker's game-play")

## Introduction
Sky Checkers is a multiplayer action based game. Knock off your enemies and be the last one standing!

Sky Checkers supports macOS, Windows, Linux, iOS, and tvOS. Stable downloads can be found on the [releases page](https://github.com/zorgiepoo/Sky-Checkers/releases). For compiling the latest changes from source, please see [INSTALL](INSTALL).

This game is based on an old N64 Kirby mini-game and was originally written during my high school years, but still keeps my interest ❤️.

## Remarks

Sky Checkers was one of my first programming projects when I was learning C and has been first released over a decade ago, longer than popular game engines (Unity, Unreal) have been mainstream. In recent years, it has received updates due to still tackling interesting problems.

This is a real-time 3-D video game written to push matrix transformations and buffers to multiple rendering backends -- Metal, Direct3D 11, OpenGL. It uses the preferred backend for the appropriate platform, which is aligned with how professional game engines operate today.

This game is now mostly written from "scratch" on most platforms, opting to use native system libraries for functionality such as relating to textures, audio, fonts, gamepads, touch, UI, and more. This gives full control of what code the game leverages; it can use present and future platform-specific features where available, and avoid any issues a major 3rd party library may abstract incorrectly. SDL libraries are used only on Linux.

Sky Checkers additionally supports [playing online](https://www.youtube.com/watch?v=NjZAAgJhsho) with friends over UDP in a client-server model. This posed many interesting challenges for smooth playing such as tagging packets, channeling real-time and important messages, minimizing data transferred, lag compensation of firing, client-side interpolation, and client-side prediction.

Other aspects expected in a game are present such as a tutorial, AI, and multiplayer mayhem 😁.

## Licensing

The source code (including OpenGL shaders) is licensed under the GPL version 3. In the unanticipated event contributions are made, I may need to extend the license. I plan to put some parts of the code under a more permissive license eventually.

The assets are licensed differently. I acquired [an embedded app license](http://typodermicfonts.com/goodfish/) for the goodfish font that only I can use. Sound assets except for main_menu.wav and dieing_stone.wav are freely distributed from [Freesound](https://freesound.org) and [PacDV](http://www.pacdv.com/sounds/). The cracked texture tile effect is acquired from [Pixabay](https://pixabay.com/illustrations/cracked-texture-overlay-distressed-1975573/). I acquired game controller mappings from [SDL_GameControllerDB](https://github.com/gabomdq/SDL_GameControllerDB) with some of my own alterations.

The main menu music is royalty free because it's mostly derived from a Garageband loop. The dieing_stone.wav effect I recorded and the [whoosh.wav effect](https://freesound.org/people/petenice/sounds/9509) are under [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/).

The app icon images, this ReadMe, INSTALL, Changelog are licensed under [CC BY-SA 3.0](https://creativecommons.org/licenses/by-sa/3.0/).

Metadata files for Linux (desktop entry, com.zgcoder.skycheckers.json, snapcraft.yaml, AppData file) are freely licensed under [FSFAP](https://spdx.org/licenses/FSFAP.html).

The Texture assets (Data/Textures) were developed by a friend and I solely for this project, and are not freely licensed.
