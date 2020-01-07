#include <Arduino.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <U8x8lib.h>
#include <WiFi.h>
#include "messages.h"

static xQueueHandle displayQueue = nullptr;

void printerStateTask(void*) {
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
        Message m{.kind = Message::printerState, .value = PrinterState::ok};
        xQueueSendToBack(displayQueue, &m, portMAX_DELAY);
      } else {
        Message m{.kind = Message::printerState,
                  .value = PrinterState::unknown};
        xQueueSendToBack(displayQueue, &m, portMAX_DELAY);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(50000));
  }
}

void displayTask(void*) {
  U8G2_ST7920_128X64_F_SW_SPI display(U8G2_R0, 21, 22, 23);
  display.begin();
  display.setFont(u8g2_font_wqy12_t_gb2312a);

  int temperature = -1;
  int humidity = -1;
  PrinterState::state printerState = PrinterState::unknown;

  while (true) {
    display.firstPage();
    display.drawUTF8(
        0, 15,
        (String("状态 ") + PrinterState::toString(printerState)).c_str());

    int line2x = 0;
    if (temperature != -1)
      display.drawUTF8(line2x++, 35, (String("温度 ") + temperature).c_str());
    if (humidity != -1)
      display.drawUTF8(line2x * 50, 35, (String("湿度 ") + humidity).c_str());
    display.nextPage();

    Message msg;
    if (xQueueReceive(displayQueue, &msg, pdMS_TO_TICKS(10000))) {
      log_printf("update display %d->%d\n", msg.kind, msg.value);
      switch (msg.kind) {
        case Message::printerState:
          printerState = static_cast<PrinterState::state>(msg.value);
          break;
        case Message::temperatureAndHumidity:
          temperature = TemperatureAndHumidity::getTemperature(msg.value);
          humidity = TemperatureAndHumidity::getHumiduty(msg.value);
          break;
      }
    }
  }
}

void temperatureTask(void*) {
  DHT dht;
  dht.setup(19);

  int lastTemperature = 0;
  int lastHumidity = 0;

  while (true) {
    // 测量温度和湿度
    int temperature = 0;
    int humidity = 0;
    if (const float t = dht.getTemperature())
      if (!isnan(t)) temperature = t;
    if (const float h = dht.getHumidity())
      if (!isnan(h)) humidity = h;

    humidity = random(50);
    temperature = random(50);

    if (lastTemperature != temperature || lastHumidity != humidity) {
      Message m{
          .kind = Message::temperatureAndHumidity,
          .value = TemperatureAndHumidity::toValue(temperature, humidity)};
      xQueueSendToBack(displayQueue, &m, portMAX_DELAY);
      lastTemperature = temperature;
      lastHumidity = humidity;
    }
    vTaskDelay(pdMS_TO_TICKS(50000));
  }
}

void setup() {
  Serial.begin(115200);

  displayQueue = xQueueCreate(10, sizeof(Message));

  xTaskCreate(displayTask, "Display", 4096, nullptr, 0, nullptr);
  xTaskCreate(printerStateTask, "Printer State", 4096, nullptr, 0, nullptr);
  xTaskCreate(temperatureTask, "Temperature & Humidity", 4096, nullptr, 0,
              nullptr);
}

void loop() { vTaskDelay(portMAX_DELAY); }
