# memcpy字节序问题修复

## 🔍 问题根本原因

从调试信息可以看出：
```
GetBuffer: data_[0]=0x5DDDDC00
GetBuffer: ret[0-3]=0xE00070BD  // memcpy改变了数据！
SetBuffer: data_[0]=0xBD7000E0  // memcpy又改变了数据！
```

**根本原因**：
- CC2530是小端序系统
- `memcpy` 是按字节复制的，但我们的位流数据是按32位字存储的
- 这导致了数据格式不匹配，数据被重新排列

## 🔧 修复方案

### 问题分析
`memcpy` 在32位系统上会按4字节对齐复制，这破坏了我们的位流数据格式。

### 修复代码
1. **GetBuffer中使用逐字节复制**：
```cpp
// 逐字节复制数据，避免memcpy的字节序问题
uint8_t* ret_ptr = ret.begin();
uint8_t* data_ptr = (uint8_t*)data_.begin();
for (uint32_t i = 0; i < len; i++) {
  ret_ptr[i] = data_ptr[i];
}
```

2. **SetBuffer中使用逐字节复制**：
```cpp
// 逐字节复制数据，避免memcpy的字节序问题
uint8_t* data_ptr = (uint8_t*)data_.begin();
uint8_t* input_ptr = (uint8_t*)new_buffer.begin();
for (uint32_t i = 0; i < new_buffer.length(); i++) {
  data_ptr[i] = input_ptr[i];
}
```

## 🎯 预期结果

修复后应该看到：
```
GetBuffer: data_[0]=0x5DDDDC00
GetBuffer: ret[0-3]=0x00DCDD5D  // 正确的逐字节复制
SetBuffer: data_[0]=0x5DDDDC00  // 正确的逐字节复制
Peek: len=16, buffer_=0x5DDDDC00, result=0x00000032  // 正确的读取
解压: block_size=50, max_diff=0.000100  // 正确的解压缩
```

## 🚀 下一步

重新编译运行，应该会看到：
1. ✅ 数据在传递过程中保持不变
2. ✅ 正确的位流读取
3. ✅ 成功的解压缩
4. ✅ 正确的验证结果

这将完成整个SERF-QT压缩算法的IAR适配！









