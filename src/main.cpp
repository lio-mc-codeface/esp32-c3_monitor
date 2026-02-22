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
uint16_t *heartBuffer;
const uint32_t imageSize = 105800; // 230x230x2
int heartSizes[] = {100, 106, 112, 118, 124, 130};
int lastHeartIndex = -1;
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
    
    // Allocate Memory
    frameBuffer = (uint16_t *)malloc(imageSize);
    heartBuffer = (uint16_t *)malloc(130 * 130 * 2);

    if (!frameBuffer || !heartBuffer) {
        Serial.println("Memory Allocation Failed!");
        while(1);
    }

    if(!LittleFS.begin(true)) {
        Serial.println("LittleFS Failed!");
        while(1);
    }

    // Initialize Sensor
    Wire.begin(I2C_SDA, I2C_SCL);
    if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
        Serial.println("MAX30102 not found.");
    } else {
        particleSensor.setup();
        particleSensor.setPulseAmplitudeRed(0x0A);
    }
    
    gfx->begin();
    gfx->fillScreen(BLACK);
    setup_i2s();
    Serial.println("System Ready");
}

void loop() {
    // 1. Audio Logic
    int32_t samples[64];
    size_t bytes_read;
    i2s_read(I2S_PORT, &samples, sizeof(samples), &bytes_read, portMAX_DELAY);
    
    float sum_sq = 0;
    for (int i = 0; i < 64; i++) {
        float s = (float)samples[i];
        sum_sq += s * s;
    }
    float rms = sqrt(sum_sq / 64);
    int imgIndex = map((int)rms, 1200000, 60000000, 0, 17);
    imgIndex = constrain(imgIndex, 0, 17);

    // 2. Heart Logic
    long irValue = particleSensor.getIR();
    int heartIndex = 0;
    if (irValue > 50000) {
        long delta = irValue - lastIR;
        lastIR = irValue;
        heartIndex = map(delta, 25, 300, 0, 5); // Tweaked for sensitivity
        heartIndex = constrain(heartIndex, 0, 5);
    }

    // 3. Draw Background Ring
    char bName[32];
    sprintf(bName, "/%d.bin", imgIndex);
    File bFile = LittleFS.open(bName, "r");
    if (bFile) {
        bFile.read((uint8_t*)frameBuffer, imageSize);
        bFile.close();
        gfx->draw16bitRGBBitmap(5, 5, frameBuffer, 230, 230);
    }

    // 4. Draw Heart Layer (Transparent/Masked)
    // 4. Draw Heart Layer (Transparent/Manual Chroma Key)
    if (irValue > 50000) {
        char hName[32];
        sprintf(hName, "/h%d.bin", heartIndex);
        File hFile = LittleFS.open(hName, "r");
        if (hFile) {
            int hSize = heartSizes[heartIndex];
            hFile.read((uint8_t*)heartBuffer, hSize * hSize * 2);
            hFile.close();
            
            int startX = (240 - hSize) / 2;
            int startY = (240 - hSize) / 2;
            
            // Manually draw pixels, skipping pure black (0x0000)
            for (int y = 0; y < hSize; y++) {
                for (int x = 0; x < hSize; x++) {
                    uint16_t color = heartBuffer[y * hSize + x];
                    if (color != 0x0000) { // Transparency Check
                        gfx->drawPixel(startX + x, startY + y, color);
                    }
                }
            }
        }
    }
}