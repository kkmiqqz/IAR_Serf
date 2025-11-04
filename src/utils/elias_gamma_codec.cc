#include "elias_gamma_codec.h"
#include <stdio.h>
// 定义静态成员变量：log2查找表
const float EliasGammaCodec::kLog2Table[17] = {
  0.0f,           // log2(0) - 不使用
  0.0f,           // log2(1) = 0
  1.0f,           // log2(2) = 1
  1.5849625f,     // log2(3) ≈ 1.585
  2.0f,           // log2(4) = 2
  2.3219281f,     // log2(5) ≈ 2.322
  2.5849625f,     // log2(6) ≈ 2.585
  2.8073549f,     // log2(7) ≈ 2.807
  3.0f,           // log2(8) = 3
  3.1699250f,     // log2(9) ≈ 3.170
  3.3219281f,     // log2(10) ≈ 3.322
  3.4594316f,     // log2(11) ≈ 3.459
  3.5849625f,     // log2(12) ≈ 3.585
  3.7004397f,     // log2(13) ≈ 3.700
  3.8073549f,     // log2(14) ≈ 3.807
  3.9068906f,     // log2(15) ≈ 3.907
  4.0f            // log2(16) = 4
};

int EliasGammaCodec::Encode(int32_t number, OutputBitStream *output_bit_stream_ptr) {
  int compressed_size_in_bits = 0;
  int n;
  if (number <= 16) {
    n = (int)floorf(kLog2Table[number]);
  } else {
    n = (int)floorf(log2f((float)number));
  }
  
  // 标准Elias Gamma编码（LSB-first）：
  // 1. 写入n个0
  // 2. 写入number的完整二进制表示（n+1位，从LSB开始）
  
  // 写入n个0
  if (n > 0) {
    compressed_size_in_bits += output_bit_stream_ptr->WriteInt(0, n);
  }
  
  // 写入完整的number（n+1位）
  compressed_size_in_bits += output_bit_stream_ptr->WriteInt((uint32_t)number, n + 1);
  
  return compressed_size_in_bits;
}

int32_t EliasGammaCodec::Decode(InputBitStream *input_bit_stream_ptr) {
  int n = 0;
  // 计数前导0（LSB-first）
  while (true) {
    if (!input_bit_stream_ptr->HasMoreData()) {
      // 数据流结束
      return 0;
    }
    
    bool bit = input_bit_stream_ptr->ReadBit();
    if (!bit) {
      // 0，继续计数
      n++;
    } else {
      // 遇到1，停止循环
      break;
    }
  }
  
  // LSB-first Elias Gamma解码：
  // 编码：写入n个0，然后写入number（n+1位，LSB-first）
  // 解码：读取0直到遇到1（这个1是number的LSB），然后读取n位
  // 重建：result = 1 | (remainder << 1)
  
  if (n == 0) {
    return 1;
  }
  
  uint32_t remainder = input_bit_stream_ptr->ReadInt(n);
  
  // LSB-first：遇到的1是number的bit0（LSB），remainder是bit1到bit_n
  // 重建：result = 1 | (remainder << 1)
  uint32_t result = 1 | (remainder << 1);
  return (int32_t)result;
}
