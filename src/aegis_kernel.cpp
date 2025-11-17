#include "aegis/kernel.h"
#include "backend.h"

namespace aegis {
  ComputeKernel::ComputeKernel(ComputeContext *context, std::unique_ptr<internal::IComputeKernel> backendKernel) : m_context(context), m_backendKernel(std::move(backendKernel)) { }
  ComputeKernel::~ComputeKernel() = default;
}