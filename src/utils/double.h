#ifndef SERF_DOUBLE_H
#define SERF_DOUBLE_H

#include <stdint.h>
#include <float.h>

// IAR适配：8051不支持64位整数，使用32位替代
// 定义64位结构体用于位操作
typedef struct {
    uint32_t low;
    uint32_t high;
} uint64_struct_t;

typedef struct {
    int32_t low;
    int32_t high;
} int64_struct_t;

// IAR适配：使用C风格头文件和float替代double
class Double {
 public:
  // 使用float的NaN值 - 使用内联函数避免重复定义
  static inline float kNan() {
    static const float nan_value = -1.0f; // 使用-1.0f作为特殊值
    return nan_value;
  }

  // 将float转换为uint32_t位模式
  static inline uint32_t FloatToLongBits(float value) {
    return *(uint32_t*)(&value);
  }

  // 将uint32_t位模式转换为float
  static inline float LongBitsToFloat(uint32_t bits) {
    return *(float*)(&bits);
  }

  // 保持double版本以兼容现有代码 - 使用32位替代64位
  static inline uint64_struct_t DoubleToLongBits(double value) {
    uint64_struct_t result;
    result.low = *(uint32_t*)(&value);
    result.high = 0; // 8051不支持64位double
    return result;
  }

  static inline double LongBitsToDouble(uint64_struct_t bits) {
    return *(double*)(&bits.low);
  }
};

#endif  // SERF_DOUBLE_H
