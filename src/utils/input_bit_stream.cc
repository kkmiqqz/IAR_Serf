#include "input_bit_stream.h"
#include <stdio.h>

// IAR适配：使用自定义字节序转换函数
static inline uint32_t be32toh(uint32_t big_endian_32bits) {
    return ((big_endian_32bits & 0xFF000000) >> 24) |
           ((big_endian_32bits & 0x00FF0000) >> 8)  |
           ((big_endian_32bits & 0x0000FF00) << 8)  |
           ((big_endian_32bits & 0x000000FF) << 24);
}

InputBitStream::InputBitStream() {
  buffer_ = 0;
  cursor_ = 0;
  bit_in_buffer_ = 0;
}

InputBitStream::InputBitStream(uint8_t *raw_data, uint32_t size) {
  data_ = Array<uint32_t>((uint32_t)ceilf((float)size / sizeof(uint32_t)));
  if (data_.is_valid()) {
    memcpy(data_.begin(), raw_data, size);
    for (uint32_t i = 0; i < data_.length(); i++) {
      data_[i] = be32toh(data_[i]);
    }
    buffer_ = data_[0];
    cursor_ = 1;
    bit_in_buffer_ = 32;
  } else {
    buffer_ = 0;
    cursor_ = 0;
    bit_in_buffer_ = 0;
  }
}

uint32_t InputBitStream::Peek(uint32_t len) {
  if (len == 0) return 0;
  if (len > 32) len = 32; // 限制最大32位
  
  // 修复位操作逻辑：与Write函数格式匹配
  uint32_t result;
  if (len == 32) {
    result = buffer_;
  } else {
    // 从高位开始读取len位，与Write函数格式匹配
    uint32_t mask = (len == 32) ? 0xFFFFFFFF : ((1U << len) - 1);
    result = (buffer_ >> (32 - len)) & mask;
  }
  
  return result;
}

void InputBitStream::Forward(uint32_t len) {
  if (len == 0) return;
  
  // 如果请求的位数超过缓冲区中的位数，限制到可用位数
  if (len > bit_in_buffer_) {
    len = bit_in_buffer_;
  }
  
  // 左移缓冲区，移除已读取的位（与Write函数格式匹配）
  buffer_ <<= len;
  bit_in_buffer_ -= len;
  
  // 如果缓冲区中的位数不足32位，尝试加载更多数据
  if (bit_in_buffer_ < 32 && cursor_ < data_.length()) {
    uint32_t next_word = data_[cursor_++];
    buffer_ |= (next_word << bit_in_buffer_);
    bit_in_buffer_ += 32;
  }
}

uint32_t InputBitStream::ReadLong(uint32_t len) {
  if (len == 0) return 0;
  if (len > 32) {
    // 对于超过32位的情况，只读取低32位
    uint32_t ret = Peek(32);
    Forward(32);
    return ret;
  }
  uint32_t ret = Peek(len);
  Forward(len);
  return ret;
}

uint32_t InputBitStream::ReadInt(uint32_t len) {
  uint32_t ret = Peek(len);
  Forward(len);
  return ret;
}

bool InputBitStream::ReadBit() {
  bool ret = (buffer_ & 0x1) != 0;  // 修复：从低位开始读取，与WriteBit格式匹配
  Forward(1);
  return ret;
}

void InputBitStream::SetBuffer(const Array<uint8_t> &new_buffer) {
  if (new_buffer.is_valid()) {
  printf("SetBuffer: input length=%lu\n", (unsigned long)new_buffer.length());
    data_ = Array<uint32_t>((uint32_t)ceilf((float)new_buffer.length() / sizeof(uint32_t)));
    if (data_.is_valid()) {
      // 重新设计数据存储格式：按字节存储位流数据
      uint8_t* input_ptr = (uint8_t*)new_buffer.begin();
      
      // 将字节数组转换为32位数据，确保不超出边界
      uint32_t words_to_copy = (new_buffer.length() + 3) / 4;
      if (words_to_copy > data_.length()) words_to_copy = data_.length();
      
      for (uint32_t i = 0; i < words_to_copy; i++) {
        uint32_t word = 0;
        for (uint32_t j = 0; j < 4 && (i * 4 + j) < new_buffer.length(); j++) {
          // 修复：使用大端序格式，与GetBuffer格式匹配
          word |= ((uint32_t)input_ptr[i * 4 + j]) << ((3 - j) * 8);
        }
        data_[i] = word;
      }
      printf("SetBuffer: data_[0]=0x%08lX\n", (unsigned long)data_[0]);
      buffer_ = data_[0];
      cursor_ = 1;
      bit_in_buffer_ = 32;  // 修复：初始化为32，表示缓冲区有32位可用数据
    }
  }
}