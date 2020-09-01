#include "unity.h"
#include <string.h>

#include "cbuf.h"

void setUp(void)
{
}

void tearDown(void)
{
}

//
void test_cbuf_init_fail(void) {
    uint8_t ret;
    cbuf_t cb;
    uint8_t buffer[10];
    uint64_t size = 8;

    ret = cbuf_init(NULL, buffer, size);
    TEST_ASSERT_EQUAL(0, ret);

    ret = cbuf_init(&cb, NULL, size);
    TEST_ASSERT_EQUAL(0, ret);

    ret = cbuf_init(&cb, buffer, 0);
    TEST_ASSERT_EQUAL(0, ret);
}

//
void test_cbuf_init_success(void) {
    uint8_t ret;
    cbuf_t cb;
    uint8_t buffer[10];
    uint64_t size = 10;

    ret = cbuf_init(&cb, buffer, size);
    TEST_ASSERT_EQUAL(ret, 1);
    TEST_ASSERT_EQUAL_PTR(cb.bufPtr, buffer);
    TEST_ASSERT_EQUAL(cb.size, size);
    TEST_ASSERT_EQUAL(cb.writePos, 0);
    TEST_ASSERT_EQUAL(cb.readPos, 0);
}

//
void test_cbuf_reset(void) {
    cbuf_t cb;
    uint8_t buffer[10];
    uint64_t size = 10;

    TEST_ASSERT_EQUAL(1, cbuf_init(&cb, buffer, size));
    cb.writePos = 5;
    cb.readPos = 9;
    TEST_ASSERT_EQUAL(1, cbuf_reset(&cb));
    TEST_ASSERT_EQUAL(cb.writePos, 0);
    TEST_ASSERT_EQUAL(cb.readPos, 0);

    TEST_ASSERT_EQUAL(0, cbuf_reset(NULL));
}

//
void test_cbuf_is_full(void) {
    cbuf_t cb;
    uint8_t buffer[10];
    uint64_t size = sizeof(buffer);

    TEST_ASSERT_EQUAL(1, cbuf_init(&cb, buffer, size));
    TEST_ASSERT_EQUAL(false, cbuf_is_full(&cb));

    cb.writePos = 9; // [r - x - x - x - x - x - x - x - x - w]
    TEST_ASSERT_EQUAL(true, cbuf_is_full(&cb));

    cb.readPos = 9; // [o - o - o - o - o - o - o - o - o - w/r]
    TEST_ASSERT_EQUAL(false, cbuf_is_full(&cb));

    cb.writePos = 3;
    cb.readPos = 4;
    TEST_ASSERT_EQUAL(true, cbuf_is_full(&cb));
}

//
void test_cbuf_is_empty(void) {
    cbuf_t cb;
    uint8_t buffer[10];
    uint64_t size = sizeof(buffer);

    TEST_ASSERT_EQUAL(1, cbuf_init(&cb, buffer, size));
    TEST_ASSERT_EQUAL(true, cbuf_is_empty(&cb));

    cb.writePos = cb.readPos = 3;
    TEST_ASSERT_EQUAL(true, cbuf_is_empty(&cb));

    cb.writePos = cb.readPos = 9;
    TEST_ASSERT_EQUAL(true, cbuf_is_empty(&cb));
}

//
void test_cbuf_get_filled_free(void) {
    cbuf_t cb;
    uint8_t buffer[10];
    uint64_t size = sizeof(buffer);

    TEST_ASSERT_EQUAL(1, cbuf_init(&cb, buffer, size));
    TEST_ASSERT_EQUAL(0, cbuf_get_filled(&cb));
    TEST_ASSERT_EQUAL(size - 1, cbuf_get_free(&cb)); // 9
    
    cb.writePos = cb.readPos = 9;
    TEST_ASSERT_EQUAL(0, cbuf_get_filled(&cb));
    TEST_ASSERT_EQUAL(size - 1, cbuf_get_free(&cb)); // 9

    // [o - o - o - o - r - x - x - x - x - w]
    cb.writePos = 9;
    cb.readPos = 4;
    TEST_ASSERT_EQUAL(5, cbuf_get_filled(&cb));
    TEST_ASSERT_EQUAL(4, cbuf_get_free(&cb));

    // [x - x - x - w - o - o - o - r - x - x]
    cb.readPos = 7;
    cb.writePos = 3;
    TEST_ASSERT_EQUAL(6, cbuf_get_filled(&cb));
    TEST_ASSERT_EQUAL(3, cbuf_get_free(&cb));

    // [r - x - x - x - x - x - x - x - x - w]
    cb.writePos = 9;
    cb.readPos = 0;
    TEST_ASSERT_EQUAL(9, cbuf_get_filled(&cb));
    TEST_ASSERT_EQUAL(0, cbuf_get_free(&cb));
}

