#include <algorithm>
#include <fmo/assert.hpp>
#include <fmo/image.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace fmo {
    namespace {
        /// Get the number of bytes of data that an image requires, given its format and dimensions.
        size_t getBytes(Image::Format format, Image::Size size) {
            size_t result = static_cast<size_t>(size.width) * static_cast<size_t>(size.height);

            switch (format) {
            case Image::Format::BGR:
                result *= 3;
                break;
            case Image::Format::GRAY:
                break;
            case Image::Format::YUV420SP:
                result = (result * 3) / 2;
                break;
            default:
                throw std::runtime_error("getBytes: unsupported format");
            }

            return result;
        }

        /// Convert the actual size to the size that is used by OpenCV. OpenCV considers YUV 4:2:0
        /// SP images 1.5x taller.
        cv::Size getCvSize(Image::Format format, Image::Size size) {
            cv::Size result{size.width, size.height};

            switch (format) {
            case Image::Format::BGR:
            case Image::Format::GRAY:
                break;
            case Image::Format::YUV420SP:
                result.height = (result.height * 3) / 2;
                break;
            default:
                throw std::runtime_error("getCvSize: unsupported format");
            }

            return result;
        }

        /// Convert the size used by OpenCV to the actual size. OpenCV considers YUV 4:2:0 SP images
        /// 1.5x taller.
        Image::Size getImageSize(Image::Format format, cv::Size size) {
            Image::Size result{size.width, size.height};

            switch (format) {
            case Image::Format::BGR:
            case Image::Format::GRAY:
                break;
            case Image::Format::YUV420SP:
                result.height = (result.height * 2) / 3;
                break;
            default:
                throw std::runtime_error("getImageSize: unsupported format");
            }

            return result;
        }

        /// Get the Mat data type used by OpenCV that corresponds to the format.
        int getCvType(Image::Format format) {
            switch (format) {
            case Image::Format::BGR:
                return CV_8UC3;
            case Image::Format::GRAY:
            case Image::Format::YUV420SP:
                return CV_8UC1;
            default:
                throw std::runtime_error("getCvType: unsupported format");
            }
        }
    }

    Image::Image(Image&& rhs) : Image() { swap(rhs); }

    Image& Image::operator=(Image&& rhs) {
        swap(rhs);
        return *this;
    }

    Image::Image(const std::string& filename, Format format) {
        cv::Mat mat;

        switch (format) {
        case Format::BGR:
            mat = cv::imread(filename, cv::IMREAD_COLOR);
            break;
        case Format::GRAY:
            mat = cv::imread(filename, cv::IMREAD_GRAYSCALE);
            break;
        default:
            throw std::runtime_error("reading image: unsupported format");
            break;
        }

        if (mat.data == nullptr) { throw std::runtime_error("failed to open image"); }

        FMO_ASSERT(mat.isContinuous(), "reading image: not continuous")
        FMO_ASSERT(mat.type() == getCvType(format), "reading image: unexpected mat type");
        Size size = getImageSize(format, mat.size());
        size_t bytes = mat.elemSize() * mat.total();
        FMO_ASSERT(getBytes(format, size) == bytes, "reading image: unexpected size");
        mData.resize(bytes);
        std::copy(mat.data, mat.data + mData.size(), mData.data());
        mFormat = format;
        mSize = size;
    }

    void Image::clear() {
        mData.clear();
        mSize = {0, 0};
        mFormat = Format::UNKNOWN;
    }

    cv::Mat Image::resize(Format format, Size size) {
        size_t bytes = getBytes(format, size);
        mData.resize(bytes);
        return cv::Mat{getCvSize(format, size), getCvType(format), data()};
    }

    cv::Mat Image::wrap() const {
        auto* ptr = const_cast<uint8_t*>(data());
        return cv::Mat{getCvSize(mFormat, mSize), getCvType(mFormat), ptr};
    }

    void Image::swap(Image& rhs) {
        mData.swap(rhs.mData);
        std::swap(mSize, rhs.mSize);
        std::swap(mFormat, rhs.mFormat);
    }

    void swap(Image& lhs, Image& rhs) { lhs.swap(rhs); }

    void Image::convert(const Image& src, Image& dest, const Format format) {
        if (src.mFormat == format) {
            // no format change -- just copy
            dest = src;
            return;
        }

        if (&src == &dest) {
            if (src.mFormat == Format::YUV420SP && format == Format::GRAY) {
                // same instance and converting YUV420SP to GRAY: easy case
                size_t bytes = getBytes(Format::GRAY, dest.mSize);
                dest.mData.resize(bytes);
                dest.mFormat = Format::GRAY;
                return;
            }

            // same instance: convert into a new, temporary Image, then move into dest
            Image temp;
            convert(src, temp, format);
            dest = std::move(temp);
            return;
        }

        enum class Status {
            ERROR,
            GOOD,
        } status = Status::ERROR;

        cv::Mat srcMat = src.wrap();
        cv::Mat destMat = dest.resize(format, src.mSize);

        switch (src.mFormat) {
        case Format::BGR:
            if (format == Format::GRAY) {
                cv::cvtColor(srcMat, destMat, cv::COLOR_BGR2GRAY, 1);
                status = Status::GOOD;
            }
            break;
        case Format::GRAY:
            if (format == Format::BGR) {
                cv::cvtColor(srcMat, destMat, cv::COLOR_GRAY2BGR, 3);
                status = Status::GOOD;
            }
            break;
        case Format::YUV420SP:
            if (format == Format::BGR) {
                cv::cvtColor(srcMat, destMat, cv::COLOR_YUV420sp2BGR, 3);
                status = Status::GOOD;
            } else if (format == Format::GRAY) {
                std::copy(srcMat.data, srcMat.data + dest.mData.size(), destMat.data);
                status = Status::GOOD;
            }
            break;
        default:
            break;
        }

        switch (status) {
        case Status::GOOD:
            dest.mFormat = format;
            dest.mSize = src.mSize;
            break;
        default:
            throw std::runtime_error("failed to perform color conversion");
        }
    }
}