#include "aegis/context.h"

#include "aegis/stream.h"
#include "aegis/buffer.h"
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

#include "aegis/context.h"

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
    // The same thing here also.
    return nullptr;
  }

  std::unique_ptr<GpuBuffer> ComputeContext::CreateBuffer(size_t byteSize, int memoryType) {
    // This is a placeholder for the logic to convert the public
    // memoryType enum to the internal GpuMemoryType enum.
    auto backendMemType = (internal::GpuMemoryType)memoryType;
    return nullptr; // Placeholder
  }

  std::unique_ptr<ComputeKernel> ComputeContext::CreateKernel(const std::string &hlslFilePath,
                                                              const std::string &entryPoint) {
    return nullptr; // Placeholder
  }

  void ComputeContext::WaitForIdle() { m_backend->WaitForIdle(); }
}
