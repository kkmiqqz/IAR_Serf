#include "serf_qt_decompressor.h"
#include <stdlib.h>
#include <stdio.h>

SerfQtDecompressor::SerfQtDecompressor() {
  // IAR适配：使用new替代std::make_unique
  input_bit_stream_ = new InputBitStream();
  block_size_ = 0;
  max_diff_ = 0.0f;
  pre_value_ = 2.0f;
}

SerfQtDecompressor::~SerfQtDecompressor() {
  if (input_bit_stream_ != NULL) {
    delete input_bit_stream_;
  }
}

Array<float> SerfQtDecompressor::Decompress(const Array<uint8_t> &bs) {
  printf("Decompress: input length=%lu\n", (unsigned long)bs.length());
  input_bit_stream_->SetBuffer(bs);
  
  block_size_ = input_bit_stream_->ReadInt(16);
  printf("ReadInt: read %lu (0x%04lX) as 16 bits\n", (unsigned long)block_size_, (unsigned long)block_size_);
  uint32_t max_diff_bits = input_bit_stream_->ReadLong(32);
  printf("ReadLong: read %lu (0x%08lX) as 32 bits\n", (unsigned long)max_diff_bits, (unsigned long)max_diff_bits);
  max_diff_ = Double::LongBitsToFloat(max_diff_bits);
  
  printf("Decompress: block_size=%lu, max_diff=%f\n", (unsigned long)block_size_, max_diff_);
  
  pre_value_ = 2.0f;
  
  // IAR适配：使用固定大小数组替代std::vector
  Array<float> decompressed_value_list(block_size_);
  if (!decompressed_value_list.is_valid()) {
    printf("Decompress failed: cannot create array of size %lu\n", (unsigned long)block_size_);
    return Array<float>(0); // 返回空数组
  }
  
  for (uint16_t i = 0; i < block_size_; i++) {
    decompressed_value_list[i] = NextValue();
  }
  
  return decompressed_value_list;
}

float SerfQtDecompressor::NextValue() {
  int32_t decodeValue = ZigZagCodec::Decode(EliasGammaCodec::Decode(input_bit_stream_) - 1);
  float recoverValue = pre_value_ + 2.0f * max_diff_ * (float)decodeValue;
  pre_value_ = recoverValue;
  return recoverValue;
}
