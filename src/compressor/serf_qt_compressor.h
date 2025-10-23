#ifndef SERF_QT_COMPRESSOR_H
#define SERF_QT_COMPRESSOR_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

// IAR适配：移除C++ STL依赖，使用C风格头文件
#include "../utils/output_bit_stream.h"
#include "../utils/array.h"
#include "../utils/double.h"
#include "../utils/elias_gamma_codec.h"
#include "../utils/zig_zag_codec.h"

/*
 * +------------+-----------------+---------------+
 * |16bits - len|64bits - max_diff|Encoded Content|
 * +------------+-----------------+---------------+
 */

class SerfQtCompressor {
 public:
  SerfQtCompressor(uint16_t block_size, float max_diff);

  void AddValue(float v);

  Array<uint8_t> compressed_bytes();

  void Close();

  uint32_t get_compressed_size_in_bits() const;

  // 析构函数
  ~SerfQtCompressor();

 private:
  const float kMaxDiff;
  const uint16_t kBlockSize;
  bool first_;
  OutputBitStream* output_bit_stream_; // 使用指针替代std::unique_ptr
  Array<uint8_t> compressed_bytes_;
  float pre_value_;
  uint32_t compressed_size_in_bits_;
  uint32_t stored_compressed_size_in_bits_;
};

#endif  // SERF_QT_COMPRESSOR_H
