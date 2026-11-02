#include "security/RateLimiter.h"

namespace mak {

bool RateLimiter::allow(uint32_t nowMs) {
  if ((uint32_t)(nowMs - windowStartMs_) >= cfg::kRateWindowMs) {
    windowStartMs_ = nowMs;
    hitsInWindow_ = 0;
  }
  if (hitsInWindow_ >= cfg::kRateMaxHitsPerWindow) {
    return false;
  }
  ++hitsInWindow_;
  return true;
}

bool RateLimiter::registerLoginFailure(uint32_t nowMs) {
  if (loginFails_ < 255) ++loginFails_;
  if (loginFails_ >= cfg::kMaxLoginFailsBeforeLockout) {
    lockoutUntilMs_ = nowMs + cfg::kLoginLockoutMs;
    return true;
  }
  return false;
}

void RateLimiter::registerLoginSuccess() {
  loginFails_ = 0;
  lockoutUntilMs_ = 0;
}

bool RateLimiter::isLockedOut(uint32_t nowMs) const {
  if (lockoutUntilMs_ == 0) return false;
  return (int32_t)(lockoutUntilMs_ - nowMs) > 0;
}

}  // namespace mak
