## Components
- Arduino Pro Mini 8 MHz 3.3 V
- N-Channel MOSFET (N-Channel 30V 62A) IRLB8721PBF
- Dragino LoRa Bee (Semtech SX1276)
- Adafruit Micro-SD Breakout board

## How to connect peripherals to the board
#### N-Channel MOSFET##### SD-Card Reader
| MOSFER PIN  | Arduino  |
|:--------:|  :-------------:   |
|    G    |         SPI\_CS (D10)          |
|    D    |         SPI\_MOSI (D11)         |  
|    S    |         SPI\_MISO (D12)         |
|    CLK   |         SPI1\_SCLK (D13)         |
|    GND   |         GND        |
|    3.3v    |         3.3v         |
|    5v    |         N/A        |
**!NB: Micro-SD card needs to be formated as FAT32**
