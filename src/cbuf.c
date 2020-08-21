#include "cbuf.h"
#include <string.h>

#define CBUF_MIN(x,y) ((x) < (y) ? (x) : (y))

// Initialize a cbuffer with a given buffer
// Maximum storage size is (sizeInBytes - 1) because of the full/empty conditions
// There is always no data at writePos
// There is always data at readPos, unless readPos == writePos

/** \brief Initialize a circular buffer.
 * Maximum storage size is (sizeInBytes - 1) due to the full/empty conditions.
 * 
 * \param[in] cb: handle to cbuf_t.
 * \param[in] buffer: internal buffer to store data, must not be directly manipulated.
 * \param[in] sizeInBytes: size of buffer in bytes.
 * \return `true` if successful, `false` otherwise.
 */
bool cbuf_init(cbuf_t *cb, void *buffer, uint64_t const sizeInBytes) {
    if (NULL == cb || NULL == buffer || 0 == sizeInBytes) {
        return false;
    }
    cb->bufPtr   = (uint8_t *)buffer;
    cb->size     = sizeInBytes;
    cb->writePos = 0;
    cb->readPos  = 0;
    return true;
}

/** \brief Reset circular buffer.
 *
 * \param[in] cb: handle to cbuf_t.
 * \return `true` if successful, `false` otherwise.
 *
 * Memsetting the internal buffer to 0 is not performed.
 */
bool cbuf_reset(cbuf_t *cb) {
    if (NULL == cb) {
        return false;
    }
    cb->writePos = cb->readPos = 0;
    return true;
}

/** \brief Check if buffer is full.
 * 
 * \param[in] cb: handle to cbuf_t.
 * \return `true` if buffer is full, `false` otherwise.
 *
 */
inline bool cbuf_is_full(cbuf_t *cb) {
    return (((cb->writePos + 1) % cb->size) == cb->readPos);
}

/** \brief Check if buffer is empty.
 * 
 * \param[in] cb: handle to cbuf_t.
 * \return `true` if buffer is empty, `false` otherwise.
 *
 */
inline bool cbuf_is_empty(cbuf_t *cb) {
    return cb->writePos == cb->readPos;
}

/** \brief Get number of free slots in buffer.
 * Max. free slots is always (size - 1)
 * 
 * \param[in] cb: handle to cbuf_t.
 * \return number of free bytes in buffer.
 *
 */
uint64_t cbuf_get_free(cbuf_t *cb) {
    return cb->size - cbuf_get_filled(cb) - 1;
}

/** \brief Get number of data bytes stored in buffer.
  * 
 * \param[in] cb: handle to cbuf_t.
 * \return number of data bytes stored in buffer.
 *
 */
uint64_t cbuf_get_filled(cbuf_t *cb) {
    if (cb->writePos >= cb->readPos) {
        return (cb->writePos - cb->readPos);
    }
    else {
        return (cb->size - cb->readPos + cb->writePos);
    }
}

/** \brief Write data to circular buffer.
 * 
 * \param[in] cb: handle to cbuf_t.
 * \param[in] data: pointer to data to be written into buffer.
 * \param[in] numbOfBytes: number of bytes to be written into buffer.
 * \return number of bytes written into buffer.
 * 
 */
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

/** \brief Read data from circular buffer.
 * 
 * \param[in] cb: handle to cbuf_t.
 * \param[out] data: pointer to buffer for storing data to be read
 * \param[in] numbOfBytes: number of bytes to be read from circular buffer.
 * \return number of bytes read from buffer.
 * 
 */
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

/** \brief Write one byte into circular buffer.
 * 
 * \param[in] cb: handle to cbuf_t.
 * \param[in] data: data to be written.
 * \return `1` if successful, `0` otherwise.
 * 
 */
uint8_t cbuf_write_single(cbuf_t *cb, uint8_t data) {
    if (cbuf_is_full(cb))
        return 0;

    cb->bufPtr[cb->writePos] = data;
    cb->writePos = (cb->writePos + 1) % cb->size; // modulo is for wrap-around
    return 1;
}

/** \brief Read one byte from circular buffer.
 * 
 * \param[in] cb: handle to cbuf_t.
 * \param[out] buffer: buffer to store data to be read.
 * \return `1` if successful, `0` otherwise.
 * 
 */
uint8_t cbuf_read_single(cbuf_t *cb, uint8_t *buffer) {
    if (cbuf_is_empty(cb)) {
        return 0;
    }

    *buffer = cb->bufPtr[cb->readPos];
    cb->readPos = (cb->readPos + 1) % cb->size; // modulo is for wrap-around
    return 1;
}
