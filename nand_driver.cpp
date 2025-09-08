
#include <Arduino.h>
#include <SPI.h>
#include <stddef.h>
#include <stdint.h>
#include "nand_commands.h"
#include "nand_driver.h"

#define CS_PIN 10 // Chip Select pin for the NAND flash

// A new Lumos board will have a fresh NAND chip (assume 0 or nothing written yet))
// Most of the time, we will write with a fresh one.
// However, if we have an interrupted data collection process, we need to resume writing from where we left off. 
// We initialize a new chip to 0. One that isn't new will have valid data. 
// Although copilot seems to suggest a bunch of things that won't work, you have to just read and find data. 
// Otherwise you need more hardware. 

struct nand_address {
    unsigned int column : 13; // bits 12:0
    unsigned int page   : 6;  // bits 18:13
    unsigned int block  : 11; // bits 28:19
    unsigned int        : 2;  // unused, pad to 32 bits
};

struct nand_address flashAddr;


int find_end_of_data() {
    // read blocks/pages/bytes until you find a non-0xFF value. 
    // return the block/page/byte where you found it. 
    // if you find nothing, return 0,0,0
    return 0; // placeholder
}


// initialize the chip, define chip select pin, start SPI, etc. 
bool begin() {
    pinMode(CS_PIN, OUTPUT);
    digitalWrite(CS_PIN, HIGH); // Should be HIGH to de-select initially

    SPI.begin();
    // This transaction can be started and ended inside specific functions
    // that actually transfer data, which is safer.
    // SPI.beginTransaction(SPISettings(133000000, MSBFIRST, SPI_MODE0));

    reset_chip();
    return true;
}

void send_command(uint8_t command) {
    // Pull chip select low to select the flash chip.
    digitalWrite(CS_PIN, LOW);
    
    // Send the command
    SPI.transfer(command);
    
    // Pull chip select high to deselect the flash chip.
    digitalWrite(CS_PIN, HIGH);
}

void reset_chip() {
    digitalWrite(CS_PIN, LOW);
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0)); // Use a safe, slow speed for commands
    SPI.transfer(RESET);
    SPI.endTransaction();
    digitalWrite(CS_PIN, HIGH);
    delayMicroseconds(625);
}

void read_ID(uint8_t* data) {
    // Pull chip select low to select the flash chip.
    digitalWrite(CS_PIN, LOW);
    
    // Send the Read ID command
    SPI.transfer(0x9F); // Read ID command
    
    // Read the ID bytes
    for (int i = 0; i < 3; i++) {
        data[i] = SPI.transfer(0x00); // Dummy byte to read data
    }
    
    // Pull chip select high to deselect the flash chip.
    digitalWrite(CS_PIN, HIGH);
    
}

void writeBytes(const uint8_t* data, uint16_t length) {
    // write enable
    send_command(WRITE_ENABLE); // Write Enable command
    
    // program load random data

    // Pull chip select low to select the flash chip.
    digitalWrite(CS_PIN, LOW);
    SPI.transfer(PROGRAM_LOAD_RANDOM_DATA_X1); // Program Load Random Data command
    SPI.transfer(0b000); // three dummy bits as prescribed in datasheet page 34
    SPI.transfer(flashAddr.column); // the column
    SPI.transfer(data, length);
    // Pull chip select high to deselect the flash chip.
    digitalWrite(CS_PIN, HIGH);
    // program execute
    send_command(WRITE_DISABLE); // Write Disable command
    // this is a shortcut because I know the buffer is 1024 bytes (1/4 page) and there's a little extra.
    flashAddr.column = flashAddr.column + length; // increment column by length
    if (flashAddr.column >= 4096){ // if we hit the end of the page, increment page and reset column
        flashAddr.page++;
        flashAddr.column = 0;
    }
    // write disable 
    send_command(0x04); // Write Disable command

}



