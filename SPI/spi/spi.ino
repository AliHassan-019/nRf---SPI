#include <SPI.h>

// Pin Definitions
#define ADS1299_CS     D0   // Chip Select
#define ADS1299_DRDY   D1   // Data Ready
#define ADS1299_RESET  D2   // Reset Pin

// ADS1299 Commands
#define ADS1299_WAKEUP  0x02
#define ADS1299_STANDBY 0x04
#define ADS1299_RESET_CMD  0x06
#define ADS1299_START   0x08
#define ADS1299_STOP    0x0A
#define ADS1299_RDATAC  0x10
#define ADS1299_SDATAC  0x11
#define ADS1299_RDATA   0x12

// ADS1299 Configuration
#define VREF   2.5      // Reference voltage in volts
#define GAIN   24       // Default Gain (CONFIG1 setting)
#define SCALE  (VREF / (pow(2, 23) - 1)) * 1000000 / GAIN  

void sendCommand(uint8_t command) {
  digitalWrite(ADS1299_CS, LOW);
  SPI.transfer(command);
  digitalWrite(ADS1299_CS, HIGH);
  delay(1);
}

void writeRegister(uint8_t reg, uint8_t value) {
  digitalWrite(ADS1299_CS, LOW);
  SPI.transfer(0x40 | reg);  // Write command
  SPI.transfer(0x00);        // Number of registers - 1
  SPI.transfer(value);
  digitalWrite(ADS1299_CS, HIGH);
  delay(1);
}

uint8_t readRegister(uint8_t reg) {
  digitalWrite(ADS1299_CS, LOW);
  SPI.transfer(0x20 | reg);  // Read command
  SPI.transfer(0x00);        // Number of registers - 1
  uint8_t value = SPI.transfer(0x00);
  digitalWrite(ADS1299_CS, HIGH);
  return value;
}

void configureADS1299() {
  sendCommand(ADS1299_RESET_CMD);
  delay(100);
  sendCommand(ADS1299_SDATAC);

  // Set CONFIG1: 500SPS, Internal Oscillator
  writeRegister(0x01, 0x96);

  // Set CONFIG2: Default settings
  writeRegister(0x02, 0xC0);

  // Set CONFIG3: Enable internal reference
  writeRegister(0x03, 0xEC);

  // Enable only 4 channels (Disable CH5-CH8)
  for (int i = 0; i < 4; i++) {
    writeRegister(0x05 + i, 0x60);  // Enable channels 1-4
  }
  for (int i = 4; i < 8; i++) {
    writeRegister(0x05 + i, 0x81);  // Power down channels 5-8
  }

  sendCommand(ADS1299_RDATAC);
  sendCommand(ADS1299_START);
}

// Function to convert 3-byte data to signed 24-bit integer
long convert24bitToSignedInt(uint8_t *data) {
  long value = (data[0] << 16) | (data[1] << 8) | data[2];
  
  // Convert to signed 24-bit integer
  if (value & 0x800000) {  
    value |= 0xFF000000;  
  }
  
  return value;
}

void readEEGData() {
  if (digitalRead(ADS1299_DRDY) == LOW) {
    digitalWrite(ADS1299_CS, LOW);
    SPI.transfer(0x00); // Read command

    uint8_t status[3];
    for (int i = 0; i < 3; i++) {
      status[i] = SPI.transfer(0x00);
    }

    uint8_t eegData[12];  // 4 Channels x 3 bytes each
    for (int i = 0; i < 12; i++) {
      eegData[i] = SPI.transfer(0x00);
    }

    digitalWrite(ADS1299_CS, HIGH);

    // Convert EEG data to voltage in µV
    long eegValues[4];
    float eegVoltages[4];
    for (int i = 0; i < 4; i++) {
      eegValues[i] = convert24bitToSignedInt(&eegData[i * 3]);
      eegVoltages[i] = eegValues[i] * SCALE; // Convert to µV
    }

    // Print EEG Data in microvolts
    Serial.print("EEG Data : ");
    for (int i = 0; i < 4; i++) {
      Serial.print(eegVoltages[i], 2);
      Serial.print(" | ");
    }
    Serial.println();
  }
}

void setup() {
  Serial.begin(115200);
  SPI.begin();
  
  pinMode(ADS1299_CS, OUTPUT);
  pinMode(ADS1299_DRDY, INPUT);
  pinMode(ADS1299_RESET, OUTPUT);
  
  digitalWrite(ADS1299_CS, HIGH);
  digitalWrite(ADS1299_RESET, HIGH);
  delay(100);
  
  Serial.println("Initializing ADS1299...");
  configureADS1299();
  Serial.println("ADS1299 Ready! Reading EEG Data...");
}

void loop() {
  readEEGData();
  //delay(1500);
}
