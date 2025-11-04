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
  
  int32_t zigzag_value = ZigZagCodec::Encode(q) + 1;
  int bits_written = EliasGammaCodec::Encode(zigzag_value, output_bit_stream_);
  printf("AddValue: q=%ld, zigzag_value=%ld, bits_written=%d, total_bits=%lu\n", 
         (long)q, (long)zigzag_value, bits_written, 
         (unsigned long)(compressed_size_in_bits_ + bits_written));
  compressed_size_in_bits_ += bits_written;
  pre_value_ = recoverValue;
}

const Array<uint8_t>& SerfQtCompressor::compressed_bytes() const {
  return compressed_bytes_;
}

void SerfQtCompressor::Close() {
  output_bit_stream_->Flush();
  uint32_t buffer_len = (uint32_t)ceilf(compressed_size_in_bits_ / 8.0f);
  
  // 先释放compressed_bytes_的缓冲区（如果存在）
  if (compressed_bytes_.is_valid()) {
    compressed_bytes_ = Array<uint8_t>(0);
  }
  
  // 创建新的缓冲区 - 使用swap避免赋值操作符的问题
  Array<uint8_t> temp_array(buffer_len);
  if (temp_array.is_valid()) {
    compressed_bytes_.swap(temp_array);
  }
  
  if (!compressed_bytes_.is_valid()) {
    output_bit_stream_->Refresh();
    first_ = true;
    pre_value_ = 2.0f;
    stored_compressed_size_in_bits_ = compressed_size_in_bits_;
    compressed_size_in_bits_ = 0;
    return;
  }
  
  // 直接复制数据到compressed_bytes_，避免临时对象
  uint8_t* dest_ptr = compressed_bytes_.begin();
  bool copy_success = output_bit_stream_->CopyBufferTo(dest_ptr, buffer_len);
  if (!copy_success) {
    printf("Close: CopyBufferTo failed\n");
  }
  
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
