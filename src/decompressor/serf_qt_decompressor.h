#ifndef SERF_QT_DECOMPRESSOR_H
#define SERF_QT_DECOMPRESSOR_H

#include <stdint.h>
#include <stdbool.h>

// IAR适配：移除STL依赖，使用C风格头文件
#include "../utils/double.h"
#include "../utils/input_bit_stream.h"
#include "../utils/zig_zag_codec.h"
#include "../utils/elias_gamma_codec.h"
#include "../utils/array.h"

class SerfQtDecompressor {
 public:
  SerfQtDecompressor();
  ~SerfQtDecompressor();
  
  // IAR适配：返回固定大小数组替代std::vector
  Array<float> Decompress(const Array<uint8_t> &bs);

 private:
  uint16_t block_size_;
  float max_diff_;
  InputBitStream* input_bit_stream_; // 使用指针替代std::unique_ptr
  float pre_value_;

  float NextValue();
};

#endif //SERF_QT_DECOMPRESSOR_H
