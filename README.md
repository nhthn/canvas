![Screenshot of Canvas](screenshot.png)

Canvas (working title) is a visual additive synthesizer that is controlled by editing an image. Scribble on the canvas and use a variety of image filters to create new and interesting sounds. Canvas is heavily inspired by [MetaSynth](https://uisoftware.com/metasynth/) and [Virtual ANS](https://warmplace.ru/soft/ans/) and aspires to offer an open source, cross-platform addition to the graphical synthesizer space.

Canvas currently uses 239 sine waves spaced at quarter tones, and offers rudimentary drawing features and several image-based audio filters such as reverb, chorus, and tremolo. Stereo is supported by using red and blue for the right and left channels, respectively. The sine waves can be morphed into other waveforms using [phase distortion synthesis](https://en.wikipedia.org/wiki/Phase_distortion_synthesis).

This software is built on PortAudio, libsndfile, SDL2, FFTW, [stb_image](https://github.com/nothings/stb/), and [NanoGUI-SDL](https://github.com/dalerank/nanogui-sdl/).

## Building

### Windows

You will need to install CMake and Visual Studio. MinGW is not yet supported.

Download and unzip the following and place them somwhere safe:

- ASIO SDK
- FFTW3 source code
- libsndfile
- Development libraries for SDL2, SDL2_image, and SDL2_ttf

Run:

    mkdir build
    cd build

    # For the -G option, use cmake --help and scroll down to the list of
    # Visual Studio versions. Find the string that matches the one you currently
    # have installed.
    cmake .. -G "Visual Studio 15 2017" -A x64 \
        -DASIOSDK_ROOT_DIR=<path to ASIO SDL> \
        -DSDL2_LIBRARY=<path to SDL2>/lib/x64/SDL2.lib \
	-DSDL2_INCLUDE_DIRS=<path to SDL2>/include/
        -DSDL2_IMAGE_PATH=<path to SDL2_image> \
        -DSDL2_TTF_PATH=<path to SDL2_ttf> \
	-DSNDFILE_PATH=<path to libsndfile> \
	-DFFTW_ROOT=<path to FFTW> \

    cmake --build . --config Release


### Linux

Debian/Ubuntu:

    sudo apt install build-essential cmake libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev portaudio19-dev libsndfile1-dev libfftw3-dev

Arch:

    sudo pacman -S cmake sdl2 sdl2_image sdl2_ttf portaudio libsndfile fftw

Build on Linux:

    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..

The run `./canvas`.
