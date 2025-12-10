#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

#include <Adafruit_TLC5947.h>
#include <Adafruit_AS7341.h>

#include "nand_driver.h"
#include "nand_log.h"

// Set to 0 to disable all Serial output, 1 to enable for debugging
#define DEBUG_SERIAL 0

#if DEBUG_SERIAL
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINT_HEX(x) Serial.print(x, HEX)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINT_HEX(x)
#endif

#define NUM_TLC5947  1
#define DATA_PIN     A3
#define CLOCK_PIN    D2
#define LATCH_PIN    D6

// Constructor (clock, data, latch)
Adafruit_TLC5947 tlc = Adafruit_TLC5947(NUM_TLC5947, CLOCK_PIN, DATA_PIN, LATCH_PIN);

// LED channels 0-17
const int numLEDs = 18;

// Logging configuration, 0 to NAND_BLOCK_COUNT - 1
const uint16_t LOG_START_BLOCK = 0;
const uint16_t LOG_END_BLOCK   = NAND_BLOCK_COUNT - 1;

// AS7341 sensor configuration
Adafruit_AS7341 as7341;
as7341_gain_t gain = AS7341_GAIN_256X;

int pdVisNum = 10;
int atime    = 50;
int astep    = 499;

uint16_t readings[10] = {0};

uint16_t intensity  = 4095; // max brightness
int delayTime       = 5;     // small extra delay after reading (optional)

bool readyForLoop = false;
int totalLogged = 0;
unsigned long startTime = 0;
unsigned long endTime = 0;

// --- Helpers ---

void clearAllLEDs() {
  for (int ch = 0; ch < 24; ch++) {
    tlc.setPWM(ch, 0);
  }
  tlc.write();
}

// Combine sensor readings and LED index into CSV
String combineData(int led) {
  String pddata = "";
  for (int i = 0; i < pdVisNum; i++) {
    pddata += String(readings[i]);
    if (i < pdVisNum - 1) {
      pddata += ",";
    }
  }
  pddata += ",";
  pddata += led;
  return pddata + ";";
}

// --- Setup ---

void setup() {

// Start Serial for debugging
#if DEBUG_SERIAL
  Serial.begin(115200);
  uint32_t t0 = millis();

  while (!Serial && (millis() - t0 < 3000)) {
    delay(10);
  }
#endif

  DEBUG_PRINTLN(F("LED 0–17 + AS7341 + CONTINUOUS NAND logging"));

  // Initialize NAND
  if (!begin()) {
    DEBUG_PRINTLN(F("NAND begin() failed"));
    while (1) {
      delay(1000);
    }
  }

  // Read NAND ID
  uint8_t mfg = 0, dev = 0;
  if (read_ID(mfg, dev)) {
    DEBUG_PRINT(F("NAND ID: MFG=0x")); DEBUG_PRINT_HEX(mfg);
    DEBUG_PRINT(F(" DEV=0x"));         DEBUG_PRINTLN(dev);
  } else {
    DEBUG_PRINTLN(F("read_ID failed"));
  }

  // Print initial status
  uint8_t sr = get_status();
  DEBUG_PRINT(F("Initial NAND status: "));
  #if DEBUG_SERIAL
    print_status(sr);
  #endif

  // Initialize logging (without formatting)
  if (!log_begin(LOG_START_BLOCK, LOG_END_BLOCK, false)) {
    DEBUG_PRINTLN(F("log_begin failed!"));
    while (1) {
      delay(1000);
    }
  }

  DEBUG_PRINT(F("Log initialized over blocks "));
  DEBUG_PRINT(LOG_START_BLOCK);
  DEBUG_PRINT(F(".."));
  DEBUG_PRINTLN(LOG_END_BLOCK);

  // Initialize LED driver
  tlc.begin();
  clearAllLEDs();
  DEBUG_PRINTLN(F("TLC5947 initialized."));

  // Test LED sweep
  DEBUG_PRINTLN(F("One-time LED sweep 0–17 (sanity check)..."));
  for (int ch = 0; ch < numLEDs; ch++) {
    clearAllLEDs();
    tlc.setPWM(ch, 4095);
    tlc.write();

    DEBUG_PRINT(F("TEST LED channel "));
    DEBUG_PRINT(ch);
    DEBUG_PRINTLN(F(" ON"));

    delay(200);
  }
  clearAllLEDs();
  DEBUG_PRINTLN(F("LED sweep complete.\n"));

  // Initialize AS7341 sensor
  if (!as7341.begin()) {
    DEBUG_PRINTLN(F("Could not find AS7341 sensor. Check wiring!"));
    while (1) {
      delay(1000);
    }
  }
  as7341.setATIME(atime);
  as7341.setASTEP(astep);
  as7341.setGain(gain);

  DEBUG_PRINTLN(F("AS7341 initialized."));
  delay(500);

  readyForLoop = true;
  DEBUG_PRINTLN(F("Setup complete. Entering continuous logging loop.\n"));

  startTime = millis();
}

