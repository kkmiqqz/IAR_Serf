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

Array<float> SerfQtDecompressor::Decompress(const Array<uint8_t> &bs, uint32_t valid_bits) {
  input_bit_stream_->SetBuffer(bs);
  if (valid_bits > 0) {
    input_bit_stream_->SetValidBits(valid_bits);
  }
  
  block_size_ = input_bit_stream_->ReadInt(16);
  uint32_t max_diff_bits = input_bit_stream_->ReadLong(32);
  max_diff_ = Double::LongBitsToFloat(max_diff_bits);
  
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

bool SerfQtDecompressor::DecompressTo(const Array<uint8_t> &bs, Array<float> &output, uint32_t valid_bits) {
  input_bit_stream_->SetBuffer(bs);
  if (valid_bits > 0) {
    input_bit_stream_->SetValidBits(valid_bits);
  }
  
  block_size_ = input_bit_stream_->ReadInt(16);
  uint32_t max_diff_bits = input_bit_stream_->ReadLong(32);
  max_diff_ = Double::LongBitsToFloat(max_diff_bits);
  
  pre_value_ = 2.0f;
  
  // 确保output数组有足够的空间
  if (!output.is_valid() || output.length() < block_size_) {
    printf("DecompressTo: output array invalid or too small\n");
    return false;
  }
  
  // 直接写入output数组
  for (uint16_t i = 0; i < block_size_; i++) {
    output[i] = NextValue();
  }
  
  return true;
}

void SerfQtDecompressor::Clear() {
  if (input_bit_stream_ != NULL) {
    input_bit_stream_->Clear();
  }
}

float SerfQtDecompressor::NextValue() {
  int32_t eliasGammaValue = EliasGammaCodec::Decode(input_bit_stream_);
  int32_t decodeValue = ZigZagCodec::Decode(eliasGammaValue - 1);
  float recoverValue = pre_value_ + 2.0f * max_diff_ * (float)decodeValue;
  pre_value_ = recoverValue;
  return recoverValue;
}
