# 位流字节序问题修复

## 问题分析

调试信息显示：
- **写入正确**：`block_size=50`, `max_diff=0.000100`
- **读取错误**：`block_size=57344`, `max_diff_bits=0x00000000`

## 根本原因

**双重字节序转换**：
1. OutputBitStream在GetBuffer时使用`htobe32`转换
2. InputBitStream在SetBuffer时又使用`be32toh`转换
3. 导致数据被错误地转换了两次

## 修复方案

### 1. 移除InputBitStream的字节序转换
```cpp
// 修复前 - 双重转换
void InputBitStream::SetBuffer(const Array<uint8_t> &new_buffer) {
  memcpy(data_.begin(), new_buffer.begin(), new_buffer.length());
  for (uint32_t i = 0; i < data_.length(); i++) {
    data_[i] = be32toh(data_[i]);  // 第二次转换
  }
}

// 修复后 - 单次转换
void InputBitStream::SetBuffer(const Array<uint8_t> &new_buffer) {
  memcpy(data_.begin(), new_buffer.begin(), new_buffer.length());
  // 不进行字节序转换，因为OutputBitStream已经转换过了
  buffer_ = data_[0];
}
```

### 2. 修复printf格式
```cpp
// 修复前
printf("MAX_ARRAY_SIZE=%d\n", MAX_ARRAY_SIZE);

// 修复后  
printf("MAX_ARRAY_SIZE=%u\n", (uint32_t)MAX_ARRAY_SIZE);
```

## 预期结果

修复后应该看到：
```
AddValue: writing block_size=50 (16 bits)
AddValue: writing max_diff=0.000100 (32 bits)
GetBuffer: requested len=23, MAX_ARRAY_SIZE=512
Decompress: read block_size=50
Decompress: read max_diff_bits=0x3A83126F
Decompress: converted max_diff=0.000100
✓ success!
```

## 验证步骤

1. **重新编译**：确保无编译错误
2. **运行测试**：查看调试输出
3. **检查数值**：
   - block_size应该是50（不是57344）
   - max_diff_bits应该是非零值（不是0x00000000）
   - MAX_ARRAY_SIZE应该是512（不是0）

现在重新编译运行，应该能看到正确的解压缩结果！












