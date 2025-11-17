#include "d3d12_kernel.h"

#if defined(AEGIS_ENABLE_D3D12)
#include <d3d12shader.h> // For reflection
#include <stdexcept>
#include <fstream>
#include <vector>

namespace aegis::internal {
   D3D12Kernel::D3D12Kernel(D3D12Backend *backend, ComPtr<ID3D12RootSignature> rootSig,
                           ComPtr<ID3D12PipelineState> pso) : m_backend(backend), m_rootSignature(std::move(rootSig)), m_pipelineState(std::move(pso)) {}

   D3D12Kernel::~D3D12Kernel() {}

  std::unique_ptr<D3D12Kernel> D3D12Kernel::Create(D3D12Backend *backend, const std::string &hlslFilePath,
                                                    const std::string &entryPoint) {
     auto device = backend->GetDevice();
     auto compiler = backend->GetCompiler();
     auto utils = backend->GetUtils();
     auto includeHandler = backend->GetIncludeHandler();

     std::ifstream shaderFile(hlslFilePath, std::ios::binary);
     if (!shaderFile.is_open()) {
       throw std::runtime_error("Failed to open HLSL file: " + hlslFilePath);
     }
     std::string hlslCode((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>()); // TODO: check size and reserve string

     ComPtr<IDxcBlobEncoding> sourceBlob;
     ThrowIfFailed(utils->CreateBlob(
        hlslCode.c_str(),
        static_cast<UINT32>(hlslCode.size()),
        CP_UTF8, // Source code is UTF-8
        &sourceBlob
     ));

     std::wstring wEntryPoint(entryPoint.begin(), entryPoint.end()); // TODO: use MultiByteToWideChar
     std::wstring wFilePath(hlslFilePath.begin(), hlslFilePath.end());

     std::vector<LPCWSTR> arguments;
     arguments.push_back(wFilePath.c_str());
     arguments.push_back(L"-E"); // Entry point
     arguments.push_back(wEntryPoint.c_str());
     arguments.push_back(L"-T"); // Target profile
     arguments.push_back(L"cs_6_0"); // Compute Shader Model 6.0
#if defined(_DEBUG)
     arguments.push_back(DXC_ARG_DEBUG); // Enable debug info
#endif
     arguments.push_back(DXC_ARG_PACK_MATRIX_ROW_MAJOR);

     DxcBuffer sourceBuffer;
     sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
     sourceBuffer.Size = sourceBlob->GetBufferSize();
     sourceBuffer.Encoding = CP_UTF8;

     ComPtr<IDxcResult> compileResult;
     ThrowIfFailed(compiler->Compile(
       &sourceBuffer,
       arguments.data(),
       static_cast<UINT32>(arguments.size()),
       includeHandler,
       IID_PPV_ARGS(&compileResult)
     ));

     HRESULT compileStatus = S_OK;
     ThrowIfFailed(compileResult->GetStatus(&compileStatus));
     if (FAILED(compileStatus)) {
       ComPtr<IDxcBlobUtf8> errors;
       compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
       std::string errStr = errors ? std::string(errors->GetStringPointer(), errors->GetStringLength()) : "Unknown DXC compile error";
       throw std::runtime_error("Shader compilation failed: " + errStr);
     }

     ComPtr<IDxcBlob> shaderBytecode;
     ThrowIfFailed(compileResult->GetOutput(
       DXC_OUT_OBJECT,
       IID_PPV_ARGS(&shaderBytecode),
       nullptr
     ));


     DxcBuffer shaderBuffer;
     shaderBuffer.Ptr = shaderBytecode->GetBufferPointer();
     shaderBuffer.Size = shaderBytecode->GetBufferSize();
     shaderBuffer.Encoding = 0;

     ComPtr<ID3D12ShaderReflection> reflection;
     ThrowIfFailed(utils->CreateReflection(
       &shaderBuffer,
       IID_PPV_ARGS(&reflection)
     ));

     D3D12_SHADER_DESC shaderDesc;
     reflection->GetDesc(&shaderDesc);

     std::vector<std::vector<D3D12_DESCRIPTOR_RANGE1>> rangesPerParam;
     std::vector<D3D12_ROOT_PARAMETER1> rootParameters;

     for (UINT i = 0; i < shaderDesc.BoundResources; ++i) {
       D3D12_SHADER_INPUT_BIND_DESC bindDesc;
       reflection->GetResourceBindingDesc(i, &bindDesc);

       auto isUAV = (bindDesc.Type == D3D_SIT_UAV_RWTYPED) ||
             (bindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED) ||
             (bindDesc.Type == D3D_SIT_UAV_RWBYTEADDRESS) ||
             (bindDesc.Type == D3D_SIT_UAV_APPEND_STRUCTURED) ||
             (bindDesc.Type == D3D_SIT_UAV_CONSUME_STRUCTURED) ||
             (bindDesc.Type == D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER) ||
             (bindDesc.Type == D3D_SIT_UAV_FEEDBACKTEXTURE);

       if (isUAV) {
         if (bindDesc.BindPoint >= static_cast<INT>(rangesPerParam.size())) {
           rangesPerParam.resize(bindDesc.BindPoint + 1);
           rootParameters.resize(bindDesc.BindPoint + 1); // keep same indices
         }

         D3D12_DESCRIPTOR_RANGE1 range = {};
         range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
         range.NumDescriptors = 1;
         range.BaseShaderRegister = bindDesc.BindPoint;
         range.RegisterSpace = bindDesc.Space;
         range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
         range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

         rangesPerParam[bindDesc.BindPoint].push_back(range);

         D3D12_ROOT_PARAMETER1 param = {};
         param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
         param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
         rootParameters[bindDesc.BindPoint] = param;
       }
     }

     for (size_t idx = 0; idx < rootParameters.size(); ++idx) {
       if (rootParameters[idx].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
         auto const &vec = rangesPerParam[idx];
         rootParameters[idx].DescriptorTable.NumDescriptorRanges = static_cast<UINT>(vec.size());
         // vec.data are stable as long we don't mutate the vector.
         rootParameters[idx].DescriptorTable.pDescriptorRanges = vec.empty() ? nullptr : vec.data();
       }
     }


     D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc = {};
     rootSigDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
     rootSigDesc.Desc_1_1.NumParameters = static_cast<UINT>(rootParameters.size());
     rootSigDesc.Desc_1_1.pParameters = rootParameters.data();
     rootSigDesc.Desc_1_1.NumStaticSamplers = 0;
     rootSigDesc.Desc_1_1.pStaticSamplers = nullptr;
     rootSigDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

     ComPtr<ID3DBlob> signatureBlob;
     ComPtr<ID3DBlob> errorBlobRS;
     ThrowIfFailed(D3D12SerializeVersionedRootSignature(
         &rootSigDesc,
         &signatureBlob,
         &errorBlobRS
     ));
     if (errorBlobRS) {
       std::string msg(static_cast<char*>(errorBlobRS->GetBufferPointer()), errorBlobRS->GetBufferSize());
       throw std::runtime_error("Root Signature serialization failed:" + msg);
     }

     ComPtr<ID3D12RootSignature> rootSignature;
     ThrowIfFailed(device->CreateRootSignature(
         0,
         signatureBlob->GetBufferPointer(),
         signatureBlob->GetBufferSize(),
         IID_PPV_ARGS(&rootSignature)
     ));

     D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
     psoDesc.pRootSignature = rootSignature.Get();
     psoDesc.CS.pShaderBytecode = shaderBytecode->GetBufferPointer();
     psoDesc.CS.BytecodeLength = shaderBytecode->GetBufferSize();
     psoDesc.NodeMask = 0;
     psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

     ComPtr<ID3D12PipelineState> pso;
     ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso)));

     // Return the new kernel
     return std::unique_ptr<D3D12Kernel>(
         new D3D12Kernel(backend, std::move(rootSignature), std::move(pso))
     );
   }
}
#endif