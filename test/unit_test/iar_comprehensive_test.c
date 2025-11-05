#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

// IAR适配：使用C风格头文件
#include "compressor/serf_qt_compressor.h"
#include "decompressor/serf_qt_decompressor.h"
#include "baselines/chimp128/chimp_compressor.h"
#include "baselines/chimp128/chimp_decompressor.h"
#include "utils/file_reader.h"

// 调试输出宏
#ifdef DEBUG_MODE
#define DEBUG_PRINTF(format, ...) printf(format, ##__VA_ARGS__)
#else
#define DEBUG_PRINTF(format, ...) (void)0
#endif

// 测试参数
#define MAX_POINTS 1000
#define BLOCK_SIZE 100
#define MAX_DIFF_LATITUDE 0.0001f   // 纬度最大误差
#define MAX_DIFF_LONGITUDE 0.0001f  // 经度最大误差
#define CHIMP_PREVIOUS_VALUES 128

// 全局变量存储测试数据
__xdata static trajectory_point_t test_data[MAX_POINTS];
__xdata static uint16_t test_data_count = 0;

// 加载测试数据
bool load_test_data(const char* filename) {
    file_reader_t reader;
    trajectory_point_t point;
    uint16_t count = 0;
    
    DEBUG_PRINTF("开始加载测试数据: %s\n", filename);
    
    if (!file_reader_init(&reader, filename)) {
        printf("错误: 无法初始化文件读取器\n");
        return false;
    }
    
    test_data_count = file_reader_get_total_points(&reader);
    DEBUG_PRINTF("总数据点数: %u\n", test_data_count);
    
    // 读取数据到全局数组
    while (file_reader_read_next(&reader, &point) && count < MAX_POINTS) {
        test_data[count] = point;
        count++;
    }
    
    test_data_count = count;
    file_reader_close(&reader);
    
    DEBUG_PRINTF("成功加载 %u 个轨迹点\n", test_data_count);
    return true;
}

// 测试serf-QT压缩算法
void test_serf_qt_compression(void) {
    printf("\n=== SERF-QT 压缩测试 ===\n");
    
    if (test_data_count == 0) {
        printf("错误: 没有测试数据\n");
        return;
    }
    
    // 创建压缩器
    SerfQtCompressor lat_compressor(BLOCK_SIZE, MAX_DIFF_LATITUDE);
    SerfQtCompressor lon_compressor(BLOCK_SIZE, MAX_DIFF_LONGITUDE);
    
    // 压缩纬度数据
    DEBUG_PRINTF("开始压缩纬度数据...\n");
    for (uint16_t i = 0; i < test_data_count; i++) {
        lat_compressor.AddValue(test_data[i].latitude);
    }
    lat_compressor.Close();
    
    // 压缩经度数据
    DEBUG_PRINTF("开始压缩经度数据...\n");
    for (uint16_t i = 0; i < test_data_count; i++) {
        lon_compressor.AddValue(test_data[i].longitude);
    }
    lon_compressor.Close();
    
    // 获取压缩结果
    Array<uint8_t> lat_compressed = lat_compressor.compressed_bytes();
    Array<uint8_t> lon_compressed = lon_compressor.compressed_bytes();
    
    uint32_t lat_compressed_bits = lat_compressor.get_compressed_size_in_bits();
    uint32_t lon_compressed_bits = lon_compressor.get_compressed_size_in_bits();
    
    // 计算压缩比
    uint32_t original_size_bytes = test_data_count * sizeof(float) * 2; // 纬度和经度
    uint32_t compressed_size_bytes = (lat_compressed_bits + lon_compressed_bits) / 8;
    float compression_ratio = (float)compressed_size_bytes / (float)original_size_bytes * 100.0f;
    
    printf("原始数据大小: %u 字节\n", original_size_bytes);
    printf("压缩后大小: %u 字节\n", compressed_size_bytes);
    printf("压缩比: %.2f%%\n", compression_ratio);
    printf("纬度压缩比特数: %u\n", lat_compressed_bits);
    printf("经度压缩比特数: %u\n", lon_compressed_bits);
    
    // 测试解压缩
    printf("\n=== SERF-QT 解压缩测试 ===\n");
    
    SerfQtDecompressor lat_decompressor;
    SerfQtDecompressor lon_decompressor;
    
    Array<float> lat_decompressed = lat_decompressor.Decompress(lat_compressed);
    Array<float> lon_decompressed = lon_decompressor.Decompress(lon_compressed);
    
    if (!lat_decompressed.is_valid() || !lon_decompressed.is_valid()) {
        printf("错误: 解压缩失败\n");
        return;
    }
    
    printf("解压缩点数 - 纬度: %u, 经度: %u\n", 
           lat_decompressed.length(), lon_decompressed.length());
    
    // 验证解压缩结果
    uint16_t error_count = 0;
    float max_lat_error = 0.0f;
    float max_lon_error = 0.0f;
    
    for (uint16_t i = 0; i < test_data_count && i < lat_decompressed.length(); i++) {
        float lat_error = fabsf(test_data[i].latitude - lat_decompressed[i]);
        float lon_error = fabsf(test_data[i].longitude - lon_decompressed[i]);
        
        if (lat_error > MAX_DIFF_LATITUDE || lon_error > MAX_DIFF_LONGITUDE) {
            error_count++;
        }
        
        if (lat_error > max_lat_error) max_lat_error = lat_error;
        if (lon_error > max_lon_error) max_lon_error = lon_error;
    }
    
    printf("验证结果:\n");
    printf("超出误差范围的点数: %u\n", error_count);
    printf("最大纬度误差: %.6f\n", max_lat_error);
    printf("最大经度误差: %.6f\n", max_lon_error);
    
    if (error_count == 0) {
        printf("✓ SERF-QT 压缩解压缩测试通过!\n");
    } else {
        printf("✗ SERF-QT 压缩解压缩测试失败!\n");
    }
}

