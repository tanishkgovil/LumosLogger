<<<<<<< HEAD
// // // #include <Arduino.h>
// // // #include <Wire.h>
// // // #include <SPI.h>

// // // #include <Adafruit_TLC5947.h>
// // // #include <Adafruit_AS7341.h>

// // // #include "tanishk_driver.h"
// // // #include "nand_log.h"
// // // #define IR_PD_PIN A0
// // // // Storage Chip Configuration
// // // #define CHIP_SELECT_PIN A1
// // // #define CS_PIN          CHIP_SELECT_PIN   // <--- FIX: define CS_PIN used below

// // // // LED Driver Configurations (not used by driver, but ok to leave)
// // // #define NUM_TLC5947   1
// // // #define LED_CLOCK     D2
// // // #define LED_DATA_PIN  A3
// // // #define LED_LATCH     D6
// // // #define LED_BLANK_PIN D7

// // // // ---------- LOG REGION ----------
// // // const uint16_t LOG_START_BLOCK = 10;
// // // const uint16_t LOG_END_BLOCK   = 199;

// // // // ---------- LED / SENSOR CONFIG ----------
// // // int      numLEDs    = 17;
// // // uint16_t intensity  = 4095;     // max brightness so you can see it
// // // int      stabTime   = 1000;
// // // int      delayTime  = 5;

// // // // ---------- AS7341 CONFIG ----------
// // // Adafruit_AS7341 as7341;
// // // as7341_gain_t gain = AS7341_GAIN_256X;

// // // int pdVisNum = 10;
// // // int atime    = 50;
// // // int astep    = 499;

// // // uint16_t readings[10] = {0};

// // // // 🔴 IMPORTANT: constructor order is (data, clock, latch), NOT (clock, data, latch)
// // // //Adafruit_TLC5947 tlc(NUM_TLC5947, LED_DATA_PIN, LED_CLOCK_PIN, LED_LATCH_PIN);
// // // Adafruit_TLC5947 tlc(NUM_TLC5947, LED_CLOCK, LED_DATA_PIN, LED_LATCH);

// // // // ---------- BATCHING ----------
// // // #define BATCH_WRITE_SIZE 16
// // // String   strBatched   = "";
// // // uint8_t  numBatched   = 0;
// // // bool     readyForLoop = false;

// // // // ---------- HELPERS ----------
// // // String combineData() {
// // //   String pddata = "";
// // //   for (int i = 0; i < pdVisNum; i++) {
// // //     pddata += String(readings[i]);
// // //     if (i < pdVisNum - 1) {
// // //       pddata += ",";
// // //     }
// // //   }
// // //   return pddata + "\n";
// // // }

// // // void clearAllLEDs() {
// // //   for (int i = 0; i < numLEDs; i++) {
// // //     tlc.setPWM(i, 0);
// // //   }
// // //   tlc.write();
// // // }

// // // void dumpAllLogs() {
// // //   Serial.println(F("\n--- Dumping all log records ---"));
// // //   log_iter_reset();

// // //   uint8_t  buf[128];
// // //   uint16_t n = 0;

// // //   while (log_iter_next(buf, sizeof(buf), &n)) {
// // //     Serial.write(buf, n);
// // //     Serial.println();
// // //   }

// // //   Serial.println(F("--- End of log dump ---\n"));
// // // }

// // // // ---------- SETUP ----------
// // // void setup() {
// // //   Serial.begin(115200);
// // //   uint32_t t0 = millis();
// // //   while (!Serial && (millis() - t0 < 3000)) delay(10);

// // //   Serial.println(F("Starting LED + AS7341 + NAND logger..."));

// // //   // NAND init
// // //   if (!begin()) {
// // //     Serial.println(F("NAND begin() failed"));
// // //     while (1) delay(1000);
// // //   }

// // //   uint8_t mfg, dev;
// // //   read_ID(mfg, dev);
// // //   Serial.print(F("NAND ID: MFG=0x")); Serial.print(mfg, HEX);
// // //   Serial.print(F(" DEV=0x"));         Serial.println(dev, HEX);

// // //   print_status(get_status());

// // //   if (!log_begin(LOG_START_BLOCK, LOG_END_BLOCK, true)) {
// // //     Serial.println(F("log_begin failed!"));
// // //     while (1) delay(1000);
// // //   }

