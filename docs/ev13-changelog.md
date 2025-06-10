
# Schematic 

### Firmware changes 
**MGMT**
- Moved UI_STAT to PB9
- Moved NET_STAT to PB7
- Moved LEDB_G to PB6
- Moved USB_DTR_MGMT to PB2
- Added UI_RTS1_MGMT to PA0
- Added UI_CTS1_MGMT to PA1
- Added USB_CC1_DETECT to PC15
- Added USB_CC2_DETECT to PC14

**UI**
- Removed KB_LED - PB10
- Removed USB_CTS_MGMT from PA11
- Removed USB_RTS_MGMT from PA12
- Moved UI_DEBUG_1 to PC15
- Moved UI_DEBUG_2 to PD2
- Added MIO_IO to PC12
- Added UI_CTS1_MGMT to PA11
- Added UI_RTS1_MGMT to PA12
- Moved UI_LED_B to PB3
- Moved UI_LEG_R to PA4
- Added UI_CTS2_NET to PA0
- Added UI_RTS2_NET to PA1
- Removed USB_CC1_DETECT
- Removed USB_CC2_DETECT
- Added SPI1:CLK,MISO,MOSI to PA5,6,7

Net
- Added NET_RTS1_MGMT to IO15
- Added NET_CTS1_MGMT to IO16
- Moved NET_DEBUG_1 to IO5
- Moved NET_DEBUG_2 to IO6
- Moved NET_DEBUG_3 to IO7
- Added UI_RTS2_NET to IO19
- Added UI_CTS2_NET to IO20
- Added NET_READY to IO8
- Added UI_READY to IO9
- Added SPI1_MOSI to IO11
- Added SPI1_CLK to IO12
- Added SPI1_MICO to IO13
### Hardware Changes (No affect to firmware)
**UI chip changes**
- Added pulldown - PB10
- Added pulldown - PB11
- Added pulldown - PC10
- Added pulldown - PC11
- Added pulldown - PB5
- Added pulldown - PB13
- Added pulldown - PA11
- Added pulldown - PA12
- Moved pulldown on UI_BOOT1 to ui section.
- Added series resistor - PB13
- Added series resistor - PB15
- Added series resistor - PC6
- Added series resistor - PC7
- Added series resistor - PC9
- Added series resistor - PC2
- Added series resistor - PC3


Net chip changes

Mgmt chip changes
- Renamed USB_RTS to USB_RTS_MGMT
- Renamed USB_DTR to USB_DTR_MGMT
- Added USB_CTS_MGMT

**Audio chip**
- Put all unused inputs and outputs for audio chip to GNDA

**General**
- Add resistor to MCO to GND on J3
- Added RC debounce circuit to ptt and ptt ai
- Added pulldown resistor to UI_BOOT0
