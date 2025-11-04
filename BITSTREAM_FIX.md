# 位流读取问题修复

## 问题分析

调试信息显示：
- `block_size=57344` - 异常大的值
- `max_diff=631349440000000000000000000000.000000` - 异常大的值
- `MAX_ARRAY_SIZE=0` - 宏定义未正确传递

## 根本原因

### 1. Peek函数错误
**问题**：`buffer_ >> (32 - len)` 读取的是高len位，不是低len位
**影响**：导致ReadInt(16)读取错误的值

### 2. ReadLong函数错误  
**问题**：复杂的位操作逻辑有误
**影响**：导致ReadLong(32)读取错误的值

### 3. MAX_ARRAY_SIZE宏定义问题
**问题**：宏定义可能被覆盖或未正确传递
**影响**：Array构造函数可能失败

## 修复方案

### 1. 修复Peek函数
```cpp
// 修复前
uint32_t InputBitStream::Peek(uint32_t len) {
  return buffer_ >> (32 - len);
}

// 修复后
uint32_t InputBitStream::Peek(uint32_t len) {
  if (len == 0) return 0;
  if (len > 32) len = 32; // 限制最大32位
  return (buffer_ >> (32 - len)) & ((1U << len) - 1);
}
```

### 2. 修复ReadLong函数
```cpp
// 修复前 - 复杂的位操作
uint32_t ret = (len > 32) * Peek(32);
Forward((len > 32) * 32);
len -= (len > 32) * 32;
ret = (ret << len) | Peek(len);

// 修复后 - 简化的逻辑
if (len > 32) {
  uint32_t ret = Peek(32);
  Forward(32);
  return ret;
}
uint32_t ret = Peek(len);
Forward(len);
return ret;
```

### 3. 修复MAX_ARRAY_SIZE宏定义
```cpp
// 在array.h中添加条件编译
#ifndef MAX_ARRAY_SIZE
#define MAX_ARRAY_SIZE 512
#endif
```

## 预期结果

修复后应该看到：
```
Decompress: block_size=50, max_diff=0.000100
GetBuffer: requested len=23, MAX_ARRAY_SIZE=512
Decompress: block_size=50, max_diff=0.000100
✓ success!
```

## 验证步骤

1. **重新编译**：确保无编译错误
2. **运行测试**：查看调试输出
3. **检查数值**：
   - block_size应该是50（不是57344）
   - max_diff应该是0.0001（不是异常大值）
   - MAX_ARRAY_SIZE应该是512（不是0）

现在重新编译运行，应该能看到正确的解压缩结果！









