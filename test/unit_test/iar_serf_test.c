#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

// IAR适配：使用C风格头文件替代C++ STL
#include "../../src/compressor/serf_qt_compressor.h"
#include "../../src/decompressor/serf_qt_decompressor.h"
#include "../../src/compressor/net_serf_qt_compressor.h"
#include "../../src/decompressor/net_serf_qt_decompressor.h"
#include "../../src/utils/file_reader.h"

// 调试输出宏
#ifdef DEBUG_MODE
#define DEBUG_PRINTF(format, ...) printf(format, ##__VA_ARGS__)
#else
#define DEBUG_PRINTF(format, ...) (void)0
#endif

// 测试参数
#define MAX_POINTS 50      // 恢复到50个点
#define BLOCK_SIZE 50      // 恢复到50
#define MAX_DIFF_LATITUDE 0.0001f   // 纬度最大误差
#define MAX_DIFF_LONGITUDE 0.0001f  // 经度最大误差

// 全局变量存储测试数据
// 移除__xdata，使用CODE区域（ROM）存储测试数据，避免XDATA空间不足
static trajectory_point_t test_data[MAX_POINTS];
static uint16_t test_data_count = 0;

// 全局静态缓冲区，用于中转压缩数据（避免栈溢出）
// 不使用__xdata，避免占用XDATA栈空间
// 限制为128字节以节省XDATA
static uint8_t g_lat_compressed_data[128];
static uint8_t g_lon_compressed_data[128];

// 全局静态缓冲区，用于存储解压缩结果（避免栈溢出）
static float g_lat_decompressed[50];
static float g_lon_decompressed[50];

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
    
    // 读取所有数据点进行流式测试
    while (file_reader_read_next(&reader, &point) && count < MAX_POINTS) {
        test_data[count] = point;
        count++;
        if (count <= 2 || count == MAX_POINTS) {
            printf("Loaded point %u: lat=%.6f, lon=%.6f\n", count, point.latitude, point.longitude);
        }
    }
    
    test_data_count = count;
    printf("loaded %u points for testing\n", count);
    file_reader_close(&reader);
    
    DEBUG_PRINTF("have load %u point\n", test_data_count);
    return true;
}

