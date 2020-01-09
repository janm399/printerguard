#pragma once
#include <Arduino.h>
#include <functional>

struct ActiveState {
  enum PrinterPowerState { on, off };
  bool fanOn;
  int lidAngle;

  PrinterPowerState printerPowerState;
};

struct PassiveState {
  struct PrinterApiState {
    enum state { unknown, off, booting, ok, shutingDown };
    static String toString(state state) {
      if (state == unknown) return "未知";
      if (state == off) return "睡眠";
      if (state == booting) return "正财启动";
      if (state == ok) return "OK";
      if (state == shutingDown) return "正在关机";
    };
  };

  int temperature;
  int humidity;
  int smokeDetected;
  PrinterApiState::state printerApiState;
};

void updateState(const std::function<void(PassiveState&)>& f);
void updateState(const std::function<void(ActiveState&)>& f);

const PassiveState& waitForPassiveStateUpdate();

const ActiveState& waitForActiveStateUpdate();