//
void test_cbuf_single_write_read(void) {
    cbuf_t cb;
#define DATA_SIZE 10
    uint8_t const data[DATA_SIZE] = {3, 4, 9, 8, 7, 5, 2, 1, 33, 99}; // Last element won't be used
    uint8_t buffer[DATA_SIZE] = {0};
    uint8_t readBuffer[DATA_SIZE] = {0};
    uint64_t size = sizeof(buffer);

    TEST_ASSERT_EQUAL(1, cbuf_init(&cb, buffer, size));

    // Currently in buffer: [r/w - o - o - o - o - o - o - o - o - o]
    uint8_t count = 0;
    for (uint8_t i = 0; i < size + 10; i++) {
        count += cbuf_write_single(&cb, data[i]); // Only (size - 1) bytes are written
    }
    TEST_ASSERT_EQUAL(size - 1, count);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(cb.bufPtr, data, sizeof(data) - 1);
    TEST_ASSERT_EQUAL(size - 1, cb.writePos); // [r/3 - 4 - 9 - 8 - 7 - 5 - 2 - 1 - 33 - w]
    TEST_ASSERT_EQUAL(0, cb.bufPtr[cb.writePos]);
    TEST_ASSERT_EQUAL(data[0], cb.bufPtr[cb.readPos]);

    // Currently in buffer: [r/3 - 4 - 9 - 8 - 7 - 5 - 2 - 1 - 33 - w]
    count = 0;
    for (uint8_t i = 0; i < size + 10; i++) {
        count += cbuf_read_single(&cb, &readBuffer[i]); // (size - 1) bytes are read
    }

    TEST_ASSERT_EQUAL(size - 1, count); // [o - o - o - o - o - o - o - o - o - r/w]
    TEST_ASSERT_EQUAL(0, cb.bufPtr[cb.readPos]); // nothing at readPos
    TEST_ASSERT_EQUAL(cb.readPos, size - 1);
    TEST_ASSERT_EQUAL(cb.readPos, cb.writePos);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(readBuffer, data, sizeof(data) - 1);

    // Currently in buffer: [o - o - o - o - o - o - o - o - o - r/w]
    uint8_t const data2[DATA_SIZE] = {51, 21, 11, 13, 17, 19, 23, 25, 27, 29};
    count = 0;
    for (uint8_t i = 0; i < size + 10; i++) {
        count += cbuf_write_single(&cb, data2[i]); // Only (size - 1) bytes are written
    }
    TEST_ASSERT_EQUAL(size - 1, count);
    TEST_ASSERT_EQUAL(8, cb.writePos); // [21 - 11 - 13 - 17 - 19 - 23 - 25 - 27 - w - r/51]
    TEST_ASSERT_EQUAL(data2[0], cb.bufPtr[size - 1]); // First element written at last position
    TEST_ASSERT_EQUAL_UINT8_ARRAY(&data2[1], &cb.bufPtr[0], size - 2);
    TEST_ASSERT_EQUAL(33, cb.bufPtr[cb.writePos]); // 33 was garbage left from data[], not overwritten yet

    // Currently in buffer: [21 - 11 - 13 - 17 - 19 - 23 - 25 - 27 - w - r/51]
    count = 0;
    for (uint8_t i = 0; i < size + 10; i++) {
        count += cbuf_read_single(&cb, &readBuffer[i]); // (size - 1) bytes are read
    }
    TEST_ASSERT_EQUAL(size - 1, count); // [21 - 11 - 13 - 17 - 19 - 23 - 25 - 27 - r/w - o]
    TEST_ASSERT_EQUAL(cb.readPos, cb.writePos);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(readBuffer, data2, sizeof(data2) - 1);

#undef DATA_SIZE
}

