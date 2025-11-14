/**
 * @file backend.h
 * @brief IComputeBackend interface
 */

#pragma once
#include <string>
#include <memory> // for std::unique_ptr

// Public facing types
class ComputeBackend;
class ComputeStream;
class GpuBuffer;
class ComputeKernel;
class ComputeEvent;

namespace aegis {
namespace internal {

  class IComputeBackend;
  class IComputeStream;
  class IGpuBuffer;
  class IComputeKernel;
  class IComputeEvent;

  /**
   * @brief Defines the types of memory for a GPU buffer
   * @note This is a hint for the backend to optimize memory placement
   */
  enum class GpuMemoryType {
    /** @brief Default GPU-only memory. Fast for GPU R/W, inaccessible by CPU. */
    DEVICE_LOCAL,
    /** @brief CPU-visible memory. For uploading data (CPU-to-GPU). */
    UPLOAD,
    /** @brief CPU-visible memory. For reading data back (GPU-to-CPU). */
    READBACK
  };

  /**
   * @brief Interface for a GPU compute event.
   * @note This is a backend-specific implementation of a synchronization
   * primitive (e.g., ID3DD12Fence or VkFence).
   * It is created by the IComputeBackend and used by an IComputeStream.
   */
  class IComputeEvent {
  public:
    virtual ~IComputeEvent() = default;
  };

  /**
   * @brief Interface for a compiled compute kernel.
   * @note This is a backend-specific implementation of a compiled shader
   * (e.g., ID3D12PiplineState + ID3D12RootSignature or VkPipeline).
   * It is created by the IComputeBackend.
   */
  class IComputeKernel {
  public:
    virtual ~IComputeKernel() = default;
  };

  /**
   * @brief Interface for a GPU memory buffer.
   * @note This is a backend-specific implementation of a GPU resource
   * (e.g., ID3D12Resource or VkBuffer).
   * It is created by the IComputeBackend.
   */
  class IGpuBuffer {
  public:
    virtual ~IGpuBuffer() = default;

    /**
     * @brief Gets the size of the buffer in bytes.
     * @return size_t The total size of the buffer.
     */
    virtual size_t GetSizeInBytes() const = 0;

    /**
     * @brief Maps the buffer's memory for CPU access.
     * @warning This operation may be slow and cause a pipeline stall
     * if the GPU is actively using the buffer.
     * @note Only buffers created with UPLOAD or READBACK type
     * can be mapper. The implementation should block until the buffer is safe to map (if a HostWait() wasn't called).
     * @return void* A CPU-writable/readable pointer to the buffer's memory.
     */
    virtual void* Map() = 0;

    /**
     * @brief Unmaps the buffer's memory.
     * @note FOR UPLOAD buffers, this signifies the data is ready for the GPU.
     * For READBACK buffers, this should be called after CPU reading is done.
     */
    virtual void Unmap() = 0;
  };

  /**
   * @brief Interface for a compute command stream (or queue).
   * @note This is the workhorse of the library. It wraps a command list
   * and a command allocator (e.g., ID3D12GraphicsCommandList).
   * It is responsible for recording and submitting all work.
   */
  class IComputeStream {
  public:
    virtual ~IComputeStream() = default;

    /**
     * @brief Records a command to dispatch the currently set kernel.
     * @note The D3D12 implementation must transition all bound buffer
     * resources to the UNORDERED_ACCESS state before this call.
     * @param threadGroupX Number of thread groups in the X dimension.
     * @param threadGroupY Number of thread groups in the Y dimension.
     * @param threadGroupZ Number of thread groups in the Z dimension.
     */
    virtual void RecordDispatch(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) = 0;

    /**
     * @brief Records a command to copy data from one GPU buffer to another.
     * @note The D3D12 implementation must transition the 'dest' buffer
     * to the COPY_DEST state and 'src' to the COPY_SOURCE state.
     * @param dest The destination GPU buffer.
     * @param src The source GPU buffer.
     */
    virtual void ResourceCopyBuffer(IGpuBuffer* dest, IGpuBuffer* src) = 0;

    /**
     * @brief Records a command to upload data from the CPU to GPU buffer.
     * @note This is a high-level convenience function. The backend
     * implementation MUST manage an internal upload buffer pool.
     * This function will sub-allocate from that pool, copy the
     * srcData into it, and record a GPU copy command.
     * @param dest The destination (DEVICE_LOCAL) buffer.
     * @param srcData A pointer to the CPU data to upload.
     * @param byteSize The size of the data to upload.
     */
    virtual void ResourceUpload(IGpuBuffer* dest, const void* srcData, size_t byteSize) = 0;