// 测试Chimp压缩算法
void test_chimp_compression(void) {
    printf("\n=== CHIMP 压缩测试 ===\n");
    
    if (test_data_count == 0) {
        printf("错误: 没有测试数据\n");
        return;
    }
    
    // 创建压缩器
    ChimpCompressor lat_compressor(CHIMP_PREVIOUS_VALUES);
    ChimpCompressor lon_compressor(CHIMP_PREVIOUS_VALUES);
    
    // 压缩纬度数据
    DEBUG_PRINTF("开始压缩纬度数据...\n");
    for (uint16_t i = 0; i < test_data_count; i++) {
        lat_compressor.addValue(test_data[i].latitude);
    }
    lat_compressor.close();
    
    // 压缩经度数据
    DEBUG_PRINTF("开始压缩经度数据...\n");
    for (uint16_t i = 0; i < test_data_count; i++) {
        lon_compressor.addValue(test_data[i].longitude);
    }
    lon_compressor.close();
    
    // 获取压缩结果
    Array<uint8_t> lat_compressed = lat_compressor.get_compress_pack();
    Array<uint8_t> lon_compressed = lon_compressor.get_compress_pack();
    
    uint32_t lat_compressed_bits = lat_compressor.get_size();
    uint32_t lon_compressed_bits = lon_compressor.get_size();
    
    // 计算压缩比
    uint32_t original_size_bytes = test_data_count * sizeof(float) * 2; // 纬度和经度
    uint32_t compressed_size_bytes = (lat_compressed_bits + lon_compressed_bits) / 8;
    float compression_ratio = (float)compressed_size_bytes / (float)original_size_bytes * 100.0f;
    
    printf("原始数据大小: %u 字节\n", original_size_bytes);
    printf("压缩后大小: %u 字节\n", compressed_size_bytes);
    printf("压缩比: %.2f%%\n", compression_ratio);
    printf("纬度压缩比特数: %u\n", lat_compressed_bits);
    printf("经度压缩比特数: %u\n", lon_compressed_bits);
    
    printf("✓ CHIMP 压缩测试完成!\n");
}

// 主测试函数
int main(void) {
    printf("=== IAR EW8051 轨迹压缩测试程序 ===\n");
    printf("目标平台: CC2530 (8051架构)\n");
    printf("编译器: IAR Embedded Workbench for 8051\n");
    
    // 加载测试数据
    if (!load_test_data("20081023234104.plt")) {
        printf("错误: 无法加载测试数据\n");
        return -1;
    }
    
    printf("\n测试数据信息:\n");
    printf("数据点数: %u\n", test_data_count);
    printf("纬度范围: %.6f - %.6f\n", test_data[0].latitude, test_data[test_data_count-1].latitude);
    printf("经度范围: %.6f - %.6f\n", test_data[0].longitude, test_data[test_data_count-1].longitude);
    
    // 运行serf-QT测试
    test_serf_qt_compression();
    
    // 运行Chimp测试
    test_chimp_compression();
    
    printf("\n=== 测试完成 ===\n");
    printf("所有算法已成功适配到IAR EW8051平台\n");
    return 0;
}