// // //   // TLC5947 init
// // //   pinMode(LED_BLANK_PIN, OUTPUT);
// // //   digitalWrite(LED_BLANK_PIN, LOW);  // keep outputs enabled
// // //   tlc.begin();
// // //   clearAllLEDs();

// // //   Serial.println(F("TLC5947 initialized."));

// // //   // AS7341 init
// // //   if (!as7341.begin()) {
// // //     Serial.println(F("Could not find AS7341 sensor"));
// // //     while (1) delay(1000);
// // //   }
// // //   as7341.setATIME(atime);
// // //   as7341.setASTEP(astep);
// // //   as7341.setGain(gain);

// // //   Serial.println(F("AS7341 initialized"));
// // //   delay(1000);

// // //   readyForLoop = true;
// // //   Serial.println(F("Setup complete.\n"));
// // // }

// // // // ---------- LOOP ----------
// // // void loop() {
// // //   if (!readyForLoop) return;

// // //   // For each LED channel
// // //   for (int i = 0; i < numLEDs; i++) {
// // //     // BLANK held LOW (outputs enabled if wired directly)
// // //     digitalWrite(LED_BLANK_PIN, LOW);

// // //     // Turn LED i ON
// // //     tlc.setPWM(i, intensity);
// // //     tlc.write();
// // //     Serial.print(F("LED "));
// // //     Serial.print(i);
// // //     Serial.println(F(" ON"));

// // //     delay(stabTime);

// // //     // --- Read AS7341 channels ---
// // //     if (!as7341.readAllChannels()) {
// // //       Serial.println(F("Error reading all channels!"));
// // //       tlc.setPWM(i, 0);
// // //       tlc.write();
// // //       continue;
// // //     }

// // //     readings[0] = as7341.getChannel(AS7341_CHANNEL_415nm_F1);
// // //     readings[1] = as7341.getChannel(AS7341_CHANNEL_445nm_F2);
// // //     readings[2] = as7341.getChannel(AS7341_CHANNEL_480nm_F3);
// // //     readings[3] = as7341.getChannel(AS7341_CHANNEL_515nm_F4);
// // //     readings[4] = as7341.getChannel(AS7341_CHANNEL_555nm_F5);
// // //     readings[5] = as7341.getChannel(AS7341_CHANNEL_590nm_F6);
// // //     readings[6] = as7341.getChannel(AS7341_CHANNEL_630nm_F7);
// // //     readings[7] = as7341.getChannel(AS7341_CHANNEL_680nm_F8);
// // //     readings[8] = as7341.getChannel(AS7341_CHANNEL_NIR);
// // //     readings[9] = as7341.getChannel(AS7341_CHANNEL_CLEAR);

// // //     delay(delayTime);

// // //     // Turn LED OFF
// // //     tlc.setPWM(i, 0);
// // //     tlc.write();

// // //     // --- Build CSV line & add to batch ---
// // //     String pdData = combineData();
// // //     Serial.print(F("PD Data: "));
// // //     Serial.print(pdData);
// // //     strBatched += pdData;

// // //     numBatched++;

// // //     // --- If batch is full, log to NAND ---
// // //     if (numBatched >= BATCH_WRITE_SIZE) {
// // //       size_t length = strBatched.length();

// // //       Serial.print(F("Logging "));
// // //       Serial.print(numBatched);
// // //       Serial.print(F(" records, "));
// // //       Serial.print(length);
// // //       Serial.println(F(" bytes..."));

// // //       bool ok = log_append(
// // //         reinterpret_cast<const uint8_t*>(strBatched.c_str()),
// // //         static_cast<uint16_t>(length)
// // //       );

// // //       if (!ok) Serial.println(F("log_append FAILED!"));
// // //       else      Serial.println(F("log_append OK."));

// // //       strBatched = "";
// // //       numBatched = 0;

// // //       Serial.println(F("Continue logging? (y/n): "));
// // //       while (!Serial.available());
// // //       char response = Serial.read();
// // //       while (Serial.available()) Serial.read();

// // //       if (response == 'n' || response == 'N') {
// // //         dumpAllLogs();
// // //         while (1) delay(1000);
// // //       }
// // //     }
// // //   }
// // // }
// // #include <Wire.h>
// // #include "Adafruit_TLC5947.h"
// // #include <Adafruit_AS7341.h>

