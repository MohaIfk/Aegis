#include "d3d12_event.h"

#if defined(AEGIS_ENABLE_D3D12)
#include <stdexcept>

namespace aegis::internal {
   D3D12Event::D3D12Event(D3D12Backend *backend) : m_backend(backend) {
     auto device = backend->GetDevice();
     ThrowIfFailed(device->CreateFence(
        0, // Initial value
        D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(&m_fence)
    ));
   }

   D3D12Event::~D3D12Event() {  }
}

#endif