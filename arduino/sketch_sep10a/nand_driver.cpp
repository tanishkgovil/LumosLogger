
#include <Arduino.h>
#include <SPI.h>
#include <stddef.h>
#include <stdint.h>
#include "nand_commands.h"
#include "nand_driver.h"

#define CS_PIN 2 // Chip Select pin for the NAND flash

//from chat testing out
#define MAX_BLOCKS      2048
#define PAGES_PER_BLOCK 64
#define PAGE_SIZE       4352
#define DATA_SIZE       4096
#define SPARE_SIZE      256


struct nand_address flashAddr;

bool readPage(uint16_t block, uint8_t page, uint8_t* buffer) {
    // Issue PAGE READ command
    digitalWrite(CS_PIN, LOW);
    SPI.transfer(PAGE_READ); // 0x13
    
    // Send 24-bit address: 7 dummy + 11 block + 6 page
    uint32_t address = (block << 6) | page;
    SPI.transfer((address >> 16) & 0x7F);
    SPI.transfer((address >> 8) & 0xFF);
    SPI.transfer(address & 0xFF);
    digitalWrite(CS_PIN, HIGH);
    
    // Wait for read completion (poll OIP bit)
    // You need to implement this status checking
    delay(1); // Crude delay - should poll status register instead
    
    // Read from cache
    digitalWrite(CS_PIN, LOW);
    SPI.transfer(READ_FROM_CACHE_1); // 0x03
    SPI.transfer(0x00); // Column address high (with 3 dummy bits)
    SPI.transfer(0x00); // Column address low
    SPI.transfer(0x00); // Dummy byte
    
    // Read the data
    for (uint16_t i = 0; i < 4352; i++) {
        buffer[i] = SPI.transfer(0x00);
    }
    digitalWrite(CS_PIN, HIGH);
    
    return true;
}

int find_end_of_data() { //cache read????
    uint8_t buffer[4352];
    bool foundData = false;
    // read blocks/pages/bytes until you find a non-0xFF value. 
    for(uint16_t block = 0; block < 2048; block++){
        for(uint8_t page = 0; page < 64; page++){
            if(!readPage(block, page, buffer)){
                continue;
            }

            bool hasData = false;
            for(uint16_t i = 0; i < 4096; i++){
                if(buffer[i] != 0xFF){
                    hasData = true;
                    foundData = true;
                    break;
                }
            }

            if(!hasData && foundData){
                flashAddr.block = block;
                flashAddr.page = page;
                flashAddr.column = 0;
                return 1;
            }
        }
    }

    if(!foundData){
        flashAddr.block = 0;
        flashAddr.page = 0;
        flashAddr.column = 0;
    }
    // return the block/page/byte where you found it. 
    // if you find nothing, return 0,0,0
    return foundData ? 1: 0; // placeholder
}



// initialize the chip, define chip select pin, start SPI, etc. 
bool begin() {
    pinMode(CS_PIN, OUTPUT);
    digitalWrite(CS_PIN, HIGH); // Should be HIGH to de-select initially

    SPI.begin();
    // This transaction can be started and ended inside specific functions
    // that actually transfer data, which is safer.
    //SPI.beginTransaction(SPISettings(133000000, MSBFIRST, SPI_MODE0));

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
    SPI.transfer(0x00); // ADDED THIS LINE 
    
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
    SPI.transfer(PROGRAM_LOAD_1);
    SPI.transfer((flashAddr.column >> 8) & 0x1F); // upper 5 bits -> 3 dummy and 2 address
    SPI.transfer(flashAddr.column & 0xFF); //lower 8 bits
    for(uint16_t i = 0; i < length; i++){
        SPI.transfer(data[i]);
    }
    digitalWrite(CS_PIN, HIGH);

    digitalWrite(CS_PIN, LOW);
    SPI.transfer(PROGRAM_EXECUTE);

    uint32_t address = (flashAddr.block << 6) | flashAddr.page;
    SPI.transfer((address >> 16) & 0x7F);
    SPI.transfer((address >> 8) & 0xFF);
    SPI.transfer(address & 0xFF);
    digitalWrite(CS_PIN, HIGH);

    flashAddr.column += length;
    if(flashAddr.column >= 4352) {
        flashAddr.column = 0;
        flashAddr.page++;
        if(flashAddr.page >= 64){
            flashAddr.page = 0;
            flashAddr.block++;
        }
    }



    //SPI.transfer(PROGRAM_LOAD_RANDOM_DATA_X1); // Program Load Random Data command
    // SPI.transfer(0b000); // three dummy bits as prescribed in datasheet page 34
    // SPI.transfer(flashAddr.column); // the column
    // SPI.transfer(data, length);   //???
    // // Pull chip select high to deselect the flash chip.
    // digitalWrite(CS_PIN, HIGH);
    // // program execute
    // send_command(WRITE_DISABLE); // Write Disable command
    // // this is a shortcut because I know the buffer is 1024 bytes (1/4 page) and there's a little extra.
    // flashAddr.column = flashAddr.column + length; // increment column by length
    // if (flashAddr.column >= 4096){ // if we hit the end of the page, increment page and reset column
    //     flashAddr.page++;
    //     flashAddr.column = 0;
    // }
    // // write disable 
    // send_command(0x04); // Write Disable command

}




