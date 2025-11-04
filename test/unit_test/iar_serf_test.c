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
#define MAX_POINTS 10      // 进一步减少到10，避免XDATA空间不足
#define BLOCK_SIZE 10      // 从25减少到10
#define MAX_DIFF_LATITUDE 0.0001f   // 纬度最大误差
#define MAX_DIFF_LONGITUDE 0.0001f  // 经度最大误差

// 全局变量存储测试数据
// 移除__xdata，使用CODE区域（ROM）存储测试数据，避免XDATA空间不足
static trajectory_point_t test_data[MAX_POINTS];
static uint16_t test_data_count = 0;

// 全局静态缓冲区，用于中转压缩数据（避免栈溢出）
// 不使用__xdata，避免占用XDATA栈空间
// 减小到64字节，与MAX_ARRAY_SIZE匹配
static uint8_t g_lat_compressed_data[64];
static uint8_t g_lon_compressed_data[64];

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
    
    // 只读取前2个点进行测试
    while (file_reader_read_next(&reader, &point) && count < 2) {
        test_data[count] = point;
        count++;
        printf("Loaded point %u: lat=%.6f, lon=%.6f\n", count, point.latitude, point.longitude);
    }
    
    test_data_count = count;
    printf("loaded %u points for testing\n", count);
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
    
    // 流式压缩：一个点压缩之后立马就解压缩
    printf("Starting stream compression and decompression test...\n");
    
    // 先压缩并保存纬度数据
    DEBUG_PRINTF("start compress latdata...\n");
    uint32_t lat_compressed_bits;
    uint16_t lat_compressed_len;
    {
        SerfQtCompressor lat_compressor(test_data_count, MAX_DIFF_LATITUDE);
        for (uint16_t i = 0; i < test_data_count; i++) {
            lat_compressor.AddValue(test_data[i].latitude);
        }
        lat_compressor.Close();
        lat_compressed_bits = lat_compressor.get_compressed_size_in_bits();
        
        // 复制压缩数据到全局缓冲区
        const Array<uint8_t>& temp_lat = lat_compressor.compressed_bytes();
        lat_compressed_len = temp_lat.length();
        printf("lat_compressed: valid=%d, length=%u\n", temp_lat.is_valid(), lat_compressed_len);
        if (temp_lat.is_valid() && lat_compressed_len <= 64) {
            for (uint16_t i = 0; i < lat_compressed_len; i++) {
                g_lat_compressed_data[i] = temp_lat[i];
            }
            printf("Copied %u bytes to g_lat_compressed_data\n", lat_compressed_len);
            printf("First 4 bytes: %02X %02X %02X %02X\n", 
                   g_lat_compressed_data[0], g_lat_compressed_data[1], 
                   g_lat_compressed_data[2], g_lat_compressed_data[3]);
        } else {
            lat_compressed_len = 0;
            printf("Failed to copy lat compressed data\n");
        }
        // lat_compressor 在此析构，释放缓冲区
    }
    
    // 再压缩并保存经度数据
    DEBUG_PRINTF("start compress londata...\n");
    uint32_t lon_compressed_bits;
    uint16_t lon_compressed_len;
    {
        SerfQtCompressor lon_compressor(test_data_count, MAX_DIFF_LONGITUDE);
        for (uint16_t i = 0; i < test_data_count; i++) {
            lon_compressor.AddValue(test_data[i].longitude);
        }
        lon_compressor.Close();
        lon_compressed_bits = lon_compressor.get_compressed_size_in_bits();
        
        // 复制压缩数据到全局缓冲区
        const Array<uint8_t>& temp_lon = lon_compressor.compressed_bytes();
        lon_compressed_len = temp_lon.length();
        printf("lon_compressed: valid=%d, length=%u\n", temp_lon.is_valid(), lon_compressed_len);
        if (temp_lon.is_valid() && lon_compressed_len <= 64) {
            for (uint16_t i = 0; i < lon_compressed_len; i++) {
                g_lon_compressed_data[i] = temp_lon[i];
            }
            printf("Copied %u bytes to g_lon_compressed_data\n", lon_compressed_len);
            printf("First 4 bytes: %02X %02X %02X %02X\n", 
                   g_lon_compressed_data[0], g_lon_compressed_data[1], 
                   g_lon_compressed_data[2], g_lon_compressed_data[3]);
        } else {
            lon_compressed_len = 0;
            printf("Failed to copy lon compressed data\n");
        }
        // lon_compressor 在此析构，释放缓冲区
    }
    
    // 创建Array包装器指向静态缓冲区数据（仅用于接口兼容）
    // 注意：这里不使用Array的缓冲区池，而是直接使用静态数据
    Array<uint8_t> lat_compressed;
    Array<uint8_t> lon_compressed;
    
    // 添加调试信息
    DEBUG_PRINTF("lat_compressed_len: %u\n", lat_compressed_len);
    DEBUG_PRINTF("lon_compressed_len: %u\n", lon_compressed_len);
    
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
    
    // 解压缩纬度数据，然后立即验证并释放
    uint16_t lat_decomp_count = 0;
    float lat_values[10];  // 使用固定数组避免Array缓冲池冲突
    {
        Array<float> lat_decompressed(test_data_count);
        if (lat_decompressed.is_valid()) {
            SerfQtDecompressor lat_decompressor;
            Array<uint8_t> temp_lat_array(lat_compressed_len);
            if (temp_lat_array.is_valid()) {
                for (uint16_t i = 0; i < lat_compressed_len; i++) {
                    temp_lat_array[i] = g_lat_compressed_data[i];
                }
                if (lat_decompressor.DecompressTo(temp_lat_array, lat_decompressed, lat_compressed_bits)) {
                    lat_decomp_count = lat_decompressed.length();
                    // 复制到固定数组
                    for (uint16_t i = 0; i < lat_decomp_count && i < 10; i++) {
                        lat_values[i] = lat_decompressed[i];
                    }
                    printf("Lat decompressed successfully: %u values\n", lat_decomp_count);
                }
            }
            lat_decompressor.Clear();
        }
        // lat_decompressed在此析构，释放缓冲池
    }
    
    // 解压缩经度数据
    uint16_t lon_decomp_count = 0;
    float lon_values[10];  // 使用固定数组避免Array缓冲池冲突
    {
        Array<float> lon_decompressed(test_data_count);
        if (lon_decompressed.is_valid()) {
            SerfQtDecompressor lon_decompressor;
            Array<uint8_t> temp_lon_array(lon_compressed_len);
            if (temp_lon_array.is_valid()) {
                for (uint16_t i = 0; i < lon_compressed_len; i++) {
                    temp_lon_array[i] = g_lon_compressed_data[i];
                }
                if (lon_decompressor.DecompressTo(temp_lon_array, lon_decompressed, lon_compressed_bits)) {
                    lon_decomp_count = lon_decompressed.length();
                    // 复制到固定数组
                    for (uint16_t i = 0; i < lon_decomp_count && i < 10; i++) {
                        lon_values[i] = lon_decompressed[i];
                    }
                    printf("Lon decompressed successfully: %u values\n", lon_decomp_count);
                }
            }
            lon_decompressor.Clear();
        }
        // lon_decompressed在此析构，释放缓冲池
    }

    
    printf("decompress count - lat: %u, lon: %u\n", lat_decomp_count, lon_decomp_count);
    
    // 验证解压缩结果
    if (lat_decomp_count == 0 || lon_decomp_count == 0) {
        printf("??? Compression and decompression test FAILED!\n");
        printf("Reason: Decompression failed - lat count=%u, lon count=%u\n",
               lat_decomp_count, lon_decomp_count);
        return;
    }
    
    uint16_t error_count = 0;
    float max_lat_error = 0.0f;
    float max_lon_error = 0.0f;
    
    // 使用固定数组进行验证
    for (uint16_t i = 0; i < test_data_count && i < lat_decomp_count && i < lon_decomp_count; i++) {
        float lat_error = fabsf(test_data[i].latitude - lat_values[i]);
        float lon_error = fabsf(test_data[i].longitude - lon_values[i]);
        
        printf("Point %u: original(%.6f, %.6f), decompressed(%.6f, %.6f), error(%.6f, %.6f)\n",
               i, test_data[i].latitude, test_data[i].longitude,
               lat_values[i], lon_values[i], lat_error, lon_error);
        
        if (lat_error > MAX_DIFF_LATITUDE || lon_error > MAX_DIFF_LONGITUDE) {
            error_count++;
        }
        
        if (lat_error > max_lat_error) max_lat_error = lat_error;
        if (lon_error > max_lon_error) max_lon_error = lon_error;
    }
    
    printf("\nVerification results:\n");
    printf("error_count: %u / %u\n", error_count, test_data_count);
    printf("max_lat_error: %.6f (limit: %.6f)\n", max_lat_error, MAX_DIFF_LATITUDE);
    printf("max_lon_error: %.6f (limit: %.6f)\n", max_lon_error, MAX_DIFF_LONGITUDE);
    
    if (error_count == 0) {
        printf("=== Compression and decompression test SUCCESS! ===\n");
    } else {
        printf("??? Compression and decompression test FAILED!\n");
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
