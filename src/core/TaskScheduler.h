// TaskScheduler.h — agendador cooperativo não bloqueante.
//
// Evita delay() nos handlers (plano §6). Cada tarefa roda a cada `intervalMs`
// quando `tick(nowMs)` é chamado. O tempo é injetado para permitir teste no
// host. Sem alocação dinâmica; nº fixo de slots.
#pragma once

#include <cstddef>
#include <cstdint>

namespace mak {

using TaskFn = void (*)(void* ctx);

class TaskScheduler {
 public:
  static constexpr size_t kMaxTasks = 12;

  // Registra uma tarefa periódica. Retorna false se não há slot livre.
  bool every(uint32_t intervalMs, TaskFn fn, void* ctx = nullptr);

  // Executa as tarefas vencidas para o instante `nowMs` (millis()).
  void tick(uint32_t nowMs);

  size_t count() const { return count_; }

 private:
  struct Task {
    uint32_t intervalMs = 0;
    uint32_t lastRunMs = 0;
    TaskFn fn = nullptr;
    void* ctx = nullptr;
    bool used = false;
  };

  Task tasks_[kMaxTasks];
  size_t count_ = 0;
};

}  // namespace mak
