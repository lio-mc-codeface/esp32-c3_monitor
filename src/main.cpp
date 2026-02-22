#include <Arduino_GFX_Library.h>
#include <driver/i2s.h>
#include "LittleFS.h"
#include <Wire.h>
#include "MAX30105.h"

// --- Pins ---
#define PIN_TFT_DC   5
#define PIN_TFT_CS   9
#define PIN_TFT_SCLK 8
#define PIN_TFT_MOSI 10
#define I2C_SDA      6
#define I2C_SCL      7
#define I2S_WS       3
#define I2S_SD       4
#define I2S_SCK      2
#define I2S_PORT     I2S_NUM_0

// --- Objects ---
Arduino_DataBus *bus = new Arduino_ESP32SPI(PIN_TFT_DC, PIN_TFT_CS, PIN_TFT_SCLK, PIN_TFT_MOSI, -1);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, -1, 0, true);
MAX30105 particleSensor;

// --- Buffers & Settings ---
uint16_t *frameBuffer;
uint16_t *heartFrames[6]; 
const uint32_t imageSize = 105800; // 230x230x2
int heartSizes[] = {100, 106, 112, 118, 124, 130};
long lastIR = 0;

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
    frameBuffer = (uint16_t *)malloc(imageSize);
    if(!LittleFS.begin(true)) while(1);

    // Pre-load hearts to RAM
    for (int i = 0; i < 6; i++) {
        char hName[16]; sprintf(hName, "/h%d.bin", i);
        File f = LittleFS.open(hName, "r");
        if (f) {
            size_t s = heartSizes[i] * heartSizes[i] * 2;
            heartFrames[i] = (uint16_t*)malloc(s);
            if (heartFrames[i]) f.read((uint8_t*)heartFrames[i], s);
            f.close();
        }
    }

    Wire.begin(I2C_SDA, I2C_SCL);
    if (particleSensor.begin(Wire, I2C_SPEED_FAST)) {
        particleSensor.setup();
        particleSensor.setPulseAmplitudeRed(0x0A);
    }
    
    gfx->begin();
    gfx->fillScreen(BLACK);
    setup_i2s();
}

void loop() {
    // 1. Audio Sensing (1ms timeout)
    int32_t samples[64];
    size_t bytes_read = 0;
    i2s_read(I2S_PORT, &samples, sizeof(samples), &bytes_read, 1); 
    
    int imgIndex = 0; 
    if (bytes_read > 0) {
        float sum_sq = 0;
        for (int i = 0; i < 64; i++) { float s = (float)samples[i]; sum_sq += s * s; }
        imgIndex = map((int)sqrt(sum_sq / 64), 1200000, 60000000, 0, 17);
        imgIndex = constrain(imgIndex, 0, 17);
    }

    // 2. Heart Sensing
    long irValue = particleSensor.getIR();
    int heartIndex = -1; 
    if (irValue > 50000) {
        long delta = irValue - lastIR;
        lastIR = irValue;
        heartIndex = 0; // Default baseline
        if (delta > 25) {
            heartIndex = map(delta, 25, 300, 1, 5);
            heartIndex = constrain(heartIndex, 1, 5);
        }
    }

    // 3. Draw Background
    static int lastImgIndex = -1;
    bool bgUpdated = false;
    if (imgIndex != lastImgIndex) {
        char bName[16]; sprintf(bName, "/%d.bin", imgIndex);
        File bFile = LittleFS.open(bName, "r");
        if (bFile) {
            bFile.read((uint8_t*)frameBuffer, imageSize);
            bFile.close();
            gfx->draw16bitRGBBitmap(5, 5, frameBuffer, 230, 230);
            lastImgIndex = imgIndex;
            bgUpdated = true;
        }
    }

    // 4. Draw Heart from RAM
    static int lastHeartIdx = -1;
    if (heartIndex != -1) {
        if (heartIndex != lastHeartIdx || bgUpdated) {
            uint16_t* hBuf = heartFrames[heartIndex];
            int hSize = heartSizes[heartIndex];
            int start = (240 - hSize) / 2;
            for (int y = 0; y < hSize; y++) {
                for (int x = 0; x < hSize; x++) {
                    uint16_t color = hBuf[y * hSize + x];
                    if (color != 0x0000) gfx->drawPixel(start + x, start + y, color);
                }
            }
            lastHeartIdx = heartIndex;
        }
    } else if (lastHeartIdx != -1) {
        lastImgIndex = -1; // Force BG redraw to wipe heart
        lastHeartIdx = -1;
    }
    
    delay(1);
}