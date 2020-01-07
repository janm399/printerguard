#include <Arduino.h>
#include <DHT.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <U8x8lib.h>

U8G2_ST7920_128X64_F_SW_SPI display(U8G2_R0, 21, 22, 23);
DHT dht;

void setup() {
  Serial.begin(115200);
  display.begin();
  // display.setFont(u8g2_font_unifont_t_chinese3);
  display.setFont(u8g2_font_wqy13_t_gb2312a);
  dht.setup(19, DHT::DHT11);
  Serial.println("Setup done");
}

void loop() {
  String line1 = String("终于成功了") + (millis() / 1000);

  String line2 =
      String("温度 ") + dht.getTemperature() + "，湿度 " + dht.getHumidity();

  display.firstPage();
  display.drawUTF8(0, 15, line1.c_str());
  display.drawUTF8(0, 35, line2.c_str());
  display.nextPage();
  delay(1000);
}
