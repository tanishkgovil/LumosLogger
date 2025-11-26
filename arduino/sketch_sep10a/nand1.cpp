// #include <Arduino.h>
// #include <SPI.h>
// #include "nand_commands.h"
// #include "nand_driver.h"


// struct nand_address flashAddr = {0, 0, 0};

// // Check device status register
// bool waitForReady() {
//     uint8_t status;
//     int timeout = 0;
    
//     do {
//         digitalWrite(CS_PIN, LOW);
//         SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
//         SPI.transfer(GET_FEATURE);
//         SPI.transfer(FEATURE_ADDR_STATUS);
//         status = SPI.transfer(0x00);
//         SPI.endTransaction();
//         digitalWrite(CS_PIN, HIGH);
        
//         delayMicroseconds(10);
//         timeout++;
        
//         if (timeout > MAX_WAIT_CYCLES) {
//             Serial.println("Timeout waiting for device ready");
//             return false;
//         }
//     } while (status & STATUS_OIP_BIT); // Wait while OIP bit is 1 (busy)
    
//     return true;
// }

// // Check if device is ready for cache operations
// bool waitForCacheReady() {
//     uint8_t status;
//     int timeout = 0;
    
//     do {
//         digitalWrite(CS_PIN, LOW);
//         SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
//         SPI.transfer(GET_FEATURE);
//         SPI.transfer(FEATURE_ADDR_STATUS);
//         status = SPI.transfer(0x00);
//         SPI.endTransaction();
//         digitalWrite(CS_PIN, HIGH);
        
//         delayMicroseconds(10);
//         timeout++;
        
//         if (timeout > MAX_WAIT_CYCLES) {
//             return false;
//         }
//     } while ((status & STATUS_OIP_BIT) || (status & 0x80)); // Wait for OIP=0 and CRBSY=0
    
//     return true;
// }

// bool readPage(uint16_t block, uint8_t page, uint8_t* buffer) {
//     // Validate parameters according to datasheet
//     if (block >= 2048 || page >= 64) {
//         Serial.println("Invalid block/page address");
//         return false;
//     }
    
//     // Issue PAGE READ command (13h)
//     digitalWrite(CS_PIN, LOW);
//     SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
//     SPI.transfer(PAGE_READ); // 0x13
    
//     // Send 24-bit address: 7 dummy bits + 11 block bits + 6 page bits
//     // Address format from datasheet: [7 dummy][10:0 block][5:0 page]
//     uint32_t address = ((uint32_t)block << 6) | page;
//     SPI.transfer((address >> 16) & 0x7F); // 7 dummy bits + upper block bits
//     SPI.transfer((address >> 8) & 0xFF);
//     SPI.transfer(address & 0xFF);
//     SPI.endTransaction();
//     digitalWrite(CS_PIN, HIGH);
    
//     // Wait for page read completion
//     if (!waitForReady()) {
//         Serial.println("Page read timeout");
//         return false;
//     }
    
//     // Check for ECC errors or read failures
//     uint8_t status;
//     digitalWrite(CS_PIN, LOW);
//     SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
//     SPI.transfer(GET_FEATURE);
//     SPI.transfer(FEATURE_ADDR_STATUS);
//     status = SPI.transfer(0x00);
//     SPI.endTransaction();
//     digitalWrite(CS_PIN, HIGH);
    
//     // Check ECC status bits [6:4] according to datasheet
//     uint8_t ecc_status = (status >> 4) & 0x07;
//     if (ecc_status == 0x02) {
//         Serial.println("Uncorrectable ECC error detected");
//         // Continue anyway to return the data
//     } else if (ecc_status != 0x00) {
//         Serial.print("ECC corrected errors detected, status: ");
//         Serial.println(ecc_status, BIN);
//     }
    
//     // Read from cache using 03h command
//     digitalWrite(CS_PIN, LOW);
//     SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
//     SPI.transfer(READ_FROM_CACHE_1); // 0x03
//     SPI.transfer(0x00); // Column address high (3 dummy bits + upper column bits)
//     SPI.transfer(0x00); // Column address low
//     SPI.transfer(0x00); // Dummy byte
    
//     // Read 4352 bytes (4096 data + 256 spare)
//     for (uint16_t i = 0; i < 4352; i++) {
//         buffer[i] = SPI.transfer(0x00);
//     }
//     SPI.endTransaction();
//     digitalWrite(CS_PIN, HIGH);
    
//     return true;
// }

// int find_end_of_data() {
//     uint8_t buffer[4352];
//     bool foundData = false;
    
