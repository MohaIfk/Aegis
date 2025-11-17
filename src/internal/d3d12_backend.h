#pragma once

#if defined(AEGIS_ENABLE_D3D12)

#include "backend.h"

#define WIN32_LEAN_AND_MEAN
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>


#include "dxcapi.h"

#undef CreateEvent
#ifdef UNICODE
#define _CreateEvent  CreateEventW
#else
#define _CreateEvent  CreateEventA
#endif

#include <string>
#include <memory> // for std::unique_ptr
#include <mutex>

#define ThrowIfFailed(hr) if(FAILED(hr)) { throw std::runtime_error(std::string("D3D12 HRESULT failed: ") + #hr); }

using Microsoft::WRL::ComPtr;

namespace aegis::internal {
  /**
   * @brief The D3D12 implementation of the compute backend interface.
   *
   * This class is the "master" D3D12 object. It owns the logical device,
   * the master command queue, and the factories for all other D3D12 objects.
   */
  class D3D12Backend: public IComputeBackend {
  public:
    ~D3D12Backend() override;

    /**
     * @brief Creates and initializes the D3D12 backend.
     * @return A unique_ptr to the new backend, or nullptr if D3D12
     * initialization fails.
     */
    static std::unique_ptr<D3D12Backend> Create();

    std::unique_ptr<IComputeStream> CreateStream() override;
    std::unique_ptr<IComputeEvent> CreateEvent() override;
    std::unique_ptr<IGpuBuffer> CreateBuffer(size_t byteSize, GpuMemoryType type) override;
    std::unique_ptr<IComputeKernel> CreateKernel(const std::string& hlslFilePath, const std::string& entryPoint) override;

    void WaitForIdle() override;

    // I will make those public so other backend classes can use them (D3D12Stream, etc.)
    ID3D12Device5* GetDevice() { return m_device.Get(); }
    //ID3D12CommandQueue* GetMasterQueue() { return m_masterCommandQueue.Get(); }
    IDxcCompiler3* GetCompiler() { return m_dxcCompiler.Get(); }
    IDxcUtils* GetUtils() { return m_dxcUtils.Get(); }
    IDxcIncludeHandler* GetIncludeHandler() { return m_dxcIncludeHandler.Get(); }

    /** @brief Provides thread-safe access to the master command queue. */
    //std::mutex& GetQueueMutex() { return m_queueMutex; }

  private:
    /**
     * @brief Private constructor. Use D3D12Backend::Create().
     */
    D3D12Backend();

    /**
     * @brief The real initialization logic.
     * @return true on success, false on failure.
     */
    bool Initialize();

    /**
     * @brief Finds the first D3D12-compatible adapter.
     */
    static bool GetHardwareAdapter(ComPtr<IDXGIFactory4> factory, ComPtr<IDXGIAdapter1>& outAdapter);

    // Core D3D12 Objects
    ComPtr<IDXGIFactory4> m_dxgiFactory;
    ComPtr<ID3D12Device5> m_device;
    //ComPtr<ID3D12CommandQueue> m_masterCommandQueue;

    // DXC (Compiler) Objects
    ComPtr<IDxcUtils> m_dxcUtils;
    ComPtr<IDxcCompiler3> m_dxcCompiler;
    ComPtr<IDxcIncludeHandler> m_dxcIncludeHandler;

    // Synchronization
    ComPtr<ID3D12Fence> m_masterFence;
    UINT64 m_masterFenceValue;
    HANDLE m_fenceEvent;

    std::mutex m_fenceMutex; // Protects m_masterFence and m_masterFenceValue
    // std::mutex m_queueMutex; // Protects m_masterCommandQueue submission
  };
}

#endif