// // // LED Driver Setup
// // #define NUM_TLC5947 1   // Number of LED drivers chained
// // #define DATA_PIN A3     // Data pin for LED driver
// // #define CLOCK_PIN D2     // Clock pin for LED driver
// // #define LATCH_PIN D6     // Latch pin for LED driver

// // Adafruit_TLC5947 tlc = Adafruit_TLC5947(NUM_TLC5947, CLOCK_PIN, DATA_PIN, LATCH_PIN);

// // // Overall LED Setup
// // int ledVisNum = 11;  // Number of Visible Part LEDs
// // int ledIRNum  = 6;   // Number of IR LEDs

// // // AS7341 Setup
// // Adafruit_AS7341 as7341;
// // int pdVisNum = 10;       // Number of PD channels for AS7341
// // int atime    = 50;       // AS7341 ATIME setting (# integration cycles)
// // int astep    = 499;      // AS7341 ASTEP setting (duration of each cycle)
// // as7341_gain_t gain = AS7341_GAIN_256X; // Gain setting for AS7341
// // int ir_pd_pin = 0;       // IR analog pin (unused in this example)

// // // LED arrays (unused here, but kept to preserve initialization structure)
// // int ledVisWavelength[11]   = {8, 9, 10, 11, 13, 14, 15, 18, 19, 22, 23};
// // int ledVisBrightness[11]   = {5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000, 5000};
// // int ledIRWavelength[6]     = {0, 1, 2, 3, 6, 7};
// // int ledIRBrightness[6]     = {5000, 5000, 5000, 5000, 5000, 5000};

// // // Delays
// // int startDelay = 3000;  // Delay after powering on

// // void setup() {
// //   // Serial Setup
// //   Serial.begin(115200);
// //   Serial.println("Serial Test Code. Example input: 0,5000");

// //   // AS7341 Setup (kept for matching initialization structure)
// //   if (!as7341.begin()) {
// //     Serial.println("Could not find AS7341 sensor. Check wiring!");
// //     while (1) { delay(1000); }
// //   }
// //   as7341.setATIME(atime);
// //   as7341.setASTEP(astep);
// //   as7341.setGain(gain);

// //   // Initialize LED driver
// //   tlc.begin();

// //   // Initial Delay (to match your original code's power-on delay)
// //   delay(startDelay);
// // }

// // void loop() {
// //   // Prompt user (you can move this prompt inside an 'if' if you only
// //   // want to see it once or every time there's a new command)
// //   Serial.println("Enter <LEDIndex>,<Intensity> then press Enter:");

// //   // Wait until something arrives over Serial
// //   while (!Serial.available()) {
// //     // do nothing, wait for user input
// //   }

// //   // Read the user input
// //   String input = Serial.readStringUntil('\n');
// //   input.trim(); // remove any trailing newlines/spaces

// //   // Parse the input looking for a comma
// //   int commaIndex = input.indexOf(',');
// //   if (commaIndex == -1) {
// //     // No comma found, invalid input
// //     Serial.println("Invalid input! Please format as <LEDIndex>,<Intensity>");
// //     return;
// //   }

// //   // Extract LED index and intensity
// //   String ledIndexStr = input.substring(0, commaIndex);
// //   String intensityStr = input.substring(commaIndex + 1);

// //   int ledIndex   = ledIndexStr.toInt();
// //   int ledIntensity = intensityStr.toInt();

// //   // Set the LED
// //   tlc.setPWM(ledIndex, ledIntensity);
// //   tlc.write();

// //   // Report back
// //   Serial.print("LED ");
// //   Serial.print(ledIndex);
// //   Serial.print(" set to intensity ");
// //   Serial.println(ledIntensity);
// // }


// // #include <Arduino.h>
// // #include <Adafruit_TLC5947.h>

// // // TLC5947 wiring from your working setup
// // #define NUM_TLC5947 1
// // #define DATA_PIN    A3
// // #define CLOCK_PIN   D2
// // #define LATCH_PIN   D6

// // // Matches your working constructor order (clock, data, latch)
// // Adafruit_TLC5947 tlc = Adafruit_TLC5947(NUM_TLC5947, CLOCK_PIN, DATA_PIN, LATCH_PIN);

// // void clearAll() {
// //   for (int ch = 0; ch < 24; ch++) {
// //     tlc.setPWM(ch, 0);
// //   }
// //   tlc.write();
// // }

