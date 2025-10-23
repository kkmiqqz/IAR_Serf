#ifndef SERF_ZIG_ZAG_CODEC_H
#define SERF_ZIG_ZAG_CODEC_H

#include <stdint.h>

// IAR适配：使用C风格头文件，使用32位替代64位
class ZigZagCodec {
 public:
  static inline int32_t Encode(int32_t value) {
    return (value << 1) ^ (value >> 31);
  }

  static inline int32_t Decode(int32_t value) {
    return (value >> 1) ^ -(value & 1);
  }
};

#endif  // SERF_ZIG_ZAG_CODEC_H
