#ifndef SERF_INPUT_BIT_STREAM_H
#define SERF_INPUT_BIT_STREAM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// IAR适配：移除STL依赖，使用C风格头文件
#include "array.h"

class InputBitStream {
 public:
  InputBitStream();

  InputBitStream(uint8_t *raw_data, uint32_t size);

  uint32_t ReadLong(uint32_t len);

  uint32_t ReadInt(uint32_t len);

  bool ReadBit();

  void SetBuffer(const Array<uint8_t> &new_buffer);
  
  // 设置有效位数（用于限制读取，避免读取填充位）
  void SetValidBits(uint32_t valid_bits);
  
  // 检查是否还有有效数据可读
  bool HasMoreData() const;
  
  // 获取已读取的总位数和最大有效位数
  uint32_t GetTotalBitsRead() const { return total_bits_read_; }
  uint32_t GetMaxValidBits() const { return max_valid_bits_; }
  
  // 清除内部缓冲区，释放内存
  void Clear();

 private:
  void Forward(uint32_t len);
  uint32_t Peek(uint32_t len);

  Array<uint32_t> data_;
  uint32_t buffer_; // 使用32位替代64位
  uint32_t cursor_;
  uint32_t bit_in_buffer_;
  uint32_t total_bits_read_;  // 已读取的总位数
  uint32_t max_valid_bits_;   // 最大有效位数（如果设置了）
};

#endif  // SERF_INPUT_BIT_STREAM_H
