#include "aegis/buffer.h"
#include "aegis/event.h"
#include "aegis/kernel.h"
#include "aegis/stream.h"
#include "backend.h"

namespace aegis {
  ComputeStream::ComputeStream(ComputeContext *context, std::unique_ptr<internal::IComputeStream> backendStream) : m_context(context), m_backendStream(std::move(backendStream)) {}

  ComputeStream::~ComputeStream() {}

  void ComputeStream::RecordDispatch(ComputeKernel &kernel, uint32_t threadGroupsX, uint32_t threadGroupsY,
                                     uint32_t threadGroupsZ) {
    m_backendStream->SetKernel(kernel.GetBackendKernel());
    m_backendStream->RecordDispatch(threadGroupsX, threadGroupsY, threadGroupsZ);
  }

  void ComputeStream::ResourceCopyBuffer(GpuBuffer &dest, GpuBuffer &src) {
    m_backendStream->ResourceCopyBuffer(dest.GetBackendBuffer(), src.GetBackendBuffer());
  }

  void ComputeStream::ResourceUpload(GpuBuffer &dest, const void *srcData, size_t byteSize) {
    m_backendStream->ResourceUpload(dest.GetBackendBuffer(), srcData, byteSize);
  }

  void ComputeStream::ResourceDownload(void *destData, GpuBuffer &src, size_t byteSize) {
    m_backendStream->ResourceDownload(destData, src.GetBackendBuffer(), byteSize);
  }

  void ComputeStream::SetBuffer(uint32_t slot, GpuBuffer &buffer) {
    m_backendStream->SetBuffer(slot, buffer.GetBackendBuffer());
  }

  void ComputeStream::Submit() {
    m_backendStream->Submit();
  }

  void ComputeStream::HostWait() {
    m_backendStream->HostWait();
  }

  void ComputeStream::StreamWait(ComputeEvent &event) {
    m_backendStream->StreamWait(event.GetBackendEvent());
  }

  void ComputeStream::RecordEvent(ComputeEvent &event) {
    m_backendStream->RecordEvent(event.GetBackendEvent());
  }

}