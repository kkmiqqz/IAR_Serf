#include "input_bit_stream.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

// IAR适配：使用LSB-first位流（与参考代码一致）

InputBitStream::InputBitStream() {
  buffer_ = 0;
  cursor_ = 0;  // 字节索引
  bit_in_buffer_ = 0;  // 当前字节内的位索引（0-7）
  total_bits_read_ = 0;
  max_valid_bits_ = 0;
}

void InputBitStream::Clear() {
  data_ = Array<uint32_t>(0);
  buffer_ = 0;
  cursor_ = 0;
  bit_in_buffer_ = 0;
  total_bits_read_ = 0;
  max_valid_bits_ = 0;
}

// LSB-first读取：从最低位开始读取
uint32_t InputBitStream::Peek(uint32_t len) {
  if (len == 0) return 0;
  if (len > 32) len = 32;
  
  uint8_t* byte_buffer = (uint8_t*)data_.begin();
  if (byte_buffer == NULL) return 0;
  
  uint32_t result = 0;
  uint32_t temp_cursor = cursor_;
  uint32_t temp_bit_index = bit_in_buffer_;
  
  // 预览len位
  for (uint32_t i = 0; i < len; i++) {
    if (temp_cursor >= data_.length() * 4) {
      // 没有更多数据，剩余位默认为0
      break;
    }
    
    // 读取当前字节的当前位
    uint8_t current_byte = byte_buffer[temp_cursor];
    uint8_t bit = (current_byte >> temp_bit_index) & 1;
    result |= ((uint32_t)bit << i);
    
    temp_bit_index++;
    if (temp_bit_index >= 8) {
      temp_cursor++;
      temp_bit_index = 0;
    }
  }
  
  return result;
}

void InputBitStream::Forward(uint32_t len) {
  if (len == 0) return;
  
  // 检查max_valid_bits_限制
  if (max_valid_bits_ > 0 && total_bits_read_ >= max_valid_bits_) {
    return;
  }
  
  if (max_valid_bits_ > 0 && total_bits_read_ + len > max_valid_bits_) {
    len = max_valid_bits_ - total_bits_read_;
    if (len == 0) return;
  }
  
  // 前进len位
  bit_in_buffer_ += len;
  total_bits_read_ += len;
  
  // 更新字节位置
  while (bit_in_buffer_ >= 8) {
    cursor_++;
    bit_in_buffer_ -= 8;
  }
}

uint32_t InputBitStream::ReadLong(uint32_t len) {
  uint32_t ret = Peek(len);
  if (len > 16) {
    printf("ReadLong: Peek(%lu)=0x%08lX, cursor_=%lu, bit_in_buffer_=%lu\n",
           (unsigned long)len, (unsigned long)ret, 
           (unsigned long)cursor_, (unsigned long)bit_in_buffer_);
  }
  Forward(len);
  return ret;
}

uint32_t InputBitStream::ReadInt(uint32_t len) {
  uint32_t ret = Peek(len);
  Forward(len);
  return ret;
}

bool InputBitStream::ReadBit() {
  // 检查是否已经达到最大有效位数
  if (max_valid_bits_ > 0 && total_bits_read_ >= max_valid_bits_) {
    return false;
  }
  
  bool ret = (Peek(1) != 0);
  Forward(1);
  return ret;
}

bool InputBitStream::HasMoreData() const {
  // 如果设置了最大有效位数，检查是否已经达到
  if (max_valid_bits_ > 0 && total_bits_read_ >= max_valid_bits_) {
    return false;
  }
  // 检查是否还有数据
  return cursor_ < data_.length() * 4;
}

void InputBitStream::SetBuffer(const Array<uint8_t> &new_buffer) {
  if (new_buffer.is_valid()) {
    // 重置状态
    buffer_ = 0;
    cursor_ = 0;
    bit_in_buffer_ = 0;
    total_bits_read_ = 0;
    max_valid_bits_ = 0;
    
    // 计算需要的uint32_t数量
    uint16_t words_needed = (uint16_t)ceilf(new_buffer.length() / 4.0f);
    
    // 释放旧的data_
    if (data_.is_valid()) {
      data_ = Array<uint32_t>(0);
    }
    
    // 分配新的data_ - 使用swap避免赋值操作符问题
    Array<uint32_t> temp_array(words_needed);
    if (!temp_array.is_valid()) {
      printf("SetBuffer: ERROR - failed to create array of %u words\n", words_needed);
      return;
    }
    data_.swap(temp_array);
    
    // 直接复制字节数据
    uint32_t* data_ptr = data_.begin();
    const uint8_t* src_ptr = new_buffer.begin();
    if (data_ptr && src_ptr) {
      memset(data_ptr, 0, words_needed * 4);
      memcpy(data_ptr, src_ptr, new_buffer.length());
      
      printf("SetBuffer: copied %u bytes, data_[0]=0x%08lX\n", 
             new_buffer.length(), (unsigned long)data_ptr[0]);
    }
  } else {
    buffer_ = 0;
    cursor_ = 0;
    bit_in_buffer_ = 0;
    total_bits_read_ = 0;
    max_valid_bits_ = 0;
  }
}

void InputBitStream::SetValidBits(uint32_t valid_bits) {
  max_valid_bits_ = valid_bits;
}
