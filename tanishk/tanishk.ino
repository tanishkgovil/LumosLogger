// #include <Arduino.h>
// #include <SPI.h>
// #include "tanishk_driver.h"
// #include "nand_log.h"

// void loop() {}

// void manualread() {
//   Serial.begin(115200);
//   uint32_t t0 = millis();
//   while (!Serial && (millis() - t0 < 3000)) { delay(10); }
//   begin();
//   unlock_all_blocks();
//   // manually read ascii bytes from log
//   Serial.println(F("Manual read of log data:"));
//   for (uint16_t blk=0; blk<=0; ++blk) {
//     for (uint8_t page=0; page<5; ++page) {
//       flashAddr.block = blk;
//       flashAddr.page = page;
//       flashAddr.column = 0;
//       uint8_t probe[2] = {0};
//       if (!read_bytes(probe, 2)) continue;
//       if (probe[0] == 0xFF && probe[1] == 0xFF) continue; // blank page
//       Serial.print(F("Block ")); Serial.print(blk);
//       Serial.print(F(" Page ")); Serial.print(page);
//       Serial.print(F(" Data: "));
//       // read full page
//       const uint16_t to_read = NAND_MAIN_BYTES;
//       uint8_t* page_buf = new uint8_t[to_read];
//       if (read_bytes(page_buf, to_read)) {
//         for (uint16_t i=0;i<to_read;++i) {
//           // uint8_t b = page_buf[i];
//           // // if (b >= 32 && b <= 126) Serial.write(b); else Serial.write('.');
//           // Serial.write(b, HEX); Serial.print(' ');
//           // Print as hex
//           uint8_t b = page_buf[i];
//           if (b < 16) Serial.print('0');
//           Serial.print(b, HEX); Serial.print(' ');
//         }
//         Serial.println();
//       }
//       delete[] page_buf;
//     }
//   }
// }

// void erase() {
//   Serial.begin(115200);
//   uint32_t t0 = millis();
//   while (!Serial && (millis() - t0 < 3000)) { delay(10); }

//   begin();

//   // Use blocks 0..20 as the log region (example)
//   log_begin(0, 20, /*format_if_blank=*/false);
//   log_format_range();
//   Serial.println(F("Log region erased"));
// }

// void test_dummy_data() {
//   Serial.begin(115200);
//   uint32_t t0 = millis();
//   while (!Serial && (millis() - t0 < 3000)) { delay(10); }

//   begin();
//   log_begin(0, 20, /*format_if_blank=*/false);
//   // format of log data (example line): 415nm,445nm,480nm,515nm,555nm,590nm,630nm,680nm,NIR,Clear,0;
//   // fill with 1000 lines of dummy data
//   const char* line = "415nm,445nm,480nm,515nm,555nm,590nm,630nm,680nm,NIR,Clear,0;";
//   const int line_len = strlen(line);
//   const int total_lines = 1000;
//   for (int i=0;i<total_lines;++i) {
//     Serial.print(F("Appending line ")); Serial.println(i);
//     if (!log_append(reinterpret_cast<const uint8_t*>(line), line_len)) {
//       Serial.print(F("Failed to append line ")); Serial.println(i);
//       break;
//     }
//   }

//   if (!log_flush()) {
//     Serial.println(F("Failed to flush log data"));
//   }

//   Serial.println(F("Dummy data log append complete"));

//   // read back the log entries
//   log_iter_reset();
//   const uint16_t max_record_size = NAND_MAIN_BYTES - sizeof(LogHdr);
//   uint8_t* buf2 = new uint8_t[max_record_size];
//   uint16_t n=0;
//   int record_count = 0;
//   while (log_iter_next(buf2, max_record_size, &n)) {
//     Serial.print(F("Record ")); Serial.print(++record_count);
//     Serial.print(F(" length=")); Serial.print(n);
//     Serial.print(F(": "));
//     for (uint16_t i=0; i < n; ++i) {
//       Serial.print(static_cast<char>(buf2[i]));
//     }
//     Serial.println();
//   }
//   delete[] buf2;
//   Serial.print(F("Total records read: ")); Serial.println(record_count);
// }

// void setup() {
//   erase();
//   test_dummy_data();
// }

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

#include <Adafruit_TLC5947.h>
#include <Adafruit_AS7341.h>

#include "tanishk_driver.h"   // low-level NAND driver: begin, read_ID, get_status, etc.
#include "nand_log.h"         // high-level logging: log_begin, log_append, log_iter_next

// ================== TLC5947 CONFIG ==================
#define NUM_TLC5947  1
#define DATA_PIN     A3      // Data pin for LED driver
#define CLOCK_PIN    D2      // Clock pin for LED driver
#define LATCH_PIN    D6      // Latch pin for LED driver

// Constructor (clock, data, latch)
Adafruit_TLC5947 tlc = Adafruit_TLC5947(NUM_TLC5947, CLOCK_PIN, DATA_PIN, LATCH_PIN);

// We will use TLC channels 0..17 (inclusive)
const int numLEDs = 18;      // LED channels 0–17

// ================== NAND LOG REGION CONFIG ==================
const uint16_t LOG_START_BLOCK = 11;
const uint16_t LOG_END_BLOCK   = 11;

// ================== AS7341 CONFIG ==================
Adafruit_AS7341 as7341;
as7341_gain_t gain = AS7341_GAIN_256X;

int pdVisNum = 10;           // F1..F8 + NIR + CLEAR
int atime    = 50;
int astep    = 499;

uint16_t readings[10] = {0};