// 测试serf-QT压缩算法 - 流式压缩和解压缩
void test_serf_qt_compression(void) {
    printf("Entering test_serf_qt_compression, test_data_count=%u\n", test_data_count);
    
    if (test_data_count == 0) {
        printf("error: no data\n");
        return;
    }
    
    printf("\n=== SERF-QT compress test ===\n");
    
    // 流式压缩：一个点压缩之后立马就解压缩
    printf("Starting stream compression and decompression test...\n");
    
    // 先压缩并保存纬度数据
    printf("Start compressing latitude data (%u points)...\n", test_data_count);
    uint32_t lat_compressed_bits;
    uint16_t lat_compressed_len;
    {
        printf("Creating lat_compressor...\n");
        SerfQtCompressor lat_compressor(test_data_count, MAX_DIFF_LATITUDE);
        printf("Adding %u values...\n", test_data_count);
        for (uint16_t i = 0; i < test_data_count; i++) {
            lat_compressor.AddValue(test_data[i].latitude);
            if (i % 10 == 9) {
                printf("  processed %u/%u points\n", i+1, test_data_count);
            }
        }
        printf("Closing compressor...\n");
        lat_compressor.Close();
        lat_compressed_bits = lat_compressor.get_compressed_size_in_bits();
        
        // 复制压缩数据到全局缓冲区
        const Array<uint8_t>& temp_lat = lat_compressor.compressed_bytes();
        lat_compressed_len = temp_lat.length();
        printf("lat_compressed: valid=%d, length=%u\n", temp_lat.is_valid(), lat_compressed_len);
        if (temp_lat.is_valid() && lat_compressed_len <= 128) {
            for (uint16_t i = 0; i < lat_compressed_len; i++) {
                g_lat_compressed_data[i] = temp_lat[i];
            }
            printf("Copied %u bytes to g_lat_compressed_data\n", lat_compressed_len);
            printf("First 4 bytes: %02X %02X %02X %02X\n", 
                   g_lat_compressed_data[0], g_lat_compressed_data[1], 
                   g_lat_compressed_data[2], g_lat_compressed_data[3]);
        } else {
            lat_compressed_len = 0;
            printf("Failed to copy lat compressed data (length=%u exceeds 128)\n", lat_compressed_len);
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
        if (temp_lon.is_valid() && lon_compressed_len <= 128) {
            for (uint16_t i = 0; i < lon_compressed_len; i++) {
                g_lon_compressed_data[i] = temp_lon[i];
            }
            printf("Copied %u bytes to g_lon_compressed_data\n", lon_compressed_len);
            printf("First 4 bytes: %02X %02X %02X %02X\n", 
                   g_lon_compressed_data[0], g_lon_compressed_data[1], 
                   g_lon_compressed_data[2], g_lon_compressed_data[3]);
        } else {
            lon_compressed_len = 0;
            printf("Failed to copy lon compressed data (length=%u exceeds 128)\n", lon_compressed_len);
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
                    // 复制到全局数组
                    for (uint16_t i = 0; i < lat_decomp_count && i < 50; i++) {
                        g_lat_decompressed[i] = lat_decompressed[i];
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
                    // 复制到全局数组
                    for (uint16_t i = 0; i < lon_decomp_count && i < 50; i++) {
                        g_lon_decompressed[i] = lon_decompressed[i];
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
    
    // 使用全局数组进行验证
    for (uint16_t i = 0; i < test_data_count && i < lat_decomp_count && i < lon_decomp_count; i++) {
        float lat_error = fabsf(test_data[i].latitude - g_lat_decompressed[i]);
        float lon_error = fabsf(test_data[i].longitude - g_lon_decompressed[i]);
        
        printf("Point %u: original(%.6f, %.6f), decompressed(%.6f, %.6f), error(%.6f, %.6f)\n",
               i, test_data[i].latitude, test_data[i].longitude,
               g_lat_decompressed[i], g_lon_decompressed[i], lat_error, lon_error);
        
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

// 测试Net-SERF-QT流式压缩算法
void test_net_serf_qt_streaming(void) {
    printf("\n=== Net-SERF-QT streaming test ===\n");
    
    if (test_data_count == 0) {
        printf("error: no data\n");
        return;
    }
    
    // 创建持久的压缩器和解压缩器（保持pre_value连续性）
    NetSerfQtCompressor lat_compressor(MAX_DIFF_LATITUDE);
    NetSerfQtCompressor lon_compressor(MAX_DIFF_LONGITUDE);
    NetSerfQtDecompressor lat_decompressor(MAX_DIFF_LATITUDE);
    NetSerfQtDecompressor lon_decompressor(MAX_DIFF_LONGITUDE);
    
    // 统计变量
    uint32_t total_compressed_bytes = 0;
    uint16_t error_count = 0;
    float max_lat_error = 0.0f;
    float max_lon_error = 0.0f;
    
    // 流式处理：逐点压缩和解压缩
    printf("Processing %u points in streaming mode...\n", test_data_count);
    
    for (uint16_t i = 0; i < test_data_count; i++) {
        double lat_decompressed, lon_decompressed;
        uint16_t lat_len, lon_len;
        
        // 压缩和解压缩纬度（使用作用域确保临时Array及时释放）
        {
            Array<uint8_t> lat_compressed = lat_compressor.Compress((double)test_data[i].latitude);
            lat_len = lat_compressed.length();
            total_compressed_bytes += lat_len;
            lat_decompressed = lat_decompressor.Decompress(lat_compressed);
            // lat_compressed在此自动析构，释放内存
        }
        
        // 压缩和解压缩经度（使用作用域确保临时Array及时释放）
        {
            Array<uint8_t> lon_compressed = lon_compressor.Compress((double)test_data[i].longitude);
            lon_len = lon_compressed.length();
            total_compressed_bytes += lon_len;
            lon_decompressed = lon_decompressor.Decompress(lon_compressed);
            // lon_compressed在此自动析构，释放内存
        }
        
        // 计算误差
        float lat_error = fabsf(test_data[i].latitude - (float)lat_decompressed);
        float lon_error = fabsf(test_data[i].longitude - (float)lon_decompressed);
        
        if (lat_error > MAX_DIFF_LATITUDE || lon_error > MAX_DIFF_LONGITUDE) {
            error_count++;
        }
        
        if (lat_error > max_lat_error) max_lat_error = lat_error;
        if (lon_error > max_lon_error) max_lon_error = lon_error;
        
        // 显示前几个点和最后一个点的结果
        if (i < 3 || i == test_data_count - 1) {
            printf("Point %u: original(%.6f, %.6f), decompressed(%.9f, %.9f), compressed_bytes=%u+%u\n",
                   i, test_data[i].latitude, test_data[i].longitude,
                   lat_decompressed, lon_decompressed, lat_len, lon_len);
            printf("         error=(%.9f, %.9f)\n", 
                   fabs(test_data[i].latitude - lat_decompressed),
                   fabs(test_data[i].longitude - lon_decompressed));
        }
        
        // 每10个点报告一次进度
        if ((i + 1) % 10 == 0) {
            uint32_t current_original_size_double = (i + 1) * 8 * 2;  // 按double(8字节)计算
            printf("Processed %u/%u points, current compression ratio vs double: %.2f%% (%lu/%lu bytes)\n",
                   i + 1, test_data_count,
                   (float)total_compressed_bytes / (float)current_original_size_double * 100.0f,
                   (unsigned long)total_compressed_bytes,
                   (unsigned long)current_original_size_double);
        }
    }
    
    // 计算压缩率（按double计算原始大小）
    // 硬编码：double=8字节，float=4字节（在某些平台上sizeof可能不准确）
    uint32_t original_size_double = test_data_count * 8 * 2;  // 8字节/double
    uint32_t original_size_float = test_data_count * 4 * 2;    // 4字节/float
    float compression_ratio_vs_double = (float)total_compressed_bytes / (float)original_size_double * 100.0f;
    float compression_ratio_vs_float = (float)total_compressed_bytes / (float)original_size_float * 100.0f;
    
    printf("\n=== Streaming test results ===\n");
    printf("Points processed: %u\n", test_data_count);
    printf("Original size (double, 8 bytes): %lu bytes\n", (unsigned long)original_size_double);
    printf("Original size (float, 4 bytes): %lu bytes\n", (unsigned long)original_size_float);
    printf("Compressed size: %lu bytes\n", (unsigned long)total_compressed_bytes);
    printf("Compression ratio vs double: %.2f%%\n", compression_ratio_vs_double);
    printf("Compression ratio vs float: %.2f%%\n", compression_ratio_vs_float);
    printf("Error count: %u / %u\n", error_count, test_data_count);
    printf("Max lat error: %.6f (limit: %.6f)\n", max_lat_error, MAX_DIFF_LATITUDE);
    printf("Max lon error: %.6f (limit: %.6f)\n", max_lon_error, MAX_DIFF_LONGITUDE);
    
    if (error_count == 0) {
        printf("=== Streaming test SUCCESS! ===\n");
    } else {
        printf("??? Streaming test FAILED!\n");
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
    
    // 运行流式测试（更适合嵌入式平台）
    test_net_serf_qt_streaming();
    
    printf("\n=== test finish ===\n");
    return 0;
}
