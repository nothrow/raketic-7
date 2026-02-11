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

-- force-include explosion particle models (spawned at runtime, not by entities)
require_model(models.spark)
require_model(models.shrapnel)

control(player)
