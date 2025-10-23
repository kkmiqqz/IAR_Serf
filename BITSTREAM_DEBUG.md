# 位流问题深度调试

## 当前状态

问题仍然存在：
- `MAX_ARRAY_SIZE=0` - 宏定义问题
- `block_size=57344` - 位流读取问题
- `max_diff=0.000000` - 位流读取问题

## 已添加的修复

### 1. Write函数修复
```cpp
// 添加位掩码确保只写入指定位数
content &= ((1U << len) - 1);
// 改进位移逻辑
content <<= (32 - len);
content >>= bit_in_buffer_;
```

### 2. MAX_ARRAY_SIZE硬编码
```cpp
// 直接硬编码，避免宏定义传递问题
#define MAX_ARRAY_SIZE 512
```

### 3. 详细调试信息
- 压缩器：显示写入的block_size和max_diff
- 解压缩器：显示读取过程的每一步
- 位流：显示原始位模式和转换结果

## 预期调试输出

正常情况应该看到：
```
AddValue: writing block_size=50 (16 bits)
AddValue: writing max_diff=0.000100 (32 bits)
Decompress: input buffer length=23
Decompress: reading block_size (16 bits)...
Decompress: read block_size=50
Decompress: reading max_diff (32 bits)...
Decompress: read max_diff_bits=0x3A83126F
Decompress: converted max_diff=0.000100
Decompress: block_size=50, max_diff=0.000100
```

异常情况可能看到：
```
Decompress: read block_size=57344  // 错误
Decompress: read max_diff_bits=0x00000000  // 错误
```

## 可能的问题

### 1. 字节序问题
- OutputBitStream写入时使用htobe32转换
- InputBitStream读取时使用be32toh转换
- 转换可能不匹配

### 2. 位流对齐问题
- Write和Read的位操作可能不对齐
- 缓冲区管理可能有问题

### 3. 数据格式问题
- 压缩的数据格式与解压缩期望不匹配

## 下一步

1. **重新编译运行**：查看详细的调试输出
2. **分析输出**：
   - 如果写入50但读取57344 → 位流对齐问题
   - 如果写入0.0001但读取0 → 字节序问题
   - 如果MAX_ARRAY_SIZE仍为0 → 编译问题

3. **针对性修复**：
   - 位流对齐问题 → 修复Write/Read逻辑
   - 字节序问题 → 修复转换逻辑
   - 编译问题 → 检查IAR设置

现在重新编译运行，详细的调试信息会告诉我们确切的问题！



