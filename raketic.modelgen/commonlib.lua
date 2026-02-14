local shipDefaults = {
  type   = "ENTITY_TYPEREF_SHIP",
  __dataType = "EntityWithSlotsData"
}

local engineDefaults = {
  type   = "PART_TYPEREF_ENGINE",
  __dataType = "EngineData"
}

local weaponDefaults = {
  type   = "PART_TYPEREF_WEAPON",
  __dataType = "WeaponData"
}

local planetDefaults = {
  type   = "ENTITY_TYPEREF_PLANET",
  __dataType = "EntityData"
}

local asteroidDefaults = {
  type   = "ENTITY_TYPEREF_ASTEROID",
  __dataType = "EntityWithSlotsData"
}

local chunkDefaults = {
  type   = "PART_TYPEREF_CHUNK",
  __dataType = "PartData"
}

local moonDefaults = {
  type   = "ENTITY_TYPEREF_MOON",
  __dataType = "EntityData"
}

local satelliteDefaults = {
  type   = "ENTITY_TYPEREF_SATELLITE",
  __dataType = "EntityWithSlotsData"
}

local radarDefaults = {
  type   = "PART_TYPEREF_RADAR",
  __dataType = "RadarData"
}

local sunDefaults = {
  type   = "ENTITY_TYPEREF_SUN",
  __dataType = "EntityData"
}

local function merge(a, b)
  local result = {}
  for k, v in pairs(a) do
    result[k] = v
  end
  for k, v in pairs(b) do
    result[k] = v
  end
  return result
end

function Ship(data)
  return Entity(merge(shipDefaults, data))
end

function Engine(data)
  return Entity(merge(engineDefaults, data))
end

function Weapon(data)
  return Entity(merge(weaponDefaults, data))
end

function Planet(data)
  return Entity(merge(planetDefaults, data))
end

function Asteroid(data)
  return Entity(merge(asteroidDefaults, data))
end

function Chunk(data)
  return Entity(merge(chunkDefaults, data))
end

function Moon(data)
  return Entity(merge(moonDefaults, data))
end

function Satellite(data)
  return Entity(merge(satelliteDefaults, data))
end

function Radar(data)
  return Entity(merge(radarDefaults, data))
end

function Sun(data)
  return Entity(merge(sunDefaults, data))
end

function deg(degrees)
  return degrees * math.pi / 180
end
