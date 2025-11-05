#include "net_serf_qt_compressor.h"
#include <math.h>
#include <stdio.h>

NetSerfQtCompressor::NetSerfQtCompressor(double error_bound) 
    : kMaxDiff(error_bound * 0.999), pre_value_(2.0) {
  output_bit_stream_ = new OutputBitStream(16);
}

NetSerfQtCompressor::~NetSerfQtCompressor() {
  if (output_bit_stream_ != NULL) {
    delete output_bit_stream_;
  }
}

Array<uint8_t> NetSerfQtCompressor::Compress(double v) {
  int written_bits_count = output_bit_stream_->WriteInt(0, 4);
  
  // 内部计算使用float以优化性能（对于GPS坐标精度足够）
  int32_t q = (int32_t)roundf((float)((v - pre_value_) / (2.0 * kMaxDiff)));
  double recover_value = pre_value_ + 2.0 * kMaxDiff * (double)q;
  
  written_bits_count += EliasGammaCodec::Encode(ZigZagCodec::Encode(q) + 1, output_bit_stream_);
  pre_value_ = recover_value;
  output_bit_stream_->Flush();
  Array<uint8_t> result = output_bit_stream_->GetBuffer((uint16_t)ceilf(written_bits_count / 8.0f));
  output_bit_stream_->Refresh();
  return result;
}
