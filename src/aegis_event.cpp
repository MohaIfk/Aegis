#include "aegis/event.h"
#include "backend.h"

namespace aegis {
  ComputeEvent::ComputeEvent(ComputeContext *context, std::unique_ptr<internal::IComputeEvent> backendEvent) : m_context(context), m_backendEvent(std::move(backendEvent)) {}

  ComputeEvent::~ComputeEvent() = default;
}