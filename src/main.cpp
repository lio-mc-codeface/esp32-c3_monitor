/**************************************************************
 * Project: ESP32-C3 Nebula Pulse Visualizer
 * Hardware: Seeed Studio XIAO ESP32-C3, GC9A01 Round Display, 
 * INMP441 I2S Microphone
 * * Pin Mapping:
 * Pin #  |   Label   |  ESP32 Pin  |       Function       
 *------------------------------------------------------------
 * 1    |   **DC** |    **6** |  Display Data/Cmd    
 * 2    |   **CS** |    **7** |  Display Chip Select 
 * 3    |   **SCLK** |    **8** |  Display SPI Clock   
 * 4    |   **SDA** |   **10** |  Display SPI MOSI    
 * 5    |  **MIC-S** |    **2** |  I2S Clock (SCK)     
 * 6    |  **MIC-W** |    **3** |  I2S Word Sel (WS)   
 * 7    |  **MIC-D** |    **4** |  I2S Data Out (SD)   
 * 8    |  **L/R** |   **GND** |  Channel Select      
 **************************************************************/

#include <Arduino_GFX_Library.h>
#include <driver/i2s.h>

// I2S Configuration
#define I2S_WS    3 
#define I2S_SD    4
#define I2S_SCK   2 
#define I2S_PORT  I2S_NUM_0

// Display Configuration
#define TFT_DC    6
#define TFT_CS    7
#define TFT_SCLK  8
#define TFT_MOSI  10

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, -1);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, -1, 0, true);

// Spring Physics Constants
float currentR = 60.0, targetR = 60.0, velocity = 0;
float stiffness = 0.55;  // Adjust for "snappiness"
float damping = 0.75;    // Adjust for "bounciness"
float beatThreshold = 200;

void setup() {
    Serial.begin(115200);
    
    // Init Display
    gfx->begin();
    gfx->fillScreen(BLACK);
    
    // Init I2S Microphone
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
        .bck_io_num = I2S_SCK, .ws_io_num = I2S_WS, .data_out_num = -1, .data_in_num = I2S_SD
    };
    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config);
}

void loop() {
    int32_t samples[32];
    size_t bytes_read = 0;
    
    i2s_read(I2S_PORT, &samples, sizeof(samples), &bytes_read, portMAX_DELAY);
    
    float peak = 0;
    if (bytes_read > 0) {
        for (int i = 0; i < 32; i++) {
            float s = abs(samples[i] >> 14); 
            if (s > peak) peak = s;
        }
    }

    // Update target radius based on mic peak
    if (peak > beatThreshold) {
        targetR = map(constrain(peak, 200, 4000), 200, 4000, 60, 115);
        beatThreshold = peak * 0.9;
    } else {
        targetR = 60;
        beatThreshold *= 0.97;
        if (beatThreshold < 200) beatThreshold = 200;
    }

    // Calculate spring physics
    velocity += (targetR - currentR) * stiffness;
    velocity *= damping;
    currentR += velocity;

    static int lastR = 60;
    int r = (int)currentR;

    if (r != lastR) {
        // Erase
        gfx->drawCircle(120, 120, lastR, BLACK);
        gfx->drawCircle(120, 120, lastR + 2, BLACK);
        
        // Draw (Dynamic color based on radius)
        uint16_t color1 = gfx->color565(r, 255 - r, 255); // Blue to White transition
        uint16_t color2 = gfx->color565(255, 50, r * 2);  // Purple glow
        
        gfx->drawCircle(120, 120, r, color1); 
        gfx->drawCircle(120, 120, r + 2, color2); 
        
        lastR = r;
    }
}