// display.cpp
#include "display.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "sensors.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Смайлы
void drawHappyFace(int x, int y) {
  display.drawCircle(x + 10, y + 10, 10, SSD1306_WHITE);
  display.fillRect(x + 5, y + 4, 3, 3, SSD1306_WHITE);
  display.fillRect(x + 12, y + 4, 3, 3, SSD1306_WHITE);
  display.drawLine(x + 5, y + 13, x + 15, y + 13, SSD1306_WHITE);
  display.drawPixel(x + 6, y + 14, SSD1306_WHITE);
  display.drawPixel(x + 14, y + 14, SSD1306_WHITE);
}

void drawNeutralFace(int x, int y) {
  display.drawCircle(x + 10, y + 10, 10, SSD1306_WHITE);
  display.fillRect(x + 5, y + 4, 3, 3, SSD1306_WHITE);
  display.fillRect(x + 12, y + 4, 3, 3, SSD1306_WHITE);
  display.drawLine(x + 5, y + 13, x + 15, y + 13, SSD1306_WHITE);
}

void drawSadFace(int x, int y) {
  display.drawCircle(x + 10, y + 10, 10, SSD1306_WHITE);
  display.fillRect(x + 5, y + 4, 3, 3, SSD1306_WHITE);
  display.fillRect(x + 12, y + 4, 3, 3, SSD1306_WHITE);
  display.drawLine(x + 5, y + 14, x + 15, y + 14, SSD1306_WHITE);
  display.drawPixel(x + 6, y + 13, SSD1306_WHITE);
  display.drawPixel(x + 14, y + 13, SSD1306_WHITE);
}

void initDisplay() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED init failed");
    for (;;);
  }
  display.setRotation(1);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Init SCD41...");
  display.display();
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Температура
  display.setTextSize(1);
  display.setCursor(20, 4);
  display.print("Temp");
  display.setTextSize(2);
  display.setCursor(4, 16);
  display.print(temperature, 1);
  display.setTextSize(1);
  display.setCursor(53, 23);
  display.print("C");

  display.drawLine(0, 36, 64, 36, SSD1306_WHITE);

  // CO2
  display.setTextSize(2);
  if (co2 < 1000) {
    display.setCursor(14, 42);
  } else {
    display.setCursor(8, 42);
  }
  display.print(co2);
  display.setTextSize(1);
  display.setCursor(25, 57);
  display.print("ppm");

  // Смайлик
  int faceX = 22;
  int faceY = 69;
  if (co2 < 600) {
    drawHappyFace(faceX, faceY);
  } else if (co2 <= 1000) {
    drawNeutralFace(faceX, faceY);
  } else {
    drawSadFace(faceX, faceY);
  }

  display.drawLine(0, 94, 64, 94, SSD1306_WHITE);

  // Влажность
  display.setTextSize(1);
  display.setCursor(7, 99);
  display.print("Humidity");
  display.setTextSize(2);
  display.setCursor(20, 109);
  display.print(humidity, 0);
  display.setTextSize(1);
  display.setCursor(50, 115);
  display.print("%");

  display.display();
}