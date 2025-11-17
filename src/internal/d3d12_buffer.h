#pragma once

#if defined(AEGIS_ENABLE_D3D12)

#include "backend.h"
#include "d3d12_backend.h" // for D3D12Backend
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;


namespace aegis::internal {
  /**
   * @brief The D3D12 implementation of a GPU buffer.
   *
   * This class wraps an ID3D12Resource and manages its lifetime,
   * state, and CPU mapping.
   */
  class D3D12Buffer : public IGpuBuffer {
  public:
    /**
     * @brief Creates a new D3D12Buffer.
     * @param backend The D3D12Backend that will create this resource.
     * @param byteSize The size of the buffer to create.
     * @param type The GpuMemoryType (heap) to place the buffer in.
     */
    D3D12Buffer(D3D12Backend* backend, size_t byteSize, GpuMemoryType type);

    ~D3D12Buffer() override;

    size_t GetSizeInBytes() const override;
    void* Map() override;
    void Unmap() override;

    /**
     * @brief Gets the underlying D3D12 resource.
     */
    ID3D12Resource* GetResource() { return m_resource.Get(); }

    /**
     * @brief Gets the GPU Virtual Address of the buffer.
     */
    D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const {
      return m_resource->GetGPUVirtualAddress();
    }

    /**
     * @brief Gets the current D3D12 state of this resource.
     * @note This is CRITICAL for barrier tracking.
     */
    D3D12_RESOURCE_STATES GetCurrentState() const { return m_currentState; }

    /**
     * @brief Sets the current D3D12 state of this resource.
     * @note This is CRITICAL for barrier tracking.
     */
    void SetCurrentState(D3D12_RESOURCE_STATES newState) { m_currentState = newState; }

  private:
    D3D12Backend* m_backend;
    ComPtr<ID3D12Resource> m_resource;
    size_t m_byteSize;
    void* m_mappedPtr;
    GpuMemoryType m_memoryType;

    /** The last known state of this resource. */
    D3D12_RESOURCE_STATES m_currentState;
  };
}

#endif
