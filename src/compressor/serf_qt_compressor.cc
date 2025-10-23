#include "serf_qt_compressor.h"
#include <stdlib.h>
#include <stdio.h>

SerfQtCompressor::SerfQtCompressor(uint16_t block_size, float max_diff) 
    : kBlockSize(block_size), kMaxDiff(max_diff * 0.999f) {
  // IAR适配：使用malloc替代std::make_unique
  output_bit_stream_ = new OutputBitStream(2 * block_size * 8);
  first_ = true;
  pre_value_ = 2.0f;
  compressed_size_in_bits_ = 0;
  stored_compressed_size_in_bits_ = 0;
}

void SerfQtCompressor::AddValue(float v) {
  if (first_) {
    first_ = false;
    printf("Write: block_size=%lu, max_diff=%f\n", (unsigned long)kBlockSize, kMaxDiff);
    compressed_size_in_bits_ += output_bit_stream_->WriteInt(kBlockSize, 16);
    uint32_t max_diff_bits = Double::FloatToLongBits(kMaxDiff);
    compressed_size_in_bits_ += output_bit_stream_->WriteLong(max_diff_bits, 32);
  }
  
  // IAR适配：使用float替代double，减少计算开销
  int32_t q = (int32_t)roundf((v - pre_value_) / (2.0f * kMaxDiff));
  float recoverValue = pre_value_ + 2.0f * kMaxDiff * (float)q;
  
  compressed_size_in_bits_ += EliasGammaCodec::Encode(ZigZagCodec::Encode(q) + 1,
                                                      output_bit_stream_);
  pre_value_ = recoverValue;
}

Array<uint8_t> SerfQtCompressor::compressed_bytes() {
  return compressed_bytes_;
}

void SerfQtCompressor::Close() {
  output_bit_stream_->Flush();
  uint32_t buffer_len = (uint32_t)ceilf(compressed_size_in_bits_ / 8.0f);
  printf("Close: compressed_size_in_bits=%lu, buffer_len=%lu\n", 
         (unsigned long)compressed_size_in_bits_, (unsigned long)buffer_len);
  compressed_bytes_ = output_bit_stream_->GetBuffer(buffer_len);
  printf("Close: compressed_bytes valid=%s, length=%lu\n", 
         compressed_bytes_.is_valid() ? "yes" : "no", (unsigned long)compressed_bytes_.length());
  output_bit_stream_->Refresh();
  first_ = true;
  pre_value_ = 2.0f;
  stored_compressed_size_in_bits_ = compressed_size_in_bits_;
  compressed_size_in_bits_ = 0;
}

uint32_t SerfQtCompressor::get_compressed_size_in_bits() const {
  return stored_compressed_size_in_bits_;
}

// 析构函数
SerfQtCompressor::~SerfQtCompressor() {
  if (output_bit_stream_ != NULL) {
    delete output_bit_stream_;
  }
}
