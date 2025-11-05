# CC2530 XDATA内存优化说明

## 问题分析

**错误信息**：`Segment XDATA_Z (size: 0x5dd7 align: 0) is too long for segment definition`

**原因**：CC2530的XDATA内存有限（约8KB），而我们的代码使用了过多XDATA内存。

## 内存优化策略

### 1. 减少数组大小
- `MAX_ARRAY_SIZE`: 2048 → 512
- `MAX_TRAJECTORY_POINTS`: 2000 → 200
- `MAX_POINTS`: 1000 → 50
- `BLOCK_SIZE`: 100 → 25

### 2. 减少缓冲区大小
- `OutputBitStream`: 1000*8 → 200*8
- `ChimpCompressor indices`: 限制最大256个元素

### 3. 减少测试数据
- 轨迹点数量：100 → 50
- 压缩块大小：100 → 25

## 优化后的内存使用

### 数组内存使用
```cpp
// 优化前
Array<uint32_t> data_(2048);           // 8KB
trajectory_point_t data[2000];        // 16KB
trajectory_point_t test_data[1000];   // 8KB

// 优化后  
Array<uint32_t> data_(512);           // 2KB
trajectory_point_t data[200];         // 1.6KB
trajectory_point_t test_data[50];     // 400B
```

### 总内存使用估算
- **优化前**: ~32KB (超出CC2530限制)
- **优化后**: ~4KB (在CC2530范围内)

## 修改的文件

### 核心文件
- `src/utils/array.h` - MAX_ARRAY_SIZE: 2048→512
- `src/utils/file_reader.c` - MAX_TRAJECTORY_POINTS: 2000→200, 数据点: 100→50

### 测试文件
- `test/unit_test/iar_serf_test.c` - MAX_POINTS: 1000→50, BLOCK_SIZE: 100→25

### 算法文件
- `test/baselines/chimp128/chimp_compressor.cc` - 缓冲区: 1000→200, 限制indices大小

### 项目配置
- `TrajectoryCompression_CC2530.ewp` - 更新宏定义

## 性能影响

### 优势
- ✅ 内存使用大幅减少
- ✅ 适合CC2530的8KB XDATA限制
- ✅ 编译链接成功

### 限制
- ⚠️ 测试数据量减少（50个点 vs 1000个点）
- ⚠️ 压缩块大小减小（25 vs 100）
- ⚠️ 可能影响压缩效果测试

## 验证方法

1. **编译测试**：确保无XDATA溢出错误
2. **功能测试**：验证压缩解压缩功能正常
3. **性能测试**：检查压缩比是否合理
4. **内存测试**：确认XDATA使用在限制内

## 使用建议

### 开发阶段
- 使用小数据集进行算法验证
- 逐步增加数据量测试内存限制
- 监控XDATA使用情况

### 生产环境
- 根据实际需求调整数据大小
- 考虑使用外部存储（EEPROM/Flash）
- 实现数据分块处理

## 预期结果

优化后应该能够：
- ✅ 成功编译和链接
- ✅ 无XDATA内存溢出错误
- ✅ 正常运行压缩测试
- ✅ 输出合理的压缩比

## 进一步优化建议

如果仍需要处理更大数据集：

1. **分块处理**：将大数据集分成小块处理
2. **外部存储**：使用EEPROM或Flash存储数据
3. **流式处理**：边读边压缩，不全部加载到内存
4. **算法优化**：使用更节省内存的压缩算法

现在代码应该能够在CC2530的8KB XDATA限制内成功编译和运行！












