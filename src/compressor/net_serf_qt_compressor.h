#ifndef NET_SERF_QT_COMPRESSOR_H
#define NET_SERF_QT_COMPRESSOR_H

#include <stdint.h>

// IAR适配：移除C++11特性
#include "../utils/array.h"
#include "../utils/output_bit_stream.h"
#include "../utils/double.h"
#include "../utils/elias_gamma_codec.h"
#include "../utils/zig_zag_codec.h"

class NetSerfQtCompressor {
 public:
  explicit NetSerfQtCompressor(double error_bound);
  ~NetSerfQtCompressor();

  // 支持double输入（GPS坐标通常使用double）
  Array<uint8_t> Compress(double v);

 private:
  const double kMaxDiff;
  double pre_value_;
  OutputBitStream* output_bit_stream_;
};

#endif  // NET_SERF_QT_COMPRESSOR_H
