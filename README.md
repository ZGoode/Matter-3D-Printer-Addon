# Matter-3D-Printer-Addon
This code for this project is split into two sections, the code for the NRF7002DK and the code for the Adafruit RP2040 USB Host board.

# Important Notes:
- This project is still very much a work in progress
- 

# Current NRF7002DK Features:
- Parse UART data sent from RP2040
- Connect to Matter smart home network
- Send temperature data over Matter network

# Future Planned NRF7002DB Features:
- Send print percentage status over Matter network
- Allow for basic hardware controls (ie. homing, change temperature, cancel print, start print, pause print, resume print)

# NRF7002DK Primary Functions:
- checkUARTDataInput(const char *input, const char *check);
- extractLongLongs(const char *input);
- extractTemperatureData(const char *input, int maxNumbers);
- longLongToChar(long long num, char *str);
- floatToChar(float num, int precision);
- floatToUint16(float floatValue);
- returnTempData(int val);
- returnSDPercentage();
- returnSDData(int val);
- returnCharBuffer();

# Adafruit RP2040 USB Host Features:
- Act as USB host for 3D printer
- Relay data over UART from USB host
- Provide serial debugging through second USB port to allow monitoring on a host computer

More details can be found on the project page over at Hackster.io: https://www.hackster.io/zachary-goode/smart-home-3d-printer-accessory-thread-matter-wifi-870723
