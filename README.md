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