//     Serial.println("Scanning for end of data (this may take a while)...");
    
//     // Scan blocks looking for data
//     for(uint16_t block = 0; block < 2048; block++){
//         for(uint8_t page = 0; page < 64; page++){
//             if(!readPage(block, page, buffer)){
//                 Serial.print("Failed to read block ");
//                 Serial.print(block);
//                 Serial.print(" page ");
//                 Serial.println(page);
//                 continue;
//             }

//             bool hasData = false;
//             // Check only the data area (first 4096 bytes)
//             for(uint16_t i = 0; i < 4096; i++){
//                 if(buffer[i] != 0xFF){
//                     hasData = true;
//                     foundData = true;
//                     break;
//                 }
//             }

//             // If this page has no data but we found data before, this is the write position
//             if(!hasData && foundData){
//                 flashAddr.block = block;
//                 flashAddr.page = page;
//                 flashAddr.column = 0;
//                 Serial.print("End of data found at block ");
//                 Serial.print(block);
//                 Serial.print(", page ");
//                 Serial.println(page);
//                 return 1;
//             }
//         }
        
//         // Progress indicator every 100 blocks
//         if (block % 100 == 0 && block > 0) {
//             Serial.print("Scanned ");
//             Serial.print(block);
//             Serial.println(" blocks...");
//         }
//     }

//     if(!foundData){
//         flashAddr.block = 0;
//         flashAddr.page = 0;
//         flashAddr.column = 0;
//         Serial.println("No data found, starting at beginning");
//     } else {
//         // If we scanned everything and found data, we're at the end
//         flashAddr.block = 2047;
//         flashAddr.page = 63;
//         flashAddr.column = 4096;
//     }
    
//     return foundData ? 1 : 0;
// }

// bool begin() {
//     pinMode(CS_PIN, OUTPUT);
//     digitalWrite(CS_PIN, HIGH);

//     SPI.begin();
//     delay(10); // Power-up delay
    
//     // Reset the chip
//     reset_chip();
//     delay(10);
    
//     // Verify chip is responding by reading ID
//     uint8_t id[3];
//     read_ID(id);
    
//     Serial.print("Device ID: ");
//     for (int i = 0; i < 3; i++) {
//         Serial.print("0x");
//         Serial.print(id[i], HEX);
//         Serial.print(" ");
//     }
//     Serial.println();
    
//     // Check for expected Micron ID (0x2C according to datasheet)
//     if (id[0] != 0x2C) {
//         Serial.print("Warning: Unexpected manufacturer ID: 0x");
//         Serial.println(id[0], HEX);
//         return false;
//     }
    
//     // Check device ID (should be 0x34 for MT29F4G01ABAFDWB)
//     if (id[1] != 0x34) {
//         Serial.print("Warning: Unexpected device ID: 0x");
//         Serial.println(id[1], HEX);
//     }
    
//     // Check initial status
//     uint8_t status;
//     digitalWrite(CS_PIN, LOW);
//     SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
//     SPI.transfer(GET_FEATURE);
//     SPI.transfer(FEATURE_ADDR_STATUS);
//     status = SPI.transfer(0x00);
//     SPI.endTransaction();
//     digitalWrite(CS_PIN, HIGH);
    
//     Serial.print("Initial status register: 0x");
//     Serial.println(status, HEX);
    
//     return true;
// }

// void reset_chip() {
//     digitalWrite(CS_PIN, LOW);
//     SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
//     SPI.transfer(RESET);
//     SPI.endTransaction();
//     digitalWrite(CS_PIN, HIGH);
//     delay(2); // Reset requires ~1.25ms according to datasheet
// }

// void read_ID(uint8_t* data) {
//     digitalWrite(CS_PIN, LOW);
//     SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
    
//     SPI.transfer(READ_ID); // 0x9F
//     SPI.transfer(0x00);    // Dummy byte
    
//     // Read manufacturer ID and device ID
//     for (int i = 0; i < 3; i++) {
//         data[i] = SPI.transfer(0x00);
//     }
    
//     SPI.endTransaction();
//     digitalWrite(CS_PIN, HIGH);
// }

// bool writeBytes(const uint8_t* data, uint16_t length) {
//     // Validate write doesn't cross page boundaries
//     if (flashAddr.column + length > 4096) {
//         Serial.println("Error: Write would cross page boundary");
//         return false;
//     }
    
//     // Validate block/page addresses
//     if (flashAddr.block >= 2048 || flashAddr.page >= 64) {
//         Serial.println("Error: Invalid write address");
//         return false;
//     }
    
