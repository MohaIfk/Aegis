#include "aegis/buffer.h"
#include "backend.h"

namespace aegis {
   GpuBuffer::GpuBuffer(ComputeContext *context, std::unique_ptr<internal::IGpuBuffer> backendBuffer) : m_context(context), m_backendBuffer(std::move(backendBuffer)) {}

  void *GpuBuffer::Map() {
   return m_backendBuffer->Map();
  }

  void GpuBuffer::Unmap() {
   m_backendBuffer->Unmap();
  }

  size_t GpuBuffer::GetSizeInBytes() const {
    return m_backendBuffer->GetSizeInBytes();
  }
}