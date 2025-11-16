#pragma once

#if defined(AEGIS_ENABLE_D3D12)

#include "backend.h"
#include "d3d12_backend.h" // for D3D12Backend
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

using Microsoft::WRL::ComPtr;

namespace aegis::internal {
  /**
   * @brief The D3D12 implementation of a compute event.
   *
   * This class wraps an ID3D12Fence, which is the D3D12
   * synchronization primitive.
   */
  class D3D12Event : public IComputeEvent {
  public:
    /**
     * @brief Creates a new D3D12Event (a fence).
     * @param backend The D3D12Backend that will create this resource.
     */
    D3D12Event(D3D12Backend* backend);
    virtual ~D3D12Event();

    ID3D12Fence* GetFence() { return m_fence.Get(); }
  private:
    D3D12Backend* m_backend;
    ComPtr<ID3D12Fence> m_fence;
  };
}

#endif