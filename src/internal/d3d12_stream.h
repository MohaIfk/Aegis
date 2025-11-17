#pragma once

#if defined(AEGIS_ENABLE_D3D12)

#include "backend.h"
#include "d3d12_backend.h"

#define WIN32_LEAN_AND_MEAN
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <mutex>
#include <queue>

using Microsoft::WRL::ComPtr;

namespace aegis::internal {
  class D3D12Buffer;
  class D3D12Kernel;

  /**
   * @brief Holds information for a pending GPU-to-CPU data transfer.
   */
  struct PendingReadback {
    IGpuBuffer* readbackBuffer;
    const void* cpuDestination;
    size_t      byteSize;
  };

  /**
   * @brief The D3D12 implementation of a compute stream.
   *
   * This class wraps an ID3D12GraphicsCommandList, ID3D12CommandAllocator,
   * and an ID3D12Fence for stream-specific synchronization.
   *
   * Tt's responsible for:
   * 1. Recording commands.
   * 2. Managing resource barriers.
   * 3. Submitting to the master command queue.
   * 4. Managing its own synchronization fence.
   * 5. Managing temporary upload/readback buffers.
   */
  class D3D12Stream: public IComputeStream {
  public:
    D3D12Stream(D3D12Backend* backend);
    virtual ~D3D12Stream();

    void RecordDispatch(uint32_t threadGroupsX, uint32_t threadGroupsY, uint32_t threadGroupsZ) override;
    void ResourceCopyBuffer(IGpuBuffer* dest, IGpuBuffer* src) override;
    void ResourceUpload(IGpuBuffer* dest, const void* srcData, size_t byteSize) override;
    void ResourceDownload(const void* destData, IGpuBuffer* src, size_t byteSize) override;
    void SetKernel(IComputeKernel* kernel) override;
    void SetBuffer(uint32_t slot, IGpuBuffer* buffer) override;

    void Submit() override;
    void HostWait() override;
    void StreamWait(IComputeEvent* event) override;
    void RecordEvent(IComputeEvent* event) override;

  private:
    /**
     * @brief Resets the command allocator and list to record new commands.
     * This is called after Submit() or on the first use.
     */
    void resetCommandList();

    /**
     * @brief Manages D3D12 resource barriers.
     * This is the magic. Before a dispatch or copy, this function
     * must be called to transition buffer states.
     * @param buffer The buffer to transition.
     * @param newState The target state (e.g., COPY_DEST, UNORDERED_ACCESS).
     */
    void transitionBarrier(D3D12Buffer* buffer, D3D12_RESOURCE_STATES newState);

    /**
     * @brief Flushes all pending barriers in m_pendingBarriers.
     */
    void flushBarriers();

    D3D12Backend* m_backend;
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12GraphicsCommandList4> m_commandList;
    ComPtr<ID3D12CommandQueue> m_queue;

    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue;
    HANDLE m_fenceEvent;

    D3D12Kernel* m_currentKernel;
    bool m_isListOpen;

    std::vector<D3D12_RESOURCE_BARRIER> m_pendingBarriers;
    std::vector<std::unique_ptr<IGpuBuffer>> m_inFlightResources;
    std::queue<PendingReadback> m_pendingReadbacks;

    // TODO: Need pools for upload and readback buffers
  };
}

#endif