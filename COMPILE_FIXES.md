# IAR EW8051 编译错误修复说明

## 已修复的问题

### 1. 头文件路径问题
- 修复了所有头文件的相对路径引用
- 在IAR项目配置中添加了正确的include路径

### 2. C++特性兼容性问题
- `nullptr` → `NULL`
- `malloc`/`free` 函数未定义 → 添加 `#include <stdlib.h>`
- `uint64_t`/`int64_t` 未定义 → 添加 `#include <stdint.h>`
- `NAN` 未定义 → 使用 `0.0f / 0.0f` 替代

### 3. 内存管理问题
- 添加了析构函数声明
- 修复了Array类的内存管理
- 使用 `{0}` 替代 `{}` 初始化

## 修复的文件

### 核心文件
- `src/utils/array.h` - 添加stdlib.h，修复nullptr
- `src/utils/double.h` - 修复NAN定义
- `src/compressor/serf_qt_compressor.h` - 修复头文件路径，添加析构函数
- `src/decompressor/serf_qt_decompressor.h` - 修复头文件路径
- `test/baselines/chimp128/chimp_compressor.h` - 修复头文件路径
- `test/baselines/chimp128/chimp_compressor.cc` - 修复NAN使用

### 测试文件
- `test/unit_test/iar_serf_test.c` - 修复头文件路径

### 项目配置
- `TrajectoryCompression_CC2530.ewp` - 添加include路径配置

## IAR项目配置要点

### 编译器选项
```
-DDEBUG_MODE
-DMAX_ARRAY_SIZE=2048
-DMAX_TRAJECTORY_POINTS=1000
-O2
--code_model=small
--data_model=small
--core=8051
```

### Include路径
```
src
src/utils
src/compressor
src/decompressor
test/baselines/chimp128
```

### 链接器选项
```
--code_model=small
--data_model=small
--core=8051
--stack_size=512
--heap_size=1024
```

## 编译步骤

1. 在IAR EW8051中打开 `TrajectoryCompression_CC2530.ewp`
2. 选择Debug或Release配置
3. 确保所有include路径正确设置
4. 编译项目

## 预期结果

编译成功后，程序将：
- 加载100个示例轨迹点
- 使用SERF-QT算法压缩经度和纬度数据
- 输出压缩比和性能统计
- 验证解压缩结果的准确性

## 注意事项

1. 确保IAR EW8051版本支持C++11特性
2. 如果仍有编译错误，检查IAR的C++标准设置
3. 内存使用量已优化，适合CC2530的8KB RAM限制
4. 所有浮点运算已优化为float类型，减少计算开销









