#include <Arduino_GFX_Library.h>
#include <driver/i2s.h>
#include "LittleFS.h"

// --- Hardware Pins (ESP32-C3) ---
#define PIN_TFT_DC   5
#define PIN_TFT_CS   9
#define PIN_TFT_SCLK 8
#define PIN_TFT_MOSI 10

// --- I2S Microphone Pins ---
#define I2S_WS       3
#define I2S_SD       4
#define I2S_SCK      2
#define I2S_PORT     I2S_NUM_0

// --- Display Setup ---
Arduino_DataBus *bus = new Arduino_ESP32SPI(PIN_TFT_DC, PIN_TFT_CS, PIN_TFT_SCLK, PIN_TFT_MOSI, -1);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, -1, 0, true);

uint16_t frameBuffer[125 * 125]; 
int lastImageIndex = -1;

void setup_i2s() {
    const i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 1024,
        .use_apll = false
    };
    const i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = -1,
        .data_in_num = I2S_SD
    };
    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config);
}

void setup() {
    Serial.begin(115200);
    if(!LittleFS.begin()) return;

    gfx->begin();

    gfx->fillScreen(BLACK);
    setup_i2s();
}

void loop() {
    // 1. Read Audio Data
    int32_t samples[64];
    size_t bytes_read;
    i2s_read(I2S_PORT, &samples, sizeof(samples), &bytes_read, portMAX_DELAY);
    
    // 2. Calculate Volume (RMS)
    float sum_sq = 0;
    int count = bytes_read / 4;
    for (int i = 0; i < count; i++) {
        float s = (float)samples[i];
        sum_sq += s * s;
    }
    float rms = sqrt(sum_sq / count);

    // --- TUNING SECTION ---
    // Based on your data, silence is ~1,000,000. 
    // Increase 'maxAudioValue' to make it LESS sensitive (requires louder sound).
    // Decrease it to make it MORE sensitive (reaches image 17 easier).
    int silenceFloor = 1200000; 
    int maxAudioValue = 6000000; // <--- this is the sensitivity

    // 3. Map to 18 images (0 through 17)
    int imgIndex = map((int)rms, silenceFloor, maxAudioValue, 0, 17);
    imgIndex = constrain(imgIndex, 0, 17);

    // 4. Update screen only if the image index changes
    if (imgIndex != lastImageIndex) {
        char filename[16];
        sprintf(filename, "/%d.bin", imgIndex);
        
        File file = LittleFS.open(filename, "r");
        if (file) {
            file.read((uint8_t*)frameBuffer, sizeof(frameBuffer));
            file.close();
            // Since you used '<H' in Python, no extra 'true' flag is needed here
            gfx->draw16bitRGBBitmap(57, 57, frameBuffer, 125, 125);
        }
        lastImageIndex = imgIndex;
    }
}