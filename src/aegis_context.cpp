#include "aegis/context.h"

#include "aegis/stream.h"
#include "aegis/kernel.h"
#include "aegis/event.h"

#include "internal/backend.h"

#if defined(AEGIS_ENABLE_D3D12)
    #include "internal/d3d12_backend.h"
#endif
#if defined(AEGIS_ENABLE_VULKAN)
    #include "internal/vulkan_backend.h" // won't be implemented untill we manage to get a working d3d12
#endif

#include <stdexcept> // for std::runtime_error

namespace aegis {
  ComputeContext::ComputeContext(std::unique_ptr<internal::IComputeBackend> backend) : m_backend(std::move(backend)) {}

  ComputeContext::~ComputeContext() {
    // Ensure all GPU work is finished before destroying the device
    if (m_backend) m_backend->WaitForIdle();
  }

  std::unique_ptr<ComputeContext> ComputeContext::Create() {
    std::unique_ptr<internal::IComputeBackend> backend = nullptr;

#if defined(AEGIS_ENABLE_D3D12)
    backend = internal::D3D12Backend::Create();
#elif defined(AEGIS_ENABLE_VULKAN)
    // backend = internal::VulkanBackend::Create(); // Not implemented
#else
    // No backend was compiled in.
    // TODO: log an error here.
    return nullptr;
#endif

    if (!backend) return nullptr;

    return std::unique_ptr<ComputeContext>(new ComputeContext(std::move(backend)));
  }

  std::unique_ptr<ComputeStream> ComputeContext::CreateStream() {
    // We will only call the backend implementation CreateStream,
    // wrap it and return it, placeholder for now.
    return nullptr;
  }

  std::unique_ptr<ComputeEvent> ComputeContext::CreateEvent() {
    auto backendEvent = m_backend->CreateEvent();
    if (!backendEvent) return nullptr;
    return std::unique_ptr<ComputeEvent>(
        new ComputeEvent(this, std::move(backendEvent))
    );
  }

  std::unique_ptr<GpuBuffer> ComputeContext::CreateBuffer(size_t byteSize, GpuBuffer::MemoryType memoryType) {
    internal::GpuMemoryType backendMemType;
    switch (memoryType) {
      case GpuBuffer::MemoryType::UPLOAD: backendMemType = internal::GpuMemoryType::UPLOAD; break;
      case GpuBuffer::MemoryType::READBACK: backendMemType = internal::GpuMemoryType::READBACK; break;
      default: backendMemType = internal::GpuMemoryType::DEVICE_LOCAL; break;
    }

    auto backendBuffer = m_backend->CreateBuffer(byteSize, backendMemType);
    if (!backendBuffer) return nullptr;
    return std::unique_ptr<GpuBuffer>(new GpuBuffer(this, std::move(backendBuffer)));
  }

  std::unique_ptr<ComputeKernel> ComputeContext::CreateKernel(const std::string &hlslFilePath, const std::string &entryPoint) {
    auto backendBuffer = m_backend->CreateKernel(hlslFilePath, entryPoint);
    return std::unique_ptr<ComputeKernel>(new ComputeKernel(this, std::move(backendBuffer)));
  }

  void ComputeContext::WaitForIdle() { m_backend->WaitForIdle(); }
}
