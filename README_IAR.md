# IAR EW8051 轨迹压缩项目

本项目将原始的PC端C++轨迹压缩代码适配到IAR Embedded Workbench for 8051 (EW8051)平台，目标硬件为CC2530。

## 项目概述

- **目标平台**: IAR EW8051 (CC2530)
- **测试数据**: 20081023234104.plt (轨迹数据文件)
- **压缩算法**: SERF-QT (主要算法) + Chimp (对比算法)
- **数据类型**: 经度和纬度浮点数并行压缩

## 主要修改

### 1. 移除C++ STL依赖
- `std::vector` → `Array<T>` (固定大小数组)
- `std::unique_ptr` → 原始指针 + 手动内存管理
- `std::string` → `char[]` (C风格字符串)
- `std::make_unique` → `new` + `delete`

### 2. 数据类型优化
- `double` → `float` (减少计算开销)
- `int` → `uint16_t`/`int16_t` (明确位宽)
- `long` → `uint32_t`/`int32_t`
- `size_t` → `uint32_t`

### 3. 内存管理
- 使用`__xdata`关键字将大型数组放置在外部RAM
- 限制动态内存分配，优先使用静态分配
- 添加内存分配失败检查

### 4. 平台适配
- 移除`endian.h`依赖，实现自定义字节序转换
- 使用IAR兼容的数学函数 (`logf`, `powf`, `ceilf`)
- 添加IAR特定的内存模型支持

## 文件结构

```
src/
├── utils/
│   ├── array.h              # 固定大小数组类
│   ├── file_reader.h/c      # 文件读取模块
│   ├── output_bit_stream.h/cc  # 位流输出
│   ├── input_bit_stream.h/cc   # 位流输入
│   ├── elias_gamma_codec.h/cc   # Elias Gamma编码
│   ├── zig_zag_codec.h          # ZigZag编码
│   └── double.h                 # 浮点数工具
├── compressor/
│   └── serf_qt_compressor.h/cc # SERF-QT压缩器
└── decompressor/
    └── serf_qt_decompressor.h/cc # SERF-QT解压缩器

test/
├── baselines/chimp128/      # Chimp对比算法
└── unit_test/
    ├── iar_serf_test.c      # SERF-QT测试
    └── iar_comprehensive_test.c # 综合测试

Makefile.iar                 # IAR项目配置
```

## 编译和使用

### 1. 环境要求
- IAR Embedded Workbench for 8051
- CC2530开发板
- 支持printf重定向的UART配置

### 2. 编译步骤
```bash
# 使用IAR命令行工具
make -f Makefile.iar all

# 或者使用IAR IDE
# 1. 打开IAR EW8051
# 2. 创建新项目
# 3. 添加源文件
# 4. 配置编译器选项
# 5. 编译并下载到CC2530
```

### 3. 运行测试
```c
// 主测试程序会输出以下信息：
// - 数据加载状态
// - 压缩比统计
// - 解压缩验证结果
// - 算法性能对比
```

## 测试数据格式

PLT文件格式 (20081023234104.plt):
```
Geolife trajectory
WGS 84
Altitude is in Feet
Reserved 3
0,2,255,My Track,0,0,2,8421376
0
40.013867,116.306473,0,226,39744.9868518518,2008-10-23,23:41:04
40.013907,116.306488,0,168,39744.986875,2008-10-23,23:41:06
...
```

程序只使用前两列：纬度和经度。

## 性能参数

### 压缩参数
- **块大小**: 100个点
- **纬度最大误差**: 0.0001度
- **经度最大误差**: 0.0001度
- **Chimp历史值**: 128

### 内存使用
- **最大轨迹点数**: 1000
- **最大数组大小**: 2048
- **栈大小**: 512字节
- **堆大小**: 1024字节

## 调试输出

程序使用`printf`进行调试输出，通过UART重定向到PC终端：

```c
#ifdef DEBUG_MODE
#define DEBUG_PRINTF(format, ...) printf(format, ##__VA_ARGS__)
#else
#define DEBUG_PRINTF(format, ...) (void)0
#endif
```

## 注意事项

1. **内存限制**: CC2530只有8KB RAM，需要谨慎管理内存
2. **浮点运算**: 8051没有FPU，浮点运算较慢
3. **栈空间**: 避免深度递归，使用迭代替代
4. **中断安全**: 在多任务环境中注意数据保护

## 故障排除

### 常见问题
1. **内存不足**: 减少`MAX_POINTS`或`MAX_ARRAY_SIZE`
2. **编译错误**: 检查IAR版本兼容性
3. **运行时错误**: 检查UART配置和printf重定向

### 调试技巧
1. 使用IAR调试器单步执行
2. 检查内存使用情况
3. 验证数据加载和解析

## 扩展功能

可以进一步优化：
1. 添加更多baseline算法 (Gorilla, FPC等)
2. 实现自适应压缩参数
3. 添加数据完整性检查
4. 优化内存使用模式

## 许可证

本项目基于原始轨迹压缩算法，遵循相应的开源许可证。









