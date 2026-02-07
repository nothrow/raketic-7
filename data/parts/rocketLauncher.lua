return Weapon {
  model = models.rocketLauncher,
  weaponType = "rocket",
  cooldownTicks = 60,  -- 2 shots per second at 120Hz
  projectileModel = models.rocket,
  projectileSpeed = 200,
  smokeModel = models.rocket_smoke,
}
