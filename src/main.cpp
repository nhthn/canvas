#include "tclap/CmdLine.h"

#include "App.hpp"
#include "common.hpp"
#include "io.hpp"


int main(int argc, char** argv) {
    bool turboMode;
    std::string inFile;
    std::string outFile;
    float sampleRate = 48000;
    float overallGain = 0.1;
    float speedInPixelsPerSecond = 100;
    int pdMode = 0;
    float pdDistort = 0;
    try {
        TCLAP::CmdLine cmd("Canvas: a visual additive synthesizer", ' ', "0.0.1");
        TCLAP::SwitchArg turboSwitch("t", "turbo", "Run in turbo mode", cmd, false);
        TCLAP::ValueArg<std::string> inFileArg(
            "i", "infile", "Input file name", false, "", "string"
        );
        cmd.add(inFileArg);
        TCLAP::ValueArg<std::string> outFileArg(
            "o", "outfile", "Output file name", false, "", "string"
        );
        cmd.add(outFileArg);
        TCLAP::ValueArg<float> sampleRateArg(
            "r",
            "rate",
            "Sample rate in Hz if output is an audio file.",
            false,
            48000,
            "float"
        );
        cmd.add(sampleRateArg);
        TCLAP::ValueArg<float> speedArg(
            "s",
            "speed",
            "Speed in pixels per second if output is an audio file.",
            false,
            100,
            "float"
        );
        cmd.add(speedArg);
        cmd.parse(argc, argv);

        turboMode = turboSwitch.getValue();
        inFile = inFileArg.getValue();
        outFile = outFileArg.getValue();
        sampleRate = sampleRateArg.getValue();
        speedInPixelsPerSecond = speedArg.getValue();
    } catch (TCLAP::ArgException e) {
        std::cerr << "Error: " << e.error() << " for arg " << e.argId() << std::endl;
        exit(1);
    }

    if (turboMode) {
        if (inFile == "") {
            std::cerr << "Error: Input file -i is required in turbo mode" << std::endl;
            exit(1);
        }
        if (outFile == "") {
            std::cerr << "Error: Output file -o is required in turbo mode" << std::endl;
            exit(1);
        }

        bool inFileIsImage = true;
        bool outFileIsImage = true;
        if (endsWith(inFile, ".wav")) {
            inFileIsImage = false;
        } else if (endsWith(inFile, ".png")) {
            inFileIsImage = true;
        }
        if (endsWith(outFile, ".wav")) {
            outFileIsImage = false;
        } else if (endsWith(outFile, ".png")) {
            outFileIsImage = true;
        }

        uint32_t* pixels = new uint32_t[k_imageWidth * k_imageHeight];
        Image image = std::make_tuple(pixels, k_imageWidth, k_imageHeight);

        if (inFileIsImage) {
            io::Status status = io::loadImage(image, inFile);
            bool success = std::get<0>(status);
            std::string message = std::get<1>(status);
            if (!success) {
                std::cerr << message << std::endl;
                exit(1);
            }
        } else {
            io::Status status = io::loadAudio(image, inFile);
            bool success = std::get<0>(status);
            std::string message = std::get<1>(status);
            if (!success) {
                std::cerr << message << std::endl;
                exit(1);
            }
        }

        if (outFileIsImage) {
            io::Status status = io::saveImage(image, outFile);
            bool success = std::get<0>(status);
            std::string message = std::get<1>(status);
            if (!success) {
                std::cerr << message << std::endl;
                exit(1);
            }
        } else {
            io::Status status = io::renderAudio(
                image,
                outFile,
                sampleRate,
                overallGain,
                speedInPixelsPerSecond,
                pdMode,
                pdDistort
            );
            bool success = std::get<0>(status);
            std::string message = std::get<1>(status);
            if (!success) {
                std::cerr << message << std::endl;
                exit(1);
            }
        }

        delete[] pixels;
    } else {
        App app;
        app.run();
    }

    return 0;
}
