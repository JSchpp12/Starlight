#include "job/tasks/actions/WriteTiffImageAction.hpp"

#include "job/tasks/actions/WriteImageActions.hpp"

#include "logging/LoggingFactory.hpp"

#include "starlight/core/Exceptions.hpp"

#include <filesystem>
#include <tiffio.h>

namespace star::job::tasks::actions
{

bool IsTiffFormat(vk::Format fmt)
{
    return fmt == vk::Format::eR32Sfloat;
}

void WriteTiffImageAction::operator()()
{
    ValidateExtension(path, ".tif");

    if (!IsTiffFormat(imageFormat))
    {
        STAR_THROW("Unsupported image format for TIFF writing. Only eR32Sfloat is supported.");
    }

    const uint32_t width = imageExtent.width;
    const uint32_t height = imageExtent.height;

    void *mapped = nullptr;
    buffer.map(&mapped);
    if (!mapped)
    {
        STAR_THROW("Failed to map buffer for TIFF image write");
    }
    const auto result = buffer.invalidate();

    const float *floatData = static_cast<const float *>(mapped);

    TIFF *tif = TIFFOpen(path.c_str(), "w");
    if (!tif)
    {
        buffer.unmap();
        STAR_THROW("Failed to open TIFF file for writing: " + path);
    }

    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 32);
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, height);

    for (uint32_t row = 0; row < height; ++row)
    {
        if (TIFFWriteScanline(tif, const_cast<void *>(static_cast<const void *>(floatData + row * width)), row,
                              0) < 0)
        {
            TIFFClose(tif);
            buffer.unmap();
            STAR_THROW("TIFFWriteScanline failed at row " + std::to_string(row));
        }
    }

    TIFFClose(tif);
    buffer.unmap();
}

} // namespace star::job::tasks::actions