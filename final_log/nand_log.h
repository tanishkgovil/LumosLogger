#pragma once
#include <Arduino.h>
#include <stdint.h>
#include "nand_driver.h"

// ====== DEVICE CONFIG (set these for your chip) ======
#ifndef NAND_MAIN_BYTES
#define NAND_MAIN_BYTES   4096    // main area per page
#endif
#ifndef NAND_SPARE_BYTES
#define NAND_SPARE_BYTES   256    // spare(OOB) per page
#endif
#ifndef NAND_PAGES_PER_BLOCK
#define NAND_PAGES_PER_BLOCK 64
#endif
#ifndef NAND_BLOCK_COUNT
#define NAND_BLOCK_COUNT  2048     // set to your device total blocks
#endif

// A small record header (power-fail safe)
struct __attribute__((packed)) LogHdr {
  uint16_t magic;       // 0xA55A
  uint16_t length;      // payload length
  uint32_t seq;         // monotonically increasing sequence number
  uint16_t crc16;       // CRC-CCITT of payload
  uint16_t hdr_crc;     // CRC-CCITT of {magic,length,seq,crc16}
};

bool log_begin(uint16_t start_block, uint16_t end_block, bool format_if_blank);
bool log_append(const uint8_t* data, uint16_t len);
bool log_flush();  // flush any buffered data to flash

// Optional: iterate forward from oldest to newest (after log_begin)
void log_iter_reset();
bool log_iter_next(uint8_t* out, uint16_t max_out, uint16_t* out_len);

// Optional helpers
void log_format_range();        // full erase of [start..end] (skips factory-bad)
uint32_t log_record_count();    // best-effort count
uint32_t log_next_seq();        // next sequence number to be written
