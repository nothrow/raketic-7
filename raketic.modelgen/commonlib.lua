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

function deg(degrees)
  return degrees * math.pi / 180
end
