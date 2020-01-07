#pragma once

struct Message {
  enum kind { printerState, temperatureAndHumidity };
  kind kind;
  int value;
};

struct TemperatureAndHumidity {
  static int toValue(int temperature, int humidity) {
    return temperature | (humidity << 8);
  }

  static int getTemperature(int value) { return value & 0xff; }
  static int getHumiduty(int value) { return (value & 0xff00) >> 8; }
};

struct PrinterState {
  enum state { unknown, off, booting, ok, shutingDown };
  static String toString(state state) {
    if (state == unknown) return "未知";
    if (state == off) return "睡眠";
    if (state == booting) return "正财启动";
    if (state == ok) return "OK";
    if (state == shutingDown) return "正在关机";
  };
};
