# SIMD Optimization for Raketic Engine

## Overview

This skill covers writing high-performance SIMD code using AVX2 intrinsics for the Raketic engine's hot paths.

## When to Apply

- Particle system updates (8 particles at once)
- Physics calculations (velocity, acceleration integration)
- Collision detection batch processing
- Any loop processing float arrays with 8+ elements

## Key Patterns

### AVX2 Register Types

```c
__m256  // 8x float (32-bit)
__m256i // 8x int32 or other integer packing
__m256d // 4x double (avoid - we use floats)
```

### Common Operations

```c
// Load/Store
__m256 data = _mm256_loadu_ps(float_array);      // unaligned load
__m256 data = _mm256_load_ps(aligned_array);    // aligned load (32-byte)
_mm256_storeu_ps(dest, result);                 // unaligned store

// Arithmetic
_mm256_add_ps(a, b);    // a + b
_mm256_sub_ps(a, b);    // a - b
_mm256_mul_ps(a, b);    // a * b
_mm256_fmadd_ps(a, b, c); // a * b + c (fused, more precise)

// Broadcast scalar
_mm256_set1_ps(scalar); // fill all 8 lanes with scalar
```

### Random Number Generation (Project Pattern)

Reference: `platform/math.c` implements SIMD xorshift:

```c
__m256 randf_8(void);           // 8 random floats in [0, 1)
__m256 randf_symmetric_8(void); // 8 random floats in [-1, 1)
```

### Memory Alignment

For best performance with `_mm256_load_ps`:

```c
__declspec(align(32)) float data[8];
// or
float* data = _aligned_malloc(size * sizeof(float), 32);
```

## Data Layout for SIMD

Structure of Arrays (SoA) is preferred over Array of Structures (AoS):

```c
// GOOD: SoA - SIMD friendly
struct particles_data {
  float* velocity_x;  // process 8 at once
  float* velocity_y;  // process 8 at once
};

// BAD: AoS - poor cache utilization for SIMD
struct particle {
  float velocity_x, velocity_y;
};
struct particle particles[N];
```

## Performance Tips

1. **Minimize scalar/vector transitions** - keep data in registers
2. **Process in multiples of 8** - pad arrays if needed
3. **Use `__restrict`** - tells compiler pointers don't alias
4. **Prefer FMA** - `_mm256_fmadd_ps` is faster and more accurate than separate mul+add

## Example: Particle Position Update

```c
void update_positions(float* __restrict x, float* __restrict y,
                      const float* __restrict vx, const float* __restrict vy,
                      float dt, uint32_t count) {
  __m256 dt_vec = _mm256_set1_ps(dt);

  for (uint32_t i = 0; i < count; i += 8) {
    __m256 pos_x = _mm256_loadu_ps(&x[i]);
    __m256 pos_y = _mm256_loadu_ps(&y[i]);
    __m256 vel_x = _mm256_loadu_ps(&vx[i]);
    __m256 vel_y = _mm256_loadu_ps(&vy[i]);

    pos_x = _mm256_fmadd_ps(vel_x, dt_vec, pos_x);
    pos_y = _mm256_fmadd_ps(vel_y, dt_vec, pos_y);

    _mm256_storeu_ps(&x[i], pos_x);
    _mm256_storeu_ps(&y[i], pos_y);
  }
}
```

## Required Header

```c
#include <immintrin.h>
```
