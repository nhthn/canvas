#include <vector>

#include "tclap/CmdLine.h"

#include "App.hpp"
#include "common.hpp"
#include "filters.hpp"
#include "io.hpp"


bool matchesFilter(const std::string& filterString, const std::string& filterName)
{
    return startsWith(filterString, filterName + "(") && endsWith(filterString, ")");
}


int main(int argc, char** argv) {
    bool turboMode;
    std::string inFile;
    std::string outFile;
    float sampleRate = 48000;
    float overallGain = 0.1;
    float speedInPixelsPerSecond = 100;
    std::string pdModeString = "pulsar";
    int pdMode = 0;
    float pdDistort = 0;
    std::vector<std::string> filterStrings;

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

        TCLAP::ValueArg<std::string> pdModeArg(
            "m",
            "pd-mode",
            "Phase distortion mode. One of pulsar, saw, square, or sine_pwm.",
            false,
            "pulsar",
            "string"
        );
        cmd.add(pdModeArg);

        TCLAP::ValueArg<float> pdDistortArg(
            "d",
            "pd-distort",
            "Phase distortion amount from 0 to 1.",
            false,
            0,
            "float"
        );
        cmd.add(pdDistortArg);

        TCLAP::MultiArg<std::string> filterArg(
            "f",
            "filter",
            "Apply a filter. Example: -f reverb(0.3,0.8,true).",
            false,
            "string"
        );
        cmd.add(filterArg);

        cmd.parse(argc, argv);

        turboMode = turboSwitch.getValue();
        inFile = inFileArg.getValue();
        outFile = outFileArg.getValue();
        sampleRate = sampleRateArg.getValue();
        speedInPixelsPerSecond = speedArg.getValue();
        pdModeString = pdModeArg.getValue();
        pdDistort = pdDistortArg.getValue();
        filterStrings = filterArg.getValue();

        if (pdModeString == "saw") {
            pdMode = 1;
        } else if (pdModeString == "square") {
            pdMode = 2;
        } else if (pdModeString == "sine_pwm") {
            pdMode = 3;
        } else {
            pdMode = 0;
        }

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

        for (auto& filterString : filterStrings) {
            if (matchesFilter(filterString, "invert")) {
                filters::applyInvert(image);
            } else if (matchesFilter(filterString, "reverb")) {
                int offset = std::string("reverb").size() + 1;
                auto arguments = filterString.substr(offset, filterString.size() - 1 - offset);
                std::vector<std::string> trimmedArguments;
                for (auto argument : split(arguments, ',')) {
                    trimmedArguments.push_back(trim(argument));
                }
                float reverbDecay;
                try {
                    reverbDecay = std::stof(trimmedArguments[0]);
                } catch (std::invalid_argument e) {
                    std::cerr << "Error: invalid float: '" << trimmedArguments[0] << "'" << std::endl;
                    exit(1);
                }
                float reverbDamping;
                try {
                    reverbDamping = std::stof(trimmedArguments[1]);
                } catch (std::invalid_argument e) {
                    std::cerr << "Error: invalid float: '" << trimmedArguments[1] << "'" << std::endl;
                    exit(1);
                }
                bool reverbReverse = trimmedArguments[2] == "true";

                filters::applyReverb(image, reverbDecay, reverbDamping, reverbReverse);
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