//
void test_cbuf_write(void) {
#define DATA_SIZE 10
    cbuf_t cb;
    uint8_t const mockData[DATA_SIZE] = {32, 50, 81, 60, 48, 58, 29, 13, 48, 57};
    uint8_t buffer[DATA_SIZE];
    memset(buffer, 0xFF, DATA_SIZE);
    uint64_t writeCount = 0;
    uint64_t numBytes, freeBytes, filledBytes = 0;

    TEST_ASSERT_EQUAL(1, cbuf_init(&cb, buffer, DATA_SIZE));
    // [w/r - o - o - o - o - o - o - o - o - o]
    
    // Write 0 bytes -> returns 0
    TEST_ASSERT_EQUAL(0, cbuf_write(&cb, mockData, 0));

    // cbuffer full -> returns 0
    cb.writePos = DATA_SIZE - 1; // [r - o - o - o - o - o - o - o - o - w]
    TEST_ASSERT_EQUAL(0, cbuf_get_free(&cb));
    TEST_ASSERT_EQUAL(0, cbuf_write(&cb, mockData, 5));

    // writePos == readPos, write till end (full)
    cbuf_reset(&cb);
    writeCount = cbuf_write(&cb, mockData, DATA_SIZE); // [r/32 - 50 - 81 - 60 - 48 - 58 - 29 - 13 - 48 - w]
    TEST_ASSERT_EQUAL(writeCount, DATA_SIZE - 1);
    TEST_ASSERT_EQUAL(DATA_SIZE - 1, cb.writePos);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(mockData, buffer, DATA_SIZE - 1);
    TEST_ASSERT_EQUAL_UINT8(0xFF, buffer[DATA_SIZE - 1]); // last element in mockData is not copied

    // writePos == readPos, write before end (not full)
    cbuf_reset(&cb);
    memset(buffer, 0xFF, sizeof(buffer));
    numBytes = 3;
    writeCount = cbuf_write(&cb, mockData, numBytes); // [r/32 - 50 - 81 - w - o - o - o - o - o - o]
    TEST_ASSERT_EQUAL(writeCount, numBytes);
    TEST_ASSERT_EQUAL(numBytes, cb.writePos);
    TEST_ASSERT_EQUAL(numBytes, cbuf_get_filled(&cb));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(mockData, buffer, numBytes);
    for (uint8_t i = numBytes; i < DATA_SIZE; i++) {
        TEST_ASSERT_EQUAL(0xFF, cb.bufPtr[i]);
    }

    // writePos > readPos, write till end (full)
    cbuf_reset(&cb);
    memset(buffer, 0xFF, sizeof(buffer));
    cb.writePos = 5; // [r - x - x - x - x - w - o - o - o - o]
    freeBytes = cbuf_get_free(&cb);
    filledBytes = cbuf_get_filled(&cb);
    TEST_ASSERT_EQUAL(4, freeBytes);
    TEST_ASSERT_EQUAL(5, filledBytes);
    writeCount = cbuf_write(&cb, mockData, DATA_SIZE); // [r - x - x - x - x - 32 - 50 - 81 - 60 - w]
    TEST_ASSERT_EQUAL(freeBytes, writeCount);
    TEST_ASSERT_EQUAL(filledBytes + writeCount, cbuf_get_filled(&cb)); // 5 old bytes, 4 new bytes
    TEST_ASSERT_EQUAL_UINT8_ARRAY(mockData, &cb.bufPtr[5], writeCount); // slot at end of cbuffer is not used
    TEST_ASSERT_EQUAL(0xFF, cb.bufPtr[cb.readPos]); // slot at beginning is not overwritten

    // writePos > readPos, write before end (not full)
    cbuf_reset(&cb);
    memset(buffer, 0xFF, sizeof(buffer));
    cb.writePos = 1; // [r - w - o - o - o - o - o - o - o - o]
    numBytes = 3;
    writeCount = cbuf_write(&cb, mockData, numBytes); // [r - 32 - 50 - 81 - w - o - o - o - o - o]
    TEST_ASSERT_EQUAL(1 + numBytes, cbuf_get_filled(&cb)); // 3 new bytes + 1 byte at readPos
    TEST_ASSERT_EQUAL(1 + numBytes, cb.writePos);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(mockData, &buffer[1], numBytes);
    for (uint8_t i = cb.writePos; i < DATA_SIZE; i++) {
        TEST_ASSERT_EQUAL(0xFF, cb.bufPtr[i]);
    }
    TEST_ASSERT_EQUAL(0xFF, cb.bufPtr[cb.readPos]); // slot at beginning is not overwritten

    // writePos == readPos, write till end then wrap-around
    cbuf_reset(&cb);
    memset(buffer, 0xFF, sizeof(buffer));
    cb.writePos = cb.readPos = 4; // [o - o - o - o - r/w - o - o - o - o - o]
    numBytes = 7;
    writeCount = cbuf_write(&cb, mockData, numBytes); // [29 - w - o - o - r/32 - 50 - 81 - 60 - 48 - 58]
    TEST_ASSERT_EQUAL(numBytes, writeCount);
    TEST_ASSERT_EQUAL(numBytes, cbuf_get_filled(&cb));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(mockData, &cb.bufPtr[4], 6);
    TEST_ASSERT_EQUAL(29, cb.bufPtr[0]); // element written at beginning, after wrap-around
    TEST_ASSERT_EQUAL((4 + numBytes) % cb.size, cb.writePos);

    // writePos > readPos, write till end then wrap-around
    cbuf_reset(&cb);
    memset(buffer, 0xFF, sizeof(buffer));
    cb.writePos = 5;
    cb.readPos = 4; // [o - o - o - o - r - w - o - o - o - o]
    numBytes = 7;
    writeCount = cbuf_write(&cb, mockData, numBytes); // [58 - 29 - w - o - r - 32 - 50 - 81 - 60 - 48]
    TEST_ASSERT_EQUAL(numBytes, writeCount);
    TEST_ASSERT_EQUAL(numBytes + 1, cbuf_get_filled(&cb)); // including one byte at readPos
    TEST_ASSERT_EQUAL_UINT8_ARRAY(mockData, &cb.bufPtr[5], 5);
    TEST_ASSERT_EQUAL(58, cb.bufPtr[0]);
    TEST_ASSERT_EQUAL(29, cb.bufPtr[1]);
    TEST_ASSERT_EQUAL((5 + numBytes) % cb.size, cb.writePos);

    // writePos < readPos
    cbuf_reset(&cb);
    memset(buffer, 0xFF, sizeof(buffer));
    cb.writePos = 2;
    cb.readPos = 5; // [x - x - w - o - o - r - x - x - x - x]
    filledBytes = cbuf_get_filled(&cb);
    freeBytes = cbuf_get_free(&cb);
    TEST_ASSERT_EQUAL(7, filledBytes);
    TEST_ASSERT_EQUAL(2, freeBytes);
    numBytes = 10;
    writeCount = cbuf_write(&cb, mockData, numBytes); // [x - x - 32 - 50 - w - r - x - x - x - x]
    numBytes = (numBytes < freeBytes) ? numBytes : freeBytes;
    TEST_ASSERT_EQUAL(freeBytes, writeCount);
    TEST_ASSERT_EQUAL(4, cb.writePos);
    TEST_ASSERT_EQUAL(filledBytes + writeCount, cbuf_get_filled(&cb));
    TEST_ASSERT_EQUAL(9, cbuf_get_filled(&cb));
    TEST_ASSERT_EQUAL(32, cb.bufPtr[2]);
    TEST_ASSERT_EQUAL(50, cb.bufPtr[3]);
#undef DATA_SIZE
}

