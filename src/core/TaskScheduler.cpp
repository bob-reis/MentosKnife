#include "core/TaskScheduler.h"

namespace mak {

bool TaskScheduler::every(uint32_t intervalMs, TaskFn fn, void* ctx) {
  if (count_ >= kMaxTasks || fn == nullptr) {
    return false;
  }
  for (auto& t : tasks_) {
    if (!t.used) {
      t.intervalMs = intervalMs;
      t.lastRunMs = 0;
      t.fn = fn;
      t.ctx = ctx;
      t.used = true;
      ++count_;
      return true;
    }
  }
  return false;
}

void TaskScheduler::tick(uint32_t nowMs) {
  for (auto& t : tasks_) {
    if (!t.used) {
      continue;
    }
    // Subtração não assinada lida corretamente com o wrap de millis().
    if ((uint32_t)(nowMs - t.lastRunMs) >= t.intervalMs) {
      t.lastRunMs = nowMs;
      t.fn(t.ctx);
    }
  }
}

}  // namespace mak
