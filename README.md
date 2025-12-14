# Lumos NAND Logger

A high-performance NAND flash logging library for Arduino and embedded systems. This library provides a robust, sequential logging system for writing structured data to SPI NAND flash memory with automatic bad block management and data integrity checking.

## Features

- **Structured Logging**: Records stored with headers containing sequence numbers, checksums, and metadata
- **Sequential Append-Only**: Writes records sequentially without wrap-around, preventing accidental data loss
- **Data Integrity**: CRC16-CCITT checksums on both headers and payloads
- **Bad Block Management**: Automatically detects and skips factory-marked bad blocks
- **Buffered Writes**: Internal payload buffering for efficient flash writes
- **Iterator API**: Read back records sequentially from oldest to newest
- **Flash-Friendly**: Respects NAND constraints (≤1 program per page before erase)

## Hardware Requirements

- Arduino-compatible microcontroller with SPI support
- SPI NAND flash memory (e.g., Winbond W25N series)
- SPI bus connections:
  - MISO, MOSI, SCK
  - CS (Chip Select) - default on pin A1

## Default Configuration

The library is configured for typical NAND flash specifications:

```cpp
#define NAND_MAIN_BYTES        4096   // Main area per page
#define NAND_SPARE_BYTES       256    // Spare (OOB) area per page
#define NAND_PAGES_PER_BLOCK   64     // Pages per block
#define NAND_BLOCK_COUNT       2048   // Total blocks in device
```

You can override these in your sketch before including the library headers.

## Installation

1. Copy the `final_log` folder to your Arduino libraries directory
2. Include the library in your sketch:

```cpp
#include "nand_driver.h"
#include "nand_log.h"
```

## API Reference

### NAND Driver Functions

#### `bool begin()`
Initializes the NAND driver. Must be called before any other driver operations.

**Returns**: `true` on success, `false` on failure

**Example**:
```cpp
if (!begin()) {
  Serial.println("NAND initialization failed!");
  while(1);
}
```

#### `void reset_chip()`
Performs a hardware reset of the NAND chip.

#### `bool read_ID(uint8_t &mfg, uint8_t &dev)`
Reads the manufacturer and device ID from the NAND chip.

**Parameters**:
- `mfg`: Reference to store manufacturer ID
- `dev`: Reference to store device ID

**Returns**: `true` on success

**Example**:
```cpp
uint8_t mfg, dev;
if (read_ID(mfg, dev)) {
  Serial.print("Manufacturer: 0x");
  Serial.print(mfg, HEX);
  Serial.print(", Device: 0x");
  Serial.println(dev, HEX);
}
```

#### `uint8_t get_status()`
Reads the NAND chip's status register.

**Returns**: Status byte (see NAND datasheet for bit definitions)

#### `bool erase_block(uint16_t block)`
Erases the specified block, preparing it for programming.

**Parameters**:
- `block`: Block number to erase (0 to NAND_BLOCK_COUNT-1)

**Returns**: `true` on success, `false` on failure

#### `void unlock_all_blocks()`
Unlocks all blocks for writing. Called automatically by `begin()`.

### Logging API Functions

#### `bool log_begin(uint16_t start_block, uint16_t end_block, bool format_if_blank)`
Initializes the logging system over a specified block range.

**Parameters**:
- `start_block`: First block to use for logging (inclusive)
- `end_block`: Last block to use for logging (inclusive)
- `format_if_blank`: If `true`, erases all blocks in range during initialization

**Returns**: `true` on success, `false` if parameters are invalid

**Example**:
```cpp
// Use blocks 0-2047 for logging, format on first use
if (!log_begin(0, 2047, true)) {
  Serial.println("Log initialization failed!");
  while(1);
}
```

**Notes**:
- Automatically builds a bad block table and skips factory-marked bad blocks
- Scans the range to find the current write position (tip) and maximum sequence number
- If the range is completely full, sets the log to full state (no wrap-around)

#### `bool log_append(const uint8_t* data, uint16_t len)`
Appends data to the log. Data is buffered internally and written when the buffer is full.

**Parameters**:
- `data`: Pointer to data buffer
- `len`: Number of bytes to append

**Returns**: `true` on success, `false` if log is full or error occurs

**Example**:
```cpp
// Log sensor readings
struct SensorData {
  uint16_t readings[10];
  uint8_t ledIndex;
} __attribute__((packed));

SensorData data;
// ... populate data ...

if (!log_append((uint8_t*)&data, sizeof(data))) {
  Serial.println("Log full or error!");
}
```

**Notes**:
- Maximum payload size is `NAND_MAIN_BYTES - sizeof(LogHdr)` (typically 4084 bytes)
- Data is buffered until the buffer fills, then written as a complete record
- Each record includes a header with magic number, length, sequence number, and checksums

#### `bool log_flush()`
Flushes any buffered data to NAND flash immediately.

**Returns**: `true` on success, `false` if log is full or error occurs

**Example**:
```cpp
log_append(data, len);
log_flush();  // Ensure data is written to flash
```

**Notes**:
- Call this before power-down or when you need to ensure data persistence
- Automatically called when the buffer fills during `log_append()`
- Clears the internal buffer after writing

#### `void log_iter_reset()`
Resets the log iterator to the beginning (oldest record).

**Example**:
```cpp
log_iter_reset();  // Start reading from oldest record
```

#### `bool log_iter_next(uint8_t* out, uint16_t max_out, uint16_t* out_len)`
Reads the next log record into the provided buffer.

**Parameters**:
- `out`: Buffer to store the record payload
- `max_out`: Maximum size of the output buffer
- `out_len`: Pointer to store actual length read (can be NULL)

**Returns**: `true` if a record was read, `false` if end of log or error

