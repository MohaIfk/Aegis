#pragma once

#if defined(AEGIS_ENABLE_D3D12)

#include "backend.h"
#include "d3d12_backend.h" // for D3D12Backend
#include <d3d12.h>
#include <wrl/client.h>
#include <string>

using Microsoft::WRL::ComPtr;

namespace aegis::internal {
  /**
 * @brief The D3D12 implementation of a compute kernel.
 *
 * This class wraps an ID3D12RootSignature and an ID3D12PipelineState.
 * Its creation is the most complex part of the D3D12 backend,
 * involving shader compilation and reflection.
 */
  class D3D12Kernel: public IComputeKernel {
  public:
    virtual ~D3D12Kernel();

    /**
     * @brief Factory function to create a new D3D12Kernel.
     *
     * This function handles loading the HLSL file, compiling it with DXC,
     * reflecting the parameters to build a root signature, and finally
     * creating the pipeline state object.
     *
     * @param backend The D3D12Backend that will own this kernel.
     * @param hlslFilePath Path to the .hlsl shader file.
     * @param entryPoint The name of the [shader("compute")] function.
     * @return A unique_ptr to the new kernel, or throws an exception on failure.
     */
    static std::unique_ptr<D3D12Kernel> Create(
        D3D12Backend* backend,
        const std::string& hlslFilePath,
        const std::string& entryPoint);

    ID3D12RootSignature* GetRootSignature() { return m_rootSignature.Get(); }
    ID3D12PipelineState* GetPipelineState() { return m_pipelineState.Get(); }
  private:
    /**
     * @brief Private constructor. Use D3D12Kernel::Create().
     */
    D3D12Kernel(D3D12Backend* backend,
                 ComPtr<ID3D12RootSignature> rootSig,
                 ComPtr<ID3D12PipelineState> pso);

    D3D12Backend* m_backend;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;
  };
}

#endif