// // void setup() {
// //   Serial.begin(115200);
// //   delay(500);

// //   Serial.println("Sweeping TLC5947 channels 0 through 17...");

// //   tlc.begin();
// //   clearAll();
// // }

// // void loop() {
// //   // Sweep only channels 0–17
// //   for (int ch = 0; ch < 18; ch++) {

// //     clearAll();
// //     tlc.setPWM(ch, 10);   // full brightness
// //     tlc.write();

// //     Serial.print("LED channel ");
// //     Serial.print(ch);
// //     Serial.println(" ON");

// //     delay(500);
// //   }
// // }


// // #include <Arduino.h>
// // #include <Wire.h>
// // #include <SPI.h>

// // #include <Adafruit_TLC5947.h>
// // #include <Adafruit_AS7341.h>

// // // ---------- TLC5947 CONFIG (MATCHES YOUR WORKING CODE) ----------
// // #define NUM_TLC5947  1
// // #define DATA_PIN     A3      // Data pin for LED driver
// // #define CLOCK_PIN    D2      // Clock pin for LED driver
// // #define LATCH_PIN    D6      // Latch pin for LED driver

// // // IMPORTANT: this constructor order matches your working sketch:
// // // Adafruit_TLC5947(NUM_TLC5947, CLOCK_PIN, DATA_PIN, LATCH_PIN)
// // Adafruit_TLC5947 tlc = Adafruit_TLC5947(NUM_TLC5947, CLOCK_PIN, DATA_PIN, LATCH_PIN);

// // // We’ll use TLC channels 0..17 (inclusive)
// // const int numLEDs = 18;      // channels 0–17

// // // ---------- AS7341 CONFIG ----------
// // Adafruit_AS7341 as7341;
// // as7341_gain_t gain = AS7341_GAIN_256X;

// // int pdVisNum = 10;           // F1..F8 + NIR + CLEAR
// // int atime    = 50;
// // int astep    = 499;

// // uint16_t readings[10] = {0};

// // // LED intensity and timing
// // uint16_t intensity  = 4095;  // max brightness to make sure you see it
// // int stabTime        = 1000;  // ms LED on before reading sensor
// // int delayTime       = 5;     // small extra delay

// // // ---------- HELPERS ----------
// // void clearAllLEDs() {
// //   for (int ch = 0; ch < 24; ch++) {
// //     tlc.setPWM(ch, 0);
// //   }
// //   tlc.write();
// // }

// // String combineData() {
// //   String pddata = "";
// //   for (int i = 0; i < pdVisNum; i++) {
// //     pddata += String(readings[i]);
// //     if (i < pdVisNum - 1) {
// //       pddata += ",";
// //     }
// //   }
// //   return pddata + "\n";
// // }

// // // ---------- SETUP ----------
// // void setup() {
// //   Serial.begin(115200);
// //   uint32_t t0 = millis();
// //   while (!Serial && (millis() - t0 < 3000)) {
// //     delay(10);
// //   }

// //   Serial.println(F("jane.ino: TLC5947 (channels 0–17) + AS7341 test (NO NAND)."));

// //   // --- Init TLC5947 (exactly like your working sketch) ---
// //   tlc.begin();
// //   clearAllLEDs();
// //   Serial.println(F("TLC5947 initialized."));

// //   // --- Quick LED sweep test 0–17 so you can confirm visually ---
// //   Serial.println(F("Running one-time LED sweep 0–17..."));
// //   for (int ch = 0; ch < numLEDs; ch++) {
// //     clearAllLEDs();
// //     tlc.setPWM(ch, 4095);
// //     tlc.write();

// //     Serial.print(F("TEST LED channel "));
// //     Serial.print(ch);
// //     Serial.println(F(" ON"));

// //     delay(250);
// //   }
// //   clearAllLEDs();
// //   Serial.println(F("LED sweep complete.\n"));

// //   // --- Init AS7341 ---
// //   if (!as7341.begin()) {
// //     Serial.println(F("Could not find AS7341 sensor. Check wiring!"));
// //     while (1) {
// //       delay(1000);
// //     }
// //   }
// //   as7341.setATIME(atime);
// //   as7341.setASTEP(astep);
// //   as7341.setGain(gain);

// //   Serial.println(F("AS7341 initialized."));
// //   delay(500);

// //   Serial.println(F("Setup complete. Entering main LED 0–17 measurement loop.\n"));
// // }

