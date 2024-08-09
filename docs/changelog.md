# UI chip
## In order of port
* PA0  - Battery Monitor -> Not connected
* PA1  - DISP_CS -> UI_LED_B
* PA2  - UI_TX2_NET -> No change
* PA3  - UI_RX2_NET -> No change
* PA4  - I2S_DACLRC -> UI_DEBUG_3
* PA5  - SPI1_CLK -> No change
* PA6  - DISP_RST -> UI_LED_R
* PA7  - SPI1_MOSI -> No Change
* PA8  - UI_STAT -> KB_ROW4
* PA9  - UI_TX1_MGMT -> No Change
* PA10 - UI_RX_MGMT -> No Change
* PA11 - Not connected -> UI_DEBUG_1
* PA12 - KB_LED -> UI_DEBUG_2
* PA13 - Not connected -> UI_SWDIO
* PA14 - Not connected -> UI_SWCLK
* PA15 - DISP_BL -> I2S_DACLRC

* PB0  - DISP_DC -> KB_ROW5
* PB1  - KB_MIC -> KB_ROW6
* PB2  - UI_BOOT1 -> No change
* PB3  - KB_ROW6 -> Not connected
* PB4  - KB_ROW7 -> I2S_ADCDAT
* PB5  - KB_COL5 -> I2S_DACDAT
* PB6  - UI_SCL -> No change
* PB7  - UI_SDA -> No change
* PB8  - KB_COL2 -> DISP_BL
* PB9  - KB_COL1 -> DISP_RST
* PB10 - Not connected -> KB_LED
* PB11 - Not connected -> KB_ROW7
* PB12 - KB_ROW1 -> No change
* PB13 - KB_ROW2 -> KB_COL1
* PB14 - KB_ROW3 -> KB_ROW2
* PB15 - KB_ROW4 -> KB_COL2

* PC0  - UI_LED_G -> PTT_BTN
* PC1  - UI_LED_R -> PTT_AI_BTN
* PC2  - Not connected -> USB_CC2_DETECT
* PC3  - Not connected -> USB_CC1_DETECT
* PC4  - Not connected -> BATTERY_MON
* PC5  - Not connected -> UI_LED_G
* PC6  - Not connected -> KB_COL3
* PC7  - Not connected -> KB_COL4
* PC8  - KB_ROW5 -> KB_ROW3
* PC9  - Not connected -> KB_COL5
* PC10 - I2S_BCLK -> No change
* PC11 - I2S_ADCDAT -> UI_STAT
* PC12 - I2S_DACDAT -> Not connected
* PC13 - Not connected -> DISP_DC
* PC14 - KB_COL3 -> DISP_CS
* PC15 - KB_COL4 -> Not connected

* PD2 -> UI_LED_B -> Not connected

## In order of change
## UI chip
* PA0  - BATTERY_MON -> Not connected
* PC4  - Not connected -> BATTERY_MON

* PA1  - DISP_CS -> UI_LED_B
* PC14 - KB_COL3 -> DISP_CS
* PD2 -> UI_LED_B -> Not connected
* PC6  - Not connected -> KB_COL3

* PA4  - I2S_DACLRC -> UI_DEBUG_3
* PA15 - DISP_BL -> I2S_DACLRC
* PB8  - KB_COL2 -> DISP_BL
* PB15 - KB_ROW4 -> KB_COL2
* PA8  - UI_STAT -> KB_ROW4
* PC11 - I2S_ADCDAT -> UI_STAT
* PB4  - KB_ROW7 -> I2S_ADCDAT
* PB11 - Not connected -> KB_ROW7

* PA6  - DISP_RST -> UI_LED_R
* PB9  - KB_COL1 -> DISP_RST
* PB13 - KB_ROW2 -> KB_COL1
* PB14 - KB_ROW3 -> KB_ROW2
* PC8  - KB_ROW5 -> KB_ROW3
* PB0  - DISP_DC -> KB_ROW5
* PC13 - Not connected -> DISP_DC
* PC1  - UI_LED_R -> PTT_AI_BTN

* PA11 - Not connected -> UI_DEBUG_1

* PA12 - KB_LED -> UI_DEBUG_2
* PB10 - Not connected -> KB_LED

* PA13 - Not connected -> UI_SWDIO

* PA14 - Not connected -> UI_SWCLK

* PB1  - KB_MIC -> KB_ROW6
* PB3  - KB_ROW6 -> Not connected

* PB5  - KB_COL5 -> I2S_DACDAT
* PC9  - Not connected -> KB_COL5
* PC12 - I2S_DACDAT -> Not connected

* PC0  - UI_LED_G -> PTT_BTN
* PC5  - Not connected -> UI_LED_G

* PC2  - Not connected -> USB_CC2_DETECT
* PC3  - Not connected -> USB_CC1_DETECT

* PC7  - Not connected -> KB_COL4
* PC15 - KB_COL4 -> Not connected

