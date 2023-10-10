#include <assert.h>
#include <stdio.h>

#define CONTINUATION_BIT_MASK 0x80 // 1000 0000
#define PAYLOAD_MASK 0x7F          // 0111 1111

void encode(u_int64_t input, u_int8_t *buffer) {
  int i = 0;
  while (input > 0) {
    // Use a bitmask to capture the lowest 7 bits
    u_int8_t chunk = input & PAYLOAD_MASK;

    // Shift the lowest 7 bits out from original input
    input = input >> 7;

    if (input > 0) {
      // If the input is greater than zero there will
      // be another byte to follow, so set the
      // continuation bit
      chunk = chunk | CONTINUATION_BIT_MASK;
    }

    buffer[i] = chunk;
    i++;
  }
}

u_int64_t decode(u_int8_t *buffer) {
  int continuation = 1;
  int i = 0;
  u_int64_t out = 0;
  u_int64_t chunk;

  while (continuation > 0) {
    chunk = (u_int64_t)buffer[i];

    // Isolate continuation bit
    continuation = chunk & CONTINUATION_BIT_MASK;

    // clear continuation bit from chunk
    // to prepare for prepend
    chunk = chunk & PAYLOAD_MASK;

    // shift the value of chunk left
    // 7 bytes * i for prepend
    chunk = chunk << (7 * i);

    // Prepend chunk to out
    out = out | chunk;

    i++;
  }

  return out;
}

void debug(u_int8_t *buffer, const char *label) {
  printf("%s:\t0x", label);
  for (int i = 0; i < 10; i++) {
    printf("%x", buffer[i]);
  }
  printf("\n");
}

void test_encode_decode(u_int64_t input, int verbose) {
  // Create a buffer that's 10 bytes long
  // since that's the max length of varint
  u_int8_t buffer[10] = {0, 0, 0, 0, 0, 0, 0, 0};

  if (verbose) {
    printf("=== Test for %llu ===\n", input);
    debug(buffer, "Initialized");
  }

  encode(input, buffer);
  if (verbose) {
    debug(buffer, "Encoded");
  }

  u_int64_t result = decode(buffer);
  if (verbose) {
    printf("Decoded: \t%llu\n\n", result);
  }

  assert(result == input);
}

int main() {
  test_encode_decode(1, 1);
  test_encode_decode(150, 1);
  test_encode_decode(2220, 1);

  for (u_int64_t i = 0; i < 1000000000; i++) {
    test_encode_decode(i, 0);
  }
}