//     // Write enable command
//     digitalWrite(CS_PIN, LOW);
//     SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
//     SPI.transfer(WRITE_ENABLE);
//     SPI.endTransaction();
//     digitalWrite(CS_PIN, HIGH);
    
//     // Program load command (02h)
//     digitalWrite(CS_PIN, LOW);
//     SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
//     SPI.transfer(PROGRAM_LOAD_1);
//     SPI.transfer((flashAddr.column >> 8) & 0x1F); // Upper 5 bits (3 dummy + 2 address)
//     SPI.transfer(flashAddr.column & 0xFF);        // Lower 8 bits
    
//     for(uint16_t i = 0; i < length; i++){
//         SPI.transfer(data[i]);
//     }
//     SPI.endTransaction();
//     digitalWrite(CS_PIN, HIGH);

//     // Program execute command (10h)
//     digitalWrite(CS_PIN, LOW);
//     SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
//     SPI.transfer(PROGRAM_EXECUTE);
    
//     // Send 24-bit address
//     uint32_t address = ((uint32_t)flashAddr.block << 6) | flashAddr.page;
//     SPI.transfer((address >> 16) & 0x7F);
//     SPI.transfer((address >> 8) & 0xFF);
//     SPI.transfer(address & 0xFF);
//     SPI.endTransaction();
//     digitalWrite(CS_PIN, HIGH);

//     // Wait for program completion
//     if (!waitForReady()) {
//         Serial.println("Program timeout");
//         return false;
//     }
    
//     // Check program status
//     uint8_t status;
//     digitalWrite(CS_PIN, LOW);
//     SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
//     SPI.transfer(GET_FEATURE);
//     SPI.transfer(FEATURE_ADDR_STATUS);
//     status = SPI.transfer(0x00);
//     SPI.endTransaction();
//     digitalWrite(CS_PIN, HIGH);
    
//     if (status & STATUS_P_FAIL_BIT) {
//         Serial.println("Program operation failed");
//         return false;
//     }

//     // Update address for next write
//     flashAddr.column += length;
//     if(flashAddr.column >= 4096) { // Stay within data area
//         flashAddr.column = 0;
//         flashAddr.page++;
//         if(flashAddr.page >= 64){
//             flashAddr.page = 0;
//             flashAddr.block++;
//             if(flashAddr.block >= 2048) {
//                 Serial.println("Warning: Reached end of flash memory");
//                 return false;
//             }
//         }
//     }
    
//     return true;
// }

// // Add these helper functions to your nand_driver.cpp file:

// uint8_t getStatus() {
//     uint8_t status;
//     digitalWrite(CS_PIN, LOW);
//     SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
//     SPI.transfer(GET_FEATURE);
//     SPI.transfer(FEATURE_ADDR_STATUS);
//     status = SPI.transfer(0x00);
//     SPI.endTransaction();
//     digitalWrite(CS_PIN, HIGH);
//     return status;
// }

// void printStatus(uint8_t status) {
//     Serial.println("Status bits:");
//     Serial.print("  OIP (Operation In Progress): ");
//     Serial.println((status & STATUS_OIP_BIT) ? "BUSY" : "READY");
//     Serial.print("  WEL (Write Enable Latch): ");
//     Serial.println((status & STATUS_WEL_BIT) ? "ENABLED" : "DISABLED");
//     Serial.print("  E_FAIL (Erase Fail): ");
//     Serial.println((status & STATUS_E_FAIL_BIT) ? "FAILED" : "OK");
//     Serial.print("  P_FAIL (Program Fail): ");
//     Serial.println((status & STATUS_P_FAIL_BIT) ? "FAILED" : "OK");
//     Serial.print("  CRBSY (Cache Read Busy): ");
//     Serial.println((status & STATUS_CRBSY_BIT) ? "BUSY" : "READY");
    
//     uint8_t ecc_status = (status & STATUS_ECC_MASK) >> 4;
//     Serial.print("  ECC Status: ");
//     switch(ecc_status) {
//         case 0: Serial.println("No errors"); break;
//         case 1: Serial.println("1-3 bit errors corrected"); break;
//         case 2: Serial.println("Uncorrectable error"); break;
//         case 3: Serial.println("4-6 bit errors corrected"); break;
//         case 5: Serial.println("7-8 bit errors corrected"); break;
//         default: Serial.print("Reserved ("); Serial.print(ecc_status); Serial.println(")"); break;
//     }
// }