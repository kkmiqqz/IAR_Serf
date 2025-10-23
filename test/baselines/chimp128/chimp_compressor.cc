#include "chimp_compressor.h"
#include <stdlib.h>

// IAR适配：定义静态常量数组
const uint16_t ChimpCompressor::leadingRep_[64] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 4, 4, 5, 5, 6, 6,
        7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7,
        7, 7, 7, 7, 7, 7, 7, 7
};

const uint16_t ChimpCompressor::leadingRnd_[64] = {
        0, 0, 0, 0, 0, 0, 0, 0,
        8, 8, 8, 8, 12, 12, 12, 12,
        16, 16, 18, 18, 20, 20, 22, 22,
        24, 24, 24, 24, 24, 24, 24, 24,
        24, 24, 24, 24, 24, 24, 24, 24,
        24, 24, 24, 24, 24, 24, 24, 24,
        24, 24, 24, 24, 24, 24, 24, 24,
        24, 24, 24, 24, 24, 24, 24, 24,
};

ChimpCompressor::ChimpCompressor(uint16_t previousValues) {
    // IAR适配：使用new替代std::make_unique，减少内存使用
    output_bit_stream_ = new OutputBitStream(200 * 8); // 从1000减少到200
    size_ = 0;
    previousValues_ = previousValues;
    previousValuesLog2_ = (uint16_t)(logf((float)previousValues_) / logf(2.0f));
    threshold_ = 6 + previousValuesLog2_;
    setLsb_ = (uint16_t)powf(2.0f, (float)(threshold_ + 1)) - 1;
    
    // 分配内存 - 限制大小避免XDATA溢出
    uint16_t indices_size = (1 << (threshold_ + 1));
    if (indices_size > 256) indices_size = 256; // 限制最大大小
    indices_ = (int16_t*)malloc(indices_size * sizeof(int16_t));
    storedValues_ = (uint32_t*)malloc(previousValues_ * sizeof(uint32_t));
    
    if (indices_ == NULL || storedValues_ == NULL) {
        // 内存分配失败
        if (indices_) free(indices_);
        if (storedValues_) free(storedValues_);
        indices_ = NULL;
        storedValues_ = NULL;
    }
    
    flagZeroSize_ = previousValuesLog2_ + 2;
    flagOneSize_ = previousValuesLog2_ + 11;
    first_ = true;
    storedLeadingZeros_ = 0x7FFF; // 使用最大int16_t值
    index_ = 0;
    current_ = 0;
}

ChimpCompressor::~ChimpCompressor() {
    if (output_bit_stream_ != NULL) {
        delete output_bit_stream_;
    }
    if (indices_ != NULL) {
        free(indices_);
    }
    if (storedValues_ != NULL) {
        free(storedValues_);
    }
}

void ChimpCompressor::addValue(float v) {
    uint32_t value = Double::FloatToLongBits(v); // 使用float版本
    if (first_) {
        first_ = false;
        storedValues_[current_] = value;
        size_ += output_bit_stream_->WriteLong(storedValues_[current_], 32);
        indices_[((int)value) & setLsb_] = index_;
    } else {
        int16_t key = (int16_t)value & setLsb_;
        uint32_t xored_value;
        int16_t previousIndex;
        int16_t trailingZeros = 0;
        int16_t curIndex = indices_[key];
        if ((index_ - curIndex) < previousValues_) {
            uint32_t tempXor = value ^ storedValues_[curIndex % previousValues_];
            trailingZeros = __builtin_ctz(tempXor);
            if (trailingZeros > threshold_) {
                previousIndex = curIndex % previousValues_;
                xored_value = tempXor;
            } else {
                previousIndex = index_ % previousValues_;
                xored_value = storedValues_[previousIndex] ^ value;
            }
        } else {
            previousIndex = index_ % previousValues_;
            xored_value = storedValues_[previousIndex] ^ value;
        }

        if (xored_value == 0) {
            size_ += output_bit_stream_->WriteInt(previousIndex, flagZeroSize_);
            storedLeadingZeros_ = 65;
        } else {
            int16_t leadingZeros = leadingRnd_[__builtin_clz(xored_value)];

            if (trailingZeros > threshold_) {
                int16_t significantBits = 32 - leadingZeros - trailingZeros;
                size_ += output_bit_stream_->WriteInt(
                        512 * (previousValues_ + previousIndex) +
                        64 * leadingRep_[leadingZeros] + significantBits,
                        flagOneSize_);
                size_ += output_bit_stream_->WriteLong(
                        xored_value >> trailingZeros, significantBits);
                storedLeadingZeros_ = 65;
            } else if (leadingZeros == storedLeadingZeros_) {
                size_ += output_bit_stream_->WriteInt(2, 2);
                int16_t significantBits = 32 - leadingZeros;
                size_ += output_bit_stream_->WriteLong(xored_value,
                                                       significantBits);
            } else {
                storedLeadingZeros_ = leadingZeros;
                int16_t significantBits = 32 - leadingZeros;
                size_ += output_bit_stream_->WriteInt(
                        24 + leadingRep_[leadingZeros], 5);
                size_ += output_bit_stream_->WriteLong(xored_value,
                                                       significantBits);
            }
        }

        current_ = (current_ + 1) % previousValues_;
        storedValues_[current_] = value;
        index_++;
        indices_[key] = index_;
    }
}

void ChimpCompressor::close() {
    addValue(0.0f / 0.0f); // 使用NaN替代std::numeric_limits
    output_bit_stream_->Flush();
}

uint32_t ChimpCompressor::get_size() {
    return size_;
}

Array<uint8_t> ChimpCompressor::get_compress_pack() {
    compress_pack_ = output_bit_stream_->GetBuffer((uint32_t)ceilf((float)size_ / 8.0f));
    return compress_pack_;
}