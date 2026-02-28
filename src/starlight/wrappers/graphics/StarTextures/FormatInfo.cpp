#include "StarTextures/FormatInfo.hpp"

#include <star_common/helper/CastHelpers.hpp>

namespace star::StarTextures
{
FormatInfo FormatInfo::Create(const vk::Format &fmt)
{
    using F = vk::Format;

    // ===========
    // COMPRESSED:
    // ===========

    // --- BC1/BC2/BC3/BC4/BC5/BC6H/BC7 ---
    switch (fmt)
    {
    // BC1: 4x4, 8 bytes
    case F::eBc1RgbUnormBlock:
    case F::eBc1RgbSrgbBlock:
    case F::eBc1RgbaUnormBlock:
    case F::eBc1RgbaSrgbBlock:
        return FormatInfo{true, 0, vk::Extent3D{4, 4, 1}, 8};

    // BC2/BC3: 4x4, 16 bytes
    case F::eBc2UnormBlock:
    case F::eBc2SrgbBlock:
    case F::eBc3UnormBlock:
    case F::eBc3SrgbBlock:
        return FormatInfo{true, 0, vk::Extent3D{4, 4, 1}, 16};

    // BC4: 4x4, 8 bytes
    case F::eBc4UnormBlock:
    case F::eBc4SnormBlock:
        return FormatInfo{true, 0, vk::Extent3D{4, 4, 1}, 8};

    // BC5: 4x4, 16 bytes
    case F::eBc5UnormBlock:
    case F::eBc5SnormBlock:
        return FormatInfo{true, 0, vk::Extent3D{4, 4, 1}, 16};

    // BC6H: 4x4, 16 bytes
    case F::eBc6HUfloatBlock:
    case F::eBc6HSfloatBlock:
        return FormatInfo{true, 0, vk::Extent3D{4, 4, 1}, 16};

    // BC7: 4x4, 16 bytes
    case F::eBc7UnormBlock:
    case F::eBc7SrgbBlock:
        return FormatInfo{true, 0, vk::Extent3D{4, 4, 1}, 16};

    default:
        break;
    }

    // --- ETC2 / EAC: 4x4 blocks, 8 or 16 bytes ---
    switch (fmt)
    {
    // ETC2 RGB: 8 bytes
    case F::eEtc2R8G8B8UnormBlock:
    case F::eEtc2R8G8B8SrgbBlock:
        return FormatInfo{true, 0, vk::Extent3D{4, 4, 1}, 8};

    // ETC2 RGBA (A1 or A8): 16 bytes
    case F::eEtc2R8G8B8A1UnormBlock:
    case F::eEtc2R8G8B8A1SrgbBlock:
    case F::eEtc2R8G8B8A8UnormBlock:
    case F::eEtc2R8G8B8A8SrgbBlock:
        return FormatInfo{true, 0, vk::Extent3D{4, 4, 1}, 16};

    // EAC R11: 8 bytes
    case F::eEacR11UnormBlock:
    case F::eEacR11SnormBlock:
        return FormatInfo{true, 0, vk::Extent3D{4, 4, 1}, 8};

    // EAC RG11: 16 bytes
    case F::eEacR11G11UnormBlock:
    case F::eEacR11G11SnormBlock:
        return FormatInfo{true, 0, vk::Extent3D{4, 4, 1}, 16};

    default:
        break;
    }

    // --- ASTC: blocks are always 16 bytes; block extent varies ---
    switch (fmt)
    {
    case F::eAstc4x4UnormBlock:
    case F::eAstc4x4SrgbBlock:
        return FormatInfo{true, 0, vk::Extent3D{4, 4, 1}, 16};
    case F::eAstc5x4UnormBlock:
    case F::eAstc5x4SrgbBlock:
        return FormatInfo{true, 0, vk::Extent3D{5, 4, 1}, 16};
    case F::eAstc5x5UnormBlock:
    case F::eAstc5x5SrgbBlock:
        return FormatInfo{true, 0, vk::Extent3D{5, 5, 1}, 16};
    case F::eAstc6x5UnormBlock:
    case F::eAstc6x5SrgbBlock:
        return FormatInfo{true, 0, vk::Extent3D{6, 5, 1}, 16};
    case F::eAstc6x6UnormBlock:
    case F::eAstc6x6SrgbBlock:
        return FormatInfo{true, 0, vk::Extent3D{6, 6, 1}, 16};
    case F::eAstc8x5UnormBlock:
    case F::eAstc8x5SrgbBlock:
        return FormatInfo{true, 0, vk::Extent3D{8, 5, 1}, 16};
    case F::eAstc8x6UnormBlock:
    case F::eAstc8x6SrgbBlock:
        return FormatInfo{true, 0, vk::Extent3D{8, 6, 1}, 16};
    case F::eAstc8x8UnormBlock:
    case F::eAstc8x8SrgbBlock:
        return FormatInfo{true, 0, vk::Extent3D{8, 8, 1}, 16};
    case F::eAstc10x5UnormBlock:
    case F::eAstc10x5SrgbBlock:
        return FormatInfo{true, 0, vk::Extent3D{10, 5, 1}, 16};
    case F::eAstc10x6UnormBlock:
    case F::eAstc10x6SrgbBlock:
        return FormatInfo{true, 0, vk::Extent3D{10, 6, 1}, 16};
    case F::eAstc10x8UnormBlock:
    case F::eAstc10x8SrgbBlock:
        return FormatInfo{true, 0, vk::Extent3D{10, 8, 1}, 16};
    case F::eAstc10x10UnormBlock:
    case F::eAstc10x10SrgbBlock:
        return FormatInfo{true, 0, vk::Extent3D{10, 10, 1}, 16};
    case F::eAstc12x10UnormBlock:
    case F::eAstc12x10SrgbBlock:
        return FormatInfo{true, 0, vk::Extent3D{12, 10, 1}, 16};
    case F::eAstc12x12UnormBlock:
    case F::eAstc12x12SrgbBlock:
        return FormatInfo{true, 0, vk::Extent3D{12, 12, 1}, 16};
    default:
        break;
    }

    // ===========
    // UNCOMPRESSED:
    // ===========

    // Common 8-bit formats
    switch (fmt)
    {
    case F::eR8Unorm:
    case F::eR8Snorm:
    case F::eR8Uint:
    case F::eR8Sint:
    case F::eR8Srgb:
        return FormatInfo{false, 1, {1, 1, 1}, 0};

    case F::eR8G8Unorm:
    case F::eR8G8Snorm:
    case F::eR8G8Uint:
    case F::eR8G8Sint:
    case F::eR8G8Srgb:
        return FormatInfo{false, 2, {1, 1, 1}, 0};

    // Note: true 24-bit formats exist; when uploading you may want to pad row to 4 bytes.
    case F::eR8G8B8Unorm:
    case F::eR8G8B8Snorm:
    case F::eR8G8B8Uint:
    case F::eR8G8B8Sint:
    case F::eR8G8B8Srgb:
    case F::eB8G8R8Unorm:
    case F::eB8G8R8Snorm:
    case F::eB8G8R8Uint:
    case F::eB8G8R8Sint:
    case F::eB8G8R8Srgb:
        return FormatInfo{false, 3, {1, 1, 1}, 0};

    case F::eR8G8B8A8Unorm:
    case F::eR8G8B8A8Snorm:
    case F::eR8G8B8A8Uint:
    case F::eR8G8B8A8Sint:
    case F::eR8G8B8A8Srgb:
    case F::eB8G8R8A8Unorm:
    case F::eB8G8R8A8Srgb:
        return FormatInfo{false, 4, {1, 1, 1}, 0};

    // 16-bit per component
    case F::eR16Unorm:
    case F::eR16Snorm:
    case F::eR16Uint:
    case F::eR16Sint:
    case F::eR16Sfloat:
        return FormatInfo{false, 2, {1, 1, 1}, 0};
    case F::eR16G16Unorm:
    case F::eR16G16Snorm:
    case F::eR16G16Uint:
    case F::eR16G16Sint:
    case F::eR16G16Sfloat:
        return FormatInfo{false, 4, {1, 1, 1}, 0};
    case F::eR16G16B16Unorm:
    case F::eR16G16B16Snorm:
    case F::eR16G16B16Uint:
    case F::eR16G16B16Sint:
    case F::eR16G16B16Sfloat:
        return FormatInfo{false, 6, {1, 1, 1}, 0};
        break;
    case F::eR16G16B16A16Unorm:
    case F::eR16G16B16A16Snorm:
    case F::eR16G16B16A16Uint:
    case F::eR16G16B16A16Sint:
    case F::eR16G16B16A16Sfloat:
        return FormatInfo{false, 8, {1, 1, 1}, 0};
        break;

    // 32-bit per component
    case F::eR32Uint:
    case F::eR32Sint:
    case F::eR32Sfloat:
        return FormatInfo{false, 4, {1, 1, 1}, 0};
        break;
    case F::eR32G32Uint:
    case F::eR32G32Sint:
    case F::eR32G32Sfloat:
        return FormatInfo{false, 8, {1, 1, 1}, 0};
        break;
    case F::eR32G32B32Uint:
    case F::eR32G32B32Sint:
    case F::eR32G32B32Sfloat:
        return FormatInfo{false, 12, {1, 1, 1}, 0};
        break;
    case F::eR32G32B32A32Uint:
    case F::eR32G32B32A32Sint:
    case F::eR32G32B32A32Sfloat:
        return FormatInfo{false, 16, {1, 1, 1}, 0};
        break;

    // Packed 10:10:10:2 and similar
    case F::eA2R10G10B10UnormPack32:
    case F::eA2R10G10B10UintPack32:
    case F::eA2B10G10R10UnormPack32:
    case F::eA2B10G10R10UintPack32:
        return FormatInfo{false, 4, {1, 1, 1}, 0};
        break;

    // sRGB BGRA already handled above; add any other formats you use similarly.
    default:
        break;
    }

    // Depth / Stencil — note that copy behavior can be aspect-dependent.
    switch (fmt)
    {
    case F::eD16Unorm:
        return FormatInfo{false, 2, {1, 1, 1}, 0};
        break;
    case F::eX8D24UnormPack32:
    case F::eD32Sfloat:
        return FormatInfo{false, 4, {1, 1, 1}, 0};
        break;
    case F::eD24UnormS8Uint:
        return FormatInfo{false, 4, {1, 1, 1}, 0}; // 32-bit packed
        break;
    case F::eD32SfloatS8Uint:
        return FormatInfo{false, 8, {1, 1, 1}, 0}; // implementation-defined packing when copying both aspects
        break;
    default:
        break;
    }

    throw std::runtime_error("Unsupported format provided to return FormatInfo");
}
} // namespace star::StarTextures