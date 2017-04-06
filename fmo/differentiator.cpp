#include "image-util.hpp"
#include "include-opencv.hpp"
#include <fmo/assert.hpp>
#include <fmo/differentiator.hpp>
#include <fmo/processing.hpp>

namespace fmo {
    Differentiator::Config::Config() : threshGray(19), threshBgr(23), threshYuv(23) {}

    struct AddAndThreshJob : public cv::ParallelLoopBody {
        using batch_t = uint8_t;

        static void impl(const uint8_t* src, const uint8_t* srcEnd, uint8_t* dst, int thresh) {
            for (; src < srcEnd; src += SRC_BATCH_SIZE, dst += DST_BATCH_SIZE) {
                *dst = ((src[0] + src[1] + src[2]) > thresh) ? uint8_t(0xFF) : uint8_t(0);
            }
        }

        enum : size_t {
            SRC_BATCH_SIZE = sizeof(batch_t) * 3,
            DST_BATCH_SIZE = sizeof(batch_t),
        };

        AddAndThreshJob(const uint8_t* src, uint8_t* dst, int thresh)
            : mSrc(src), mDst(dst), mThresh(thresh) {}

        virtual void operator()(const cv::Range& pieces) const override {
            size_t firstSrc = size_t(pieces.start) * SRC_BATCH_SIZE;
            size_t lastSrc = size_t(pieces.end) * SRC_BATCH_SIZE;
            size_t firstDst = size_t(pieces.start) * DST_BATCH_SIZE;
            const uint8_t* src = mSrc + firstSrc;
            const uint8_t* srcEnd = mSrc + lastSrc;
            uint8_t* dst = mDst + firstDst;
            impl(src, srcEnd, dst, mThresh);
        }

    private:
        const uint8_t* const mSrc;
        uint8_t* const mDst;
        const int mThresh;
    };

    void addAndThresh(const Image& src, Image& dst, int thresh) {
        const Format format = src.format();
        const Dims dims = src.dims();
        const size_t pixels = size_t(dims.width) * size_t(dims.height);
        const size_t pieces = pixels / AddAndThreshJob::DST_BATCH_SIZE;

        if (getPixelStep(format) != 3) { throw std::runtime_error("addAndThresh(): bad format"); }

        // run the job in parallel
        dst.resize(Format::GRAY, dims);
        AddAndThreshJob job{src.data(), dst.data(), thresh};
        cv::parallel_for_(cv::Range{0, int(pieces)}, job, cv::getNumThreads());

        // process the last few bytes individually
        size_t lastIndex = pieces * AddAndThreshJob::DST_BATCH_SIZE;
        const uint8_t* data = src.data() + (lastIndex * 3);
        uint8_t* out = dst.data() + lastIndex;
        uint8_t* outEnd = dst.data() + pixels;
        for (; out < outEnd; out++, data += 3) {
            *out = ((data[0] + data[1] + data[2]) > thresh) ? uint8_t(0xFF) : uint8_t(0);
        }
    }

    void Differentiator::operator()(const Config& config, const Mat& src1, const Mat& src2,
                                    Image& dst, int adjust) {
        absdiff(src1, src2, mDiff);
        Format format = mDiff.format();

        switch (format) {
        case Format::GRAY: {
            uint8_t adjusted = uint8_t(int(config.threshGray) + adjust);
            greater_than(mDiff, dst, adjusted);
            return;
        }
        case Format::BGR:
        case Format::YUV: {
            bool bgr = format == Format::BGR;
            int thresh = bgr ? config.threshBgr : config.threshYuv;
            addAndThresh(mDiff, dst, thresh + adjust);
            return;
        }
        default:
            throw std::runtime_error("Differentiator: unsupported format");
        }
    }
}
