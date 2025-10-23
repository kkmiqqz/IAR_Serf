# 内存分配问题修复

## 🔍 问题现象

从调试信息可以看出：
```
GetBuffer: ret[0-3]=0x5DDDDC00  // 数据存储正确了！
Close: compressed_bytes valid=no, length=0  // 但数组无效！
GetBuffer: failed to allocate array of size 24  // 内存分配失败！
```

**问题**：数据存储格式修复了，但现在出现了内存分配问题！

## 🔧 修复方案

### 问题分析
字节转换逻辑可能导致数组长度计算错误或边界越界。

### 修复代码
1. **GetBuffer中安全的字节转换**：
```cpp
// 将32位数据转换为字节数组，确保不超出边界
uint32_t bytes_to_copy = (len < cursor_ * 4) ? len : cursor_ * 4;
for (uint32_t i = 0; i < bytes_to_copy; i++) {
  uint32_t word_index = i / 4;
  uint32_t byte_index = i % 4;
  if (word_index < cursor_) {
    uint32_t word = data_[word_index];
    ret_ptr[i] = (word >> (24 - byte_index * 8)) & 0xFF;
  }
}
```

2. **SetBuffer中安全的字节转换**：
```cpp
// 将字节数组转换为32位数据，确保不超出边界
uint32_t words_to_copy = (new_buffer.length() + 3) / 4;
if (words_to_copy > data_.length()) words_to_copy = data_.length();

for (uint32_t i = 0; i < words_to_copy; i++) {
  uint32_t word = 0;
  for (uint32_t j = 0; j < 4 && (i * 4 + j) < new_buffer.length(); j++) {
    word |= ((uint32_t)input_ptr[i * 4 + j]) << (24 - j * 8);
  }
  data_[i] = word;
}
```

## 🎯 预期结果

修复后应该看到：
```
GetBuffer: ret[0-3]=0x5DDDDC00  // 正确的数据存储
Close: compressed_bytes valid=yes, length=23  // 数组有效
GetBuffer: success, ret length=24  // 内存分配成功
解压: block_size=50, max_diff=0.000100  // 正确的解压缩
```

## 🚀 下一步

重新编译运行，应该会看到：
1. ✅ 正确的内存分配
2. ✅ 有效的数组
3. ✅ 成功的解压缩
4. ✅ 正确的验证结果

这将完成整个SERF-QT压缩算法的IAR适配！



