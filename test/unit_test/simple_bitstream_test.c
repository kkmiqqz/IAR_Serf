// 简化的位流实现
// 用于替换复杂的位操作逻辑

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// 简化的位流类
class SimpleBitStream {
public:
    uint32_t buffer_;
    uint32_t bit_in_buffer_;
    uint32_t data_[10];  // 固定大小数组
    uint32_t cursor_;
    
    SimpleBitStream() : buffer_(0), bit_in_buffer_(0), cursor_(0) {}
    
    uint32_t Write(uint32_t content, uint32_t len) {
        printf("SimpleWrite: content=0x%08lX, len=%lu, bit_in_buffer_=%lu\n", 
               (unsigned long)content, (unsigned long)len, (unsigned long)bit_in_buffer_);
        
        if (len == 0) return 0;
        if (len > 32) len = 32;
        
        // 确保content只包含len位
        content &= ((1U << len) - 1);
        
        // 将content左移到高位
        content <<= (32 - len);
        
        // 将content右移到正确位置
        content >>= bit_in_buffer_;
        
        // 合并到缓冲区
        buffer_ |= content;
        bit_in_buffer_ += len;
        
        printf("SimpleWrite: after buffer_=0x%08lX, bit_in_buffer_=%lu\n", 
               (unsigned long)buffer_, (unsigned long)bit_in_buffer_);
        
        // 如果缓冲区满了，存储并重置
        if (bit_in_buffer_ >= 32) {
            data_[cursor_++] = buffer_;
            printf("SimpleWrite: stored data_[%lu]=0x%08lX\n", 
                   (unsigned long)(cursor_-1), (unsigned long)data_[cursor_-1]);
            buffer_ = 0;
            bit_in_buffer_ -= 32;
        }
        
        return len;
    }
    
    void Flush() {
        printf("SimpleFlush: bit_in_buffer_=%lu, buffer_=0x%08lX\n", 
               (unsigned long)bit_in_buffer_, (unsigned long)buffer_);
        if (bit_in_buffer_ > 0) {
            data_[cursor_++] = buffer_;
            printf("SimpleFlush: stored data_[%lu]=0x%08lX\n", 
                   (unsigned long)(cursor_-1), (unsigned long)data_[cursor_-1]);
            buffer_ = 0;
            bit_in_buffer_ = 0;
        }
    }
    
    void PrintData() {
        printf("SimpleBitStream data:\n");
        for (uint32_t i = 0; i < cursor_; i++) {
            printf("  data_[%lu]=0x%08lX\n", (unsigned long)i, (unsigned long)data_[i]);
        }
    }
};

// 测试函数
int test_simple_bitstream() {
    printf("=== 简单位流测试 ===\n");
    
    SimpleBitStream stream;
    
    // 测试写入block_size=50 (16位)
    stream.Write(50, 16);
    
    // 测试写入max_diff的位模式 (32位)
    stream.Write(0x38D18167, 32);
    
    stream.Flush();
    stream.PrintData();
    
    return 0;
}