local player = spawn(entities.basicShip {
  position = vec(0, 0),
})

local planet = spawn(entities.planet {
  position = vec(500, 300),
})

local moon1 = spawn(entities.moon {
  position = vec(500, 500),
  orbit = planet,
})

local sat1 = spawn(entities.satellite {
  position = vec(500, 420),
  orbit = planet,
})

local asteroid1 = spawn(entities.asteroid {
  position = vec(200, 0),
})

-- asteroids drifting slowly toward the planet
local asteroid2 = spawn(entities.asteroid {
  position = vec(-300, -200),
  velocity = vec(2.5, 1.6),
})

local asteroid3 = spawn(entities.asteroid {
  position = vec(1300, -100),
  velocity = vec(-2.7, 1.3),
})

local asteroid4 = spawn(entities.asteroid {
  position = vec(300, 900),
  velocity = vec(0.8, -2.4),
})

local asteroid5 = spawn(entities.asteroid {
  position = vec(-200, 500),
  velocity = vec(1.9, -0.5),
})

local asteroid6 = spawn(entities.asteroid {
  position = vec(1200, 800),
  velocity = vec(-2.0, -1.5),
})

-- force-include explosion particle models (spawned at runtime, not by entities)
require_model(models.spark)
require_model(models.shrapnel)

control(player)
