// #include "DefaultFinalRenderControlPolicy.hpp"

// #include "ConfigFile.hpp"

// namespace star
// {

// void star::DefaultFinalRenderControlPolicy::init(StarApplication &application)
// {
// }

// std::unique_ptr<star::StarWindow> star::DefaultFinalRenderControlPolicy::CreateStarWindow()
// {
//     return StarWindow::Builder()
//         .setWidth(std::stoi(ConfigFile::getSetting(star::Config_Settings::resolution_x)))
//         .setHeight(std::stoi(ConfigFile::getSetting(star::Config_Settings::resolution_y)))
//         .setTitle(ConfigFile::getSetting(star::Config_Settings::app_name))
//         .build();
// }
// } // namespace star