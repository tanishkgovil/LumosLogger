#include <Arduino.h>
#include <SPI.h>
#include "tanishk_driver.h"
#include "nand_log.h"

void loop() {}

void manualread() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0 < 3000)) { delay(10); }
  begin();
  unlock_all_blocks();
  // manually read ascii bytes from log
  Serial.println(F("Manual read of log data:"));
  for (uint16_t blk=0; blk<=0; ++blk) {
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

void erase() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0 < 3000)) { delay(10); }

  begin();

  // Use blocks 0..20 as the log region (example)
  log_begin(0, 20, /*format_if_blank=*/false);
  log_format_range();
  Serial.println(F("Log region erased"));
}

void test_dummy_data() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && (millis() - t0 < 3000)) { delay(10); }

  begin();
  log_begin(0, 20, /*format_if_blank=*/false);
  // format of log data (example line): 415nm,445nm,480nm,515nm,555nm,590nm,630nm,680nm,NIR,Clear,0;
  // fill with 1000 lines of dummy data
  const char* line = "415nm,445nm,480nm,515nm,555nm,590nm,630nm,680nm,NIR,Clear,0;";
  const int line_len = strlen(line);
  const int total_lines = 1000;
  for (int i=0;i<total_lines;++i) {
    Serial.print(F("Appending line ")); Serial.println(i);
    if (!log_append(reinterpret_cast<const uint8_t*>(line), line_len)) {
      Serial.print(F("Failed to append line ")); Serial.println(i);
      break;
    }
  }

  if (!log_flush()) {
    Serial.println(F("Failed to flush log data"));
  }

  Serial.println(F("Dummy data log append complete"));

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

void setup() {
  erase();
  test_dummy_data();
}