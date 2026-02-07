local player = spawn(entities.basicShip {
  position = vec(0, 0),
})

local planet = spawn(entities.planet {
  position = vec(100, 100),
})

local asteroid1 = spawn(entities.asteroid {
  position = vec(200, 0),
})

-- force-include explosion particle models (spawned at runtime, not by entities)
require_model(models.spark)
require_model(models.shrapnel)

control(player)