//
void test_cbuf_read(void) {
#define DATA_SIZE 10
    cbuf_t cb;
    uint8_t const mockData[DATA_SIZE] = {32, 50, 81, 60, 48, 58, 29, 13, 48, 57};
    uint8_t readBuffer[DATA_SIZE];
    uint8_t buffer[DATA_SIZE];
    memset(buffer, 0xFF, DATA_SIZE);
    memset(readBuffer, 0, DATA_SIZE);
    uint64_t readCount = 0;
    uint64_t numBytes, freeBytes, filledBytes = 0;

    TEST_ASSERT_EQUAL(1, cbuf_init(&cb, buffer, DATA_SIZE));
    
    // Request to read 0 bytes -> function returns 0
    TEST_ASSERT_EQUAL(0, cbuf_read(&cb, readBuffer, 0));

    // // c-buffer empty -> function returns 0
    TEST_ASSERT_EQUAL(0, cbuf_get_filled(&cb));
    TEST_ASSERT_EQUAL(0, cbuf_read(&cb, readBuffer, 5));

    // readPos > writePos, read before end (not read all)
    // [w - o - o - o - r - x - x - x - x - x]
    cb.readPos = 4; 
    cb.writePos = 0;
    filledBytes = cbuf_get_filled(&cb);
    TEST_ASSERT_EQUAL(6, filledBytes);
    memcpy(&cb.bufPtr[cb.readPos], mockData, filledBytes); // [w - o - o - o - r/32 - 50 - 81 - 60 - 48 - 58]
    readCount = cbuf_read(&cb, readBuffer, filledBytes - 2); // Read 4 bytes: [w - o - o - o - o - o - o - o - r/48 - 58]
    TEST_ASSERT_EQUAL(filledBytes - 2, readCount);
    TEST_ASSERT_EQUAL(8, cb.readPos);
    TEST_ASSERT_EQUAL(48, cb.bufPtr[cb.readPos]);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(mockData, readBuffer, filledBytes - 2);
    TEST_ASSERT_EQUAL(filledBytes - readCount, cbuf_get_filled(&cb)); // 2 bytes left

    // readPos > writePos, read till end (read all)
    // [w - o - o - o - r - x - x - x - x - x]
    cbuf_reset(&cb);
    memset(readBuffer, 0, sizeof(readBuffer));
    memset(buffer, 0xFF, sizeof(buffer));
    cb.readPos = 4; 
    cb.writePos = 0;
    filledBytes = cbuf_get_filled(&cb);
    TEST_ASSERT_EQUAL(6, filledBytes);
    memcpy(&cb.bufPtr[cb.readPos], mockData, filledBytes); // [w - o - o - o - r/32 - 50 - 81 - 60 - 48 - 58]
    readCount = cbuf_read(&cb, readBuffer, filledBytes + 2); // Read 6 bytes: [r/w - o - o - o - o - o - o - o - o - o]
    TEST_ASSERT_EQUAL(filledBytes, readCount);
    TEST_ASSERT_EQUAL(0, cb.readPos); // wrapped-around
    TEST_ASSERT_EQUAL(0xFF, cb.bufPtr[cb.readPos]);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(mockData, readBuffer, readCount);
    for (uint8_t i = 0; i < sizeof(readBuffer); i++) {
        TEST_ASSERT_NOT_EQUAL_UINT8(0xFF, readBuffer[i]); // no unwanted values from the cbuffer copied into the read buffer
    }
    TEST_ASSERT_EQUAL(0, cbuf_get_filled(&cb)); // 0 bytes left

    // readPos > writePos, read till end then wrap around then continue reading (not read all)
    // [x - x - x - w - o - o - r - x - x - x]
    cbuf_reset(&cb);
    memset(readBuffer, 0, sizeof(readBuffer));
    memset(buffer, 0xFF, sizeof(buffer));
    cb.readPos = 6; 
    cb.writePos = 3;
    filledBytes = cbuf_get_filled(&cb);
    TEST_ASSERT_EQUAL(7, filledBytes);
    memcpy(&cb.bufPtr[cb.readPos], mockData, DATA_SIZE - cb.readPos); // [x - x - x - w - o - o - r/32 - 50 - 81 - 60]
    memcpy(&cb.bufPtr[0], &mockData[DATA_SIZE - cb.readPos], cb.writePos); // [48 - 58 - 29 - w - o - o - r/32 - 50 - 81 - 60]
    readCount = cbuf_read(&cb, readBuffer, filledBytes - 1); // Read 6 bytes: [o - o - r/29 - w - o - o - o - o - o - o]
    TEST_ASSERT_EQUAL(filledBytes - 1, readCount);
    TEST_ASSERT_EQUAL(2, cb.readPos);
    TEST_ASSERT_EQUAL(29, cb.bufPtr[cb.readPos]);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(mockData, readBuffer, readCount);
    TEST_ASSERT_EQUAL(0, readBuffer[6]); // Element 6 is still 0
    for (uint8_t i = 0; i < sizeof(readBuffer); i++) {
        TEST_ASSERT_NOT_EQUAL_UINT8(29, readBuffer[i]); // byte 7 in mockData not copied
    }
    TEST_ASSERT_EQUAL(1, cbuf_get_filled(&cb)); // 1 byte left in cbuffer

    // readPos > writePos, read till end then wrap around then continue reading (read all)
    // [x - x - x - w - o - o - r - x - x - x]
    cbuf_reset(&cb);
    memset(readBuffer, 0, sizeof(readBuffer));
    memset(buffer, 0xFF, sizeof(buffer));
    cb.readPos = 6; 
    cb.writePos = 3;
    filledBytes = cbuf_get_filled(&cb);
    TEST_ASSERT_EQUAL(7, filledBytes);
    memcpy(&cb.bufPtr[cb.readPos], mockData, DATA_SIZE - cb.readPos); // [x - x - x - w - o - o - r/32 - 50 - 81 - 60]
    memcpy(&cb.bufPtr[0], &mockData[DATA_SIZE - cb.readPos], cb.writePos); // [48 - 58 - 29 - w - o - o - r/32 - 50 - 81 - 60]
    readCount = cbuf_read(&cb, readBuffer, filledBytes); // Read 7 bytes: [o - o - o - w/r - o - o - o - o - o - o]
    TEST_ASSERT_EQUAL(filledBytes, readCount);
    TEST_ASSERT_EQUAL(3, cb.readPos);
    TEST_ASSERT_EQUAL(0xFF, cb.bufPtr[cb.readPos]);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(mockData, readBuffer, readCount);
    TEST_ASSERT_EQUAL(0, readBuffer[7]); // Element 7 is still 0
    for (uint8_t i = 0; i < sizeof(readBuffer); i++) {
        TEST_ASSERT_NOT_EQUAL_UINT8(13, readBuffer[i]); // byte 8 in mockData not copied
    }
    TEST_ASSERT_EQUAL(0, cbuf_get_filled(&cb)); // 0 byte left in cbuffer

    // readPos < writePos
    // [o - o - o - r - x - x - w - o - o - o]
    cbuf_reset(&cb);
    memset(readBuffer, 0, sizeof(readBuffer));
    memset(buffer, 0xFF, sizeof(buffer));
    cb.readPos = 3;
    cb.writePos = 6;
    filledBytes = cbuf_get_filled(&cb);
    TEST_ASSERT_EQUAL(3, filledBytes);
    memcpy(&cb.bufPtr[cb.readPos], mockData, cb.writePos - cb.readPos); // [o - o - o - r/32 - 50 - 81 - w - o - o - o]
    readCount = cbuf_read(&cb, readBuffer, filledBytes); // read 3 bytes: [o - o - o - o - o - o - r/w - x - x - x]
    TEST_ASSERT_EQUAL(filledBytes, readCount);
    TEST_ASSERT_EQUAL(cb.writePos, cb.readPos);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(mockData, readBuffer, readCount);
#undef DATA_SIZE
}

