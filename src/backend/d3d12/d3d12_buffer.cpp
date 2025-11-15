#include "d3d12_buffer.h"

#if defined(AEGIS_ENABLE_D3D12)

#include <stdexcept>

namespace aegis::internal {
  D3D12_HEAP_PROPERTIES GetHeapProperties(GpuMemoryType type) {
    switch (type) {
      case GpuMemoryType::UPLOAD:
        return D3D12_HEAP_PROPERTIES{D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0};
      case GpuMemoryType::READBACK:
        return D3D12_HEAP_PROPERTIES{D3D12_HEAP_TYPE_READBACK, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0};
      // case GpuMemoryType::DEVICE_LOCAL:
      default:
        return D3D12_HEAP_PROPERTIES{D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 0, 0};
    }
  }

  D3D12_RESOURCE_STATES GetInitialState(GpuMemoryType type) {
    switch (type) {
      case GpuMemoryType::UPLOAD:
        return D3D12_RESOURCE_STATE_GENERIC_READ;
      case GpuMemoryType::READBACK:
        return D3D12_RESOURCE_STATE_COPY_DEST;
      // case GpuMemoryType::DEVICE_LOCAL:
      default:
        return D3D12_RESOURCE_STATE_COMMON;
    }
  }

  D3D12Buffer::D3D12Buffer(D3D12Backend *backend, size_t byteSize, GpuMemoryType type) : m_backend(backend), m_byteSize(byteSize), m_mappedPtr(nullptr) {
    auto device = backend->GetDevice();
    auto heapProps = GetHeapProperties(type);
    m_currentState = GetInitialState(type);

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = m_byteSize;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    if (type == GpuMemoryType::DEVICE_LOCAL) {
      bufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    } else {
      bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    }

    ThrowIfFailed(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        m_currentState,
        nullptr,
        IID_PPV_ARGS(&m_resource)
    ));
  }

  D3D12Buffer::~D3D12Buffer() {
    if (m_mappedPtr) {
      Unmap();
    }
  }

  size_t D3D12Buffer::GetSizeInBytes() const {
    return m_byteSize;
  }

  void *D3D12Buffer::Map() {
    if (m_mappedPtr) {
      return m_mappedPtr;
    }
    D3D12_RANGE readRange{0, 0};
    // TODO: If this is a READBACK buffer, set readRange = { 0, m_byteSize };

    ThrowIfFailed(m_resource->Map(0, nullptr, &m_mappedPtr));
    return m_mappedPtr;
  }

  void D3D12Buffer::Unmap() {
    D3D12_RANGE writeRange{0, 0};
    // TODO: If this is an UPLOAD buffer, specify the written range.

    m_resource->Unmap(0, &writeRange);
    m_mappedPtr = nullptr;
  }
}

#endif