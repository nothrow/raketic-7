return Weapon {
  model = models.rocketLauncher,
  weaponType = "rocket",
  cooldownTicks = 60,  -- 2 shots per second at 120Hz
  projectileModel = models.rocket,
  projectileSpeed = 200,
  smokeModel = models.rocket_smoke,
  rocketThrust = 50,
  rocketFuelTicks = 120,       -- 1 second of burn at 120Hz
  rocketLifetimeTicks = 600,   -- 5 seconds total
}