// // // ---------- LOOP ----------
// // void loop() {
// //   // For each LED channel 0..17
// //   for (int led = 0; led < numLEDs; led++) {

// //     // Turn this LED ON
// //     tlc.setPWM(led, intensity);
// //     tlc.write();
// //     Serial.print(F("LED channel "));
// //     Serial.print(led);
// //     Serial.println(F(" ON"));

// //     delay(stabTime);   // let the light stabilize at the sensor

// //     // Read AS7341 channels
// //     if (!as7341.readAllChannels()) {
// //       Serial.println(F("Error reading all channels!"));
// //       // Turn LED off and continue
// //       tlc.setPWM(led, 0);
// //       tlc.write();
// //       continue;
// //     }

// //     readings[0] = as7341.getChannel(AS7341_CHANNEL_415nm_F1);
// //     readings[1] = as7341.getChannel(AS7341_CHANNEL_445nm_F2);
// //     readings[2] = as7341.getChannel(AS7341_CHANNEL_480nm_F3);
// //     readings[3] = as7341.getChannel(AS7341_CHANNEL_515nm_F4);
// //     readings[4] = as7341.getChannel(AS7341_CHANNEL_555nm_F5);
// //     readings[5] = as7341.getChannel(AS7341_CHANNEL_590nm_F6);
// //     readings[6] = as7341.getChannel(AS7341_CHANNEL_630nm_F7);
// //     readings[7] = as7341.getChannel(AS7341_CHANNEL_680nm_F8);
// //     readings[8] = as7341.getChannel(AS7341_CHANNEL_NIR);
// //     readings[9] = as7341.getChannel(AS7341_CHANNEL_CLEAR);

// //     delay(delayTime);

// //     // Turn LED OFF
// //     tlc.setPWM(led, 0);
// //     tlc.write();

// //     // Print PD data
// //     String pdData = combineData();
// //     Serial.print(F("PD Data (LED "));
// //     Serial.print(led);
// //     Serial.print(F("): "));
// //     Serial.print(pdData);    // already has newline
// //   }
// // }


// #include <Arduino.h>
// #include <Wire.h>
// #include <SPI.h>

// #include <Adafruit_TLC5947.h>
// #include <Adafruit_AS7341.h>

// #include "tanishk_driver.h"   // low-level NAND driver: begin, read_ID, get_status, etc.
// #include "nand_log.h"         // high-level logging: log_begin, log_append, log_iter_next

// // ================== TLC5947 CONFIG (MATCHES WORKING TEST) ==================
// #define NUM_TLC5947  1
// #define DATA_PIN     A3      // Data pin for LED driver
// #define CLOCK_PIN    D2      // Clock pin for LED driver
// #define LATCH_PIN    D6      // Latch pin for LED driver

// // IMPORTANT: this constructor order matches your known-working sketch:
// // Adafruit_TLC5947(NUM_TLC5947, CLOCK_PIN, DATA_PIN, LATCH_PIN)
// Adafruit_TLC5947 tlc = Adafruit_TLC5947(NUM_TLC5947, CLOCK_PIN, DATA_PIN, LATCH_PIN);

// // We will use TLC channels 0..17 (inclusive)
// const int numLEDs = 18;      // LED channels 0–17

// // ================== NAND LOG REGION CONFIG ==================
// const uint16_t LOG_START_BLOCK = 10;
// const uint16_t LOG_END_BLOCK   = 199;   // 190 blocks total

// // ================== AS7341 CONFIG ==================
// Adafruit_AS7341 as7341;
// as7341_gain_t gain = AS7341_GAIN_256X;

// int pdVisNum = 10;           // F1..F8 + NIR + CLEAR
// int atime    = 50;
// int astep    = 499;

// uint16_t readings[10] = {0};

// // ================== LED TIMING / BRIGHTNESS ==================
// uint16_t intensity  = 4095;  // max brightness to make sure you see it
// int stabTime        = 1000;  // ms LED on before reading sensor
// int delayTime       = 5;     // small extra delay after reading

// // ================== LOGGING / BATCHING STATE ==================
// #define BATCH_WRITE_SIZE 16
// String   strBatched   = "";
// uint8_t  numBatched   = 0;
// bool     readyForLoop = false;

// // ================== HELPERS ==================

