# 解压缩失败深度调试

## 当前状态

解压缩仍然失败，输出"error: decompress fault"，但没有显示调试信息，说明问题出现在很早期。

## 已添加的调试信息

### 1. 压缩器调试
```cpp
// 在Close()函数中
printf("Close: compressed_size_in_bits=%u, buffer_len=%u\n", compressed_size_in_bits_, buffer_len);
printf("Close: compressed_bytes valid=%s, length=%u\n", 
       compressed_bytes_.is_valid() ? "yes" : "no", compressed_bytes_.length());
```

### 2. 位流调试
```cpp
// 在GetBuffer()函数中
printf("GetBuffer: requested len=%u, MAX_ARRAY_SIZE=%d\n", len, MAX_ARRAY_SIZE);
printf("GetBuffer: failed to allocate array of size %u\n", len);
printf("GetBuffer: success, ret length=%u\n", ret.length());
```

### 3. 解压缩器调试
```cpp
// 在Decompress()函数中
printf("Decompress: block_size=%u, max_diff=%f\n", block_size_, max_diff_);
printf("Decompress: failed to create array for size %u\n", block_size_);
```

### 4. 测试文件调试
```cpp
// 在测试函数中
DEBUG_PRINTF("lat_compressed valid: %s, length: %u\n", 
             lat_compressed.is_valid() ? "yes" : "no", lat_compressed.length());
```

## 可能的问题

### 1. 内存分配失败
- Array构造函数可能因为内存不足而失败
- MAX_ARRAY_SIZE=512可能仍然太大
- malloc可能返回NULL

### 2. 位流操作问题
- Write/Read操作可能不匹配
- 字节序转换可能有问题
- 位操作可能不正确

### 3. 数据格式问题
- 压缩的数据格式与解压缩期望的不匹配
- block_size读取错误
- max_diff读取错误

## 下一步调试

1. **重新编译运行**：查看新增的调试输出
2. **分析输出**：
   - 如果看到"GetBuffer: failed to allocate" → 内存问题
   - 如果看到"Decompress: block_size=0" → 位流读取问题
   - 如果看到"Decompress: failed to create array" → 内存分配问题

3. **进一步优化**：
   - 如果内存问题，进一步减少MAX_ARRAY_SIZE
   - 如果位流问题，检查Write/Read的对称性
   - 如果数据问题，检查压缩格式

## 预期调试输出

正常情况应该看到：
```
Close: compressed_size_in_bits=182, buffer_len=23
GetBuffer: requested len=23, MAX_ARRAY_SIZE=512
GetBuffer: success, ret length=23
Close: compressed_bytes valid=yes, length=23
Decompress: block_size=50, max_diff=0.000100
```

异常情况可能看到：
```
GetBuffer: failed to allocate array of size 23
Close: compressed_bytes valid=no, length=0
error: decompress fault
```

## 临时解决方案

如果内存分配持续失败，可以：
1. 进一步减少MAX_ARRAY_SIZE到256或128
2. 使用静态数组替代动态分配
3. 分块处理数据

现在重新编译运行，调试信息会告诉我们确切的问题所在！












