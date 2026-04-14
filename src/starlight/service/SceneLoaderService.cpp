#include "starlight/service/SceneLoaderService.hpp"

#include "starlight/command/CreateObject.hpp"
#include "starlight/common/ConfigFile.hpp"
#include "starlight/common/io/JSONFileWriter.hpp"
#include "starlight/service/detail/scene_loader/LightReader.hpp"
#include "starlight/service/detail/scene_loader/LightWriter.hpp"
#include "starlight/service/detail/scene_loader/ObjectReader.hpp"
#include "starlight/service/detail/scene_loader/ObjectUtils.hpp"
#include "starlight/service/detail/scene_loader/ObjectWriter.hpp"

#include <nlohmann/json.hpp>

namespace star::service
{
SceneLoaderService::SceneLoaderService()
    : m_objectTracker(), m_onCreate(*this), m_onSceneSave(*this), m_onCreateLight(*this)
{
}

SceneLoaderService::SceneLoaderService(SceneLoaderService &&other) noexcept
    : m_objectTracker(std::move(other.m_objectTracker)), m_onCreate(*this), m_onSceneSave(*this),
      m_onCreateLight(*this), m_deviceCommandBus(other.m_deviceCommandBus)
{

    if (m_deviceCommandBus != nullptr)
    {
        other.cleanup();
        initListeners(*m_deviceCommandBus);
    }
}

SceneLoaderService &SceneLoaderService::operator=(SceneLoaderService &&other) noexcept
{
    if (this != &other)
    {
        m_objectTracker = std::move(other.m_objectTracker);
        m_deviceCommandBus = other.m_deviceCommandBus;
        if (m_deviceCommandBus != nullptr)
        {
            other.cleanup();
            initListeners(*m_deviceCommandBus);
        }
    }

    return *this;
}

void SceneLoaderService::init()
{
    assert(m_deviceCommandBus != nullptr);

    initListeners(*m_deviceCommandBus);
}

void SceneLoaderService::setInitParameters(star::service::InitParameters &params)
{
    m_deviceCommandBus = &params.commandBus;
}

void SceneLoaderService::shutdown()
{
    if (m_deviceCommandBus != nullptr)
    {
        cleanupListeners(*m_deviceCommandBus);
    }
}

void SceneLoaderService::cleanup()
{
    if (m_deviceCommandBus != nullptr)
    {
        cleanupListeners(*m_deviceCommandBus);
    }
}

void SceneLoaderService::cleanupListeners(core::CommandBus &commandBus) noexcept
{
    m_onCreate.cleanup(commandBus);
    m_onSceneSave.cleanup(commandBus);
}

std::optional<nlohmann::json> TryReadFile(const std::string &path)
{
    // Load file
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs)
        return std::nullopt;

    nlohmann::json root;
    try
    {
        ifs >> root;
    }
    catch (const std::exception &ex)
    {
        return std::nullopt;
    }

    // Navigate to /Scene/objects/<name>
    if (!root.contains("Scene") || !root["Scene"].contains("Objects"))
        return std::nullopt;

    return root;
}

void SceneLoaderService::onCreateObject(command::CreateObject &event)
{
    const auto &uniqueName = event.getUniqueName();
    if (m_objectTracker.contains(uniqueName))
    {
        STAR_THROW("Duplicate key provided for create object. Every object needs to have its own unique key. " +
                   uniqueName);
    }

    auto newObject = event.load();
    newObject->createInstance();

    m_objectTracker.insert(std::make_pair(uniqueName, newObject));
    auto data = TryReadFile(star::ConfigFile::getSetting(star::Config_Settings::scene_file));
    if (data.has_value())
    {
        auto reader = scene_loader::ObjectLoader();
        const auto &objectData = data.value()["Scene"]["Objects"];

        if (reader.canLoad(objectData, uniqueName))
        {
            reader.load(objectData, uniqueName, *newObject);
        }
        else
        {
            star::core::logging::info("Requested object key does not exist in scene file : " + uniqueName);
        }
    }
    else
    {
        star::core::logging::info("Provided scene file is not valid: " +
                                  star::ConfigFile::getSetting(star::Config_Settings::scene_file));
    }

    event.getReply().set(newObject);
}

void SceneLoaderService::onSaveSceneState(command::SaveSceneState &event)
{
    (void)event;

    nlohmann::json root = nlohmann::json::object();
    root["Scene"] = nlohmann::json::object();
    root["Scene"]["Objects"] = nlohmann::json::object();
    for (const auto &ele : m_objectTracker)
    {
        const std::string &name = ele.first;
        const auto &obj = ele.second;
        const auto writer = scene_loader::ObjectWriter();
        auto data = writer.write(*obj);

        root["Scene"]["Objects"][name] = std::move(data);
    }

    for (const auto &ele : m_lightTracker)
    {
        const std::string &name = ele.first;
        const auto &light = ele.second;

        scene_loader::LightWriter writer;
        auto data = writer.write(*light);

        root["Scene"]["Lights"][name] = std::move(data);
    }

    auto writerPayload = star::common::io::JSONFileWriter(std::move(root));
    writerPayload(star::ConfigFile::getSetting(star::Config_Settings::scene_file));
}

void SceneLoaderService::onCreateLight(star::command::CreateLight &cmd)
{
    const auto &name = cmd.getName();
    if (m_lightTracker.contains(name))
    {
        STAR_THROW("Duplicate unique name for light detected");
    }
    auto newLight = std::make_shared<std::vector<Light>>();
    command::create_light::SceneAddResult result{command::create_light::fail};

    auto data = TryReadFile(star::ConfigFile::getSetting(star::Config_Settings::scene_file));
    if (data.has_value())
    {
        const auto &sData = data.value()["Scene"];
        if (sData.contains("Lights"))
        {
            const auto &lData = sData["Lights"];

            const scene_loader::LightReader reader;
            if (reader.canLoad(name, lData))
            {
                result = command::create_light::success;
                reader.load(lData, name, *newLight);
            }
        }
    }

    m_lightTracker[name] = newLight;

    cmd.getReply().set(std::make_pair<command::create_light::SceneAddResult, std::shared_ptr<std::vector<Light>>>(
        std::move(result), std::move(newLight)));
}

void SceneLoaderService::initListeners(core::CommandBus &commandBus) noexcept
{
    m_onCreate.init(commandBus);
    m_onSceneSave.init(commandBus);
    m_onCreateLight.init(commandBus);
}
} // namespace star::service