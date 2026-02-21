/**************************************************************
 * Pin #  |   Label   |  ESP32 Pin  |       Function       
 *------------------------------------------------------------
 * 1    |   **DC** |    **5** |  Display Data/Cmd (IO5)  
 * 2    |   **CS** |    **9** |  Display Chip Select(IO9)
 * 3    |   **SCLK** |    **8** |  Display SPI Clock (IO8) 
 * 4    |   **SDA** |   **10** |  Display SPI MOSI (IO10) 
 * 5    |  **MIC-S** |    **2** |  I2S Clock (IO2)     
 * 6    |  **MIC-W** |    **3** |  I2S Word Sel (IO3)   
 * 7    |  **MIC-D** |    **4** |  I2S Data Out (IO4)   
 * 8    |  **HB-SDA**|    **6** |  Heartbeat SDA (IO6)  
 * 9    |  **HB-SCL**|    **7** |  Heartbeat SCL (IO7)  
 **************************************************************/

#include <Arduino_GFX_Library.h>
#include <driver/i2s.h>
#include <Wire.h>
#include "MAX30105.h"

// Hardware Mapping
#define PIN_TFT_DC   5
#define PIN_TFT_CS   9
#define PIN_TFT_SCLK 8
#define PIN_TFT_MOSI 10

#define PIN_I2S_SCK  2
#define PIN_I2S_WS   3
#define PIN_I2S_SD   4

#define PIN_I2C_SDA  6
#define PIN_I2C_SCL  7

Arduino_DataBus *bus = new Arduino_ESP32SPI(PIN_TFT_DC, PIN_TFT_CS, PIN_TFT_SCLK, PIN_TFT_MOSI, -1);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, -1, 0, true);
MAX30105 hbSensor;

// Spring Physics Constants
float currentR = 60.0, targetR = 60.0, velocity = 0;
float stiffness = 0.55;  
float damping = 0.75;    
float beatThreshold = 200;

void setup() {
    Serial.begin(115200);
    
    // 1. Init I2C and Heartbeat
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 100000);
    if (hbSensor.begin(Wire, 100000)) {
        hbSensor.setup();
    }

    // 2. Init Display
    gfx->begin();
    gfx->fillScreen(BLACK);
    
    // 3. Init I2S Microphone
    const i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 128,
        .use_apll = false
    };
    const i2s_pin_config_t pin_config = {
        .bck_io_num = PIN_I2S_SCK, .ws_io_num = PIN_I2S_WS, .data_out_num = -1, .data_in_num = PIN_I2S_SD
    };
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
}

void loop() {
    // --- AUDIO PROCESSING ---
    int32_t samples[32];
    size_t bytes_read = 0;
    i2s_read(I2S_NUM_0, &samples, sizeof(samples), &bytes_read, portMAX_DELAY);
    
    float peak = 0;
    if (bytes_read > 0) {
        for (int i = 0; i < 32; i++) {
            float s = abs(samples[i] >> 14); 
            if (s > peak) peak = s;
        }
    }

    // Physics Engine Update
    if (peak > beatThreshold) {
        targetR = map(constrain(peak, 200, 4000), 200, 4000, 60, 115);
        beatThreshold = peak * 0.9;
    } else {
        targetR = 60;
        beatThreshold *= 0.97;
        if (beatThreshold < 200) beatThreshold = 200;
    }

    velocity += (targetR - currentR) * stiffness;
    velocity *= damping;
    currentR += velocity;

    // --- HEARTBEAT DATA (Ready for next step) ---
    long irValue = hbSensor.getIR();

    // --- RENDERING ---
    static int lastR = 60;
    int r = (int)currentR;

    if (r != lastR) {
        gfx->drawCircle(120, 120, lastR, BLACK);
        gfx->drawCircle(120, 120, lastR + 2, BLACK);
        
        uint16_t color1 = gfx->color565(r, 255 - r, 255); 
        uint16_t color2 = gfx->color565(255, 50, r * 2);  
        
        gfx->drawCircle(120, 120, r, color1); 
        gfx->drawCircle(120, 120, r + 2, color2); 
        
        lastR = r;
    }
}