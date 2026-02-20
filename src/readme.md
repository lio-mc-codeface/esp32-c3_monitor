. | Pin # | Label | ESP32 Pin | Function |
. |:-----:|:------------:|:-------------:|---------------------------|
. | D0 | I2S WS | IO2 | Mic Word Select |
. | D1 | I2S SCK | IO3 | Mic Bit Clock |
. | D2 | I2S SD | IO4 | Mic Serial Data |
. | D3 | SPI CS | IO5 | GC9A01 CS (Chip Select) |
. | D4 | I2C SDA | IO6 | MAX3010x SDA |
. | D5 | I2C SCL | IO7 | MAX3010x SCL |
. | D6 | ENC-SW | IO12 | Second Encoder Switch |
. | D7 | SPI DC | IO21 | GC9A01 DC (Data/Cmd) |
. | D8 | SPI SCK | IO8 | GC9A01 SCL (Clock) |
. | D10 | SPI MOSI | IO10 | GC9A01 SDA (Data) |
. | A2 | ENC-A | IO32 | Second Encoder A |
. | A3 | ENC-B | IO33 | Second Encoder B |
. | N/A | SPI RES | 3V3 | Tie Reset to 3.3V |

. | GC9A01 Pin | XIAO Pin | ESP32 Pin |
. |:----------:|:------------:|:-------------:|
. |     GND    |    GND |     GND |
. |     VCC    |    3V3 |     3V3 |
. |     SCL    |    D8 |     IO8 |
. |     SDA    |    D10 |     IO10 |
. |     RES    |    3V3 |   Physical|
. |     DC     |    D7 |     IO21 |
. |     CS     |    D3 |     IO5 |

Pin # | Label | ESP32 Pin | Function
  1   |  **D3** |   **IO5** | TFT_CS
  2   |  **D4** |   **IO6** | I2C_SDA (Sensor)
  3   |  **D5** |   **IO7** | I2C_SCL (Sensor)
  4   |  **D8** |   **IO8** | TFT_SCLK
  5   |  **D10** |   **IO10** | TFT_MOSI
  6   |  **D7** |   **IO21** | TFT_DC