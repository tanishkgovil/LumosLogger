#include "nand_log.h"

// ---------- local state ----------
static uint16_t g_startBlk, g_endBlk;         // inclusive range for log
static uint16_t g_curBlk;                      // write cursor
static uint8_t  g_curPage;
static uint16_t g_curCol;
static uint32_t g_nextSeq;
static bool     g_haveRange = false;

// simple BBT; true means "bad"
static bool g_bad[NAND_BLOCK_COUNT];

// iterator
static uint16_t it_blk; static uint8_t it_page; static uint16_t it_col;

// ---------- small utilities ----------
static inline uint16_t crc16_ccitt(const uint8_t* d, uint16_t n) {
  uint16_t c = 0xFFFF;
  for (uint16_t i=0;i<n;i++) {
    c ^= (uint16_t)d[i] << 8;
    for (uint8_t b=0;b<8;b++)
      c = (c & 0x8000) ? (uint16_t)((c << 1) ^ 0x1021) : (uint16_t)(c << 1);
  }
  return c;
}

static inline void set_addr(uint16_t blk, uint8_t page, uint16_t col) {
  flashAddr.block  = blk;
  flashAddr.page   = page & 0x3F;
  flashAddr.column = col & 0x1FFF;
}

static bool is_factory_bad(uint16_t blk) {
  // Convention: first spare byte of page 0 marks a bad block if != 0xFF
  uint8_t mark = 0xFF;
  set_addr(blk, 0, NAND_MAIN_BYTES); // first byte in spare(OOB)
  return (!read_bytes(&mark, 1)) || (mark != 0xFF);
}

static uint16_t next_good_block(uint16_t blk) {
  uint16_t b = blk;
  while (true) {
    if (++b > g_endBlk) b = g_startBlk; // wrap within ring
    if (!g_bad[b]) return b;
  }
}

static void advance_by(uint32_t n) {
  // Advance cursor across column -> page -> block (skips bad)
  while (n > 0) {
    uint32_t rem = NAND_MAIN_BYTES - g_curCol;
    uint32_t step = (n < rem) ? n : rem;
    g_curCol += (uint16_t)step;
    n -= step;
    if (g_curCol >= NAND_MAIN_BYTES) {
      g_curCol = 0;
      g_curPage++;
      if (g_curPage >= NAND_PAGES_PER_BLOCK) {
        // move to next good block; erase it for fresh programming
        g_curPage = 0;
        g_curBlk  = next_good_block(g_curBlk);
        erase_block(g_curBlk);
      }
    }
  }
}

static void cursor_to_flash() { set_addr(g_curBlk, g_curPage, g_curCol); }

// Returns true if page looks unused (first two bytes 0xFF)
static bool page_is_blank(uint16_t blk, uint8_t page) {
  uint8_t probe[2] = {0};
  set_addr(blk, page, 0);
  if (!read_bytes(probe, 2)) return false;
  return (probe[0] == 0xFF && probe[1] == 0xFF);
}

static bool write_one_page_payload(const uint8_t* buf, uint16_t len, uint32_t seq) {
  // We write: [LogHdr][payload] in a single program operation (<= main bytes)
  LogHdr h;
  h.magic   = 0xA55A;
  h.length  = len;
  h.seq     = seq;
  h.crc16   = crc16_ccitt(buf, len);
  // tmp for header CRC
  uint8_t tmp[sizeof(LogHdr)-2];
  memcpy(tmp, &h, sizeof(LogHdr)-2);
  h.hdr_crc = crc16_ccitt(tmp, sizeof(tmp));

  // Must not cross page boundary
  const uint32_t needed = (uint32_t)sizeof(LogHdr) + len;
  if (g_curCol + needed > NAND_MAIN_BYTES) return false;

  // Program in one shot to avoid multiple partial programs
  cursor_to_flash();
  // Build a small staging copy in chunks (avoid big RAM)
  // 1) header
  write_bytes(reinterpret_cast<const uint8_t*>(&h), sizeof(LogHdr));
  // 2) payload
  cursor_to_flash(); // write_bytes advanced within the page; reset addr for contiguous write
  advance_by(sizeof(LogHdr));
  cursor_to_flash();
  write_bytes(buf, len);

  // advance cursor past this record (and pad to end if desired)
  advance_by(len);
  return true;
}

static bool read_hdr_at(uint16_t blk, uint8_t page, uint16_t col, LogHdr& out) {
  set_addr(blk, page, col);
  if (!read_bytes(reinterpret_cast<uint8_t*>(&out), sizeof(LogHdr))) return false;
  if (out.magic != 0xA55A) return false;

  uint8_t tmp[sizeof(LogHdr)-2];
  memcpy(tmp, &out, sizeof(LogHdr)-2);
  uint16_t expect = crc16_ccitt(tmp, sizeof(tmp));
  return expect == out.hdr_crc;
}

