# Firmware


## Firmware Setup (ESP8266)

> Assumes Windows as host OS

### Hardware
- Board: generic ESP8266 dev board (NodeMCU-style clone), FCC ID 2A4RQ-ESP8266
- USB-serial chip: CP2102 (driver usually auto-installs via Windows Update;
  if not, install Silicon Labs CP210x driver) **see drivers below**
- Connector: verify Micro-USB vs Mini-USB on your specific board before
  buying cables -- **must be a data cable, not charge-only**

### PlatformIO Setup
```bash
cd firmware
pio project init --board nodemcuv2 --ide vscode
```
- `nodemcuv2` is the correct board id for this class of board (maps to
  ESP-12E, 4MB flash, standard NodeMCU pinout)

### Build & Flash
```bash
pio run --target upload
```


### Drivers: 
- [Silicon Labs Driver](https://www.silabs.com/software-and-tools/usb-to-uart-bridge-vcp-drivers?tab=downloads)
    - Install the Silicon Labs CP210x VCP driver -- "CP210x Universal Windows Driver" from Silicon Labs' download page.
    - Unzip, then in Device Manager right-click the CP2102 with the yellow bang -> Update driver -> point it at the unzipped folder

