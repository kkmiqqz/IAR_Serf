#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

// IAR适配：使用C风格头文件替代C++ STL
#include "../../src/compressor/serf_qt_compressor.h"
#include "../../src/decompressor/serf_qt_decompressor.h"
#include "../../src/utils/file_reader.h"

// 调试输出宏
#ifdef DEBUG_MODE
#define DEBUG_PRINTF(format, ...) printf(format, ##__VA_ARGS__)
#else
#define DEBUG_PRINTF(format, ...) (void)0
#endif

// 测试参数
#define MAX_POINTS 50      // 从1000减少到50
#define BLOCK_SIZE 25      // 从100减少到25
#define MAX_DIFF_LATITUDE 0.0001f   // 纬度最大误差
#define MAX_DIFF_LONGITUDE 0.0001f  // 经度最大误差

// 全局变量存储测试数据
__xdata static trajectory_point_t test_data[MAX_POINTS];
__xdata static uint16_t test_data_count = 0;

// 加载测试数据
bool load_test_data(const char* filename) {
    file_reader_t reader;
    trajectory_point_t point;
    uint16_t count = 0;
    
    DEBUG_PRINTF("start load: %s\n", filename);
    
    if (!file_reader_init(&reader, filename)) {
        printf("error: cantread\n");
        return false;
    }
    
    test_data_count = file_reader_get_total_points(&reader);
    DEBUG_PRINTF("total_data_count: %u\n", test_data_count);
    
    // 读取数据到全局数组
    while (file_reader_read_next(&reader, &point) && count < MAX_POINTS) {
        test_data[count] = point;
        count++;
    }
    
    test_data_count = count;
    file_reader_close(&reader);
    
    DEBUG_PRINTF("have load %u point\n", test_data_count);
    return true;
}

// 测试serf-QT压缩算法 - 流式压缩和解压缩
void test_serf_qt_compression(void) {
    printf("\n=== SERF-QT compress test ===\n");
    
    if (test_data_count == 0) {
        printf("error: no data\n");
        return;
    }
    
    // 创建压缩器 - 使用实际数据量作为block_size
    SerfQtCompressor lat_compressor(test_data_count, MAX_DIFF_LATITUDE);
    SerfQtCompressor lon_compressor(test_data_count, MAX_DIFF_LONGITUDE);
    
    // 流式压缩：一个点压缩之后立马就解压缩
    printf("Starting stream compression and decompression test...\n");
    
    // 压缩纬度数据
    DEBUG_PRINTF("start compress latdata...\n");
    for (uint16_t i = 0; i < test_data_count; i++) {
        lat_compressor.AddValue(test_data[i].latitude);
    }
    lat_compressor.Close();
    
    // 压缩经度数据
    DEBUG_PRINTF("start compress londata...\n");
    for (uint16_t i = 0; i < test_data_count; i++) {
        lon_compressor.AddValue(test_data[i].longitude);
    }
    lon_compressor.Close();
    
    // 获取压缩结果
    Array<uint8_t> lat_compressed = lat_compressor.compressed_bytes();
    Array<uint8_t> lon_compressed = lon_compressor.compressed_bytes();
    
    // 添加调试信息
    DEBUG_PRINTF("lat_compressed valid: %s, length: %u\n", 
                 lat_compressed.is_valid() ? "yes" : "no", lat_compressed.length());
    DEBUG_PRINTF("lon_compressed valid: %s, length: %u\n", 
                 lon_compressed.is_valid() ? "yes" : "no", lon_compressed.length());
    
    uint32_t lat_compressed_bits = lat_compressor.get_compressed_size_in_bits();
    uint32_t lon_compressed_bits = lon_compressor.get_compressed_size_in_bits();
    
    // 计算压缩比
    uint32_t original_size_bytes = test_data_count * sizeof(float) * 2; // 纬度和经度
    uint32_t compressed_size_bytes = (lat_compressed_bits + lon_compressed_bits) / 8;
    float compression_ratio = (float)compressed_size_bytes / (float)original_size_bytes * 100.0f;
    
    printf("original_size: %lu bytes\n", (unsigned long)original_size_bytes);
    printf("compressed_size: %lu bytes\n", (unsigned long)compressed_size_bytes);
    printf("compression_ratio: %.2f%%\n", compression_ratio);
    printf("lat_compressed_bits: %lu\n", (unsigned long)lat_compressed_bits);
    printf("lon_compressed_bits: %lu\n", (unsigned long)lon_compressed_bits);
    
    // 测试解压缩
    printf("\n=== SERF-QT decompress test ===\n");
    
    SerfQtDecompressor lat_decompressor;
    SerfQtDecompressor lon_decompressor;
    
    Array<float> lat_decompressed = lat_decompressor.Decompress(lat_compressed);
    Array<float> lon_decompressed = lon_decompressor.Decompress(lon_compressed);
    
    if (!lat_decompressed.is_valid() || !lon_decompressed.is_valid()) {
        printf("error: decompress fault\n");
        return;
    }
    
    printf("decompress count - lat: %u, lon: %u\n", 
           lat_decompressed.length(), lon_decompressed.length());
    
    // 添加调试信息
    DEBUG_PRINTF("block_size: %u\n", test_data_count);
    DEBUG_PRINTF("lat_decompressed valid: %s\n", lat_decompressed.is_valid() ? "yes" : "no");
    DEBUG_PRINTF("lon_decompressed valid: %s\n", lon_decompressed.is_valid() ? "yes" : "no");
    DEBUG_PRINTF("lat_decompressed length: %u\n", lat_decompressed.length());
    DEBUG_PRINTF("lon_decompressed length: %u\n", lon_decompressed.length());
    
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
    
    printf("Verification results:\n");
    printf("error_count: %u / %u\n", error_count, test_data_count);
    printf("max_lat_error: %.6f\n", max_lat_error);
    printf("max_lon_error: %.6f\n", max_lon_error);
    
    if (error_count == 0) {
        printf("✅ Compression and decompression test SUCCESS!\n");
    } else {
        printf("❌ Compression and decompression test FAILED!\n");
    }
}

// 主测试函数
int main(void) {
    printf("=== IAR EW8051 Trajectory Compression Test ===\n");
    
    // 加载测试数据
    if (!load_test_data("20081023234104.plt")) {
        printf("error: cant load data\n");
        return -1;
    }
    
    // 运行serf-QT测试
    test_serf_qt_compression();
    
    printf("\n=== test finish ===\n");
    return 0;
}
