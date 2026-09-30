#pragma once
#include <stdint.h>
#include <stddef.h>

// A strict bounded decimal parser; rejects overflow, trailing bytes and junk.
inline bool parseCommand(const uint8_t *data, size_t length, char &command, uint32_t &id,
                  uint32_t &sequence, int &left, int &right) {
  if (length < 9 || length > 64 || data[1] != ',') return false;
  command = char(data[0]);
  if (command != 'A' && command != 'D' && command != 'S') return false;
  int64_t numbers[4] = {};
  size_t pos = 2;
  for (int n = 0; n < 4; n++) {
    bool negative = pos < length && data[pos] == '-';
    if (negative) pos++;
    if (pos == length || data[pos] < '0' || data[pos] > '9') return false;
    while (pos < length && data[pos] >= '0' && data[pos] <= '9') {
      numbers[n] = numbers[n] * 10 + data[pos++] - '0';
      if (numbers[n] > UINT32_MAX) return false;
    }
    if (negative) numbers[n] = -numbers[n];
    if (n < 3 && (pos == length || data[pos++] != ',')) return false;
  }
  if (pos != length || numbers[0] <= 0 || numbers[1] <= 0 ||
      numbers[2] < -100 || numbers[2] > 100 || numbers[3] < -100 || numbers[3] > 100)
    return false;
  id = uint32_t(numbers[0]); sequence = uint32_t(numbers[1]);
  left = int(numbers[2]); right = int(numbers[3]);
  return true;
}

