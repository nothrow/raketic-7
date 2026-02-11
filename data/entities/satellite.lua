return Satellite {
  model  = models.satellite,
  mass   = 5,
  health = 80,
  radius = models.satellite.radius,
  slots = {
    engine1 = parts.weakEngine,
    engine2 = parts.weakEngine,
    turret1 = parts.turretLaser,
    turret2 = parts.turretLaser
  }
}
