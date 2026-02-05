# Physics Implementation in Raketic

## Overview

Raketic uses realistic Newtonian physics with gravitational simulation, running at a fixed 120Hz timestep.

## Core Concepts

### Fixed Timestep

```c
#define PHYSICS_HZ 120
#define PHYSICS_DT (1.0f / PHYSICS_HZ)  // ~8.33ms
```

### Integration Methods

| System | Integrator | Why |
|--------|------------|-----|
| Objects | Yoshida 4th order | Accuracy for orbital mechanics |
| Particles | Euler | Speed over accuracy (short-lived) |

## Yoshida Integrator

4th-order symplectic integrator preserves energy in orbital simulations:

```c
// Yoshida coefficients
static const float w0 = -1.7024143839193153f;
static const float w1 =  1.3512071919596578f;
static const float c[4] = { w1/2, (w0+w1)/2, (w0+w1)/2, w1/2 };
static const float d[3] = { w1, w0, w1 };

void yoshida_step(float* x, float* v, float dt, float (*accel)(float)) {
  for (int i = 0; i < 4; i++) {
    *x += c[i] * (*v) * dt;
    if (i < 3) {
      *v += d[i] * accel(*x) * dt;
    }
  }
}
```

## Gravitational Physics

### Newton's Law

```
F = G * m1 * m2 / r²
a = G * M / r²  (acceleration toward mass M)
```

### Implementation Pattern

```c
void apply_gravity(float* ax, float* ay,
                   float px, float py,           // object position
                   float planet_x, float planet_y, float planet_mass) {
  float dx = planet_x - px;
  float dy = planet_y - py;
  float r_sq = dx*dx + dy*dy;
  float r = sqrtf(r_sq);

  // Avoid division by zero / extreme forces
  if (r < MIN_DISTANCE) return;

  float accel = G * planet_mass / r_sq;

  // Normalize direction and apply
  *ax += accel * dx / r;
  *ay += accel * dy / r;
}
```

### SIMD Gravity (8 objects at once)

```c
void apply_gravity_8(__m256* ax, __m256* ay,
                     __m256 px, __m256 py,
                     float planet_x, float planet_y, float planet_mass) {
  __m256 dx = _mm256_sub_ps(_mm256_set1_ps(planet_x), px);
  __m256 dy = _mm256_sub_ps(_mm256_set1_ps(planet_y), py);

  __m256 r_sq = _mm256_fmadd_ps(dx, dx, _mm256_mul_ps(dy, dy));
  __m256 r = _mm256_sqrt_ps(r_sq);
  __m256 r_inv = _mm256_div_ps(_mm256_set1_ps(1.0f), r);

  __m256 accel = _mm256_div_ps(_mm256_set1_ps(G * planet_mass), r_sq);

  *ax = _mm256_fmadd_ps(accel, _mm256_mul_ps(dx, r_inv), *ax);
  *ay = _mm256_fmadd_ps(accel, _mm256_mul_ps(dy, r_inv), *ay);
}
```

## Velocity and Position Updates

### Euler (Simple, for particles)

```c
velocity += acceleration * dt;
position += velocity * dt;
```

### Leapfrog/Verlet (Better energy conservation)

```c
// Half-step velocity
velocity += 0.5f * acceleration * dt;
// Full-step position
position += velocity * dt;
// Recalculate acceleration at new position
acceleration = compute_acceleration(position);
// Complete velocity step
velocity += 0.5f * acceleration * dt;
```

## Collision Response

### Elastic Collision (Conservation of momentum)

```c
void elastic_collision(float* v1x, float* v1y, float m1,
                       float* v2x, float* v2y, float m2,
                       float nx, float ny) {  // collision normal
  // Relative velocity along normal
  float dvn = ((*v1x - *v2x) * nx + (*v1y - *v2y) * ny);

  // Impulse scalar
  float j = 2.0f * dvn / (m1 + m2);

  // Apply impulse
  *v1x -= j * m2 * nx;
  *v1y -= j * m2 * ny;
  *v2x += j * m1 * nx;
  *v2y += j * m1 * ny;
}
```

## Data Structures

Position and orientation stored in SoA format:

```c
typedef struct {
  float* x;
  float* y;
  float* orientation_x;  // cos(angle)
  float* orientation_y;  // sin(angle)
} position_orientation_t;
```

Using cos/sin instead of angle avoids repeated trig calculations.

## Orbital Mechanics Notes

### Hill Sphere

Region where a body's gravity dominates over parent body:

```
r_hill ≈ a * (m / 3M)^(1/3)
```

Where `a` = orbital semi-major axis, `m` = body mass, `M` = parent mass.

### Orbital Velocity

Circular orbit velocity:

```
v = sqrt(G * M / r)
```

### Escape Velocity

```
v_escape = sqrt(2 * G * M / r)
```
