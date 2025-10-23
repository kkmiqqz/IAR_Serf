#include "output_bit_stream.h"
#include <string.h>
#include <stdio.h>

OutputBitStream::OutputBitStream(uint32_t buffer_size) {
  data_ = Array<uint32_t>(buffer_size / 4 + 1);
  buffer_ = 0;
  cursor_ = 0;
  bit_in_buffer_ = 0;
}

uint32_t OutputBitStream::Write(uint32_t content, uint32_t len) {
  if (len == 0) return 0;
  if (len > 32) len = 32;
  
  // 确保content只包含len位
  if (len == 32) {
    // 32位数据不需要掩码
  } else {
    content &= ((1U << len) - 1);
  }
  
  // 修复位流格式：将content放在缓冲区的低位
  buffer_ |= (content << bit_in_buffer_);
  bit_in_buffer_ += len;
  
  // 如果缓冲区满了，存储并重置
  if (bit_in_buffer_ >= 32) {
    data_[cursor_++] = buffer_;
    buffer_ = 0;
    bit_in_buffer_ -= 32;
  }
  
  return len;
}

uint32_t OutputBitStream::WriteLong(uint32_t content, uint32_t len) {
  if (len == 0) return 0;
  if (len > 32) {
    // 分两次写入，避免位移过大
    uint32_t high_bits = len - 32;
    if (high_bits <= 32) {
      Write(content >> high_bits, 32);
      Write(content, high_bits);
    } else {
      Write(content, 32); // 只写入低32位
    }
    return len;
  }
  return Write(content, len);
}

uint32_t OutputBitStream::WriteInt(uint32_t content, uint32_t len) {
  return Write(content, len);
}

uint32_t OutputBitStream::WriteBit(bool bit) {
  return Write(bit ? 1 : 0, 1);
}

Array<uint8_t> OutputBitStream::GetBuffer(uint32_t len) {
  printf("GetBuffer: requested len=%lu, MAX_ARRAY_SIZE=%lu\n", 
         (unsigned long)len, (unsigned long)MAX_ARRAY_SIZE);
  printf("GetBuffer: cursor_=%lu, data_[0]=0x%08lX\n", 
         (unsigned long)cursor_, (unsigned long)data_[0]);
  Array<uint8_t> ret(len);
  if (!ret.is_valid()) {
    printf("GetBuffer: failed to allocate array of size %lu\n", (unsigned long)len);
    return Array<uint8_t>(0); // 返回空数组
  }
  
  // 重新设计数据存储格式：按字节存储位流数据
  uint8_t* ret_ptr = ret.begin();
  uint32_t* data_ptr = data_.begin();
  
  // 将32位数据转换为字节数组，确保不超出边界
  uint32_t bytes_to_copy = (len < cursor_ * 4) ? len : cursor_ * 4;
  for (uint32_t i = 0; i < bytes_to_copy; i++) {
    uint32_t word_index = i / 4;
    uint32_t byte_index = i % 4;
    if (word_index < cursor_) {
      uint32_t word = data_ptr[word_index];  // 使用data_ptr而不是data_[word_index]
      // 修复：使用大端序格式，确保位流数据正确传输
      ret_ptr[i] = (word >> ((3 - byte_index) * 8)) & 0xFF;
    }
  }
  
  return ret;
}

void OutputBitStream::Flush() {
  if (bit_in_buffer_) {
    data_[cursor_++] = buffer_;
    buffer_ = 0;
    bit_in_buffer_ = 0;
  }
}

void OutputBitStream::Refresh() {
  cursor_ = 0;
  bit_in_buffer_ = 0;
  buffer_ = 0;
}