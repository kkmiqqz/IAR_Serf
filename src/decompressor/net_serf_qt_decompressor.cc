#include "net_serf_qt_decompressor.h"

NetSerfQtDecompressor::NetSerfQtDecompressor(double max_diff) 
    : kMaxDiff(max_diff * 0.999), pre_value_(2.0) {
  input_bit_stream_ = new InputBitStream();
}

NetSerfQtDecompressor::~NetSerfQtDecompressor() {
  if (input_bit_stream_ != NULL) {
    delete input_bit_stream_;
  }
}

double NetSerfQtDecompressor::Decompress(Array<uint8_t> &bs) {
  input_bit_stream_->SetBuffer(bs);
  input_bit_stream_->ReadInt(4);
  int32_t eliasGammaValue = EliasGammaCodec::Decode(input_bit_stream_);
  int32_t decodeValue = ZigZagCodec::Decode(eliasGammaValue - 1);
  pre_value_ = pre_value_ + 2.0 * kMaxDiff * (double)decodeValue;
  return pre_value_;
}
