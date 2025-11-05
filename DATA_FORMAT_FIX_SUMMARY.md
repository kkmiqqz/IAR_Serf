# 数据存储格式问题修复

## 🔍 问题根本原因

从调试信息中发现了关键问题：

```
GetBuffer: data_[0]=0x5DDDDC00
GetBuffer: data_ptr[0-7]=0xE00070BD0800204A  // 问题在这里！
```

**根本原因**：
- `data_[0]=0x5DDDDC00` 的字节内容是 `0xE00070BD0800204A`
- 我们的位流数据不是按32位字存储的，而是按字节存储的
- 但我们在读取时把它当作32位字来处理，导致数据格式不匹配

## 🔧 修复方案

### 问题分析
位流数据存储格式与读取格式不匹配，需要重新设计数据存储格式。

### 修复代码
1. **GetBuffer中按字节存储**：
```cpp
// 将32位数据转换为字节数组
for (uint32_t i = 0; i < cursor_; i++) {
  uint32_t word = data_[i];
  ret_ptr[i * 4 + 0] = (word >> 24) & 0xFF;
  ret_ptr[i * 4 + 1] = (word >> 16) & 0xFF;
  ret_ptr[i * 4 + 2] = (word >> 8) & 0xFF;
  ret_ptr[i * 4 + 3] = word & 0xFF;
}
```

2. **SetBuffer中按字节读取**：
```cpp
// 将字节数组转换为32位数据
for (uint32_t i = 0; i < new_buffer.length() / 4; i++) {
  uint32_t word = ((uint32_t)input_ptr[i * 4 + 0] << 24) |
                  ((uint32_t)input_ptr[i * 4 + 1] << 16) |
                  ((uint32_t)input_ptr[i * 4 + 2] << 8) |
                  (uint32_t)input_ptr[i * 4 + 3];
  data_[i] = word;
}
```

## 🎯 预期结果

修复后应该看到：
```
GetBuffer: data_[0]=0x5DDDDC00
GetBuffer: ret[0-3]=0x5DDDDC00  // 正确的字节存储
SetBuffer: data_[0]=0x5DDDDC00  // 正确的字节读取
Peek: len=16, buffer_=0x5DDDDC00, result=0x00000032  // 正确的读取
解压: block_size=50, max_diff=0.000100  // 正确的解压缩
```

## 🚀 下一步

重新编译运行，应该会看到：
1. ✅ 正确的数据存储格式
2. ✅ 正确的数据读取格式
3. ✅ 成功的解压缩
4. ✅ 正确的验证结果

这将完成整个SERF-QT压缩算法的IAR适配！












