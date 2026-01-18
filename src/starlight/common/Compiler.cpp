#include "Compiler.hpp"

#include "core/graphics/shader/BasicIncluder.hpp"
#include "logging/LoggingFactory.hpp"
#include "starlight/core/Exceptions.hpp"

namespace star
{

#ifdef NDEBUG
bool Compiler::compileDebug = false;
#else
bool Compiler::compileDebug = true;
#endif

std::vector<uint32_t> Compiler::compile(const std::string &pathToFile, bool optimize)
{
    shaderc::Compiler shaderCompiler;
    shaderc::CompileOptions compilerOptions = getCompileOptions(pathToFile);

    auto stageC = getShaderCStageFlag(pathToFile);
    auto name = file_helpers::GetFullPath(pathToFile);
    auto fileCode = file_helpers::ReadFile(pathToFile, true);

    std::string preprocessed = preprocessShader(shaderCompiler, compilerOptions, name, stageC, fileCode.c_str());

    shaderc::SpvCompilationResult compileResult =
        shaderCompiler.CompileGlslToSpv(preprocessed.c_str(), stageC, name.c_str(), compilerOptions);

    if (compileResult.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        std::ostringstream oss;
        oss << "Failed to compile shader with error: " << compileResult.GetErrorMessage() << std::endl;
        STAR_THROW(oss.str());
    }
    return std::vector<uint32_t>{compileResult.cbegin(), compileResult.cend()};
}

shaderc_shader_kind Compiler::getShaderCStageFlag(const std::string &pathToFile)
{
    auto extension = file_helpers::GetFileExtension(pathToFile);

    if (extension == ".vert")
    {
        return shaderc_shader_kind::shaderc_vertex_shader;
    }
    else if (extension == ".frag")
    {
        return shaderc_shader_kind::shaderc_fragment_shader;
    }
    else if (extension == ".comp")
    {
        return shaderc_shader_kind::shaderc_compute_shader;
    }
    else if (extension == ".geom")
    {
        return shaderc_shader_kind::shaderc_geometry_shader;
    }

    throw std::runtime_error("Compiler::GetShaderCStageFlag invalid shader stage");
}

std::string Compiler::preprocessShader(shaderc::Compiler &compiler, shaderc::CompileOptions options,
                                       const std::string &sourceName, shaderc_shader_kind stage,
                                       const std::string &source)
{
    // Like -DMY_DEFINE=1
    // options.AddMacroDefinition("MY_DEFINE", "1");

    shaderc::PreprocessedSourceCompilationResult result =
        compiler.PreprocessGlsl(source, stage, sourceName.c_str(), options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        std::cerr << result.GetErrorMessage();
        throw std::runtime_error("Failed to preprocess shader");
    }

    return {result.cbegin(), result.cend()};
}

shaderc::CompileOptions Compiler::getCompileOptions(const std::string &filePath)
{
    shaderc::CompileOptions options;

    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
    options.SetTargetSpirv(shaderc_spirv_version_1_6);
    if (!m_precompilerMacros.empty())
    {
        options.AddMacroDefinition(m_precompilerMacros);
    }

    if (compileDebug)
        options.SetGenerateDebugInfo();

    boost::filesystem::path parent;
    try
    {
        parent = file_helpers::GetParentDirectory(filePath).value();
    }
    catch (const std::exception &e)
    {
        core::logging::log(boost::log::trivial::error, "Unable to find parent directory. Includes may not function");
        parent = filePath;
    }
    options.SetIncluder(
        std::make_unique<core::graphics::shader::BasicIncluder>(std::vector<std::string>{parent.string()}));

    return options;
}
} // namespace star