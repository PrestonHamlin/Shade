#pragma once

#include "Util.h"
#include "Dx12RenderEngine.h"
//#include <d3dcompiler.h>
#include "imported/dxc/dxcapi.h"

#include "imported/dxc/DxilConstants.h"


using ShaderKind = hlsl::DXIL::ShaderKind;


class Shader
{
public:
    Shader();
    ~Shader();

    HRESULT Compile(std::string filename, const std::string entry, const std::string target);

    ID3DBlob* GetBlob() const      {return m_pBlob.Get();};
    const IDxcBlobUtf8* GetErrorBlob() const {return m_pErrors.Get();};

private:
    // TODO: static compiler instance, or shader cache/manager
    // compiler and compiler utilities instances
    ComPtr<IDxcCompiler3>   m_pCompiler;
    ComPtr<IDxcUtils>       m_pUtils;

    std::string             m_filename;
    std::string             m_shaderText;
    std::string             m_entrypoint;


    ComPtr<ID3DBlob>        m_pBlob;
    ComPtr<IDxcBlobUtf8>    m_pErrors;
    bool m_compiled;
};
