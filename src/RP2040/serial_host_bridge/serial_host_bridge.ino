/* Requirements:
   - For rp2040:
     - [Pico-PIO-USB](https://github.com/sekigon-gonnoc/Pico-PIO-USB) library
     - 2 consecutive GPIOs: D+ is defined by PIN_USB_HOST_DP, D- = D+ +1
     - Provide VBus (5v) and GND for peripheral
     - CPU Speed must be either 120 or 240 Mhz. Selected via "Menu -> CPU Speed" (240MHz is recommended)
*/

/* This program will forward all characters from SerialUSB to SerialUART and vice versa.
*/

// USBHost is defined in usbh_helper.h
#include "usbh_helper.h"

#include <Adafruit_NeoPixel.h>

#include <Wire.h>

#define UARTRX 1
#define UARTTX 0

#define I2CSDA 2
#define I2CSCL 3
#define resetPin 4

#define neoPixel 5

#define NUMPIXELS 1

Adafruit_NeoPixel pixels(NUMPIXELS, neoPixel, NEO_GRB + NEO_KHZ800);

#define deviceAddress 63

const char* testResponse = "ok"; // Change this to the expected response
const int testResponseLength = sizeof(testResponse); // Length of the expected response including the null terminator
unsigned long responseTimeout = 1000; // Initial timeout duration in milliseconds

void updateUARTBaud();
void updateUSBBaud();
void restartMCU();
void sendCurrentUARTBaud();
void sendCurrentUSBBaud();
void checkUSBConnection();
void disablePassthrough();
void enablePassthrough();
void I2C_TxHandler();
void I2C_RxHandler(int RXnum);

void reportTemperature();
void printPercentage();
void stopPrint();
void pausePrint();
void resumePrint();
void homePrinter();
void autoLevel();
void currentPrintTime();

void forward_serial();

int checkBaudRate();

int uartSerial = 9;
int usbSerial = 9;

unsigned long baudRates[] = {300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 74880, 115200, 230400, 250000, 500000, 1000000, 2000000};

volatile uint8_t commandBuffer[16];
volatile uint8_t commandRequest = 0;

bool USBStatus = false;
bool passthrough = false;

/* void I2C_RxHandler(int RXnum) {
  uint8_t i = 0;

  while (0 < Wire.available()) { // Read Any Received Data
    commandBuffer[i] = Wire.read();

    i++;
  }

  if (commandBuffer[0] == deviceAddress) {
    if (commandBuffer[1] == 101) {
      updateUARTBaud();
    } else if (commandBuffer[1] == 102) {
      updateUSBBaud();
    } else if (commandBuffer[1] == 103) {
      restartMCU();
    } else if (commandBuffer[1] == 104) {
      sendCurrentUARTBaud();
    } else if (commandBuffer[1] == 105) {
      sendCurrentUSBBaud();
    } else if (commandBuffer[1] == 106) {
      checkUSBConnection();
    } else if (commandBuffer[1] == 107) {
      disablePassthrough();
    } else if (commandBuffer[1] == 108) {
      disablePassthrough();
    } else if (commandBuffer[1] == 201) {
      reportTemperature();
    } else if (commandBuffer[1] == 202) {
      printPercentage();
    } else if (commandBuffer[1] == 203) {
      stopPrint();
    } else if (commandBuffer[1] == 204) {
      pausePrint();
    } else if (commandBuffer[1] == 205) {
      resumePrint();
    } else if (commandBuffer[1] == 206) {
      homePrinter();
    } else if (commandBuffer[1] == 207) {
      autoLevel();
    } else if (commandBuffer[1] == 208) {
      currentPrintTime();
    }
  }

  }

  void I2C_TxHandler() {
  commandRequest = 1;
  }
*/

// CDC Host object
Adafruit_USBH_CDC SerialHost;

// forward Seral <-> SerialHost
void forward_serial() {
  uint8_t buf[64];

  // Serial -> SerialHost
  if (Serial1.available()) {
    size_t count = Serial1.readBytes(buf, sizeof(buf));
    if (SerialHost && SerialHost.connected()) {
      SerialHost.write(buf, count);
      SerialHost.flush();
    }
  }

  // SerialHost -> Serial
  if (SerialHost.connected() && SerialHost.available() && passthrough) {
    size_t count = SerialHost.read(buf, sizeof(buf));
    Serial1.write(buf, count);
    Serial1.flush();
  }
}

int checkBaudRate() {
  for (size_t i = 0; i < sizeof(baudRates) / sizeof(baudRates[0]); ++i) {
    Serial.print("Testing baud rate ");
    Serial.print(baudRates[i]);
    Serial.println("...");

    SerialHost.end();  // End SerialHost to allow for restarting for testing
    SerialHost.begin(baudRates[i]); // Set baud rate for testing

    SerialHost.println("M500"); // Send an empty line to trigger a response, adjust as needed

    unsigned long startTime = millis(); // Record the start time

    while (millis() - startTime < responseTimeout) {
      if (SerialHost.available() >= testResponseLength - 1) { // Check if enough characters are available
        char receivedResponse[testResponseLength]; // Buffer to store received response
        SerialHost.readBytes(receivedResponse, testResponseLength - 1); // Read received characters
        receivedResponse[testResponseLength - 1] = '\0'; // Null-terminate the array

        if (strcmp(receivedResponse, testResponse) == 0) { // Compare received response with expected response
          unsigned long elapsedTime = millis() - startTime; // Calculate elapsed time
          responseTimeout = max(responseTimeout, elapsedTime + 100); // Adjust timeout dynamically

          Serial.println("Correct baud rate detected: " + String(baudRates[i]));
          // You can add additional actions or logic here if the correct baud rate is detected
          return 1; // Exit function if correct baud rate detected
        } else {
          Serial.println("Received incorrect response: " + String(receivedResponse));
        }
      }
    }

    Serial.println("No response received for baud rate " + String(baudRates[i]));
    delay(100); // Delay before testing the next baud rate
  }

  Serial.println("Baud rate not detected.");

  return 0;
}

