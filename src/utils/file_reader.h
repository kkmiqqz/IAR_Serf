#ifndef FILE_READER_H
#define FILE_READER_H

#include <stdint.h>
#include <stdbool.h>

// 轨迹点结构体
typedef struct {
    float latitude;
    float longitude;
} trajectory_point_t;

// 文件读取器结构体
typedef struct {
    const char* filename;
    uint16_t current_line;
    uint16_t total_points;
    bool file_opened;
} file_reader_t;

// 最大轨迹点数量
#define MAX_TRAJECTORY_POINTS 200

// 函数声明
bool file_reader_init(file_reader_t* reader, const char* filename);
bool file_reader_read_next(file_reader_t* reader, trajectory_point_t* point);
void file_reader_reset(file_reader_t* reader);
void file_reader_close(file_reader_t* reader);
uint16_t file_reader_get_total_points(file_reader_t* reader);

#endif // FILE_READER_H
