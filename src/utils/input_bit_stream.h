#ifndef SERF_INPUT_BIT_STREAM_H
#define SERF_INPUT_BIT_STREAM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// IAR适配：移除STL依赖，使用C风格头文件
#include "array.h"

class InputBitStream {
 public:
  InputBitStream();

  InputBitStream(uint8_t *raw_data, uint32_t size);

  uint32_t ReadLong(uint32_t len);

  uint32_t ReadInt(uint32_t len);

  bool ReadBit();

  void SetBuffer(const Array<uint8_t> &new_buffer);

 private:
  void Forward(uint32_t len);
  uint32_t Peek(uint32_t len);

  Array<uint32_t> data_;
  uint32_t buffer_; // 使用32位替代64位
  uint32_t cursor_;
  uint32_t bit_in_buffer_;
};

#endif  // SERF_INPUT_BIT_STREAM_H
