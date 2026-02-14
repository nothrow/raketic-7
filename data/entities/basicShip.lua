return Ship {
  model  = models.ship,
  mass   = 1,
  radius = models.ship.radius,
  slots = {
    engine1 = parts.basicEngine,
    engine2 = parts.basicEngine,
    weapon1 = parts.laser,
    weapon2 = parts.rocketLauncher,
    radar1  = parts.shipRadar
  }
}
