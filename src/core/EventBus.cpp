#include "core/EventBus.h"

namespace mak {

uint32_t EventBus::publish(const Event& ev) {
  ring_[head_] = ev;
  head_ = (head_ + 1) % cfg::kEventRingCapacity;
  if (count_ < cfg::kEventRingCapacity) {
    ++count_;
  }
  ++totalPublished_;
  return totalPublished_;
}

const Event* EventBus::at(size_t index) const {
  if (index >= count_) {
    return nullptr;
  }
  // O mais antigo está em (head_ - count_) módulo capacidade.
  size_t start = (head_ + cfg::kEventRingCapacity - count_) % cfg::kEventRingCapacity;
  size_t pos = (start + index) % cfg::kEventRingCapacity;
  return &ring_[pos];
}

const Event* EventBus::latestOfType(EventType type) const {
  // Varre do mais novo para o mais antigo.
  for (size_t i = count_; i > 0; --i) {
    const Event* ev = at(i - 1);
    if (ev && ev->type == type) {
      return ev;
    }
  }
  return nullptr;
}

void EventBus::clear() {
  for (size_t i = 0; i < cfg::kEventRingCapacity; ++i) {
    ring_[i] = Event{};
  }
  head_ = 0;
  count_ = 0;
  // totalPublished_ é mantido como contador de sessão; zere no desligamento.
}

}  // namespace mak
