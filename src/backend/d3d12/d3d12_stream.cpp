#include "d3d12_stream.h"
#include "d3d12_buffer.h"
#include "d3d12_kernel.h"
#include "d3d12_event.h"

#if defined(AEGIS_ENABLE_D3D12)
#include <stdexcept>

namespace aegis::internal {
  D3D12Stream::D3D12Stream(D3D12Backend *backend) :
      m_backend(backend), m_fenceValue(0), m_currentKernel(nullptr), m_isListOpen(false) {
    auto device = m_backend->GetDevice();

    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));

    ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
    m_commandList->Close();

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_queue)));

    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    m_fenceValue = 1;

    m_fenceEvent = _CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_fenceEvent == nullptr) {
      ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }
  }

  D3D12Stream::~D3D12Stream() {
    CloseHandle(m_fenceEvent);
  }

  void D3D12Stream::resetCommandList() {
    if (!m_isListOpen) {
      ThrowIfFailed(m_commandAllocator->Reset());
      ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));
      m_isListOpen = true;
    }
  }

  void D3D12Stream::transitionBarrier(D3D12Buffer *buffer, D3D12_RESOURCE_STATES newState) {
    if (buffer->GetCurrentState() != newState) {
      D3D12_RESOURCE_BARRIER barrier = {};
      barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
      barrier.Transition.pResource = buffer->GetResource();
      barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      barrier.Transition.StateBefore = buffer->GetCurrentState();
      barrier.Transition.StateAfter = newState;

      m_pendingBarriers.push_back(barrier);
      buffer->SetCurrentState(newState);
    }
  }

  void D3D12Stream::flushBarriers() {
    if (!m_pendingBarriers.empty()) {
      m_commandList->ResourceBarrier(static_cast<UINT>(m_pendingBarriers.size()), m_pendingBarriers.data());
      m_pendingBarriers.clear();
    }
  }

  void D3D12Stream::SetKernel(IComputeKernel *kernel) {
    resetCommandList();

    m_currentKernel = static_cast<D3D12Kernel*>(kernel);
    m_commandList->SetPipelineState(m_currentKernel->GetPipelineState());
    m_commandList->SetComputeRootSignature(m_currentKernel->GetRootSignature());
  }

  void D3D12Stream::SetBuffer(uint32_t slot, IGpuBuffer *buffer) {
    // This is a MASSIVE simplification.
    // For prod the library needs to manage descriptor heaps.
    // We assumes the root signature just has UAVs directly in the root parameters
    D3D12Buffer* d3dBuffer = static_cast<D3D12Buffer*>(buffer);

    transitionBarrier(d3dBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    // TODO: take care of this
    m_commandList->SetComputeRootUnorderedAccessView(
        slot,
        d3dBuffer->GetGpuVirtualAddress()
    );
  }

  void D3D12Stream::RecordDispatch(uint32_t threadGroupsX, uint32_t threadGroupsY, uint32_t threadGroupsZ) {
    resetCommandList();
    if (!m_currentKernel) {
      throw std::runtime_error("No kernel set before dispatch.");
    }

    flushBarriers();

    m_commandList->Dispatch(threadGroupsX, threadGroupsY, threadGroupsZ);
  }

  void D3D12Stream::ResourceCopyBuffer(IGpuBuffer *dest, IGpuBuffer *src) {
    resetCommandList();
    D3D12Buffer* d3dDest = static_cast<D3D12Buffer*>(dest);
    D3D12Buffer* d3dSrc = static_cast<D3D12Buffer*>(src);

    transitionBarrier(d3dDest, D3D12_RESOURCE_STATE_COPY_DEST);
    transitionBarrier(d3dSrc, D3D12_RESOURCE_STATE_COPY_SOURCE);
    flushBarriers();

    m_commandList->CopyResource(d3dDest->GetResource(), d3dSrc->GetResource());
  }

  void D3D12Stream::Submit() {
    if (!m_isListOpen) {
      return;
    }

    flushBarriers();

    ThrowIfFailed(m_commandList->Close());
    m_isListOpen = false;

    ID3D12CommandList* const ppCommandLists[] = { m_commandList.Get() };

    auto queue = m_queue.Get();

    queue->ExecuteCommandLists(1, ppCommandLists);

    ThrowIfFailed(queue->Signal(m_fence.Get(), m_fenceValue));
  }

  void D3D12Stream::HostWait() {
    if (m_fence->GetCompletedValue() < m_fenceValue) {
      ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent));
      WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    m_fenceValue++;

    while (!m_pendingReadbacks.empty()) {
      auto readback = m_pendingReadbacks.front();
      m_pendingReadbacks.pop();

      void* pGpuData = readback.readbackBuffer->Map();
      memcpy(const_cast<void *>(readback.cpuDestination), pGpuData, readback.byteSize);
      readback.readbackBuffer->Unmap();
    }

    m_inFlightResources.clear();
  }

  void D3D12Stream::StreamWait(IComputeEvent *event) {
    D3D12Event* d3dEvent = static_cast<D3D12Event*>(event);
    ID3D12Fence* fence = d3dEvent->GetFence();

    UINT64 valueToWaitFor = d3dEvent->m_fenceValue.load();

    ThrowIfFailed(m_queue->Wait(fence, valueToWaitFor));
  }

  void D3D12Stream::RecordEvent(IComputeEvent *event) {
    D3D12Event* d3dEvent = static_cast<D3D12Event*>(event);
    ID3D12Fence* fence = d3dEvent->GetFence();

    UINT64 valueToSignal = ++(d3dEvent->m_fenceValue);

    ThrowIfFailed(m_queue->Signal(fence, valueToSignal));
  }

  // TODO: Add a pool of upload/readback buffers.
  void D3D12Stream::ResourceUpload(IGpuBuffer *dest, const void *srcData, size_t byteSize) {
    // A temp upload buffer
    auto tempUploadBuffer = m_backend->CreateBuffer(byteSize, GpuMemoryType::UPLOAD);
    D3D12Buffer* d3dUploadBuffer = static_cast<D3D12Buffer*>(tempUploadBuffer.get());

    void* pData = d3dUploadBuffer->Map();
    memcpy(pData, srcData, byteSize);
    d3dUploadBuffer->Unmap();

    ResourceCopyBuffer(dest, d3dUploadBuffer);

    m_inFlightResources.push_back(std::move(tempUploadBuffer));
    // TODO: Need to keep tempUploadBuffer alive until the copy is done.
  }

  void D3D12Stream::ResourceDownload(const void *destData, IGpuBuffer *src, size_t byteSize) {
    // A temp upload readback
    auto tempReadbackBuffer = m_backend->CreateBuffer(byteSize, GpuMemoryType::READBACK);
    D3D12Buffer* d3dReadbackBuffer = static_cast<D3D12Buffer*>(tempReadbackBuffer.get());

    ResourceCopyBuffer(d3dReadbackBuffer, src);

    m_pendingReadbacks.push({tempReadbackBuffer.get(), destData, byteSize});

    m_inFlightResources.push_back(std::move(tempReadbackBuffer));
  }

}

#endif