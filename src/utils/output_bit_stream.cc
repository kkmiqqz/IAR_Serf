#include "output_bit_stream.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

// IAR适配：使用LSB-first位流（与参考代码一致）
// 使用简单的字节buffer，每个字节内LSB优先

OutputBitStream::OutputBitStream(uint32_t buffer_size) {
  // data_存储字节数据，但使用uint32_t数组来利用ArrayBufferPool
  uint16_t array_size = buffer_size / 4 + 1;
  Array<uint32_t> temp_array(array_size);
  data_.swap(temp_array);
  
  if (!data_.is_valid()) {
    printf("OutputBitStream: ERROR - failed to allocate data array\n");
  }
  
  // 清零
  if (data_.is_valid()) {
    uint32_t* ptr = data_.begin();
    if (ptr) {
      memset(ptr, 0, data_.length() * 4);
    }
  }
  
  buffer_ = 0;  // 当前正在构建的字节
  cursor_ = 0;  // 字节索引
  bit_in_buffer_ = 0;  // 当前字节内的位索引（0-7）
}

// LSB-first写入：从最低位开始写入
uint32_t OutputBitStream::Write(uint32_t content, uint32_t len) {
  if (len == 0 || len > 32) return 0;
  
  uint8_t* byte_buffer = (uint8_t*)data_.begin();
  if (byte_buffer == NULL) return 0;
  
  // 逐位写入（从LSB开始）
  for (uint32_t i = 0; i < len; i++) {
    uint32_t bit = (content >> i) & 1;
    
    if (bit) {
      buffer_ |= (1 << bit_in_buffer_);
    }
    
    bit_in_buffer_++;
    
    // 如果当前字节满了，存储并移到下一个字节
    if (bit_in_buffer_ >= 8) {
      byte_buffer[cursor_] = (uint8_t)buffer_;
      cursor_++;
      bit_in_buffer_ = 0;
      buffer_ = 0;
    }
  }
  
  return len;
}

uint32_t OutputBitStream::WriteLong(uint32_t content, uint32_t len) {
  return Write(content, len);
}

uint32_t OutputBitStream::WriteInt(uint32_t content, uint32_t len) {
  return Write(content, len);
}

uint32_t OutputBitStream::WriteBit(bool bit) {
  return Write(bit ? 1 : 0, 1);
}

void OutputBitStream::Flush() {
  if (bit_in_buffer_ > 0) {
    uint8_t* byte_buffer = (uint8_t*)data_.begin();
    if (byte_buffer) {
      byte_buffer[cursor_] = (uint8_t)buffer_;
      cursor_++;
      bit_in_buffer_ = 0;
      buffer_ = 0;
    }
  }
}

Array<uint8_t> OutputBitStream::GetBuffer(uint32_t len) {
  Flush();
  
  Array<uint8_t> ret(len);
  if (!ret.is_valid()) {
    return ret;
  }
  
  // 直接复制字节数据
  uint8_t* ret_ptr = ret.begin();
  uint8_t* data_ptr = (uint8_t*)data_.begin();
  if (ret_ptr && data_ptr) {
    memcpy(ret_ptr, data_ptr, len);
  }
  
  return ret;
}

bool OutputBitStream::CopyBufferTo(uint8_t* dest, uint32_t len) {
  if (dest == NULL || len == 0) {
    return false;
  }
  
  // 确保所有位都已写入
  Flush();
  
  uint8_t* data_ptr = (uint8_t*)data_.begin();
  if (data_ptr == NULL) {
    return false;
  }
  
  // 直接复制字节
  memcpy(dest, data_ptr, len);
  
  return true;
}

void OutputBitStream::Refresh() {
  cursor_ = 0;
  bit_in_buffer_ = 0;
  buffer_ = 0;
  
  // 清零data_数组
  if (data_.is_valid()) {
    uint8_t* data_ptr = (uint8_t*)data_.begin();
    if (data_ptr) {
      memset(data_ptr, 0, data_.length() * 4);
    }
  }
}