//--------------------------------------------------------------------+
// For RP2040 use both core0 for device stack, core1 for host stack
//--------------------------------------------------------------------+

//------------- Core0 -------------//
void setup() {
  // Set up RX/TX for UART
  Serial1.setTX(UARTTX);
  Serial1.setRX(UARTRX);

  // Set up SDA/SCL for I2C Slave
  // Wire.setSDA(I2CSDA);
  // Wire.setSCL(I2CSCL);

  pinMode(resetPin, OUTPUT);
  digitalWrite(resetPin, HIGH);

  pixels.begin();
  pixels.clear();

  /*
    // Set up I2C Slave
    Wire.begin(0x55); // Initialize I2C (Slave Mode: address=0x55)
    Wire.onReceive(I2C_RxHandler);
    Wire.onRequest(I2C_TxHandler);
  */

  Serial.begin(115200);
  Serial1.begin(baudRates[uartSerial]);
}

void loop() {
  forward_serial();


}

//------------- Core1 -------------//
void setup1() {
  // configure pio-usb: defined in usbh_helper.h
  rp2040_configure_pio_usb();

  // run host stack on controller (rhport) 1
  // Note: For rp2040 pico-pio-usb, calling USBHost.begin() on core1 will have most of the
  // host bit-banging processing works done in core1 to free up core0 for other works
  USBHost.begin(1);

  // Initialize SerialHost
  SerialHost.begin(baudRates[usbSerial]);
}

void loop1() {
  USBHost.task();
}

//--------------------------------------------------------------------+
// TinyUSB Host callbacks
//--------------------------------------------------------------------+
extern "C" {

  // Invoked when a device with CDC interface is mounted
  // idx is index of cdc interface in the internal pool.
  void tuh_cdc_mount_cb(uint8_t idx) {
    // bind SerialHost object to this interface index
    SerialHost.mount(idx);
    Serial.println("SerialHost is connected to a new CDC device");
    USBStatus = true;

    pixels.setPixelColor(0, pixels.Color(0, 0, 255));
    pixels.show();
    delay(500);

    if (checkBaudRate() == 1) {
      // Set NeoPixel green to show if USB device is successfully connected
      pixels.setPixelColor(0, pixels.Color(0, 255, 0));
      pixels.show();
    }
  }

  // Invoked when a device with CDC interface is unmounted
  void tuh_cdc_umount_cb(uint8_t idx) {
    SerialHost.umount(idx);
    Serial.println("SerialHost is disconnected");
    USBStatus = false;

    // Set NeoPixel red to show if USB device is not successfully connected
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));
    pixels.show();
  }

}

void updateUARTBaud() {
  Serial1.flush();
  Serial1.end();

  delay(20);

  if (commandBuffer[2] < sizeof(baudRates)) {
    uartSerial = commandBuffer[2];
    Serial1.begin(baudRates[uartSerial]);

    Serial1.println(baudRates[uartSerial]);

    Wire.write('t');
  } else {
    Wire.write('f');
  }
}

void updateUSBBaud() {
  SerialHost.flush();
  SerialHost.end();

  delay(20);

  if (commandBuffer[2] < sizeof(baudRates)) {
    usbSerial = commandBuffer[2];
    SerialHost.begin(baudRates[usbSerial]);

    Serial1.println(baudRates[usbSerial]);

    Wire.write('t');
  } else {
    Wire.write('f');
  }
}

void restartMCU() {
  SerialHost.flush();
  SerialHost.end();
  Serial1.flush();
  Serial1.end();

  Wire.write('y');

  Serial1.println("reset trigger");

  delay(500);

  digitalWrite(resetPin, LOW);
}

void sendCurrentUARTBaud() {
  Wire.write(uartSerial);

  Serial1.println(baudRates[uartSerial]);
}

void sendCurrentUSBBaud() {
  Wire.write(usbSerial);

  Serial1.println(baudRates[usbSerial]);
}

void checkUSBConnection() {
  if (USBStatus) {
    Wire.write('c');

    Serial1.println("connected");
  } else if (!USBStatus) {
    Wire.write('d');

    Serial1.println("not connected");
  }
}

void disablePassthrough() {
  passthrough = false;
}

void enablePassthrough() {
  passthrough = true;
}

/*
  //PRINT COMMANDS
  void reportTemperature() {
  SerialHost.write("M155");
  }

  void printPercentage() {
  SerialHost.write("M27");
  }

  void stopPrint() {
  SerialHost.write("M524");
  }

  void pausePrint() {
  SerialHost.write("M0");
  }

  void resumePrint() {
  SerialHost.write("M108");
  }

  void homePrinter() {
  SerialHost.write("G28");
  }

  void autoLevel() {
  SerialHost.write("G29");
  }

  void currentPrintTime() {
  SerialHost.write("M31");
  }
*/
