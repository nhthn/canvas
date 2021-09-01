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

std::vector<std::string> getFilterArguments(
    const std::string& filterString,
    const std::string& filterName
)
{
    int offset = std::string(filterName).size() + 1;
    auto arguments = filterString.substr(offset, filterString.size() - 1 - offset);
    std::vector<std::string> trimmedArguments;
    for (auto argument : split(arguments, ',')) {
        trimmedArguments.push_back(trim(argument));
    }
    return trimmedArguments;
}

float parseFloatArgument(const std::string& argument)
{
    try {
        float result = std::stof(argument);
        return result;
    } catch (std::invalid_argument e) {
        std::cerr << "Error: invalid float: '" << argument << "'" << std::endl;
        exit(1);
    }
}

bool parseBoolArgument(const std::string& argument)
{
    return argument == "true";
}

int parseLilypondNoteName(const std::string& noteName)
{
    std::vector<char> baseNames = { 'c', 'd', 'e', 'f', 'g', 'a', 'b' };
    std::vector<int> basePitches = { 0, 2, 4, 5, 7, 9, 11 };
    if (noteName.size() == 0 || noteName.size() > 2) {
        std::cerr << "Error: invalid note name: '" << noteName << "'";
        exit(1);
    }
    char baseName = noteName[0];
    bool baseNameFound;
    int pitch;
    for (int i = 0; i < baseNames.size(); i++) {
        if (baseName == baseNames[i]) {
            pitch = basePitches[i];
            baseNameFound = true;
            break;
        }
    }
    if (!baseNameFound) {
        std::cerr << "Error: invalid note name: '" << noteName << "'";
        exit(1);
    }
    if (noteName.size() == 2) {
        char inflection = noteName[1];
        if (inflection == 's') {
            pitch += 1;
        } else if (inflection == 'f') {
            pitch -= 1;
        } else {
            std::cerr << "Error: invalid note name: '" << noteName << "'";
            exit(1);
        }
    }
    return pitch % 12;
}

int getIndexOf(const std::vector<std::string>& vector, const std::string& string)
{
    for (int i = 0; i < vector.size(); i++) {
        if (string == vector[i]) {
            return i;
        }
    }
    return -1;
}

void applyFilterString(Image image, const std::string& filterString) {
    if (matchesFilter(filterString, "invert")) {
        filters::applyInvert(image);
    } else if (matchesFilter(filterString, "reverb")) {
        auto arguments = getFilterArguments(filterString, "reverb");
        if (arguments.size() != 3) {
            std::cerr << "Error: expected 3 arguments to reverb filter" << std::endl;
            exit(1);
        }
        float reverbDecay = parseFloatArgument(arguments[0]);
        float reverbDamping = parseFloatArgument(arguments[1]);
        bool reverbReverse = parseBoolArgument(arguments[2]);
        filters::applyReverb(image, reverbDecay, reverbDamping, reverbReverse);
    } else if (matchesFilter(filterString, "scale_filter")) {
        auto arguments = getFilterArguments(filterString, "scale_filter");
        if (arguments.size() != 2) {
            std::cerr << "Error: expected 2 arguments to scale filter" << std::endl;
            exit(1);
        }
        auto root = parseLilypondNoteName(arguments[0]);
        auto scaleClassString = arguments[1];
        std::vector<std::string> scaleClassNames = {
            "major",
            "minor",
            "acoustic",
            "harmonic_major",
            "harmonic_mainor",
            "whole_tone",
            "octatonic",
            "hexatonic"
        };
        int scaleClass;
        bool scaleClassFound = false;
        for (int i = 0; i < scaleClassNames.size(); i++) {
            if (scaleClassString == scaleClassNames[i]) {
                scaleClass = i;
                scaleClassFound = true;
                break;
            }
        }
        if (!scaleClassFound) {
            std::cerr
                << "Error: invalid scale class: " << scaleClassString << std::endl;
        }
        filters::applyScaleFilter(image, root, scaleClass);
    } else if (matchesFilter(filterString, "chorus")) {
        auto arguments = getFilterArguments(filterString, "chorus");
        if (arguments.size() != 2) {
            std::cerr << "Error: expected 2 arguments to chorus filter" << std::endl;
            exit(1);
        }
        float chorusRate = parseFloatArgument(arguments[0]);
        float chorusDepth = parseFloatArgument(arguments[1]);
        filters::applyChorus(image, chorusRate, chorusDepth);
    } else if (matchesFilter(filterString, "tremolo")) {
        auto arguments = getFilterArguments(filterString, "tremolo");
        if (arguments.size() != 4) {
            std::cerr << "Error: expected 4 arguments to tremolo filter" << std::endl;
            exit(1);
        }
        float tremoloRate = parseFloatArgument(arguments[0]);
        float tremoloDepth = parseFloatArgument(arguments[1]);
        std::string tremoloShapeString = arguments[2];
        std::vector<std::string> tremoloShapeNames = {
            "sine", "triangle", "square", "saw_down", "saw_up"
        };
        int tremoloShape = getIndexOf(tremoloShapeNames, tremoloShapeString);
        if (tremoloShape == -1) {
            std::cerr << "Error: Invalid tremolo shape: '" << tremoloShapeString << "'";
            exit(1);
        }
        float tremoloStereo = parseFloatArgument(arguments[3]);
        filters::applyTremolo(
            image, tremoloRate, tremoloDepth, tremoloShape, tremoloStereo
        );
    }
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
            applyFilterString(image, filterString);
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
