#include <Arduino.h>
#include <SPI.h>
#include "tanishk_driver.h"
#include "nand_log.h"

void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
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
