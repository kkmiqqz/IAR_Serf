#ifndef NET_SERF_QT_DECOMPRESSOR_H
#define NET_SERF_QT_DECOMPRESSOR_H

#include <stdint.h>

// IAR适配：移除C++11特性
#include "../utils/input_bit_stream.h"
#include "../utils/zig_zag_codec.h"
#include "../utils/elias_gamma_codec.h"
#include "../utils/array.h"

class NetSerfQtDecompressor {
 public:
  explicit NetSerfQtDecompressor(double max_diff);
  ~NetSerfQtDecompressor();

  // 返回double（GPS坐标通常使用double）
  double Decompress(Array<uint8_t> &bs);

 private:
  const double kMaxDiff;
  double pre_value_;
  InputBitStream* input_bit_stream_;
};

#endif  // NET_SERF_QT_DECOMPRESSOR_H