// ================== LED TIMING / BRIGHTNESS ==================
uint16_t intensity  = 4095;  // max brightness so you can see it
int stabTime        = 1000;  // ms LED on before reading sensor
int delayTime       = 5;     // small extra delay after reading

// ================== LOGGING / BATCHING STATE ==================
bool readyForLoop = false;
int totalLogged = 0;
unsigned long startTime = 0;
unsigned long endTime = 0;

// ================== HELPERS ==================

void clearAllLEDs() {
  for (int ch = 0; ch < 24; ch++) {
    tlc.setPWM(ch, 0);
  }
  tlc.write();
}

// Combine one AS7341 reading into CSV: "r0,r1,...,r9;"
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

// ================== SETUP ==================

void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();

  // Wait a bit for Serial (for boards with native USB)
  while (!Serial && (millis() - t0 < 3000)) {
    delay(10);
  }

  Serial.println(F("jane.ino: LED 0–17 + AS7341 + CONTINUOUS NAND logging"));

  // --- NAND low-level init ---
  if (!begin()) {
    Serial.println(F("NAND begin() failed"));
    while (1) {
      delay(1000);
    }
  }

  uint8_t mfg = 0, dev = 0;
  if (read_ID(mfg, dev)) {
    Serial.print(F("NAND ID: MFG=0x")); Serial.print(mfg, HEX);
    Serial.print(F(" DEV=0x"));         Serial.println(dev, HEX);
  } else {
    Serial.println(F("read_ID failed"));
  }

  uint8_t sr = get_status();
  Serial.print(F("Initial NAND status: "));
  print_status(sr);

  // --- Log layer init over blocks [10..199] ---
  // NOTE: format_if_blank = true will erase this region at startup.
  if (!log_begin(LOG_START_BLOCK, LOG_END_BLOCK, /*format_if_blank=*/false)) {
    Serial.println(F("log_begin failed!"));
    while (1) {
      delay(1000);
    }
  }

  Serial.print(F("Log initialized over blocks "));
  Serial.print(LOG_START_BLOCK);
  Serial.print(F(".."));
  Serial.println(LOG_END_BLOCK);

  // --- TLC5947 init ---
  tlc.begin();
  clearAllLEDs();
  Serial.println(F("TLC5947 initialized."));

  // One quick sweep 0–17 to verify LEDs still work with NAND present
  Serial.println(F("One-time LED sweep 0–17 (sanity check)..."));
  for (int ch = 0; ch < numLEDs; ch++) {
    clearAllLEDs();
    tlc.setPWM(ch, 4095);
    tlc.write();

    Serial.print(F("TEST LED channel "));
    Serial.print(ch);
    Serial.println(F(" ON"));

    delay(200);
  }
  clearAllLEDs();
  Serial.println(F("LED sweep complete.\n"));

  // --- AS7341 init ---
  if (!as7341.begin()) {
    Serial.println(F("Could not find AS7341 sensor. Check wiring!"));
    while (1) {
      delay(1000);
    }
  }
  as7341.setATIME(atime);
  as7341.setASTEP(astep);
  as7341.setGain(gain);

  Serial.println(F("AS7341 initialized."));
  delay(500);

  readyForLoop = true;
  Serial.println(F("Setup complete. Entering continuous logging loop.\n"));
  // print current time
  startTime = millis();
}

// ================== LOOP ==================

void loop() {
  if (!readyForLoop) return;

  // Sweep LED channels 0..17 continuously
  for (int led = 0; led < numLEDs; led++) {

    // Turn this LED ON
    tlc.setPWM(led, intensity);
    tlc.write();
    Serial.print(F("LED channel "));
    Serial.print(led);
    Serial.println(F(" ON"));

    // delay(stabTime);   // let light stabilize at the sensor

    // Read AS7341 channels
    if (!as7341.readAllChannels()) {
      Serial.println(F("Error reading all AS7341 channels!"));
      // Turn LED off and continue
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

    // Build CSV line and print
    String pdData = combineData(led);
    Serial.print(F("PD Data (LED "));
    Serial.print(led);
    Serial.print(F("): "));
    Serial.print(pdData);    // already includes newline

    // When batch is full, append to NAND log
    size_t length = pdData.length();
    totalLogged += length;
    
    Serial.print(length);
    Serial.println(F(" bytes. Logging to NAND..."));

    Serial.print(F("Total logged so far: "));
    Serial.print(totalLogged);
    Serial.println(F(" bytes"));

    bool ok = log_append(
      reinterpret_cast<const uint8_t*>(pdData.c_str()),
      static_cast<uint16_t>(length)
    );

    if (!ok) {
      Serial.println(F("log_append FAILED! Stopping logging."));
      readyForLoop = false;
      endTime = millis();
      Serial.print(F("Total duration (ms): "));
      Serial.println(endTime - startTime);
      break;
    } else {
      Serial.println(F("log_append OK."));
    }

    Serial.println(F("Continuing continuous logging...\n"));
  }   // end for each LED 0..17
}

void readback() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0 < 3000)) { delay(10); }

  begin();
  log_begin(LOG_START_BLOCK, LOG_END_BLOCK, /*format_if_blank=*/false);
  // format of log data (example line): 65535,65535,65535,65535,65535,65535,65535,65535,65535,65535,4095,16;
  // fill with 1000 lines of dummy data

  // read back the log entries
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
  begin();
  for (uint16_t b = 0; b < NAND_BLOCK_COUNT; ++b) {
    erase_block(b);
  }
  Serial.println(F("All blocks erased."));
}