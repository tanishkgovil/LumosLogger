#define RESET 0xFF 
#define GET_FEATURE 0x0F 
#define SET_FEATURE 0x1F 
#define READ_ID 0x9F 
#define PAGE_READ 0x13 
#define READ_PAGE_CACHE_RANDOM 0x30 
#define READ_PAGE_CACHE_LAST 0x3F 
#define READ_FROM_CACHE_1 0x03 
#define READ_FROM_CACHE_1_ALT 0x0B 
#define READ_FROM_CACHE_2 0x3B 
#define READ_FROM_CACHE_X4 0x6B 
#define READ_FROM_CACHE_DUAL_IO 0xBB 
#define READ_FROM_CACHE_QUAD_IO 0xEB 
#define WRITE_ENABLE 0x06 
#define WRITE_DISABLE 0x04 
#define BLOCK_ERASE 0xD8 
#define PROGRAM_EXECUTE 0x10 
#define PROGRAM_LOAD_1 0x02 
#define PROGRAM_LOAD_X2 0xA2 
#define PROGRAM_LOAD_RANDOM_DATA_X2 0x44 
#define PROGRAM_LOAD_X4 0x32 
#define PROGRAM_LOAD_RANDOM_DATA_X1 0x84 
#define PROGRAM_LOAD_RANDOM_DATA_X4 0x34 
#define PROTECT 0x2C

// //from chat testing out
// #define STATUS_OIP_BIT      0x01  // Operation In Progress
// #define STATUS_WEL_BIT      0x02  // Write Enable Latch
// #define STATUS_E_FAIL_BIT   0x04  // Erase Fail
// #define STATUS_P_FAIL_BIT   0x08  // Program Fail
// #define STATUS_ECC_MASK     0x70  // ECC Status bits [6:4]
// #define STATUS_CRBSY_BIT    0x80  // Cache Read Busy

// // Feature addresses (from datasheet)
// #define FEATURE_ADDR_STATUS     0xC0
// #define FEATURE_ADDR_CONFIG     0xB0
// #define FEATURE_ADDR_BLOCK_LOCK 0xA0