// void clearAllLEDs() {
//   for (int ch = 0; ch < 24; ch++) {
//     tlc.setPWM(ch, 0);
//   }
//   tlc.write();
// }

// // Combine one AS7341 reading into CSV: "r0,r1,...,r9\n"
// String combineData() {
//   String pddata = "";
//   for (int i = 0; i < pdVisNum; i++) {
//     pddata += String(readings[i]);
//     if (i < pdVisNum - 1) {
//       pddata += ",";
//     }
//   }
//   return pddata + "\n";
// }

// // Dump all log records from NAND
// void dumpAllLogs() {
//   Serial.println(F("\n--- Dumping all log records from NAND ---"));
//   log_iter_reset();

//   uint8_t  buf[128];
//   uint16_t n = 0;

//   while (log_iter_next(buf, sizeof(buf), &n)) {
//     Serial.write(buf, n);
//     Serial.println();
//   }

//   Serial.println(F("--- End of log dump ---\n"));
// }

// // ================== SETUP ==================

// void setup() {
//   Serial.begin(115200);
//   uint32_t t0 = millis();
//   while (!Serial && (millis() - t0 < 3000)) {
//     delay(10);
//   }

//   Serial.println(F("jane.ino: LED 0–17 + AS7341 + NAND logging"));

//   // --- NAND low-level init ---
//   if (!begin()) {
//     Serial.println(F("NAND begin() failed"));
//     while (1) {
//       delay(1000);
//     }
//   }

//   uint8_t mfg = 0, dev = 0;
//   if (read_ID(mfg, dev)) {
//     Serial.print(F("NAND ID: MFG=0x")); Serial.print(mfg, HEX);
//     Serial.print(F(" DEV=0x"));         Serial.println(dev, HEX);
//   } else {
//     Serial.println(F("read_ID failed"));
//   }

//   uint8_t sr = get_status();
//   Serial.print(F("Initial NAND status: "));
//   print_status(sr);

//   // --- Log layer init over blocks [10..199] ---
//   // Use format_if_blank = true for now (erases this range at startup).
//   if (!log_begin(LOG_START_BLOCK, LOG_END_BLOCK, /*format_if_blank=*/true)) {
//     Serial.println(F("log_begin failed!"));
//     while (1) {
//       delay(1000);
//     }
//   }

//   Serial.print(F("Log initialized over blocks "));
//   Serial.print(LOG_START_BLOCK);
//   Serial.print(F(".."));
//   Serial.println(LOG_END_BLOCK);

//   // --- TLC5947 init (exactly as in your working test) ---
//   tlc.begin();
//   clearAllLEDs();
//   Serial.println(F("TLC5947 initialized."));

//   // One quick sweep 0–17 to verify LEDs still work with NAND in the picture
//   Serial.println(F("One-time LED sweep 0–17 (sanity check)..."));
//   for (int ch = 0; ch < numLEDs; ch++) {
//     clearAllLEDs();
//     tlc.setPWM(ch, 4095);
//     tlc.write();

//     Serial.print(F("TEST LED channel "));
//     Serial.print(ch);
//     Serial.println(F(" ON"));

//     delay(200);
//   }
//   clearAllLEDs();
//   Serial.println(F("LED sweep complete.\n"));

//   // --- AS7341 init ---
//   if (!as7341.begin()) {
//     Serial.println(F("Could not find AS7341 sensor. Check wiring!"));
//     while (1) {
//       delay(1000);
//     }
//   }
//   as7341.setATIME(atime);
//   as7341.setASTEP(astep);
//   as7341.setGain(gain);

//   Serial.println(F("AS7341 initialized."));
//   delay(500);

//   readyForLoop = true;
//   Serial.println(F("Setup complete. Entering main logging loop.\n"));
// }

// // ================== LOOP ==================

// void loop() {
//   if (!readyForLoop) return;

//   // Sweep LED channels 0..17
//   for (int led = 0; led < numLEDs; led++) {

//     // Turn this LED ON
//     tlc.setPWM(led, intensity);
//     tlc.write();
//     Serial.print(F("LED channel "));
//     Serial.print(led);
//     Serial.println(F(" ON"));

//     delay(stabTime);   // let light stabilize at the sensor

//     // Read AS7341 channels
//     if (!as7341.readAllChannels()) {
//       Serial.println(F("Error reading all AS7341 channels!"));
//       // Turn LED off and continue
//       tlc.setPWM(led, 0);
//       tlc.write();
//       continue;
//     }

