#include "aegis/event.h"

namespace aegis {
   ComputeEvent::ComputeEvent(ComputeContext *context, std::unique_ptr<internal::IComputeEvent> backendEvent) : m_context(context), m_backendEvent(std::move(backendEvent)) {}
}