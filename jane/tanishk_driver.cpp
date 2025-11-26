#include <Arduino.h>
#include <SPI.h>
#include "nand_commands.h"
#include "tanishk_driver.h"

// IR PD Configuration
#define IR_PD_PIN A0
// Storage Chip Configuration
#define CHIP_SELECT_PIN A1
#define CS_PIN          CHIP_SELECT_PIN   // <--- FIX: define CS_PIN used below

// LED Driver Configurations (not used by driver, but ok to leave)
#define NUM_TLC5947   1
#define LED_CLOCK     D2
#define LED_DATA_PIN  A3
#define LED_LATCH     D6
#define LED_BLANK_PIN D7

nand_address flashAddr;

static inline void csLow()  { digitalWrite(CS_PIN, LOW); }
static inline void csHigh() { digitalWrite(CS_PIN, HIGH); }

static SPISettings nandSpi(4000000, MSBFIRST, SPI_MODE0);

void send_command(uint8_t command) {
  csLow();
  SPI.transfer(command);
  csHigh();
}

bool begin() {
  pinMode(CS_PIN, OUTPUT);
  csHigh();

  SPI.begin();
  reset_chip();
  unlock_all_blocks();
  return true;
}

void reset_chip() {
  SPI.beginTransaction(nandSpi);
  csLow();
  SPI.transfer(RESET);
  csHigh();
  SPI.endTransaction();

  delayMicroseconds(625);
}

bool read_ID(uint8_t &mfg, uint8_t &dev) {

  SPI.beginTransaction(nandSpi);
  csLow();
  SPI.transfer(READ_ID);
  SPI.transfer(0x00);
  mfg = SPI.transfer(0x00);
  dev = SPI.transfer(0x00);
  csHigh();
  SPI.endTransaction();

  return true;
}

void print_status(uint8_t sr) {
  Serial.print("Status 0x"); Serial.print(sr, HEX);
  Serial.print("  OIP=");  Serial.print(sr & 0x01);
  Serial.print(" WEL=");   Serial.print((sr >> 1) & 0x01);
  Serial.print(" E_Fail=");Serial.print((sr >> 2) & 0x01);
  Serial.print(" P_Fail=");Serial.print((sr >> 3) & 0x01);
  Serial.print(" ECCS=");  Serial.print((sr >> 4) & 0x07, BIN);
  Serial.print(" CRBSY="); Serial.println((sr >> 7) & 0x01);
}

static uint8_t get_feature(uint8_t addr) {
  SPI.beginTransaction(nandSpi);
  csLow();
  SPI.transfer(GET_FEATURE);
  SPI.transfer(addr);
  uint8_t v = SPI.transfer(0x00);
  csHigh();
  SPI.endTransaction();
  return v;
}

uint8_t get_status() { return get_feature(FEATURE_ADDR_STATUS); }

static void set_feature(uint8_t addr, uint8_t val) {
  SPI.beginTransaction(nandSpi);
  csLow();
  SPI.transfer(SET_FEATURE);
  SPI.transfer(addr);
  SPI.transfer(val);
  csHigh();
  SPI.endTransaction();
}

static bool wait_ready(uint32_t timeout_ms) {
  uint32_t t0 = millis();
  while (millis() - t0 < timeout_ms) {
    uint8_t sr = get_status();
    if ((sr & 0x01) == 0) return true;
    delayMicroseconds(200);
  }
  return false;
}

void unlock_all_blocks() {
  set_feature(FEATURE_ADDR_BLOCK_LOCK, 0x00);
}

bool erase_block(uint16_t block) {
  SPI.beginTransaction(nandSpi);
  send_command(WRITE_ENABLE);
  SPI.endTransaction();

  const uint32_t row = ((uint32_t)block << 6) | 0;

  SPI.beginTransaction(nandSpi);
  csLow();
  SPI.transfer(BLOCK_ERASE);
  SPI.transfer((row >> 16) & 0xFF);
  SPI.transfer((row >> 8)  & 0xFF);
  SPI.transfer(row & 0xFF);
  csHigh();
  SPI.endTransaction();

  if (!wait_ready(20)) {
    return false;
  }
  
  uint8_t sr = get_status();
  return (sr & 0x04) == 0;
}

void program_load(uint16_t col, uint16_t length, const uint8_t* data) {
  SPI.beginTransaction(nandSpi);
  csLow();
  SPI.transfer(PROGRAM_LOAD_1);
  SPI.transfer((col >> 8) & 0x1F); // first 5 bits (3 dummy)
  SPI.transfer(col & 0xFF);        // low 8 bits
  for (uint16_t i = 0; i < length; ++i) {
    SPI.transfer(data[i]);
  }
  csHigh();
  SPI.endTransaction();
}

void program_execute(uint32_t row) {
  SPI.beginTransaction(nandSpi);
  csLow();
  SPI.transfer(PROGRAM_EXECUTE);
  SPI.transfer((row >> 16) & 0xFF);
  SPI.transfer((row >> 8) & 0xFF);
  SPI.transfer(row & 0xFF);
  csHigh();
  SPI.endTransaction();
}

int write_bytes(const uint8_t* data, uint16_t length) {

  if (!data || length == 0) {
    return 0;
  }

  const uint16_t col = (flashAddr.column & 0x1FFF);
  const uint32_t row = ((uint32_t) flashAddr.block << 6) | (flashAddr.page & 0x3F);
  uint32_t max_from_col = 4352u - (uint32_t) col;
  if (length > max_from_col) {
    length = (uint16_t) max_from_col;
  }
  
  // 1. Write enable
  SPI.beginTransaction(nandSpi);
  send_command(WRITE_ENABLE);
  SPI.endTransaction();

  // 2. Program Load
  program_load(col, length, data);

  // 3. Program execute
  program_execute(row);

  // 4. Get Feature until OIP=0, then check P_Fail
  if (!wait_ready(20)) {
    Serial.println(F("[PROGRAM] Timeout"));
    return -1;
  }

  uint8_t sr = get_status();
  if (sr & 0x08) {
    Serial.println(F("[PROGRAM] Failed (P_Fail=1). Is the block locked?"));
    return -1;
  }

  return length;
}

bool read_bytes(uint8_t *out, uint16_t length) {
  if (!out || length == 0) {
    return false;
  }

  const uint16_t col = (flashAddr.column & 0x1FFF);
  const uint32_t row = ((uint32_t) flashAddr.block << 6) | (flashAddr.page & 0x3F);

  uint32_t max_from_col = 4352u - (uint32_t)col;
  if (length > max_from_col) {
    length = (uint16_t) max_from_col;
  }

  // 1. Page read
  SPI.beginTransaction(nandSpi);
  csLow();
  SPI.transfer(PAGE_READ);
  SPI.transfer((row >> 16) & 0xFF);
  SPI.transfer((row >> 8)  & 0xFF);
  SPI.transfer(row & 0xFF);
  csHigh();
  SPI.endTransaction();

  if (!wait_ready(20)) return false;

  // 2. Read from cache x1
  SPI.beginTransaction(nandSpi);
  csLow();
  SPI.transfer(READ_FROM_CACHE_1);
  SPI.transfer((col >> 8) & 0x1F);
  SPI.transfer(col & 0xFF);
  SPI.transfer(0x00);

  for (uint16_t i = 0; i < length; ++i) {
    out[i] = SPI.transfer(0x00);
  }
  csHigh();
  SPI.endTransaction();
  return true;
}