**Example**:
```cpp
uint8_t buffer[4096];
uint16_t len;

log_iter_reset();
while (log_iter_next(buffer, sizeof(buffer), &len)) {
  Serial.print("Record length: ");
  Serial.println(len);
  // Process buffer[0..len-1]
}
```

**Notes**:
- Automatically validates header and payload checksums
- Returns `false` on checksum mismatch (corrupted data)
- Iterates from oldest to newest records
- Stops at the first blank page (write tip)

#### `void log_format_range()`
Erases all blocks in the configured range, resetting the log to empty.

**Example**:
```cpp
log_format_range();  // Erase all log data
```

**Warning**: This permanently erases all logged data!

#### `uint32_t log_record_count()`
Returns the total number of records written to the log.

**Returns**: Number of complete records

**Example**:
```cpp
Serial.print("Total records: ");
Serial.println(log_record_count());
```

#### `uint32_t log_next_seq()`
Returns the next sequence number that will be assigned.

**Returns**: Next sequence number

**Example**:
```cpp
uint32_t seq = log_next_seq();
```

## Record Format

Each log record consists of a header followed by the payload:

### LogHdr Structure
```cpp
struct LogHdr {
  uint16_t magic;      // 0xA55A - identifies valid record
  uint16_t length;     // Payload length in bytes
  uint32_t seq;        // Sequence number (monotonically increasing)
  uint16_t crc16;      // CRC16-CCITT of payload
  uint16_t hdr_crc;    // CRC16-CCITT of header (excluding this field)
};
```

Total header size: 12 bytes

## Usage Example

Here's a complete example showing the logging workflow:

```cpp
#include <SPI.h>
#include "nand_driver.h"
#include "nand_log.h"

void setup() {
  Serial.begin(115200);
  
  // Initialize NAND driver
  if (!begin()) {
    Serial.println("NAND init failed!");
    while(1);
  }
  
  // Check chip ID
  uint8_t mfg, dev;
  read_ID(mfg, dev);
  Serial.print("Chip ID: 0x");
  Serial.print(mfg, HEX);
  Serial.print(" 0x");
  Serial.println(dev, HEX);
  
  // Initialize logging over blocks 0-2047
  // Format on first use
  if (!log_begin(0, 2047, true)) {
    Serial.println("Log init failed!");
    while(1);
  }
  
  Serial.println("Logging initialized!");
  Serial.print("Records in log: ");
  Serial.println(log_record_count());
}

void loop() {
  // Example: log temperature reading
  struct TempReading {
    uint32_t timestamp;
    float temperature;
  } __attribute__((packed));
  
  TempReading reading;
  reading.timestamp = millis();
  reading.temperature = 25.5;  // Replace with actual sensor
  
  // Append to log
  if (!log_append((uint8_t*)&reading, sizeof(reading))) {
    Serial.println("Log append failed!");
  }
  
  // Flush periodically or before sleep
  log_flush();
  
  delay(1000);
}

// Function to read back all records
void readAllRecords() {
  uint8_t buffer[4096];
  uint16_t len;
  uint32_t count = 0;
  
  log_iter_reset();
  while (log_iter_next(buffer, sizeof(buffer), &len)) {
    Serial.print("Record #");
    Serial.print(count++);
    Serial.print(" Length: ");
    Serial.println(len);
    // Process record data in buffer
  }
}
```

## Important Considerations

### NAND Flash Constraints
- **One Program Per Page**: A page can only be programmed once between erases
- The library automatically manages this by moving to a new page after each write
- Never call `write_bytes()` multiple times to the same page without erasing

### Sequential Logging Behavior
- The log operates in **append-only mode** with no wrap-around
- When the end block is reached, the log enters a "full" state and stops accepting new writes
- To continue logging, call `log_format_range()` to erase and restart, or reinitialize with a different block range
- This design prevents overwriting old data unintentionally

### Buffer Management
- Internal buffer size: `NAND_MAIN_BYTES - sizeof(LogHdr)` (typically 4084 bytes)
- Data is automatically written when buffer fills
- Always call `log_flush()` before power-down to ensure data persistence

### Bad Block Handling
- The library automatically detects factory-marked bad blocks during `log_begin()`
- Bad blocks are skipped during write operations
- The first spare byte of page 0 is checked (convention: 0xFF = good, other = bad)

### Data Integrity
- All records include CRC16-CCITT checksums for both header and payload
- Corrupted records are automatically detected during iteration
- The library does not perform automatic error correction (ECC is typically handled by the NAND chip itself)

## Troubleshooting

### Log immediately reports full
- Check that blocks are properly erased before first use
- Verify block range parameters in `log_begin()`
- Ensure sufficient good blocks in the range

### Records fail checksum validation
- Check for electrical noise or poor SPI connections
- Verify SPI speed is within chip specifications (default 4 MHz)
- Ensure proper power supply stability

### Cannot write to flash
- Verify chip is not write-protected
- Check that `unlock_all_blocks()` was called
- Ensure block was erased before programming

## Performance Characteristics

- **Write Speed**: Limited by SPI bus speed and NAND program time (~300-500 µs per page)
- **Read Speed**: Limited by SPI bus speed and NAND read time (~25-100 µs per page)
- **Erase Time**: ~3-10 ms per block (chip-dependent)
- **Endurance**: Typically 100,000 erase cycles per block (chip-dependent)

## License

This project is developed for educational and research purposes.

## Version History

- **v1.0**: Initial release with core logging functionality
  - Sequential block allocation
  - CRC16 data integrity
  - Bad block management
  - Iterator-based readback

## Authors

**Tanishk Govil** and **Jane Byun**

Developed for the Watson Research Lab (WRL) at the University of Virginia.