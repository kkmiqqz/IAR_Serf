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
  printf("Decompress: input length=%lu, valid_bits=%lu\n", (unsigned long)bs.length(), (unsigned long)valid_bits);
  input_bit_stream_->SetBuffer(bs);
  if (valid_bits > 0) {
    input_bit_stream_->SetValidBits(valid_bits);
  }
  
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
  
  printf("Decompress: returning array, valid=%d, length=%u\n",
         decompressed_value_list.is_valid(), decompressed_value_list.length());
  if (decompressed_value_list.is_valid() && decompressed_value_list.length() > 0) {
    printf("  values: [0]=%f", decompressed_value_list[0]);
    if (decompressed_value_list.length() > 1) {
      printf(", [1]=%f", decompressed_value_list[1]);
    }
    printf("\n");
  }
  
  return decompressed_value_list;
}

bool SerfQtDecompressor::DecompressTo(const Array<uint8_t> &bs, Array<float> &output, uint32_t valid_bits) {
  printf("DecompressTo: input length=%lu, valid_bits=%lu\n", (unsigned long)bs.length(), (unsigned long)valid_bits);
  input_bit_stream_->SetBuffer(bs);
  if (valid_bits > 0) {
    input_bit_stream_->SetValidBits(valid_bits);
  }
  
  block_size_ = input_bit_stream_->ReadInt(16);
  uint32_t max_diff_bits = input_bit_stream_->ReadLong(32);
  max_diff_ = Double::LongBitsToFloat(max_diff_bits);
  
  printf("DecompressTo: block_size=%lu, max_diff=%f\n", (unsigned long)block_size_, max_diff_);
  
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
  
  printf("DecompressTo: success, decompressed %u values\n", block_size_);
  return true;
}

void SerfQtDecompressor::Clear() {
  if (input_bit_stream_ != NULL) {
    input_bit_stream_->Clear();
  }
}

float SerfQtDecompressor::NextValue() {
  printf("NextValue: called\n");
  int32_t eliasGammaValue = EliasGammaCodec::Decode(input_bit_stream_);
  printf("NextValue: EliasGamma decoded=%ld\n", (long)eliasGammaValue);
  int32_t decodeValue = ZigZagCodec::Decode(eliasGammaValue - 1);
  printf("NextValue: ZigZag decoded=%ld\n", (long)decodeValue);
  float recoverValue = pre_value_ + 2.0f * max_diff_ * (float)decodeValue;
  printf("NextValue: recoverValue=%f\n", recoverValue);
  pre_value_ = recoverValue;
  return recoverValue;
}
