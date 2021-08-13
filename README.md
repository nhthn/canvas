Canvas (working title) is a visual additive synthesizer that is controlled by editing an image. Scribble on the canvas and use a variety of image filters to create new and interesting sounds. Canvas is heavily inspired by MetaSynth and Virtual ANS and aspires to offer an open source, cross-platform alternative.

Canvas currently uses 256 sine waves spaced at quarter tones, and offers rudimentary drawing features and several image-based audio filters such as reverb, chorus, and tremolo. Stereo is supported by using red and blue for the right and left channels, respectively.

Canvas is built on PortAudio, SDL2, and NanoGUI-SDL.

## Building

For installation on Arch:

    sudo pacman -S cmake sdl2 sdl2_image sdl2_ttf

For other platforms: no clue

To build on Linux:

    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..

The run `./canvas`.
