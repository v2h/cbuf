#include "cbuf.h"

#include <stdlib.h>
#include <string.h>

#define CBUF_MIN(x,y) ((x) < (y) ? (x) : (y))

// Initialize a cbuffer with a given buffer
// Maxium storage size is (sizeInBytes - 1) because of the full/empty conditions
// There is always no data at writePos
// There is always data at readPos, unless readPos == writePos
uint8_t cbuf_init(cbuf_t *cb, void *buffer, uint64_t const sizeInBytes) {
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
uint8_t cbuf_reset(cbuf_t *cb) {
    if (NULL == cb) {
        return 0;
    }
    cb->writePos = cb->readPos = 0;
    return 1;
}

//
inline bool cbuf_is_full(cbuf_t *cb) {
    return (((cb->writePos + 1) % cb->size) == cb->readPos);
}

//
inline bool cbuf_is_empty(cbuf_t *cb) {
    return cb->writePos == cb->readPos;
}

//
// Max. free slots is always (size - 1)
uint64_t cbuf_get_free(cbuf_t *cb) {
    return cb->size - cbuf_get_filled(cb) - 1;
}

//
uint64_t cbuf_get_filled(cbuf_t *cb) {
    if (cb->writePos >= cb->readPos) {
        return (cb->writePos - cb->readPos);
    }
    else {
        return (cb->size - cb->readPos + cb->writePos);
    }
}


//
uint64_t cbuf_write(cbuf_t *cb, void const *data, uint64_t numOfBytes) {
    uint64_t bytesToWrite = CBUF_MIN(numOfBytes, cbuf_get_free(cb));
    if (0 == bytesToWrite) {
        return 0;
    }
    if (cb->writePos >= cb->readPos) {
        uint64_t bytesTillEnd = CBUF_MIN(bytesToWrite, cb->size - cb->writePos);
        memcpy(&cb->bufPtr[cb->writePos], data, bytesTillEnd);
        cb->writePos = (cb->writePos + bytesTillEnd) % cb->size;
        bytesToWrite -= bytesTillEnd;
        if (0 == bytesToWrite) {
            return bytesTillEnd;
        }
        else { // Back to start of buffer
            memcpy(&cb->bufPtr[0], (uint8_t *)data + bytesTillEnd, bytesToWrite);
            cb->writePos += bytesToWrite;
            return bytesToWrite + bytesTillEnd;
        }
    }
    else {
        memcpy(&cb->bufPtr[cb->writePos], data, bytesToWrite);
        cb->writePos += bytesToWrite;
        return bytesToWrite;
    }
}

//
uint64_t cbuf_read(cbuf_t *cb, void * const buffer, uint64_t numOfBytes) {
    uint64_t bytesToRead = CBUF_MIN(numOfBytes, cbuf_get_filled(cb));
    if (0 == bytesToRead) {
        return 0;
    }
    if (cb->readPos > cb->writePos) {
        uint64_t bytesTillEnd = CBUF_MIN(bytesToRead, cb->size - cb->readPos);
        memcpy(buffer, &cb->bufPtr[cb->readPos], bytesTillEnd);
        cb->readPos = (cb->readPos + bytesTillEnd) % cb->size;
        bytesToRead -= bytesTillEnd;
        if (0 == bytesToRead) {
            return bytesTillEnd;
        }
        else { // Back to start of buffer
            memcpy((uint8_t *)buffer + bytesTillEnd, &cb->bufPtr[0], bytesToRead);
            cb->readPos += bytesToRead;
            return bytesToRead + bytesTillEnd;
        }
    }
    else { // The (readPos == writePos) condition won't happen because it's been checked with cbuf_get_filled()
        memcpy(buffer, &cb->bufPtr[cb->readPos], bytesToRead);
        cb->readPos += bytesToRead;
        return bytesToRead;
    }
}

//
uint8_t cbuf_write_single(cbuf_t *cb, uint8_t data) {
    if (cbuf_is_full(cb))
        return 0;

    cb->bufPtr[cb->writePos] = data;
    cb->writePos = (cb->writePos + 1) % cb->size; // modulo is for wrap-around
    return 1;
}

//
uint8_t cbuf_read_single(cbuf_t *cb, uint8_t *buffer) {
    if (cbuf_is_empty(cb)) {
        return 0;
    }

    *buffer = cb->bufPtr[cb->readPos];
    cb->readPos = (cb->readPos + 1) % cb->size; // modulo is for wrap-around
    return 1;
}