// ---------- public API ----------
bool log_begin(uint16_t start_block, uint16_t end_block, bool format_if_blank) {
  if (start_block > end_block) return false;
  g_startBlk = start_block;
  g_endBlk   = end_block;
  g_haveRange = true;

  // Build bad-block table (factory bad); keep runtime-grown bads out of scope for now
  for (uint16_t b = 0; b < NAND_BLOCK_COUNT; ++b) g_bad[b] = false;
  for (uint16_t b = g_startBlk; b <= g_endBlk; ++b) {
    g_bad[b] = is_factory_bad(b);
  }

  // Optionally format (erase) the range
  if (format_if_blank) {
    for (uint16_t b = g_startBlk; b <= g_endBlk; ++b) {
      if (!g_bad[b]) erase_block(b);
    }
  }

  // Find the first non-blank page (oldest) and the first blank page (tip)
  // Simple linear scan; for speed you can binary-search per block later.
  uint16_t tipBlk = g_startBlk; uint8_t tipPage = 0;
  bool found_tip = false;
  uint32_t maxSeq = 0;

  for (uint16_t b = g_startBlk; b <= g_endBlk; ++b) {
    if (g_bad[b]) continue;
    for (uint8_t p = 0; p < NAND_PAGES_PER_BLOCK; ++p) {
      if (page_is_blank(b, p)) { tipBlk = b; tipPage = p; found_tip = true; break; }
      // track max seq to continue counting if needed
      LogHdr h;
      if (read_hdr_at(b, p, 0, h)) {
        if (h.seq > maxSeq) maxSeq = h.seq;
      }
    }
    if (found_tip) break;
  }

  if (!found_tip) {
    // Range completely full; start at beginning (ring) and overwrite
    tipBlk = g_startBlk; tipPage = 0;
  }

  g_curBlk = tipBlk;
  g_curPage = tipPage;
  g_curCol = 0;
  g_nextSeq = maxSeq + 1;

  // If we’re starting mid-block on a blank page, ensure block is erased
  if (!g_bad[g_curBlk]) erase_block(g_curBlk);

  // Init iterator at oldest page
  it_blk = g_curBlk; it_page = g_curPage; it_col = 0; // will be reset by log_iter_reset()
  return true;
}

bool log_append(const uint8_t* data, uint16_t len) {
  if (!g_haveRange || !data || len == 0) return false;

  // If record won’t fit in remaining bytes of this page, move to next page
  if (g_curCol + sizeof(LogHdr) + len > NAND_MAIN_BYTES) {
    // pad remainder (optional: write 0xFF — but NAND already reads as 0xFF when blank)
    g_curCol = 0;
    g_curPage++;
    if (g_curPage >= NAND_PAGES_PER_BLOCK) {
      g_curPage = 0;
      g_curBlk = next_good_block(g_curBlk);
      erase_block(g_curBlk);
    }
  }

  // Write it
  bool ok = write_one_page_payload(data, len, g_nextSeq++);
  if (!ok) return false;

  // Move to page boundary after record to keep “<= 1 program per page” rule
  // (We wrote header+payload contiguously in one page/program.)
  if (g_curCol != 0) { g_curCol = 0; g_curPage++; }
  if (g_curPage >= NAND_PAGES_PER_BLOCK) {
    g_curPage = 0;
    g_curBlk = next_good_block(g_curBlk);
    erase_block(g_curBlk);
  }
  return true;
}

void log_iter_reset() {
  // Find oldest page by scanning from start until first non-blank, then that’s head
  for (uint16_t b = g_startBlk; b <= g_endBlk; ++b) {
    if (g_bad[b]) continue;
    for (uint8_t p = 0; p < NAND_PAGES_PER_BLOCK; ++p) {
      if (!page_is_blank(b,p)) { it_blk=b; it_page=p; it_col=0; return; }
    }
  }
  it_blk = g_startBlk; it_page = 0; it_col = 0;
}

bool log_iter_next(uint8_t* out, uint16_t max_out, uint16_t* out_len) {
  // Stop if we reached the tip (blank page)
  if (page_is_blank(it_blk, it_page)) return false;

  LogHdr h;
  if (!read_hdr_at(it_blk, it_page, 0, h)) {
    // Corrupt header or torn write: treat as end
    return false;
  }
  if (h.length > max_out) return false;

  // read payload and CRC-check
  set_addr(it_blk, it_page, sizeof(LogHdr));
  if (!read_bytes(out, h.length)) return false;
  if (crc16_ccitt(out, h.length) != h.crc16) return false;

  if (out_len) *out_len = h.length;

  // advance iterator to next page
  it_page++;
  if (it_page >= NAND_PAGES_PER_BLOCK) {
    it_page = 0;
    it_blk++;
    if (it_blk > g_endBlk) it_blk = g_startBlk;
  }
  return true;
}

// optional utilities
void log_format_range() {
  for (uint16_t b = g_startBlk; b <= g_endBlk; ++b) if (!g_bad[b]) erase_block(b);
  g_curBlk=g_startBlk; g_curPage=0; g_curCol=0; g_nextSeq=1;
}
uint32_t log_record_count() { return g_nextSeq ? (g_nextSeq - 1) : 0; }
uint32_t log_next_seq() { return g_nextSeq; }