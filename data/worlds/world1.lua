local player = spawn(entities.basicShip {
  position = vec(0, 0),
})

local planet = spawn(entities.planet {
  position = vec(100, 100),
})

control(player)
