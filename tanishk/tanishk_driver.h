#ifndef NAND_DRIVER_H
#define NAND_DRIVER_H

#include <stdint.h>
#include <Arduino.h> // Include Arduino here for types like uint8_t

// Define the address struct ONCE here.
struct nand_address {
    unsigned int column : 13;
    unsigned int page   : 6;
    unsigned int block  : 11;
    unsigned int        : 2; //2
};

// struct nand_address {
//   uint16_t column;
//   uint8_t page;
//   uint16_t block;
// };

extern nand_address flashAddr;

// --- Function Declarations ---
bool begin();
// void send_command(uint8_t command);
void reset_chip();
bool read_ID(uint8_t &mfg, uint8_t &dev);
uint8_t get_status();
void write_bytes(const uint8_t* data, uint16_t length);
void send_command(uint8_t command);
void print_status(uint8_t sr);
void unlock_all_blocks();
bool erase_block(uint16_t block);
bool read_bytes(uint8_t* out, uint16_t length);
// bool readPage(uint16_t block, uint8_t page, uint8_t* buffer);
// int find_end_of_data();


// bool waitForReady();
// bool waitForCacheReady();
// uint8_t getStatus();
// bool eraseBlock(uint16_t block);
// void printStatus(uint8_t status);

// / Helper functions for address validation
// inline bool isValidBlock(uint16_t block) {
//     return block < MAX_BLOCKS;
// }

// inline bool isValidPage(uint8_t page) {
//     return page < PAGES_PER_BLOCK;
// }

// inline bool isValidColumn(uint16_t column) {
//     return column < PAGE_SIZE;
// }

// // Helper function to create proper 24-bit address for NAND commands
// inline uint32_t createAddress(uint16_t block, uint8_t page) {
//     // Format: [7 dummy bits][11 block bits][6 page bits]
//     return ((uint32_t)block << 6) | page;
// }

#endif

