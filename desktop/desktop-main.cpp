#define _CRT_SECURE_NO_WARNINGS // using std::localtime is insecure
#include "config.hpp"
#include "desktop-opencv.hpp"
#include <algorithm>
#include <ctime>
#include <fmo/explorer.hpp>
#include <fmo/processing.hpp>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>

#define TOSTR_INNER(x) #x
#define TOSTR(x) TOSTR_INNER(x)
const char* const windowName = TOSTR(FMO_BINARY_NAME);

int main(int argc, char** argv) try {
    readConfigFromCommandLine(argc, argv);

    auto& cfg = getConfig();
    bool haveInput = cfg.input != "";
    bool haveCamera = cfg.camera != -1;
    bool haveOutDir = cfg.outDir != "";
    bool haveWait = cfg.wait != -1;

    if ((haveInput ^ haveCamera) == 0) {
        throw std::runtime_error("exactly one of --input, --camera must be set");
    }

    cv::VideoCapture capture;
    if (haveInput) {
        capture.open(cfg.input);
        if (!capture.isOpened()) { throw std::runtime_error("could not open input"); }
    } else {
        capture.open(cfg.camera);
        if (!capture.isOpened()) { throw std::runtime_error("could not open camera"); }
    }

#if CV_MAJOR_VERSION == 2
    double fps = capture.get(CV_CAP_PROP_FPS);
    cv::Size size;
    size.width = (int)capture.get(CV_CAP_PROP_FRAME_WIDTH);
    size.height = (int)capture.get(CV_CAP_PROP_FRAME_HEIGHT);
#elif CV_MAJOR_VERSION == 3
    double fps = capture.get(cv::CAP_PROP_FPS);
    cv::Size size;
    size.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
    size.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);
#endif

    cv::VideoWriter writer;
    if (haveOutDir) {
        time_t time = std::time(nullptr);
        std::tm* ltm = std::localtime(&time);
        std::ostringstream outFile;
        outFile << std::setfill('0');
        outFile << cfg.outDir << '/' << (ltm->tm_year + 1900) << '-' << std::setw(2)
                << (ltm->tm_mon + 1) << '-' << std::setw(2) << (ltm->tm_mday) << '-' << std::setw(2)
                << (ltm->tm_hour) << std::setw(2) << (ltm->tm_min) << std::setw(2) << (ltm->tm_sec)
                << ".avi";
        int fourCC = CV_FOURCC('D', 'I', 'V', 'X');
        fps = std::max(double(15), fps);
        writer.open(outFile.str(), fourCC, fps, size, true);
        if (!writer.isOpened()) { throw std::runtime_error("could not start recording"); }
    }

    cv::namedWindow(windowName, cv::WINDOW_NORMAL);

    if (size.height > 600) {
        cv::resizeWindow(windowName, size.width / 2, size.height / 2);
    } else {
        cv::resizeWindow(windowName, size.width, size.height);
    }

    bool paused = false;
    bool step = false;

    int waitMs = 30;
    if (haveCamera) { waitMs = 1; }
    if (haveWait) { waitMs = cfg.wait; }

    fmo::Explorer::Config explorerCfg;
    explorerCfg.dims = {size.width, size.height};
    fmo::Explorer explorer{explorerCfg};
    fmo::Image input;
    input.resize(fmo::Format::GRAY, explorerCfg.dims);

    for (int frameNum = 0; true; frameNum++) {
        if (step || !paused || frameNum == 0) {
            // read
            cv::Mat frame;
            capture >> frame;
            if (frame.empty()) break;

            // write
            if (haveOutDir) { writer << frame; }

            // process
            cv::cvtColor(frame, input.wrap(), cv::COLOR_BGR2GRAY);
            explorer.setInput(input);

            // visualize
            cv::imshow(windowName, explorer.getDebugImage().wrap());
        }

        step = false;
        int key = cv::waitKey(waitMs);
        if (key == 27) break;
        if (key == 10 || key == 13) step = true;
        if (key == int(' ')) paused = !paused;
    }

} catch (std::exception& e) {
    std::cerr << "An error occured: " << e.what() << '\n';
    std::cerr << "Usage:  " TOSTR(FMO_BINARY_NAME) " ";
    std::cerr << "{--input <path> | --camera <num>} [--out-dir <path>] [--wait <ms>]\n";
    std::cerr << "Options: --input   Input video file.\n";
    std::cerr << "         --camera  Camera device ID.\n";
    std::cerr << "         --out-dir Output directory to save video to.\n";
    std::cerr << "         --wait    Additional delay after each frame.\n";
    return -1;
}
