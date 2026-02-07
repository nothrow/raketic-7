#pragma once

// Spawn a hybrid explosion (sparks + shrapnel) at the given position.
// vx, vy: velocity of the exploding object (particles inherit some momentum)
// intensity: controls particle count and speed (typically the object's mass)
void explosion_spawn(float x, float y, float vx, float vy, float intensity);
