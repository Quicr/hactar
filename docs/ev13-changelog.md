
# Schematic 

### Firmware changes 
**MGMT**
- Moved UI_STAT to PB9
- Moved NET_STAT to PB7
- Moved LEDB_G to PB6
- Moved USB_DTR_MGMT to PB2
- Added UI_RTS1_MGMT to PA0
- Added UI_CTS1_MGMT to PA1

**UI**
- Removed KB_LED - PB10
- Added USB_CTS_MGMT to PA11
- Added USB_RTS_MGMT to PA12
- Moved UI_DEBUG_1 to PC12
- Moved UI_DEBUG_2 to PD2
- Added UI_CTS1_MGMT to PA11
- Added UI_RTS1_MGMT to PA12
- Moved UI_LED_B to PB3
- Added UI_CTS2_NET to PA0
- Added UI_RTS2_NET to PA1

Net
- Added NET_RTS1_MGMT to IO15
- Added NET_CTS1_MGMT to IO16
- Moved NET_DEBUG_1 to IO5
- Moved NET_DEBUG_1 to IO6
- Moved NET_DEBUG_1 to IO7
- Added UI_RTS2_NET to IO19
- Added UI_CTS2_NET to IO20
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

Net chip changes

Mgmt chip changes
- Renamed USB_RTS to USB_RTS_MGMT
- Renamed USB_DTR to USB_DTR_MGMT
- Added USB_CTS_MGMT

**Audio chip**
- Put all unused inputs and outputs for audio chip to GNDA