#ifndef CBUF_H
#define CBUF_H

#include <stdint.h>
#include <stdbool.h>

typedef struct cbuf {
  uint8_t *bufPtr;
  uint64_t writePos;
  uint64_t readPos;
  uint64_t size;
} cbuf_t;

//extern cbuf_t * cb_init_dynamic(cbuf_t *cb, uint16_t const max_number_elements);

bool     cbuf_init(cbuf_t *cb, void *buffer, uint64_t const sizeInBytes);
bool     cbuf_reset(cbuf_t *cb);
bool     cbuf_is_full(cbuf_t *cb);
bool     cbuf_is_empty(cbuf_t *cb);
uint64_t cbuf_get_free(cbuf_t *cb);
uint64_t cbuf_get_filled(cbuf_t *cb);
uint64_t cbuf_write(cbuf_t *cb, void const *data, uint64_t numOfBytes);
uint64_t cbuf_read(cbuf_t *cb, void * const buffer, uint64_t numOfBytes);
uint8_t  cbuf_write_single(cbuf_t *cb, uint8_t data);
uint8_t  cbuf_read_single(cbuf_t *cb, uint8_t *buffer);

#endif // CBUF_H