//
void test_cbuf_peek(void) {
#define DATA_SIZE 10
    cbuf_t cb;
    uint8_t const mockData[DATA_SIZE] = {32, 50, 81, 60, 48, 58, 29, 13, 48, 57};
    uint8_t readBuffer[DATA_SIZE];
    uint8_t originalBuffer[DATA_SIZE];
    uint8_t buffer[DATA_SIZE];
    memset(buffer, 0xFF, DATA_SIZE);
    memset(readBuffer, 0, DATA_SIZE);
    uint64_t peekCount = 0;
    uint64_t numBytes, freeBytes, filledBytes = 0;

    TEST_ASSERT_EQUAL(1, cbuf_init(&cb, buffer, DATA_SIZE));
    
    // Request to read 0 bytes -> function returns 0
    TEST_ASSERT_EQUAL(0, cbuf_peek(&cb, readBuffer, 0));

    // // c-buffer empty -> function returns 0
    TEST_ASSERT_EQUAL(0, cbuf_get_filled(&cb));
    TEST_ASSERT_EQUAL(0, cbuf_peek(&cb, readBuffer, 5));

    // readPos > writePos, peek before end (not peek all)
    // [w - o - o - o - r - x - x - x - x - x]
    cb.readPos = 4; 
    cb.writePos = 0;
    filledBytes = cbuf_get_filled(&cb);
    TEST_ASSERT_EQUAL(6, filledBytes);
    memcpy(&cb.bufPtr[cb.readPos], mockData, filledBytes); // [w - o - o - o - r/32 - 50 - 81 - 60 - 48 - 58]
    memcpy(originalBuffer, cb.bufPtr, DATA_SIZE); // Set originalBuffer to compare back later
    peekCount = cbuf_peek(&cb, readBuffer, filledBytes - 2); // peek 4 bytes: [w - o - o - o - r/32 - 50 - 81 - 60 - p/48 - 58]
    TEST_ASSERT_EQUAL(filledBytes - 2, peekCount);
    TEST_ASSERT_EQUAL(4, cb.readPos); // readPos didn't move
    TEST_ASSERT_EQUAL_UINT8_ARRAY(cb.bufPtr, originalBuffer, DATA_SIZE); // Data in cbuffer didn't change
    TEST_ASSERT_EQUAL_UINT8_ARRAY(mockData, readBuffer, filledBytes - 2); // 4 bytes copied
    TEST_ASSERT_EQUAL(filledBytes, cbuf_get_filled(&cb)); // Still 6 bytes left

    // readPos > writePos, read till end (read all)
    // [w - o - o - o - r - x - x - x - x - x]
    cbuf_reset(&cb);
    memset(readBuffer, 0, sizeof(readBuffer));
    memset(buffer, 0xFF, sizeof(buffer));
    memset(originalBuffer, 0xFF, sizeof(originalBuffer));
    cb.readPos = 4; 
    cb.writePos = 0;
    filledBytes = cbuf_get_filled(&cb);
    TEST_ASSERT_EQUAL(6, filledBytes);
    memcpy(&cb.bufPtr[cb.readPos], mockData, filledBytes); // [w - o - o - o - r/32 - 50 - 81 - 60 - 48 - 58]
    memcpy(originalBuffer, cb.bufPtr, DATA_SIZE); // Set originalBuffer to compare back later
    peekCount = cbuf_peek(&cb, readBuffer, filledBytes + 2); // Read all 6 bytes: // [p/w - o - o - o - r/32 - 50 - 81 - 60 - 48 - 58]
    TEST_ASSERT_EQUAL(filledBytes, peekCount);
    TEST_ASSERT_EQUAL(4, cb.readPos); // readPos didn't move
    TEST_ASSERT_EQUAL_UINT8_ARRAY(mockData, readBuffer, peekCount); // 6 bytes copied
    TEST_ASSERT_EQUAL_UINT8_ARRAY(cb.bufPtr, originalBuffer, DATA_SIZE); // Data in cbuffer didn't change
    TEST_ASSERT_EQUAL(filledBytes, cbuf_get_filled(&cb)); // Still 6 bytes left

    // readPos > writePos, read till end then wrap around then continue reading (not read all)
    // [x - x - x - w - o - o - r - x - x - x]
    cbuf_reset(&cb);
    memset(readBuffer, 0, sizeof(readBuffer));
    memset(buffer, 0xFF, sizeof(buffer));
    memset(originalBuffer, 0xFF, sizeof(originalBuffer));
    cb.readPos = 6; 
    cb.writePos = 3;
    filledBytes = cbuf_get_filled(&cb);
    TEST_ASSERT_EQUAL(7, filledBytes);
    memcpy(&cb.bufPtr[cb.readPos], mockData, DATA_SIZE - cb.readPos); // [x - x - x - w - o - o - p/r/32 - 50 - 81 - 60]
    memcpy(&cb.bufPtr[0], &mockData[DATA_SIZE - cb.readPos], cb.writePos); // [48 - 58 - 29 - w - o - o - p/r/32 - 50 - 81 - 60]
    memcpy(originalBuffer, cb.bufPtr, DATA_SIZE); // Set originalBuffer to compare back later
    peekCount = cbuf_peek(&cb, readBuffer, filledBytes - 1); // Read 6 bytes: // [48 - 58 - p/29 - w - o - o - r/32 - 50 - 81 - 60]
    TEST_ASSERT_EQUAL(filledBytes - 1, peekCount);
    TEST_ASSERT_EQUAL(6, cb.readPos);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(mockData, readBuffer, peekCount);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(cb.bufPtr, originalBuffer, DATA_SIZE); // Data in cbuffer didn't change
    TEST_ASSERT_EQUAL(filledBytes, cbuf_get_filled(&cb)); // Still 7 bytes left

    // readPos > writePos, read till end then wrap around then continue reading (read all)
    // [x - x - x - w - o - o - r - x - x - x]
    cbuf_reset(&cb);
    memset(readBuffer, 0, sizeof(readBuffer));
    memset(buffer, 0xFF, sizeof(buffer));
    memset(originalBuffer, 0xFF, sizeof(originalBuffer));
    cb.readPos = 6; 
    cb.writePos = 3;
    filledBytes = cbuf_get_filled(&cb);
    TEST_ASSERT_EQUAL(7, filledBytes);
    memcpy(&cb.bufPtr[cb.readPos], mockData, DATA_SIZE - cb.readPos); // [x - x - x - w - o - o - r/32 - 50 - 81 - 60]
    memcpy(&cb.bufPtr[0], &mockData[DATA_SIZE - cb.readPos], cb.writePos); // [48 - 58 - 29 - w - o - o - r/32 - 50 - 81 - 60]
    memcpy(originalBuffer, cb.bufPtr, DATA_SIZE); // Set originalBuffer to compare back later
    peekCount = cbuf_peek(&cb, readBuffer, filledBytes); // Read 7 bytes: // [48 - 58 - 29 - p/w - o - o - r/32 - 50 - 81 - 60]
    TEST_ASSERT_EQUAL(filledBytes, peekCount);
    TEST_ASSERT_EQUAL(6, cb.readPos);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(mockData, readBuffer, peekCount);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(cb.bufPtr, originalBuffer, DATA_SIZE); // Data in cbuffer didn't change
    TEST_ASSERT_EQUAL(filledBytes, cbuf_get_filled(&cb)); // Still 7 bytes left in cbuffer

    // readPos < writePos
    // [o - o - o - r - x - x - w - o - o - o]  
    cbuf_reset(&cb);
    memset(readBuffer, 0, sizeof(readBuffer));
    memset(buffer, 0xFF, sizeof(buffer));
    memset(originalBuffer, 0xFF, sizeof(originalBuffer));
    cb.readPos = 3;
    cb.writePos = 6;
    filledBytes = cbuf_get_filled(&cb);
    TEST_ASSERT_EQUAL(3, filledBytes);
    memcpy(&cb.bufPtr[cb.readPos], mockData, cb.writePos - cb.readPos); // [o - o - o - r/32 - 50 - 81 - w - o - o - o]
    memcpy(originalBuffer, cb.bufPtr, DATA_SIZE); // Set originalBuffer to compare back later
    peekCount = cbuf_peek(&cb, readBuffer, filledBytes + 99); // read 3 bytes: // [o - o - o - r/32 - 50 - 81 - p/w - o - o - o]
    TEST_ASSERT_EQUAL(filledBytes, peekCount);
    TEST_ASSERT_EQUAL(3, cb.readPos);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(cb.bufPtr, originalBuffer, DATA_SIZE); // Data in cbuffer didn't change
    TEST_ASSERT_EQUAL_UINT8_ARRAY(mockData, readBuffer, peekCount);
    TEST_ASSERT_EQUAL(filledBytes, cbuf_get_filled(&cb)); // Still 3 bytes left in cbuffer
#undef DATA_SIZE
}

