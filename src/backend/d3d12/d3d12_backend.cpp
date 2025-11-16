#include "d3d12_backend.h"

#if defined(AEGIS_ENABLE_D3D12)
#include <stdexcept>
#include <vector>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxcompiler.lib")

#undef CreateEvent

// #include "d3d12_stream.h"
// #include "d3d12_event.h"
#include "d3d12_buffer.h"
#include "d3d12_kernel.h"s

namespace aegis::internal {
  std::unique_ptr<D3D12Backend> D3D12Backend::Create() {
    auto backend = std::unique_ptr<D3D12Backend>(new D3D12Backend());
    if (!backend->Initialize()) {
      return nullptr;
    }
    return backend;
  }

  D3D12Backend::D3D12Backend() : m_masterFenceValue(0), m_fenceEvent(nullptr) {}

  D3D12Backend::~D3D12Backend() {
    if (m_masterCommandQueue) {
      // Wait for all final commands to finish before releasing resources
      WaitForIdle();
    }
    if (m_fenceEvent) {
      CloseHandle(m_fenceEvent);
    }
  }

  bool D3D12Backend::Initialize() {
    try {
      UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
      ComPtr<ID3D12Debug> debugController;
      if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
      }
#endif

      ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_dxgiFactory)));

      ComPtr<IDXGIAdapter1> hardwareAdapter;
      if (!GetHardwareAdapter(m_dxgiFactory.Get(), hardwareAdapter)) {
        return false;
      }

      ThrowIfFailed(D3D12CreateDevice(
          hardwareAdapter.Get(),
          D3D_FEATURE_LEVEL_12_0,
          IID_PPV_ARGS(&m_device)
      ));

      D3D12_COMMAND_QUEUE_DESC queueDesc = {};
      queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
      queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
      ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_masterCommandQueue)));

      ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_masterFence)));
      m_masterFenceValue = 1;

      m_fenceEvent = _CreateEvent(nullptr, FALSE, FALSE, nullptr);
      if (m_fenceEvent == nullptr) {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
      }

      ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_dxcUtils)));
      ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_dxcCompiler)));
      ThrowIfFailed(m_dxcUtils->CreateDefaultIncludeHandler(&m_dxcIncludeHandler));
    } catch (const std::runtime_error& e) {
      // TODO: log error
      return false;
    }
    return true;
  }

  bool D3D12Backend::GetHardwareAdapter(ComPtr<IDXGIFactory4> factory, ComPtr<IDXGIAdapter1> &outAdapter) {
    for (UINT adapterIndex = 0; ; ++adapterIndex) {
      ComPtr<IDXGIAdapter1> adapter;
      if (DXGI_ERROR_NOT_FOUND == factory->EnumAdapters1(adapterIndex, &adapter)) {
        break;
      }

      DXGI_ADAPTER_DESC1 desc;
      adapter->GetDesc1(&desc);

      if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
        continue; // Skip the "Microsoft Basic Render Driver"
      }

      // Check if the adapter supports D3D12
      if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr))) {
        outAdapter = adapter;
        return true;
      }
    }
    return false;
  }

  std::unique_ptr<IComputeStream> D3D12Backend::CreateStream() {
    return std::make_unique<D3D12Stream>(this);
  }

  std::unique_ptr<IComputeEvent> D3D12Backend::CreateEvent() {
    return std::make_unique<D3D12Event>(this);
  }

  std::unique_ptr<IGpuBuffer> D3D12Backend::CreateBuffer(size_t byteSize, GpuMemoryType type) {
    return std::make_unique<D3D12Buffer>(this, byteSize, type);
  }

  std::unique_ptr<IComputeKernel> D3D12Backend::CreateKernel(const std::string& hlslFilePath, const std::string& entryPoint) {
    try {
      return D3D12Kernel::Create(this, hlslFilePath, entryPoint);
    } catch (const std::exception& e) {
      // TODO: log this error like "shader compilation failed".
      return nullptr;
    }
  }

  void D3D12Backend::WaitForIdle() {
    // Stop the world
    std::lock_guard<std::mutex> queueLock(m_queueMutex);
    std::lock_guard<std::mutex> lock(m_fenceMutex);

    const UINT64 fenceValueToSignal = m_masterFenceValue;
    ThrowIfFailed(m_masterCommandQueue->Signal(m_masterFence.Get(), fenceValueToSignal));
    m_masterFenceValue++;

    if (m_masterFence->GetCompletedValue() < fenceValueToSignal) {
      ThrowIfFailed(m_masterFence->SetEventOnCompletion(fenceValueToSignal, m_fenceEvent));
      WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
    }
  }
}

#endif