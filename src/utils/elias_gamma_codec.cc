#include "elias_gamma_codec.h"

// IAR适配：定义静态常量数组
const float EliasGammaCodec::kLog2Table[17] = {
    Double::kNan(),
    0.0f,
    1.0f,
    1.584962500721156f,
    2.0f,
    2.321928094887362f,
    2.584962500721156f,
    2.807354922057604f,
    3.0f,
    3.169925001442312f,
    3.321928094887362f,
    3.459431618637297f,
    3.584962500721156f,
    3.700439718141092f,
    3.807354922057604f,
    3.906890595608519f,
    4.0f
};

int EliasGammaCodec::Encode(int32_t number, OutputBitStream *output_bit_stream_ptr) {
  int compressed_size_in_bits = 0;
  int n;
  if (number <= 16) {
    n = (int)floorf(kLog2Table[number]);
  } else {
    n = (int)floorf(log2f((float)number));
  }
  compressed_size_in_bits += output_bit_stream_ptr->WriteInt(0, n);
  compressed_size_in_bits += output_bit_stream_ptr->WriteInt((uint32_t)number, n + 1);
  return compressed_size_in_bits;
}

int32_t EliasGammaCodec::Decode(InputBitStream *input_bit_stream_ptr) {
  int n = 0;
  while (!input_bit_stream_ptr->ReadBit()) n++;
  return n == 0 ? 1 :  (1 << n) | input_bit_stream_ptr->ReadInt(n);
}
