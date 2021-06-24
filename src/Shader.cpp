#include "Shader.h"


Shader::Shader() :
    m_pBlob(nullptr),
    m_pErrors(nullptr)
{
    if (m_pCompiler == nullptr)
    {
        CheckResult(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_pCompiler)), "Shader Compiler Instance");
        CheckResult(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_pUtils)), "DXC Utils Instance");
    }
}

Shader::~Shader()
{

}


HRESULT Shader::Compile(std::string filename, const std::string entry, const std::string target)
{
    HRESULT result = S_OK;

    // load shader text
    m_filename = filename;
    std::ifstream ifile(m_filename);
    if (!ifile.good()) PrintMessage(Error, "Shader {} could not be opened", filename);
    std::stringstream buffer;
    buffer << ifile.rdbuf();
    m_shaderText = buffer.str();

    // prepare input and output args
    ComPtr<IDxcResult> pResults;
    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr = m_shaderText.c_str();
    sourceBuffer.Size = m_shaderText.size();
    sourceBuffer.Encoding = CP_UTF8;
    const std::wstring wideEntry = ToWideString(entry);
    const std::wstring wideTarget = ToWideString(target);
    LPCWSTR compileArgs[] =
    {
        L"-E", wideEntry.c_str(),
        L"-T", wideTarget.c_str(),
        //L"shaders.hlsl",
        //L"-E", L"PSMain",
        //L"-T", L"ps_6_0",
        //L"-Zi"
    };
    result = m_pCompiler->Compile(&sourceBuffer, compileArgs, _countof(compileArgs), nullptr, IID_PPV_ARGS(&pResults));

    if (pResults != nullptr) // check for compilation outputs
    {
        ComPtr<IDxcBlobUtf16> pOutputName;
        pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&m_pErrors), &pOutputName);
    }
    if (m_pErrors != nullptr && m_pErrors->GetStringLength() != 0) // check for errors or warnings
    {
        if (SUCCEEDED(result))
        {
            PrintMessage(Warning, "{} {} {}:\n {}", filename, entry, target, m_pErrors->GetStringPointer());
        }
        else
        {
            PrintMessage(Error, "Shader compilation failed!\n {}", m_pErrors->GetStringPointer());
        }
    }
    if (SUCCEEDED(result)) // save shader blob
    {
        // TODO: PDB blob required for -Zi
        pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&m_pBlob), nullptr);
        m_compiled = true;
    }

    return result;
}
