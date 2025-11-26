#include "nand_driver.h"
#include "nand_commands.h"
#include <SPI.h>

#define CS_PIN 10
// void setup() {
//     Serial.begin(115200);
//     while (!Serial && millis() < 5000) delay(100);
//     delay(1000);
    
//     Serial.println("=== RAW SPI TEST ===");
    
//     pinMode(10, OUTPUT); // CS pin
//     digitalWrite(10, HIGH);
    
//     // Try different SPI configurations
//     SPI.begin();
    
//     Serial.println("Testing SPI communication...");
    
//     // Test 1: Default SPI settings
//     digitalWrite(10, LOW);
//     SPI.transfer(0x9F); // READ_ID command
//     SPI.transfer(0x00); // Dummy byte
//     uint8_t id1 = SPI.transfer(0x00);
//     uint8_t id2 = SPI.transfer(0x00);
//     uint8_t id3 = SPI.transfer(0x00);
//     digitalWrite(10, HIGH);
    
//     Serial.print("Default SPI: ");
//     Serial.print(id1, HEX);
//     Serial.print(" ");
//     Serial.print(id2, HEX);
//     Serial.print(" ");
//     Serial.println(id3, HEX);
    
//     // Test 2: Different SPI mode
//     SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE3));
//     digitalWrite(10, LOW);
//     SPI.transfer(0x9F);
//     SPI.transfer(0x00);
//     id1 = SPI.transfer(0x00);
//     id2 = SPI.transfer(0x00);
//     id3 = SPI.transfer(0x00);
//     digitalWrite(10, HIGH);
//     SPI.endTransaction();
    
//     Serial.print("Mode 3: ");
//     Serial.print(id1, HEX);
//     Serial.print(" ");
//     Serial.print(id2, HEX);
//     Serial.print(" ");
//     Serial.println(id3, HEX);
// }

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 5000) delay(100);
    delay(1000);
    
    Serial.println("=== NAND CONNECTION TEST ===");
    
    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH); // Deselect NAND
    
    SPI.begin();
    delay(100); // Let NAND stabilize
    
    // Test 1: What do we read with NAND connected but not selected?
    uint8_t idle_read = SPI.transfer(0xAA);
    Serial.print("Idle SPI read (CS high): 0x");
    Serial.println(idle_read, HEX);
    
    // Test 2: Select NAND and try to read
    digitalWrite(10, LOW); // Select NAND
    delayMicroseconds(1);
    uint8_t selected_read = SPI.transfer(0x9F); // READ_ID command
    Serial.print("Response to READ_ID command: 0x");
    Serial.println(selected_read, HEX);
    
    // Try to read ID bytes
    uint8_t dummy = SPI.transfer(0x00);
    uint8_t id1 = SPI.transfer(0x00);
    uint8_t id2 = SPI.transfer(0x00);
    digitalWrite(10, HIGH); // Deselect
    
    Serial.print("Dummy: 0x");
    Serial.print(dummy, HEX);
    Serial.print(", ID1: 0x");
    Serial.print(id1, HEX);
    Serial.print(", ID2: 0x");
    Serial.println(id2, HEX);
}
void loop() {
    // Empty
}