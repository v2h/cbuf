#include "cbuf.h"


#include <stdlib.h>
#include <string.h>

#define CBUF_MIN(x,y) ((x) < (y) ? (x) : (y))

// Initialize using malloc -> extremely careful, not advised to use
// Returns a pointer to the cbuffer created
//cbuf_t * cbuf_init_dynamic(cbuf_t *cb, uint16_t const max_number_elements)
//{
//    cbuf_t *cbuf = (struct cbuf *)(malloc(sizeof(*cbuf)));
//    uint8_t *buffer = (uint8_t *)malloc((max_number_elements + 1)* sizeof(uint8_t));
//    cbuf->bufPtr = buffer; // or = &buffer[0]
//    cbuf->size = max_number_elements + 1;
//    cbuf->writePos = 0;
//    cbuf->readPos = 0;
//
//    return cbuf;
//}

//void cbuf_free(cbuf_t *cb)
//{
//    free(cb->bufPtr);
//    cb->bufPtr = NULL;
//    cb->writePos = 0;
//    cb->readPos = 0;
//    cb->size = 0;
//}

// Initialize a cbuffer with a given static buffer
uint8_t cbuf_init(cbuf_t *cb, void *buffer, uint64_t const sizeInBytes)
{
    if (NULL == cb || NULL == buffer || 0 == sizeInBytes) {
        return 0;
    }
    cb->bufPtr   = (uint8_t *)buffer;
    cb->size     = sizeInBytes;
    cb->writePos = 0;
    cb->readPos  = 0;
    return 1;
}

//
bool cbuf_is_full(cbuf_t *cb)
{
    return (((cb->writePos + 1) % cb->size) == cb->readPos);
}

//
bool cbuf_is_empty(cbuf_t *cb)
{
    return cb->writePos == cb->readPos;
}

//
uint64_t cbuf_get_free(cbuf_t *cb) {
    return cb->size - cbuf_get_filled(cb) - 1;
}

//
uint64_t cbuf_get_filled(cbuf_t *cb)
{
    if (cb->writePos >= cb->readPos) {
        return (cb->writePos - cb->readPos);
    }
    else {
        return (cb->size - cb->readPos + cb->writePos);
    }
}


//
uint64_t cbuf_write(cbuf_t *cb, const void *data, uint64_t numOfBytes)
{
    const uint8_t *src = (uint8_t *)data;
    uint64_t bytesToWrite = CBUF_MIN(numOfBytes, cbuf_get_free(cb));
    if (0 == bytesToWrite) {
        return 0;
    }
    if (cb->writePos > cb->readPos) {
        uint64_t bytesTillEnd = CBUF_MIN(bytesToWrite, cb->size - cb->writePos);
        memcpy(&cb->bufPtr[cb->writePos], src, bytesTillEnd);
        cb->writePos = (cb->writePos + bytesTillEnd) % cb->size;
        bytesToWrite -= bytesTillEnd;
        if (0 == bytesToWrite) {
            return bytesTillEnd;
        }
        else { // Back to start of buffer
            memcpy(&cb->bufPtr[0], &src[bytesTillEnd], bytesToWrite);
            cb->writePos += bytesToWrite;
            return bytesToWrite + bytesTillEnd;
        }
    }
    else if (cb->writePos < cb->readPos) {
        memcpy(&cb->bufPtr[cb->writePos], src, bytesToWrite);
        cb->writePos += bytesToWrite;
        return bytesToWrite;
    }
    return 0;
}

//
uint64_t cbuf_read(cbuf_t *cb, void *buffer, uint64_t numOfBytes)
{
    const uint8_t *dst = (uint8_t *)buffer;
    uint64_t bytesToRead = CBUF_MIN(numOfBytes, cbuf_get_filled(cb));
    if (0 == bytesToRead) {
        return 0;
    }
    if (cb->readPos > cb->writePos) {
        uint64_t bytesTillEnd = CBUF_MIN(bytesToRead, cb->size - cb->readPos);
        memcpy(dst, &cb->bufPtr[cb->readPos], bytesTillEnd);
        cb->readPos = (cb->readPos + bytesTillEnd) % cb->size;
        bytesToRead -= bytesTillEnd;
        if (0 == bytesToRead) {
            return bytesTillEnd;
        }
        else { // Back to start of buffer
            memcpy(&dst[bytesTillEnd], &cb->bufPtr[0], bytesToRead);
            cb->readPos += bytesToRead;
            return bytesToRead + bytesTillEnd;
        }
    }
    else if (cb->readPos < cb->writePos) {
        memcpy(dst, &cb->bufPtr[cb->readPos], bytesToRead);
        cb->readPos += bytesToRead;
        return bytesToRead;
    }
    return 0;
}

//
uint8_t cbuf_write_single(cbuf_t *cb, uint8_t data)
{
    if (cbuf_is_full(cb))
        return 0;

    cb->bufPtr[cb->writePos] = data;
    cb->writePos = (cb->writePos + 1) % cb->size; // modulo is for wrap-around
    return 1;
}

//
uint8_t cbuf_read_single(cbuf_t *cb, uint8_t *buffer)
{
    if (cbuf_is_empty(cb)) {
        return 0;
    }

    *buffer = cb->bufPtr[cb->readPos];
    cb->readPos = (cb->readPos + 1) % cb->size; // modulo is for wrap-around
    return 1;
}