//
void test_cbuf_write_read(void) {
    /* The cbuf_write() and cbuf_read() functions are tested against the..
    ** cbuf_write_single() and cbuf_read_single() functions
    */
#define DATA_SIZE 10
    uint8_t const data[DATA_SIZE] = {3, 4, 9, 8, 7, 5, 2, 1, 33, 99};
    uint8_t buffer[DATA_SIZE] = {0}, bufferSingle[DATA_SIZE] = {0};
    uint8_t readBuf[DATA_SIZE], readBufSingle[DATA_SIZE] = {0};
    cbuf_t cb, cbSingle;

    cbuf_init(&cb, buffer, DATA_SIZE);
    cbuf_init(&cbSingle, bufferSingle, DATA_SIZE);

    TEST_ASSERT_EQUAL(DATA_SIZE, cb.size);
    TEST_ASSERT_EQUAL(0, cbuf_write(&cb, data, 0));
    TEST_ASSERT_EQUAL(0, cbuf_read(&cb, readBuf, 0));
    TEST_ASSERT_EQUAL(0, cbuf_get_filled(&cb));
    TEST_ASSERT_EQUAL(0, cbuf_get_filled(&cbSingle));
    TEST_ASSERT_EQUAL(DATA_SIZE - 1, cbuf_get_free(&cb));
    TEST_ASSERT_EQUAL(DATA_SIZE - 1, cbuf_get_free(&cbSingle));

    // Currently in buffer: [r/w - o - o - o - o - o - o - o - o - o]
    uint8_t count = 0, countSingle = 0;
    count = cbuf_write(&cb, data, DATA_SIZE + 10); // Only (DATA_SIZE - 1) bytes are written
    for (uint8_t i = 0; i < DATA_SIZE + 10; i++) {
        countSingle += cbuf_write_single(&cbSingle, data[i]); // Only (DATA_SIZE - 1) bytes are written
    }
    // Currently in buffer: [r/3 - 4 - 9 - 8 - 7 - 5 - 2 - 1 - 33 - w]
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data, buffer, DATA_SIZE - 1);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(buffer, bufferSingle, DATA_SIZE - 1);
    TEST_ASSERT_EQUAL(DATA_SIZE - 1, cbuf_get_filled(&cb));
    TEST_ASSERT_EQUAL(DATA_SIZE - 1, cbuf_get_filled(&cbSingle));
    TEST_ASSERT_EQUAL(0, cbuf_get_free(&cb));
    TEST_ASSERT_EQUAL(0, cbuf_get_free(&cbSingle));
    
    count = cbuf_read(&cb, readBuf, DATA_SIZE + 10); // Only (DATA_SIZE - 1) bytes are read
    countSingle = 0;
    for (uint8_t i = 0; i < DATA_SIZE + 10; i++) {
        countSingle += cbuf_read_single(&cbSingle, &readBufSingle[i]); // Only (DATA_SIZE - 1) bytes are read
    }
    
    // Currently in buffer: [o - o - o - o - o - o - o - o - o - r/w]
    TEST_ASSERT_EQUAL(DATA_SIZE - 1, count);
    TEST_ASSERT_EQUAL(count, countSingle);
    TEST_ASSERT_EQUAL(DATA_SIZE - 1, cb.readPos);
    TEST_ASSERT_EQUAL(cb.readPos, cbSingle.readPos);
    TEST_ASSERT_EQUAL(cb.writePos, cb.readPos);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data, readBuf, DATA_SIZE - 1);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(readBuf, readBufSingle, DATA_SIZE - 1);
    TEST_ASSERT_EQUAL(DATA_SIZE - 1, cbuf_get_free(&cb));
    TEST_ASSERT_EQUAL(DATA_SIZE - 1, cbuf_get_free(&cbSingle));
    TEST_ASSERT_EQUAL(0, cbuf_get_filled(&cb));
    TEST_ASSERT_EQUAL(0, cbuf_get_filled(&cbSingle));

    // Currently in buffer: [o - o - o - o - o - o - o - o - o - r/w]
    uint8_t const data2[DATA_SIZE] = {51, 21, 11, 13, 17, 19, 23, 25, 27, 29};
    count = cbuf_write(&cb, data2, DATA_SIZE + 10); // Only (size - 1) bytes are written
    countSingle = 0;
    for (uint8_t i = 0; i < DATA_SIZE + 10; i++) {
        countSingle += cbuf_write_single(&cbSingle, data2[i]); // Only (size - 1) bytes are written
    }
    // Currently in buffer: [21 - 11 - 13 - 17 - 19 - 23 - 25 - 27 - w - r/51]
    TEST_ASSERT_EQUAL(DATA_SIZE - 1, count);
    TEST_ASSERT_EQUAL(count, countSingle);
    TEST_ASSERT_EQUAL(DATA_SIZE - 2, cb.writePos);
    TEST_ASSERT_EQUAL(cb.writePos, cbSingle.writePos);
    TEST_ASSERT_EQUAL(data2[0], cb.bufPtr[DATA_SIZE - 1]); // First byte of data2[] pushed in at last position
    TEST_ASSERT_EQUAL(data2[0], cbSingle.bufPtr[DATA_SIZE - 1]);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(&data2[1], &cb.bufPtr[0], DATA_SIZE - 2);
    TEST_ASSERT_EQUAL(DATA_SIZE - 1, cbuf_get_filled(&cb));
    TEST_ASSERT_EQUAL(DATA_SIZE - 1, cbuf_get_filled(&cbSingle));
    TEST_ASSERT_EQUAL(0, cbuf_get_free(&cb));
    TEST_ASSERT_EQUAL(0, cbuf_get_free(&cbSingle));

    count = cbuf_read(&cb, readBuf, DATA_SIZE + 10); // (DATA_SIZE - 1) bytes are read
    countSingle = 0;
    for (uint8_t i = 0; i < DATA_SIZE + 10; i++) {
        countSingle += cbuf_read_single(&cbSingle, &readBufSingle[i]); // (DATA_SIZE - 1) bytes are read
    }
    // Currently in buffer: [o - o - o - o - o - o - o - o - r/w - o]
    TEST_ASSERT_EQUAL(DATA_SIZE - 1, count);
    TEST_ASSERT_EQUAL(DATA_SIZE - 1, countSingle);
    TEST_ASSERT_EQUAL(cb.writePos, cb.readPos);
    TEST_ASSERT_EQUAL(cbSingle.readPos, cbSingle.writePos);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data2, readBuf, DATA_SIZE - 1);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data2, readBufSingle, DATA_SIZE - 1);
    TEST_ASSERT_EQUAL(0, cbuf_get_filled(&cb));
    TEST_ASSERT_EQUAL(0, cbuf_get_filled(&cbSingle));

    const uint8_t data3[1] = {34};
    count = cbuf_write(&cb, data3, sizeof(data3));
    countSingle = cbuf_write_single(&cbSingle, data3[0]);
    // Currently in buffer: [o - o - o - o - o - o - o - o - r/34 - w]
    TEST_ASSERT_EQUAL(cb.writePos, DATA_SIZE - 1);
    TEST_ASSERT_EQUAL(cbSingle.writePos, DATA_SIZE - 1);
    TEST_ASSERT_EQUAL(DATA_SIZE - 2, cbuf_get_free(&cb));
    TEST_ASSERT_EQUAL(DATA_SIZE - 2, cbuf_get_free(&cbSingle));
    TEST_ASSERT_EQUAL(data3[0], cb.bufPtr[cb.writePos - 1]);
    TEST_ASSERT_EQUAL(data3[0], cbSingle.bufPtr[cbSingle.writePos - 1]);
    TEST_ASSERT_EQUAL(1, cbuf_get_filled(&cb));
    TEST_ASSERT_EQUAL(1, cbuf_get_filled(&cbSingle));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(cb.bufPtr, cbSingle.bufPtr, DATA_SIZE);

    const uint8_t data4[7] = {44, 41, 45, 42, 54, 52, 50};
    count = cbuf_write(&cb, data4, sizeof(data4));
    countSingle = 0;
    for (uint8_t i = 0; i < sizeof(data4); i++) {
        countSingle += cbuf_write_single(&cbSingle, data4[i]);
    }
    // Currently in buffer: [41 - 45 - 42 - 54 - 52 - 50 - w - o - r/34 - 44]
    TEST_ASSERT_EQUAL(6, cb.writePos);
    TEST_ASSERT_EQUAL(6, cbSingle.writePos);
    TEST_ASSERT_EQUAL(sizeof(data3) + sizeof(data4), cbuf_get_filled(&cb));
    TEST_ASSERT_EQUAL(sizeof(data3) + sizeof(data4), cbuf_get_filled(&cbSingle));
    TEST_ASSERT_EQUAL(data4[0], cb.bufPtr[DATA_SIZE - 1]);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(&data4[1], cb.bufPtr, sizeof(data4) - 1);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(cb.bufPtr, cbSingle.bufPtr, DATA_SIZE);

    count = cbuf_read(&cb, readBuf, 2);
    countSingle = 0;
    for (uint8_t i = 0; i < 2; i++) {
        cbuf_read_single(&cbSingle, &readBufSingle[i]);
    }
    // Currently in buffer: [r/41 - 45 - 42 - 54 - 52 - 50 - w - o - o - o]
    TEST_ASSERT_EQUAL(0, cb.readPos);
    TEST_ASSERT_EQUAL(0, cbSingle.readPos);
    TEST_ASSERT_EQUAL_UINT8(34, readBuf[0]);
    TEST_ASSERT_EQUAL_UINT8(34, readBufSingle[0]);
    TEST_ASSERT_EQUAL_UINT8(44, readBuf[1]);
    TEST_ASSERT_EQUAL_UINT8(44, readBufSingle[1]);
    TEST_ASSERT_EQUAL(6, cbuf_get_filled(&cb));
    TEST_ASSERT_EQUAL(3, cbuf_get_free(&cb));

    count = cbuf_read(&cb, readBuf, 6);
    countSingle = 0;
    for (uint8_t i = 0; i < 6; i++) {
        cbuf_read_single(&cbSingle, &readBufSingle[i]);
    }
    // Currently in buffer: [o - o - o - o - o - o - r/w - o - o - o]
    TEST_ASSERT_EQUAL(6, cbSingle.readPos);
    TEST_ASSERT_EQUAL(cbSingle.readPos, cb.readPos);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(&data4[1], readBuf, 6);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(readBuf, readBufSingle, 6);

    // Currently in buffer: [o - o - o - o - o - o - r/w - o - o - o]
    uint8_t const data5[4] = {64, 51, 60, 30}; //
    count = cbuf_write(&cb, data5, 4);
    countSingle = 0;
    for (uint8_t i = 0; i < sizeof(data5); i++) {
        countSingle += cbuf_write_single(&cbSingle, data5[i]);
    }
    // Currently in buffer: [w - o - o - o - o - o - r/64 - 51 - 60 - 30]
    TEST_ASSERT_EQUAL(sizeof(data5), count);
    TEST_ASSERT_EQUAL(sizeof(data5), countSingle);
    TEST_ASSERT_EQUAL(0, cb.writePos);
    TEST_ASSERT_EQUAL(0, cbSingle.writePos);

    uint8_t const data6[5] = {58, 23, 40, 92, 13};
    count = cbuf_write(&cb, data6, sizeof(data6));
    countSingle = 0;
    for (uint8_t i = 0; i < sizeof(data6); i++) {
        countSingle += cbuf_write_single(&cbSingle, data6[i]);
    }
    // Currently in buffer: [58 - 23 - 40 - 92 - 13 - w - r/64 - 51 - 60 - 30]
    TEST_ASSERT_EQUAL(sizeof(data6), count);
    TEST_ASSERT_EQUAL(sizeof(data6), countSingle);
    TEST_ASSERT_EQUAL(5, cb.writePos);
    TEST_ASSERT_EQUAL(5, cbSingle.writePos);

    count = cbuf_read(&cb, readBuf, sizeof(data5) + sizeof(data6));
    countSingle = 0;
    for (uint8_t i=0; i < sizeof(data5) + sizeof(data6); i++) {
        countSingle += cbuf_read_single(&cbSingle, &readBufSingle[i]);
    }
    TEST_ASSERT_EQUAL(sizeof(data5) + sizeof(data6), count);
    TEST_ASSERT_EQUAL(sizeof(data5) + sizeof(data6), countSingle);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data5, readBuf, sizeof(data5));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(data6, &readBuf[sizeof(data5)], sizeof(data6));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(readBuf, readBufSingle, sizeof(data5) + sizeof(data6));
#undef DATA_SIZE
}