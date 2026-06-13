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

static const StarBuffers::Buffer *WriteFloat32(TIFF *tif, uint32_t width, uint32_t height,
                                                const ImageDataSource &dataSource,
                                                WriteTiffImageAction::Compression compressionOption)
{
    const float *floatData = nullptr;
    const StarBuffers::Buffer *bufferToUnmap = nullptr;

    if (auto *bufSrc = std::get_if<VulkanBufferSource>(&dataSource))
    {
        void *mapped = nullptr;
        bufSrc->buffer.map(&mapped);
        if (!mapped)
        {
            TIFFClose(tif);
            STAR_THROW("Failed to map buffer for TIFF image write");
        }
        bufSrc->buffer.invalidate();
        floatData = static_cast<const float *>(mapped);
        bufferToUnmap = &bufSrc->buffer;
    }
    else if (auto *rawSrc = std::get_if<RawFloatSource>(&dataSource))
    {
        floatData = rawSrc->data;
    }
    else
    {
        TIFFClose(tif);
        STAR_THROW("Float32 TIFF writing requires VulkanBufferSource or RawFloatSource data source");
    }

    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 32);
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);

    switch (compressionOption)
    {
    case (WriteTiffImageAction::Compression::none):
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
        break;
    case (WriteTiffImageAction::Compression::zstd):
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_ZSTD);
        TIFFSetField(tif, TIFFTAG_PREDICTOR, PREDICTOR_FLOATINGPOINT);
        break;
    case (WriteTiffImageAction::Compression::lzw):
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
        TIFFSetField(tif, TIFFTAG_PREDICTOR, PREDICTOR_FLOATINGPOINT);
        break;
    default:
        TIFFClose(tif);
        if (bufferToUnmap)
        {
            bufferToUnmap->unmap();
        }
        STAR_THROW("Invalid compression option encountered when attempting to write tif");
        break;
    }

    for (uint32_t row = 0; row < height; ++row)
    {
        void *rowPtr = (void *)(&floatData[row * width]);

        if (TIFFWriteScanline(tif, rowPtr, row, 0) < 0)
        {
            TIFFClose(tif);
            if (bufferToUnmap)
            {
                bufferToUnmap->unmap();
            }
            STAR_THROW("TIFFWriteScanline failed at row " + std::to_string(row));
        }
    }

    return bufferToUnmap;
}

static void WriteUint16(TIFF *tif, uint32_t width, uint32_t height, const ImageDataSource &dataSource,
                         WriteTiffImageAction::Compression compressionOption)
{
    const uint16_t *uint16Data = nullptr;

    if (auto *rawSrc = std::get_if<RawUint16Source>(&dataSource))
    {
        uint16Data = rawSrc->data;
    }
    else
    {
        TIFFClose(tif);
        STAR_THROW("Uint16 TIFF writing requires RawUint16Source data source");
    }

    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 16);
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);

    switch (compressionOption)
    {
    case (WriteTiffImageAction::Compression::none):
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
        break;
    case (WriteTiffImageAction::Compression::zstd):
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_ZSTD);
        TIFFSetField(tif, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL);
        break;
    case (WriteTiffImageAction::Compression::lzw):
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
        TIFFSetField(tif, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL);
        break;
    default:
        TIFFClose(tif);
        STAR_THROW("Invalid compression option encountered when attempting to write tif");
        break;
    }

    for (uint32_t row = 0; row < height; ++row)
    {
        void *rowPtr = (void *)(&uint16Data[row * width]);

        if (TIFFWriteScanline(tif, rowPtr, row, 0) < 0)
        {
            TIFFClose(tif);
            STAR_THROW("TIFFWriteScanline failed at row " + std::to_string(row));
        }
    }
}

static void WriteUint8(TIFF *tif, uint32_t width, uint32_t height, const ImageDataSource &dataSource,
                        WriteTiffImageAction::Compression compressionOption)
{
    const uint8_t *uint8Data = nullptr;

    if (auto *rawSrc = std::get_if<RawUint8Source>(&dataSource))
    {
        uint8Data = rawSrc->data;
    }
    else
    {
        TIFFClose(tif);
        STAR_THROW("Uint8 TIFF writing requires RawUint8Source data source");
    }

    TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);

    switch (compressionOption)
    {
    case (WriteTiffImageAction::Compression::none):
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
        break;
    case (WriteTiffImageAction::Compression::zstd):
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_ZSTD);
        TIFFSetField(tif, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL);
        break;
    case (WriteTiffImageAction::Compression::lzw):
        TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
        TIFFSetField(tif, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL);
        break;
    default:
        TIFFClose(tif);
        STAR_THROW("Invalid compression option encountered when attempting to write tif");
        break;
    }

    for (uint32_t row = 0; row < height; ++row)
    {
        void *rowPtr = (void *)(&uint8Data[row * width]);

        if (TIFFWriteScanline(tif, rowPtr, row, 0) < 0)
        {
            TIFFClose(tif);
            STAR_THROW("TIFFWriteScanline failed at row " + std::to_string(row));
        }
    }
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

    TIFF *tif = TIFFOpen(path.c_str(), "w");
    if (!tif)
    {
        STAR_THROW("Failed to open TIFF file for writing: " + path);
    }

    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, height);

    const StarBuffers::Buffer *bufferToUnmap = nullptr;

    if (precision == Precision::Float32)
    {
        bufferToUnmap = WriteFloat32(tif, width, height, dataSource, compressionOption);
    }
    else if (precision == Precision::Uint16)
    {
        WriteUint16(tif, width, height, dataSource, compressionOption);
    }
    else if (precision == Precision::Uint8)
    {
        WriteUint8(tif, width, height, dataSource, compressionOption);
    }

    TIFFClose(tif);

    if (bufferToUnmap)
    {
        bufferToUnmap->unmap();
    }
}

} // namespace star::job::tasks::actions