#ifndef SERF_ELIAS_GAMMA_CODEC_H_
#define SERF_ELIAS_GAMMA_CODEC_H_

#include <stdint.h>
#include <math.h>

// IAR适配：使用C风格头文件
#include "output_bit_stream.h"
#include "input_bit_stream.h"
#include "double.h"

class EliasGammaCodec {
 public:
  static int Encode(int32_t number, OutputBitStream *output_bit_stream_ptr);

  static int32_t Decode(InputBitStream *input_bit_stream_ptr);

 private:
  // IAR适配：使用const数组替代constexpr
  static const float kLog2Table[17];
};

#endif //SERF_ELIAS_GAMMA_CODEC_H_