//     readings[0] = as7341.getChannel(AS7341_CHANNEL_415nm_F1);
//     readings[1] = as7341.getChannel(AS7341_CHANNEL_445nm_F2);
//     readings[2] = as7341.getChannel(AS7341_CHANNEL_480nm_F3);
//     readings[3] = as7341.getChannel(AS7341_CHANNEL_515nm_F4);
//     readings[4] = as7341.getChannel(AS7341_CHANNEL_555nm_F5);
//     readings[5] = as7341.getChannel(AS7341_CHANNEL_590nm_F6);
//     readings[6] = as7341.getChannel(AS7341_CHANNEL_630nm_F7);
//     readings[7] = as7341.getChannel(AS7341_CHANNEL_680nm_F8);
//     readings[8] = as7341.getChannel(AS7341_CHANNEL_NIR);
//     readings[9] = as7341.getChannel(AS7341_CHANNEL_CLEAR);

//     delay(delayTime);

//     // Turn LED OFF
//     tlc.setPWM(led, 0);
//     tlc.write();

//     // Build CSV line and print
//     String pdData = combineData();
//     Serial.print(F("PD Data (LED "));
//     Serial.print(led);
//     Serial.print(F("): "));
//     Serial.print(pdData);    // already includes newline

//     // Add to batch string
//     strBatched += pdData;
//     numBatched++;

//     // When batch is full, append to NAND log
//     if (numBatched >= BATCH_WRITE_SIZE) {
//       size_t length = strBatched.length();

//       Serial.print(F("\nBatch full: "));
//       Serial.print(numBatched);
//       Serial.print(F(" records, "));
//       Serial.print(length);
//       Serial.println(F(" bytes. Logging to NAND..."));

//       bool ok = log_append(
//         reinterpret_cast<const uint8_t*>(strBatched.c_str()),
//         static_cast<uint16_t>(length)
//       );

//       if (!ok) {
//         Serial.println(F("log_append FAILED!"));
//       } else {
//         Serial.println(F("log_append OK."));
//       }

//       // Reset batch
//       strBatched = "";
//       numBatched = 0;

//       // Ask user whether to continue or dump logs
//       Serial.println(F("Continue logging? (y/n): "));
//       while (!Serial.available()) {
//         // wait for user input
//       }

//       char response = Serial.read();
//       // Clear any extra bytes in the buffer
//       while (Serial.available()) {
//         Serial.read();
//       }

//       if (response == 'n' || response == 'N') {
//         Serial.println(F("\nStopping data collection and reading from NAND..."));
//         dumpAllLogs();

//         // Stop forever
//         while (1) {
//           delay(500);
//         }
//       }

//       Serial.println(F("Continuing logging...\n"));
//     } // end batch check
//   }   // end for each LED 0..17
// }


#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

#include <Adafruit_TLC5947.h>
#include <Adafruit_AS7341.h>

#include "tanishk_driver.h"   // low-level NAND driver: begin, read_ID, get_status, etc.
#include "nand_log.h"         // high-level logging: log_begin, log_append, log_iter_next

// ================== TLC5947 CONFIG (MATCHES WORKING TEST) ==================
#define NUM_TLC5947  1
#define DATA_PIN     A3      // Data pin for LED driver
#define CLOCK_PIN    D2      // Clock pin for LED driver
#define LATCH_PIN    D6      // Latch pin for LED driver

// Constructor (matches your known-working sketch: clock, data, latch)
Adafruit_TLC5947 tlc = Adafruit_TLC5947(NUM_TLC5947, CLOCK_PIN, DATA_PIN, LATCH_PIN);

// We will use TLC channels 0..17 (inclusive)
const int numLEDs = 18;      // LED channels 0–17

// ================== NAND LOG REGION CONFIG ==================
const uint16_t LOG_START_BLOCK = 10;
const uint16_t LOG_END_BLOCK   = 199;   // 190 blocks total (ring region)

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
#define BATCH_WRITE_SIZE 16
String   strBatched   = "";
uint8_t  numBatched   = 0;
bool     readyForLoop = false;

// ================== HELPERS ==================

void clearAllLEDs() {
  for (int ch = 0; ch < 24; ch++) {
    tlc.setPWM(ch, 0);
  }
  tlc.write();
}

