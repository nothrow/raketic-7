-- Sun at the center of the system
local sun = spawn(entities.sun {
  position = vec(0, 0),
})

-- Planet orbiting the sun at distance 5000
-- Hill sphere ≈ 747, so moons within ~250 are stable
local planet = spawn(entities.planet {
  position = vec(5000, 0),
  orbit = sun,
})

-- Moon orbiting the planet at distance 200 (27% of Hill sphere)
local moon1 = spawn(entities.moon {
  position = vec(5000, 200),
  orbit = planet,
})

-- Satellite orbiting the planet at distance 180
local sat1 = spawn(entities.satellite {
  position = vec(4820, 0),
  orbit = planet,
})

-- Player starts near the planet
local player = spawn(entities.basicShip {
  position = vec(4850, -150),
})

-- Asteroids drifting slowly toward the planet
local asteroid1 = spawn(entities.asteroid {
  position = vec(4500, -400),
  velocity = vec(1.8, 1.2),
})

local asteroid2 = spawn(entities.asteroid {
  position = vec(5500, -300),
  velocity = vec(-2.5, 1.0),
})

local asteroid3 = spawn(entities.asteroid {
  position = vec(5200, 600),
  velocity = vec(-1.2, -2.0),
})

local asteroid4 = spawn(entities.asteroid {
  position = vec(4400, 300),
  velocity = vec(2.0, -0.8),
})

local asteroid5 = spawn(entities.asteroid {
  position = vec(5300, -500),
  velocity = vec(-1.5, 2.2),
})

-- force-include explosion particle models (spawned at runtime, not by entities)
require_model(models.spark)
require_model(models.shrapnel)

control(player)
