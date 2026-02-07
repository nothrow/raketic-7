return Asteroid {
  model  = models.asteroid,
  mass   = 50,
  health = 100,
  radius = models.asteroid.radius,
  slots = {
    chunk1 = parts.chunk,
    chunk2 = parts.chunk
  }
}
