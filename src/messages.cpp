#include "messages.h"
#include <FreeRTOS.h>
#include <algorithm>
#include <functional>
#include <set>

static std::set<xTaskHandle> activeStateNotifications;
static std::set<xTaskHandle> passiveStateNotifications;
static ActiveState activeState;
static PassiveState passiveState;

void updateState(const std::function<void(ActiveState&)>& f) {
  f(activeState);
  for (const auto& handle : activeStateNotifications) xTaskNotifyGive(handle);
}

void updateState(const std::function<void(PassiveState&)>& f) {
  f(passiveState);

  for (const auto& handle : passiveStateNotifications) {
    xTaskNotifyGive(handle);
    log_printf("Task %d notified\n", handle);
  }
}

const PassiveState& waitForPassiveStateUpdate() {
  const auto handle = xTaskGetCurrentTaskHandle();
  passiveStateNotifications.emplace(handle);
  log_printf("Task %d waiting\n", handle);
  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  return passiveState;
}
