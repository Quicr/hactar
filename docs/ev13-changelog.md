
# Schematic 

### Firmware changes 
**UI**
- Removed KB_LED - PB10
- Added USB_CTS_MGMT to PA11
- Added USB_RTS_MGMT to PA12

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

Net chip changes

Mgmt chip changes
- Renamed USB_RTS to USB_RTS_MGMT
- Renamed USB_DTR to USB_DTR_MGMT
- Added USB_CTS_MGMT