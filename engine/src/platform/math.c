#include "math.h"
#include <immintrin.h>

// quarter table: 0-90° only (91 values = 364 bytes instead of 1440)
static const float sin_q[91] = {
  0.00000f, 0.01745f, 0.03490f, 0.05234f, 0.06976f, 0.08716f, 0.10453f, 0.12187f, 0.13917f, 0.15643f, 0.17365f,
  0.19081f, 0.20791f, 0.22495f, 0.24192f, 0.25882f, 0.27564f, 0.29237f, 0.30902f, 0.32557f, 0.34202f, 0.35837f,
  0.37461f, 0.39073f, 0.40674f, 0.42262f, 0.43837f, 0.45399f, 0.46947f, 0.48481f, 0.50000f, 0.51504f, 0.52992f,
  0.54464f, 0.55919f, 0.57358f, 0.58779f, 0.60182f, 0.61566f, 0.62932f, 0.64279f, 0.65606f, 0.66913f, 0.68200f,
  0.69466f, 0.70711f, 0.71934f, 0.73135f, 0.74314f, 0.75471f, 0.76604f, 0.77715f, 0.78801f, 0.79864f, 0.80902f,
  0.81915f, 0.82904f, 0.83867f, 0.84805f, 0.85717f, 0.86603f, 0.87462f, 0.88295f, 0.89101f, 0.89879f, 0.90631f,
  0.91355f, 0.92050f, 0.92718f, 0.93358f, 0.93969f, 0.94552f, 0.95106f, 0.95630f, 0.96126f, 0.96593f, 0.97030f,
  0.97437f, 0.97815f, 0.98163f, 0.98481f, 0.98769f, 0.99027f, 0.99255f, 0.99452f, 0.99619f, 0.99756f, 0.99863f,
  0.99939f, 0.99985f, 1.00000f,
};

static uint32_t rng_state = 2463534242u;

uint32_t rand32(void) {
  rng_state ^= rng_state << 13;
  rng_state ^= rng_state >> 17;
  rng_state ^= rng_state << 5;
  return rng_state;
}

void _math_initialize(void) {}

float lut_sin(int32_t deg) {
  deg = ((deg % 360) + 360) % 360;
  if (deg <= 90)
    return sin_q[deg];
  if (deg <= 180)
    return sin_q[180 - deg];
  if (deg <= 270)
    return -sin_q[deg - 180];
  return -sin_q[360 - deg];
}

float lut_cos(int32_t deg) {
  return lut_sin(deg + 90);
}

// SIMD xorshift - 8 parallel states for 8 random numbers
static __m256i rng_state_8 = { 0 };
static int rng_state_8_initialized = 0;

static void _init_rng_state_8(void) {
  if (rng_state_8_initialized) return;
  // Initialize 8 different states from the scalar RNG
  __declspec(align(32)) uint32_t states[8];
  for (int i = 0; i < 8; i++) {
    states[i] = rand32();
  }
  rng_state_8 = _mm256_load_si256((const __m256i*)states);
  rng_state_8_initialized = 1;
}

// SIMD xorshift32 - generates 8 random uint32s
static __m256i _rand32_8(void) {
  _init_rng_state_8();

  // xorshift: state ^= state << 13; state ^= state >> 17; state ^= state << 5;
  __m256i state = rng_state_8;
  state = _mm256_xor_si256(state, _mm256_slli_epi32(state, 13));
  state = _mm256_xor_si256(state, _mm256_srli_epi32(state, 17));
  state = _mm256_xor_si256(state, _mm256_slli_epi32(state, 5));
  rng_state_8 = state;
  return state;
}

__m256 randf_8(void) {
  __m256i rand_ints = _rand32_8();
  // Mask to get lower 23 bits (mantissa), then convert to float in [1, 2) and subtract 1
  __m256i mantissa_mask = _mm256_set1_epi32(0x007FFFFF);
  __m256i one_bits = _mm256_set1_epi32(0x3F800000);  // 1.0f in IEEE 754
  __m256i float_bits = _mm256_or_si256(_mm256_and_si256(rand_ints, mantissa_mask), one_bits);
  return _mm256_sub_ps(_mm256_castsi256_ps(float_bits), _mm256_set1_ps(1.0f));
}

__m256 randf_symmetric_8(void) {
  __m256 rand01 = randf_8();
  return _mm256_sub_ps(_mm256_mul_ps(rand01, _mm256_set1_ps(2.0f)), _mm256_set1_ps(1.0f));
}
