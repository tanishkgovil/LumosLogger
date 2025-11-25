#include <Arduino.h>
#include <SPI.h>
#include "tanishk_driver.h"
#include "nand_log.h"

void loop() {}

void read() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0 < 3000)) { delay(10); }
  begin();
  unlock_all_blocks();
  // manually read ascii bytes from log
  Serial.println(F("Manual read of log data:"));
  for (uint16_t blk=10; blk<=10; ++blk) {
    for (uint8_t page=0; page<5; ++page) {
      flashAddr.block = blk;
      flashAddr.page = page;
      flashAddr.column = 0;
      uint8_t probe[2] = {0};
      if (!read_bytes(probe, 2)) continue;
      if (probe[0] == 0xFF && probe[1] == 0xFF) continue; // blank page
      Serial.print(F("Block ")); Serial.print(blk);
      Serial.print(F(" Page ")); Serial.print(page);
      Serial.print(F(" Data: "));
      // read full page
      const uint16_t to_read = NAND_MAIN_BYTES;
      uint8_t* page_buf = new uint8_t[to_read];
      if (read_bytes(page_buf, to_read)) {
        for (uint16_t i=0;i<to_read;++i) {
          // uint8_t b = page_buf[i];
          // // if (b >= 32 && b <= 126) Serial.write(b); else Serial.write('.');
          // Serial.write(b, HEX); Serial.print(' ');
          // Print as hex
          uint8_t b = page_buf[i];
          if (b < 16) Serial.print('0');
          Serial.print(b, HEX); Serial.print(' ');
        }
        Serial.println();
      }
      delete[] page_buf;
    }
  }
}

void buffer() {
  // create a log entry that is buffered in parts then flushed
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0 < 3000)) { delay(10); }

  begin();

  // Use blocks 10..20 as the log region (example)
  log_begin(10, 20, /*format_if_blank=*/false);
  const uint16_t total_size = 10000; // less than a page
  uint8_t* payload = new uint8_t[total_size];
  for (uint16_t i=0;i<total_size;++i) payload[i] = (uint8_t)(i & 0xFF);
  
  // append in two parts
  bool success = log_append(payload, 2500);
  success = success && log_append(&payload[2500], 2500);
  success = success && log_append(&payload[5000], 2500);
  success = success && log_append(&payload[7500], 2500);
  if (success) {
    Serial.println(F("Log append of buffered payload succeeded"));
  } else {
    Serial.println(F("Log append of buffered payload FAILED"));
  }
  delete[] payload;
  
  // read back the log entry
  log_iter_reset();
  uint8_t buf2[128]; uint16_t n=0;
  while (log_iter_next(buf2, sizeof(buf2), &n)) {
    Serial.write(buf2, n); Serial.println();
  }
}

void erase() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0 < 3000)) { delay(10); }

  begin();

  // Use blocks 10..20 as the log region (example)
  log_begin(10, 20, /*format_if_blank=*/false);
  log_format_range();
  Serial.println(F("Log region erased"));
}

void setup() {
  erase();
  buffer();
  read();
}