// ================== LOOP ==================

void loop() {
  if (!readyForLoop) return;

  // Sweep LED channels
  for (int led = 0; led < numLEDs; led++) {

    // Turn LED on
    tlc.setPWM(led, intensity);
    tlc.write();
    DEBUG_PRINT(F("LED channel "));
    DEBUG_PRINT(led);
    DEBUG_PRINTLN(F(" ON"));

    // Read all PD channels
    if (!as7341.readAllChannels()) {
      DEBUG_PRINTLN(F("Error reading all AS7341 channels!"));
      tlc.setPWM(led, 0);
      tlc.write();
      continue;
    }

    readings[0] = as7341.getChannel(AS7341_CHANNEL_415nm_F1);
    readings[1] = as7341.getChannel(AS7341_CHANNEL_445nm_F2);
    readings[2] = as7341.getChannel(AS7341_CHANNEL_480nm_F3);
    readings[3] = as7341.getChannel(AS7341_CHANNEL_515nm_F4);
    readings[4] = as7341.getChannel(AS7341_CHANNEL_555nm_F5);
    readings[5] = as7341.getChannel(AS7341_CHANNEL_590nm_F6);
    readings[6] = as7341.getChannel(AS7341_CHANNEL_630nm_F7);
    readings[7] = as7341.getChannel(AS7341_CHANNEL_680nm_F8);
    readings[8] = as7341.getChannel(AS7341_CHANNEL_NIR);
    readings[9] = as7341.getChannel(AS7341_CHANNEL_CLEAR);

    delay(delayTime);

    // Turn LED OFF
    tlc.setPWM(led, 0);
    tlc.write();

    // Convert to CSV line
    String pdData = combineData(led);
    DEBUG_PRINT(F("PD Data (LED "));
    DEBUG_PRINT(led);
    DEBUG_PRINT(F("): "));
    DEBUG_PRINTLN(pdData);

    size_t length = pdData.length();
    totalLogged += length;
    
    DEBUG_PRINT(length);
    DEBUG_PRINTLN(F(" bytes. Logging to NAND..."));

    DEBUG_PRINT(F("Total logged so far: "));
    DEBUG_PRINT(totalLogged);
    DEBUG_PRINTLN(F(" bytes"));

    // Append to log
    bool ok = log_append(
      reinterpret_cast<const uint8_t*>(pdData.c_str()),
      static_cast<uint16_t>(length)
    );

    if (!ok) {
      DEBUG_PRINTLN(F("log_append FAILED! Stopping logging."));
      readyForLoop = false;
      endTime = millis();
      DEBUG_PRINT(F("Total duration (ms): "));
      DEBUG_PRINTLN(endTime - startTime);
      break;
    } else {
      DEBUG_PRINTLN(F("log_append OK."));
    }

    DEBUG_PRINTLN(F("Continuing continuous logging...\n"));
  }
}

void readback() {

  // Begin Serial for output
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0 < 3000)) { delay(10); }

  begin();
  log_begin(LOG_START_BLOCK, LOG_END_BLOCK, false);

  // Read back the log entries
  log_iter_reset();
  const uint16_t max_record_size = NAND_MAIN_BYTES - sizeof(LogHdr);
  uint8_t* buf2 = new uint8_t[max_record_size];
  uint16_t n=0;
  int record_count = 0;
  while (log_iter_next(buf2, max_record_size, &n)) {
    Serial.print(F("Record ")); Serial.print(++record_count);
    Serial.print(F(" length=")); Serial.print(n);
    Serial.print(F(": "));
    for (uint16_t i=0; i < n; ++i) {
      Serial.print(static_cast<char>(buf2[i]));
    }
    Serial.println();
  }
  delete[] buf2;
  Serial.print(F("Total records read: ")); Serial.println(record_count);
}

void erase() {
  // Erase all blocks
  begin();
  for (uint16_t b = 0; b < NAND_BLOCK_COUNT; ++b) {
    erase_block(b);
  }
  Serial.println(F("All blocks erased."));
}