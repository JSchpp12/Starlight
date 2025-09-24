#include "ManagerController_RenderResource_TextureFile.hpp"

#include "FileHelpers.hpp"
#include "SharedCompressedTexture.hpp"
#include "TransferRequest_CompressedTextureFile.hpp"
#include "TransferRequest_TextureFile.hpp"


#include <boost/filesystem.hpp>

star::ManagerController::RenderResource::TextureFile::TextureFile(const std::string &nFilePath)
{
    this->filePath = GetFilePath(nFilePath);
    this->isCompressedTexture = IsCompressedFileType(this->filePath);
}

std::unique_ptr<star::TransferRequest::Texture> star::ManagerController::RenderResource::TextureFile::
    createTransferRequest(star::core::device::StarDevice &device)
{

    std::shared_ptr<star::SharedCompressedTexture> compressedTexture =
        std::make_shared<star::SharedCompressedTexture>(this->filePath, device.getPhysicalDevice());

    if (this->isCompressedTexture)
    {
        return std::make_unique<TransferRequest::CompressedTextureFile>(
            device.getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex(),
            device.getPhysicalDevice().getProperties(), compressedTexture, 0);
    }
    else
    {
        return std::make_unique<TransferRequest::TextureFile>(
            this->filePath, device.getDefaultQueue(star::Queue_Type::Tgraphics).getParentQueueFamilyIndex(),
            device.getPhysicalDevice().getProperties());
    }
}

bool star::ManagerController::RenderResource::TextureFile::IsCompressedFileType(const std::string &filePath)
{
    return file_helpers::GetFileExtension(filePath) == ".ktx2";
}

std::string star::ManagerController::RenderResource::TextureFile::GetFilePath(const std::string &filePath)
{
    if (file_helpers::FileExists(filePath))
    {
        return filePath;
    }

    const std::string name = file_helpers::GetFileNameWithoutExtension(filePath);

    boost::filesystem::path parentDir = boost::filesystem::path(filePath);
    auto result = file_helpers::FindFileInDirectoryWithSameNameIgnoreFileType(parentDir.parent_path().string(), name);

    if (!result.has_value()){
        std::ostringstream oss; 
        oss << "Failed to find matching file for texture copy: " << filePath; 
        throw std::runtime_error(oss.str()); 
    }

    return result.value();
}
