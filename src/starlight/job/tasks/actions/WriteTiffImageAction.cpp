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

    const float *floatData = nullptr;
    bool needsUnmap = false;
    const StarBuffers::Buffer *bufferToUnmap = nullptr;

    if (auto *bufSrc = std::get_if<VulkanBufferSource>(&dataSource))
    {
        void *mapped = nullptr;
        bufSrc->buffer.map(&mapped);
        if (!mapped)
        {
            STAR_THROW("Failed to map buffer for TIFF image write");
        }
        bufSrc->buffer.invalidate();
        floatData = static_cast<const float *>(mapped);
        needsUnmap = true;
        bufferToUnmap = &bufSrc->buffer;
    }
    else if (auto *rawSrc = std::get_if<RawFloatSource>(&dataSource))
    {
        floatData = rawSrc->data;
    }
    else
    {
        STAR_THROW("TIFF writing requires VulkanBufferSource or RawFloatSource data source");
    }

    TIFF *tif = TIFFOpen(path.c_str(), "w");
    if (!tif)
    {
        if (needsUnmap)
        {
            bufferToUnmap->unmap();
        }
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
            if (needsUnmap)
            {
                bufferToUnmap->unmap();
            }
            STAR_THROW("TIFFWriteScanline failed at row " + std::to_string(row));
        }
    }

    TIFFClose(tif);

    if (needsUnmap)
    {
        bufferToUnmap->unmap();
    }
}

} // namespace star::job::tasks::actions