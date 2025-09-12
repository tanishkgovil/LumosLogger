//#include <Arduino.h>

#include "nand_driver.h"
#include "nand_commands.h"
//#include "nand_driver.cpp"

void setup() {
    Serial.begin(115200);
    
    // Critical for nRF52840 - wait for Serial connection
    while (!Serial && millis() < 5000) {
        delay(100);
    }
    delay(1000);
    
    Serial.println("=== NAND TEST STARTING ===");
    Serial.flush(); // Ensure message is sent



    Serial.begin(115200);
    Serial.println("Starting NAND Flash Test...");
    
    // Initialize the NAND flash
    if (begin()) {
        Serial.println("NAND flash initialized successfully");
    } else {
        Serial.println("Failed to initialize NAND flash");
        return;
    }
    
    // Read device ID
    uint8_t id[3];
    read_ID(id);
    Serial.print("Device ID: ");
    for (int i = 0; i < 3; i++) {
        Serial.print("0x");
        Serial.print(id[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    
    // Find where data ends (for resuming interrupted collections)
    Serial.println("Scanning for end of existing data...");
    int result = find_end_of_data();
    if (result) {
        Serial.print("Found end of data at Block: ");
        Serial.print(flashAddr.block);
        Serial.print(", Page: ");
        Serial.print(flashAddr.page);
        Serial.print(", Column: ");
        Serial.println(flashAddr.column);
    } else {
        Serial.println("No existing data found, starting from beginning");
    }
}

void loop() {
    // Test 1: Write and read back data
    testWriteRead();
    
    // Test 2: Test multiple page writes
    testMultipleWrites();
    
    // Test 3: Verify data persistence 
    testDataPersistence();
    
    Serial.println("=== Test cycle complete. Waiting 10 seconds ===\n");
    delay(10000); // Wait 10 seconds between test cycles
}

void testWriteRead() {
    Serial.println("TEST 1: Basic Write/Read Test");
    
    // Write test data
    uint8_t writeData[] = "Hello NAND World! This is a test string.";
    uint8_t dataLen = sizeof(writeData) - 1; // Exclude null terminator
    
    Serial.print("Writing ");
    Serial.print(dataLen);
    Serial.println(" bytes...");
    
    // Record where we're writing
    uint16_t testBlock = flashAddr.block;
    uint8_t testPage = flashAddr.page;
    uint16_t testColumn = flashAddr.column;
    
    writeBytes(writeData, dataLen);
    
    Serial.print("Data written to Block:");
    Serial.print(testBlock);
    Serial.print(" Page:");
    Serial.print(testPage);
    Serial.print(" Column:");
    Serial.println(testColumn);
    
    // Read it back
    uint8_t readBuffer[4352];
    if (readPage(testBlock, testPage, readBuffer)) {
        Serial.println("Read successful. Comparing data...");
        
        // Compare the data
        bool match = true;
        for (int i = 0; i < dataLen; i++) {
            if (readBuffer[testColumn + i] != writeData[i]) {
                match = false;
                Serial.print("Mismatch at byte ");
                Serial.print(i);
                Serial.print(": wrote 0x");
                Serial.print(writeData[i], HEX);
                Serial.print(", read 0x");
                Serial.println(readBuffer[testColumn + i], HEX);
                break;
            }
        }
        
        if (match) {
            Serial.println("✓ DATA MATCH! Write/Read successful");
            Serial.print("Read back: \"");
            for (int i = 0; i < dataLen; i++) {
                Serial.print((char)readBuffer[testColumn + i]);
            }
            Serial.println("\"");
        } else {
            Serial.println("✗ Data verification failed");
        }
    } else {
        Serial.println("✗ Read failed");
    }
    Serial.println();
}

void testMultipleWrites() {
    Serial.println("TEST 2: Multiple Write Test");
    
    for (int i = 0; i < 5; i++) {
        String testStr = "Test write #" + String(i) + " timestamp:" + String(millis());
        Serial.print("Write ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.println(testStr);
        
        writeBytes((uint8_t*)testStr.c_str(), testStr.length());
        delay(100); // Small delay between writes
    }
    Serial.println("Multiple writes complete\n");
}

void testDataPersistence() {
    Serial.println("TEST 3: Data Persistence Test");
    Serial.println("Scanning first few pages for written data...");
    
    uint8_t readBuffer[4352];
    
    for (uint16_t block = 0; block < 2 && block <= flashAddr.block; block++) {
        for (uint8_t page = 0; page < 5 && (block < flashAddr.block || page <= flashAddr.page); page++) {
            if (readPage(block, page, readBuffer)) {
                // Check if page has non-0xFF data
                bool hasData = false;
                for (int i = 0; i < 100; i++) { // Check first 100 bytes
                    if (readBuffer[i] != 0xFF) {
                        hasData = true;
                        break;
                    }
                }
                
                if (hasData) {
                    Serial.print("Block ");
                    Serial.print(block);
                    Serial.print(", Page ");
                    Serial.print(page);
                    Serial.print(" contains data: \"");
                    
                    // Print first readable characters
                    for (int i = 0; i < 50; i++) {
                        if (readBuffer[i] != 0xFF && readBuffer[i] >= 32 && readBuffer[i] <= 126) {
                            Serial.print((char)readBuffer[i]);
                        } else if (readBuffer[i] != 0xFF) {
                            Serial.print(".");
                        } else {
                            break;
                        }
                    }
                    Serial.println("\"");
                }
            }
        }
    }
    Serial.println();
}