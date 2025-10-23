#include "file_reader.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 假设数据文件已经预加载到内存中
// 由于CC2530内存限制，我们使用静态数组存储轨迹数据
#define MAX_TRAJECTORY_POINTS 200  // 从2000减少到200

// 静态存储轨迹数据 - 使用XDATA节省内部RAM
__xdata static trajectory_point_t trajectory_data[MAX_TRAJECTORY_POINTS];
__xdata static uint16_t trajectory_count = 0;
__xdata static bool data_loaded = false;

// 预加载轨迹数据（在实际嵌入式环境中，这可能是从EEPROM或Flash读取）
static bool load_trajectory_data_internal(const char* filename) {
    if (data_loaded) {
        return true; // 数据已经加载
    }
    
    // 模拟数据加载 - 在实际实现中，这里会从存储设备读取
    // 为了演示，我们使用一些示例数据
    trajectory_count = 50; // 示例：50个点（从100减少到50）
    
    // 生成示例轨迹数据（实际应用中从文件读取）
    for (uint16_t i = 0; i < trajectory_count; i++) {
        trajectory_data[i].latitude = 40.013867f + (i * 0.0001f);
        trajectory_data[i].longitude = 116.306473f + (i * 0.0001f);
    }
    
    data_loaded = true;
    return true;
}

bool file_reader_init(file_reader_t* reader, const char* filename) {
    if (reader == NULL || filename == NULL) {
        return false;
    }
    
    reader->filename = filename;
    reader->current_line = 0;
    reader->total_points = 0;
    reader->file_opened = false;
    
    // 加载轨迹数据
    if (!load_trajectory_data_internal(filename)) {
        return false;
    }
    
    reader->total_points = trajectory_count;
    reader->file_opened = true;
    
    return true;
}

bool file_reader_read_next(file_reader_t* reader, trajectory_point_t* point) {
    if (reader == NULL || point == NULL || !reader->file_opened) {
        return false;
    }
    
    if (reader->current_line >= reader->total_points) {
        return false; // 已到达文件末尾
    }
    
    // 从预加载的数据中读取
    *point = trajectory_data[reader->current_line];
    reader->current_line++;
    
    return true;
}

void file_reader_reset(file_reader_t* reader) {
    if (reader != NULL) {
        reader->current_line = 0;
    }
}

void file_reader_close(file_reader_t* reader) {
    if (reader != NULL) {
        reader->file_opened = false;
        reader->current_line = 0;
    }
}

uint16_t file_reader_get_total_points(file_reader_t* reader) {
    if (reader != NULL) {
        return reader->total_points;
    }
    return 0;
}
