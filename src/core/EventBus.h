// EventBus.h — ring buffer de eventos efêmeros em RAM.
//
// Sem alocação dinâmica; descarta o mais antigo ao encher (plano §5 "ring
// buffer com descarte automático"). `clear()` zera tudo no STOP/desligamento.
// Código puro (sem Arduino) para ser testável no host (env:native).
#pragma once

#include <cstddef>
#include <cstdint>

#include "Event.h"

namespace mak {

class EventBus {
 public:
  EventBus() = default;

  // Publica um evento (cópia). Retorna o total já publicado desde o boot.
  uint32_t publish(const Event& ev);

  // Nº de eventos atualmente retidos no buffer.
  size_t size() const { return count_; }
  bool empty() const { return count_ == 0; }
  size_t capacity() const { return cfg::kEventRingCapacity; }

  // Total publicado desde o boot (inclui os já descartados).
  uint32_t totalPublished() const { return totalPublished_; }

  // Acesso ordenado do mais antigo (index 0) ao mais novo.
  // Retorna nullptr se index >= size().
  const Event* at(size_t index) const;

  // Último evento de um tipo (nullptr se não houver).
  const Event* latestOfType(EventType type) const;

  // Apaga toda a telemetria efêmera.
  void clear();

 private:
  Event ring_[cfg::kEventRingCapacity];
  size_t head_ = 0;      // posição do próximo write
  size_t count_ = 0;     // itens válidos
  uint32_t totalPublished_ = 0;
};

}  // namespace mak