    /**
     * @brief Records a command to download data from a GPU buffer to the CPU.
     * @note This is a high-level convenience function. The backend
     * implementation MUST manage an internal readback buffer pool.
     * This function will record a GPU copy to the readback buffer.
     * The data will NOT be available until HostWait() is called.
     * @param destData A pointer to the CPU memory to receive the data.
     * @param src The source (DEVICE_LOCAL) buffer.
     * @param byteSize The size of the data to download.
     */
    virtual void ResourceDownload(const void* destData, IGpuBuffer* src, size_t byteSize) = 0;

    /**
     * @brief Binds a compute kernel to the stream for the next dispatch.
     * @note The D3D12 implementation should call SetPipelineState()
     * and SetComputeRootSignature().
     * @param kernel The kernel to set.
     */
    virtual void SetKernel(IComputeKernel* kernel) = 0;

    /**
     * @brief Binds a GPU buffer to a specific slot (register).
     * @note The D3D12 implementation should call
     * SetComputeRootUnorderedAccessView() or
     * SetComputeRootShaderResourceView() based on the shader.
     * This binding must be persistent until a new kernel is set.
     * @param slot The register slot (e.g., u0, u1, t0...).
     * @param buffer The buffer to bind.
     */
    virtual void SetBuffer(uint32_t slot, IGpuBuffer* buffer) = 0;

    /**
     * @brief Submits all recorded commands to the GPU for execution.
     * @note This closes the internal command list, executes it on the
     * command queue, and resets the allocator/list for new commands.
     * This function returns immediately (it is asynchronous).
     */
    virtual void Submit() = 0;

    /**
     * @brief Blocks the C++ thread until all work in *this stream* is finished.
     * @note This function MUST also handle processing any pending readbacks
     * from RecordDownload() calls. After this function returns,
     * the CPU pointers from RecordDownload should be filled.
     */
    virtual void HostWait() = 0;

    /**
     * @brief Records a command for this stream to wait for an event.
     * @note The D3D12 implementation should call commandQueue->Wait().
     * This forces this stream (on the GPU) to pause until the
     * event is signaled by another stream.
     * @param event The event to wait on.
     */
    virtual void StreamWait(IComputeEvent* event) = 0;

    /**
     * @brief Records a command for this stream to signal an event.
     * @note The D3D12 implementation should call commandQueue->Signal().
     * This signals that all work *prior* to this call in this
     * stream is complete.
     * @param event The event to signal.
     */
    virtual void RecordEvent(IComputeEvent* event) = 0;
  };

  /**
   * @brief Main interface for the compute backend (the "Device").
   * @note This is the primary factory class. It is responsible for
   * device creation and object creation. It will be owned
   * by the public-facing ComputeContext.
   */
  class IComputeBackend {
  public:
    virtual ~IComputeBackend() = default;

    /**
     * @brief Creates a new compute stream.
     * @note The D3D12 implementation must create an ID3D12CommandQueue
     * (or reuse one), an ID3D12CommandAllocator, and an
     * ID3D12GraphicsCommandList.
     * @return std::unique_ptr<IComputeStream> The new stream object.
     */
    virtual std::unique_ptr<IComputeStream> CreateStream() = 0;

    /**
     * @brief Creates a new compute event.
     * @note The D3D12 implementation must create an ID3D12Fence.
     * @return std::unique_ptr<IComputeEvent> The new event object.
     */
    virtual std::unique_ptr<IComputeEvent> CreateEvent() = 0;

    /**
     * @brief Creates a new GPU buffer.
     * @note The D3D12 implementation must create an ID3D12Resource
     * and place it in the correct heap (DEFAULT, UPLOAD, or READBACK)
     * based on the memory type. It must also create the
     * necessary Unordered Access View (UAV) for the buffer.
     * @param byteSize The size of the buffer to create.
     * @param type The type of memory heap to place the buffer in.
     * @return std::unique_ptr<IGpuBuffer> The new buffer object.
     */
    virtual std::unique_ptr<IGpuBuffer> CreateBuffer(size_t byteSize, GpuMemoryType type) = 0;

    /**
     * @brief Compiles an HLSL shader and creates a compute kernel.
     * @note The D3D12 implementation must:
     * 1. Call DXC (dxcompiler) to compile the HLSL file to bytecode.
     * 2. Use reflection to automatically create an ID3D12RootSignature.
     * 3. Create an ID3D12PipelineState object.
     * The returned kernel object will wrap both the PSO and Root Signature.
     * @param hlslFilePath Path to the .hlsl shader file.
     * @param entryPoint The name of the [shader("compute")] function (e.g., "main_cs").
     * @return std::unique_ptr<IComputeKernel> The new kernel object.
     */
    virtual std::unique_ptr<IComputeKernel> CreateKernel(const std::string& hlslFilePath, const std::string& entryPoint) = 0;

    /**
     * @brief Blocks the C++ thread until ALL streams are idle.
     * @note This is a "stop the world" synchronization.
     */
    virtual void WaitForIdle() = 0;
  };

};
};