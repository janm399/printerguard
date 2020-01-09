#include <Arduino.h>
#include <DHTesp.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <U8x8lib.h>
#include <WiFi.h>
#include "messages.h"

void printerStateTask(void *) {
  Preferences preferences;
  preferences.begin("default");
  // 把下面的SSID和password改变成你的Wi-Fi的
  // preferences.putString("SSID", "***");
  // preferences.putString("password", "***");
  const auto ssid = preferences.getString("SSID");
  const auto password = preferences.getString("password");
  log_printf("WiFi connect with %s %s\n", ssid.c_str(), password.c_str());
  WiFi.begin(ssid.c_str(), password.c_str());
  WiFi.setAutoConnect(true);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    log_printf("WiFi not connected %d\n", WiFi.status());
  } else {
    log_printf("WiFi connected %s\n", WiFi.localIP().toString().c_str());
  }

  while (true) {
    if (WiFi.isConnected()) {
      HTTPClient http;
      http.begin("http://192.168.1.240/api/printer");
      http.addHeader("X-Api-Key", "981d3e22ec47f07cebc8b1df57c8ba89");
      if (http.GET() == 200) {
        const auto response = http.getString();
        updateState([](PassiveState &state) {
          state.printerApiState = PassiveState::PrinterApiState::ok;
        });
      } else {
        updateState([](PassiveState &state) {
          state.printerApiState = PassiveState::PrinterApiState::unknown;
        });
      }
    } else {
      updateState([](PassiveState &state) {
        state.printerApiState = PassiveState::PrinterApiState::unknown;
      });
    }
    vTaskDelay(pdMS_TO_TICKS(50000));
  }
}

void displayTask(void *) {
  U8G2_ST7920_128X64_F_SW_SPI display(U8G2_R0, 21, 22, 23);
  display.begin();
  display.setFont(u8g2_font_wqy12_t_gb2312a);

  while (true) {
    const auto &ps = waitForPassiveStateUpdate();
    display.firstPage();
    const auto printerState =
        PassiveState::PrinterApiState::toString(ps.printerApiState);
    display.drawUTF8(0, 15, (String("打印机的状态 ") + printerState).c_str());

    int line2x = 0;
    if (ps.temperature != -1)
      display.drawUTF8(line2x++, 35,
                       (String("温度 ") + ps.temperature).c_str());
    if (ps.humidity != -1)
      display.drawUTF8(line2x * 50, 35,
                       (String("湿度 ") + ps.humidity).c_str());
    if (ps.smokeDetected > 0) {
      display.drawUTF8(0, 50,
                       (String("烟雾被探测了 ") + ps.smokeDetected).c_str());
    }
    display.nextPage();
  }
}

void sensorsTask(void *) {
  DHTesp dht;
  dht.setup(19, DHTesp::DHT22);
  pinMode(18, INPUT_PULLUP);

  int smokeDetected = 0;

  while (true) {
    // 测量温度和湿度
    int temperature = 0;
    int humidity = 0;
    if (const float t = dht.getTemperature())
      if (!isnan(t)) temperature = t;
    if (const float h = dht.getHumidity())
      if (!isnan(h)) humidity = h;

    if (const bool s = digitalRead(18) == LOW)
      smokeDetected++;
    else
      smokeDetected = 0;

    updateState([&](PassiveState &state) {
      state.temperature = temperature;
      state.humidity = humidity;
      state.smokeDetected = smokeDetected;
    });
    vTaskDelay(pdMS_TO_TICKS(50000));
  }
}

void setup() {
  Serial.begin(115200);

  xTaskCreate(displayTask, "Display", 4096, nullptr, 0, nullptr);
  xTaskCreate(printerStateTask, "Printer State", 4096, nullptr, 0, nullptr);
  xTaskCreate(sensorsTask, "Sensors", 4096, nullptr, 0, nullptr);
}

void loop() { vTaskDelay(portMAX_DELAY); }
