#include "Adafruit_GC9A01A.h"
#include <Wire.h>
#include <driver/i2s.h>

#define TFT_CS 5
#define TFT_DC 21
#define TFT_MOSI 10
#define TFT_SCLK 8
#define TFT_RST -1
#define I2S_PORT I2S_NUM_0

Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// Physics: Tuned for snappiness
float currentR = 60.0, targetR = 60.0, velocity = 0;
float stiffness = 0.85; 
float damping = 0.6; 
float beatThreshold = 150;

void setupI2S() {
    const i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 2,
        .dma_buf_len = 32 // Low latency
    };
    const i2s_pin_config_t pin_config = {.bck_io_num = 3, .ws_io_num = 2, .data_out_num = -1, .data_in_num = 4};
    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config);
}

void setup() {
    tft.begin(80000000); // Max SPI Clock
    tft.setRotation(0);
    tft.fillScreen(GC9A01A_BLACK);
    setupI2S();
}

void loop() {
    int32_t samples[16]; // Tiny sample window for instant triggers
    size_t bytes_read;
    i2s_read(I2S_PORT, &samples, sizeof(samples), &bytes_read, portMAX_DELAY);
    
    float peak = 0;
    for (int i = 0; i < 16; i++) {
        float s = abs(samples[i] >> 16); 
        if (s > peak) peak = s;
    }

    // TRIGGER LOGIC
    if (peak > beatThreshold && peak > 100) {
        targetR = map(constrain(peak, 100, 2000), 100, 2000, 70, 115);
        beatThreshold = peak * 0.85;
    } else {
        targetR = 60.0;
        beatThreshold *= 0.94;
        if (beatThreshold < 150) beatThreshold = 150;
    }

    // SPRING PHYSICS
    velocity += (targetR - currentR) * stiffness;
    velocity *= damping;
    currentR += velocity;

    static int lastR = 60;
    int r = (int)currentR;

    if (r != lastR) {
        // 1. ERASE PREVIOUS (Black)
        tft.drawCircle(120, 120, lastR, GC9A01A_BLACK);
        tft.drawCircle(120, 120, lastR + 2, GC9A01A_BLACK);
        tft.drawCircle(120, 120, lastR + 4, GC9A01A_BLACK);

        // 2. DRAW NEW (Nebula Colors)
        tft.drawCircle(120, 120, r, tft.color565(0, 255, 255));      // Cyan
        tft.drawCircle(120, 120, r + 2, tft.color565(180, 50, 255)); // Purple
        tft.drawCircle(120, 120, r + 4, tft.color565(120, 0, 80));   // Dark Red/Purple
        
        lastR = r;
    }
}