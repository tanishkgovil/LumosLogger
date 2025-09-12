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
extern struct nand_address flashAddr;

// --- Function Declarations ---
bool begin();
void send_command(uint8_t command);
void reset_chip();
void read_ID(uint8_t* data);
void writeBytes(const uint8_t* data, uint16_t length);
bool readPage(uint16_t block, uint8_t page, uint8_t* buffer);
int find_end_of_data();

#endif