// Combine one AS7341 reading into CSV: "r0,r1,...,r9\n"
String combineData() {
  String pddata = "";
  for (int i = 0; i < pdVisNum; i++) {
    pddata += String(readings[i]);
    if (i < pdVisNum - 1) {
      pddata += ",";
    }
  }
  return pddata + "\n";
}

// ================== SETUP ==================
=======
#include <Arduino.h>
#include <SPI.h>
#include "tanishk_driver.h"
#include "nand_log.h"
>>>>>>> 2d865d63e86ea5c23b085de60ee4fc40cafec81d

void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
<<<<<<< HEAD
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
  if (!log_begin(LOG_START_BLOCK, LOG_END_BLOCK, /*format_if_blank=*/true)) {
    Serial.println(F("log_begin failed!"));
    while (1) {
      delay(1000);
    }
  }

  Serial.print(F("Log initialized over blocks "));
  Serial.print(LOG_START_BLOCK);
  Serial.print(F(".."));
  Serial.println(LOG_END_BLOCK);

  // --- TLC5947 init (exactly as in your working test) ---
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

    delay(stabTime);   // let light stabilize at the sensor

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
    String pdData = combineData();
    Serial.print(F("PD Data (LED "));
    Serial.print(led);
    Serial.print(F("): "));
    Serial.print(pdData);    // already includes newline

    // Add to batch string
    strBatched += pdData;
    numBatched++;

    // When batch is full, append to NAND log
    if (numBatched >= BATCH_WRITE_SIZE) {
      size_t length = strBatched.length();

      Serial.print(F("\nBatch full: "));
      Serial.print(numBatched);
      Serial.print(F(" records, "));
      Serial.print(length);
      Serial.println(F(" bytes. Logging to NAND..."));

      bool ok = log_append(
        reinterpret_cast<const uint8_t*>(strBatched.c_str()),
        static_cast<uint16_t>(length)
      );

      if (!ok) {
        Serial.println(F("log_append FAILED!"));
      } else {
        Serial.println(F("log_append OK."));
      }

      // Reset batch
      strBatched = "";
      numBatched = 0;

      Serial.println(F("Continuing continuous logging...\n"));
    } // end batch check
  }   // end for each LED 0..17
}
=======
  while (!Serial && (millis() - t0 < 3000)) { delay(10); }

  begin();

  uint8_t mfg = 0, dev = 0;
  read_ID(mfg, dev);
  Serial.print(F("ID: MFG=0x")); Serial.print(mfg, HEX);
  Serial.print(F(" DEV=0x"));     Serial.println(dev, HEX);
  uint8_t status = get_status();
  print_status(status);

  flashAddr.block = 10;
  flashAddr.page = 0;
  flashAddr.column = 0;

  if (!erase_block(flashAddr.block)) {
    Serial.println(F("[TEST] Erase failed"));
  }
  const char msg[] = "HELLO WORLD";
  write_bytes((const uint8_t*) msg, (uint16_t) (sizeof(msg) - 1));
  

  uint8_t buf[sizeof(msg)-1] = {0};
  if (!read_bytes(buf, sizeof(buf))) {
    Serial.println(F("[TEST] Read failed"));
    return;
  }

  bool ok = true;
  for (size_t i=0;i<sizeof(buf);++i) if (buf[i] != (uint8_t)msg[i]) { ok=false; break; }
  Serial.println(ok ? F("[VERIFY] OK") : F("[VERIFY] MISMATCH"));

  Serial.print(F("HEX: "));
  for (uint8_t b: buf) { if (b<16) Serial.print('0'); Serial.print(b, HEX); Serial.print(' '); }
  Serial.print(F(" | ASCII: "));
  for (uint8_t b: buf) Serial.print((char)b);
  Serial.println();

  status = get_status();
  print_status(status);


  // Use blocks 10..200 as the log region (example)
  log_begin(10, 200, /*format_if_blank=*/false);

  const char msg2[] = "TEMP=24.8 HR=62";
  log_append((const uint8_t*)msg2, sizeof(msg2)-1);

  // Later, read them back
  log_iter_reset();
  uint8_t buf2[128]; uint16_t n=0;
  while (log_iter_next(buf2, sizeof(buf2), &n)) {
    Serial.write(buf2, n); Serial.println();
  }
}

void loop() {}
>>>>>>> 2d865d63e86ea5c23b085de60ee4fc40cafec81d
