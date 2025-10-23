#ifndef SERF_OUTPUT_BIT_STREAM_H
#define SERF_OUTPUT_BIT_STREAM_H

#include <stdint.h>
#include <stdbool.h>

// IAR适配：移除endian.h依赖，使用自定义字节序转换
// 由于CC2530是小端序，我们实现简单的字节序转换函数

// 字节序转换函数（小端序到网络字节序）
static inline uint32_t htobe32(uint32_t host_32bits) {
    return ((host_32bits & 0xFF000000) >> 24) |
           ((host_32bits & 0x00FF0000) >> 8)  |
           ((host_32bits & 0x0000FF00) << 8)  |
           ((host_32bits & 0x000000FF) << 24);
}

static inline uint32_t be32toh(uint32_t big_endian_32bits) {
    return ((big_endian_32bits & 0xFF000000) >> 24) |
           ((big_endian_32bits & 0x00FF0000) >> 8)  |
           ((big_endian_32bits & 0x0000FF00) << 8)  |
           ((big_endian_32bits & 0x000000FF) << 24);
}

#include "array.h"

class OutputBitStream {
 public:
  explicit OutputBitStream(uint32_t buffer_size);

  uint32_t Write(uint32_t content, uint32_t len);

  uint32_t WriteLong(uint32_t content, uint32_t len);

  uint32_t WriteInt(uint32_t content, uint32_t len);

  uint32_t WriteBit(bool bit);

  void Flush();

  Array<uint8_t> GetBuffer(uint32_t len);

  void Refresh();

 private:
  Array<uint32_t> data_;
  uint32_t cursor_;
  uint32_t bit_in_buffer_;
  uint32_t buffer_; // 使用32位替代64位
};

#endif  // SERF_OUTPUT_BIT_STREAM_H
