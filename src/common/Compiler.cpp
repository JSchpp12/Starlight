#include "Compiler.hpp"

namespace star {

#ifdef NDEBUG
    bool Compiler::compileDebug = false;
#else
    bool Compiler::compileDebug = true;
#endif

    std::unique_ptr<std::vector<uint32_t>> Compiler::compile(const std::string& pathToFile, bool optimize) {
        shaderc::Compiler shaderCompiler;
        shaderc::CompileOptions compilerOptions;

        if (compileDebug)
            compilerOptions.SetGenerateDebugInfo(); 

        auto stageC = getShaderCStageFlag(pathToFile);
        auto name = FileHelpers::GetFileNameWithExtension(pathToFile);
        auto fileCode = FileHelpers::ReadFile(pathToFile, true);

        std::string preprocessed = preprocessShader(name, stageC, fileCode.c_str());

        shaderc::SpvCompilationResult compileResult = shaderCompiler.CompileGlslToSpv(preprocessed.c_str(), stageC, name.c_str(), compilerOptions);

        if (compileResult.GetCompilationStatus() != shaderc_compilation_status_success) {
            std::cerr << compileResult.GetErrorMessage();
            throw std::runtime_error("Failed to compile shader");
        }
        return std::make_unique<std::vector<uint32_t>>(std::vector<uint32_t>{ compileResult.cbegin(), compileResult.cend() });
    }

    shaderc_shader_kind Compiler::getShaderCStageFlag(const std::string& pathToFile) {

        auto extension = FileHelpers::GetFileExtension(pathToFile);

        if (extension == ".vert") {
            return shaderc_shader_kind::shaderc_vertex_shader;
        }
        else if (extension == ".frag") {
            return shaderc_shader_kind::shaderc_fragment_shader;
        }
        else if (extension == ".comp") {
            return shaderc_shader_kind::shaderc_compute_shader;
        }

        throw std::runtime_error("Compiler::GetShaderCStageFlag invalid shader stage");
    }

    std::string Compiler::preprocessShader(const std::string& sourceName, shaderc_shader_kind stage, const std::string& source) {
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;

        // Like -DMY_DEFINE=1
        options.AddMacroDefinition("MY_DEFINE", "1");

        shaderc::PreprocessedSourceCompilationResult result =
            compiler.PreprocessGlsl(source, stage, sourceName.c_str(), options);

        if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
            std::cerr << result.GetErrorMessage();
            return "";
        }

        return { result.cbegin(), result.cend() };
    }
}