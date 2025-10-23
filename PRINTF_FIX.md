# printf编译错误修复

## 问题
添加调试printf语句后，编译报错：
```
Error[Pe020]: identifier "printf" is undefined
```

## 原因
C++文件中使用printf需要包含`<stdio.h>`头文件。

## 修复
在以下文件中添加`#include <stdio.h>`：

### 1. src/compressor/serf_qt_compressor.cc
```cpp
#include "serf_qt_compressor.h"
#include <stdlib.h>
#include <stdio.h>  // 新增
```

### 2. src/decompressor/serf_qt_decompressor.cc
```cpp
#include "serf_qt_decompressor.h"
#include <stdlib.h>
#include <stdio.h>  // 新增
```

### 3. src/utils/output_bit_stream.cc
```cpp
#include "output_bit_stream.h"
#include <string.h>
#include <stdio.h>  // 新增
```

## 验证
修复后应该能够：
- ✅ 成功编译所有文件
- ✅ 运行调试版本
- ✅ 看到详细的调试输出

## 下一步
重新编译运行，现在应该能看到调试信息，帮助我们诊断解压缩失败的具体原因。



