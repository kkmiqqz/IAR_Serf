#ifndef CHIMP_COMPRESSOR_H
#define CHIMP_COMPRESSOR_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <float.h>

// IAR适配：使用C风格头文件
#include "../../../src/utils/output_bit_stream.h"
#include "../../../src/utils/double.h"
#include "../../../src/utils/array.h"

class ChimpCompressor {
public:
    explicit ChimpCompressor(uint16_t previousValues);

    void addValue(float v);

    void close();

    Array<uint8_t> get_compress_pack();

    uint32_t get_size();

    // 析构函数
    ~ChimpCompressor();

private:
    // IAR适配：使用const数组替代constexpr
    static const uint16_t leadingRep_[64];
    static const uint16_t leadingRnd_[64];

    OutputBitStream* output_bit_stream_; // 使用指针替代std::unique_ptr

    int16_t storedLeadingZeros_;

    uint16_t index_;

    uint16_t current_;

    uint32_t size_;

    uint16_t previousValues_;

    uint16_t previousValuesLog2_;

    uint16_t threshold_;

    uint16_t setLsb_;

    int16_t* indices_; // 使用指针替代std::unique_ptr

    uint32_t* storedValues_; // 使用32位替代64位

    uint16_t flagZeroSize_;

    uint16_t flagOneSize_;

    bool first_;

    Array<uint8_t> compress_pack_;
};

#endif // CHIMP_COMPRESSOR_H