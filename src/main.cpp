#include "Adafruit_GC9A01A.h"
#include <Wire.h>
#include "MAX30105.h"

// Screen Pins
#define TFT_CS   5
#define TFT_DC   21
#define TFT_MOSI 10
#define TFT_SCLK 8
#define TFT_RST  -1

Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
MAX30105 particleSensor;

// Logic Variables
long movingAverage = 0;
const int filterWeight = 20; // How "smooth" the average is
bool heartShowing = false;
unsigned long heartStartTime = 0;

void drawHeart(int x, int y, int size, uint16_t color) {
  tft.fillCircle(x - size/4, y - size/4, size/4, color);
  tft.fillCircle(x + size/4, y - size/4, size/4, color);
  tft.fillTriangle(x - size/2, y - size/6, x + size/2, y - size/6, x, y + size/2, color);
}

void setup() {
  delay(3000);
  Serial.begin(115200);
  tft.begin();
  tft.fillScreen(GC9A01A_BLUE);
  
  Wire.begin(6, 7);
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) while(1);
  particleSensor.setup();
}

void loop() {
  long irValue = particleSensor.getIR();

  if (irValue > 50000) { // Finger is present
    // 1. Update Moving Average
    // Formula: Average = (Current + (Average * (N-1))) / N
    movingAverage = (irValue + (movingAverage * (filterWeight - 1))) / filterWeight;

    // 2. Detection Logic: If current value drops significantly below average
    // We use a sensitivity threshold (e.g., 200 units drop)
    if (irValue < (movingAverage - 250) && !heartShowing) {
      drawHeart(120, 100, 80, GC9A01A_RED);
      heartShowing = true;
      heartStartTime = millis();
      Serial.println("BEAT DETECTED!");
    }

    // 3. Timing Logic: Hide heart after 0.3 seconds
    if (heartShowing && (millis() - heartStartTime > 300)) {
      // Erase the heart by drawing a blue circle over it
      tft.fillCircle(120, 100, 45, GC9A01A_BLUE); 
      heartShowing = false;
    }

    // Optional: Print to Serial Plotter to see the Average vs Raw
    Serial.print("Raw:"); Serial.print(irValue);
    Serial.print(",");
    Serial.print("Avg:"); Serial.println(movingAverage);

  } else {
    // No finger: Reset and show message
    if (movingAverage != 0) {
        tft.fillScreen(GC9A01A_BLUE);
        tft.setCursor(50, 110);
        tft.setTextColor(GC9A01A_WHITE);
        tft.print("READY...");
        movingAverage = 0;
    }
  }
}