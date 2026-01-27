local shipDefaults = {
  type   = "ENTITY_TYPEREF_SHIP",
  __dataType = "EntityWithSlotsData"
}

local engineDefaults = {
  type   = "PART_TYPEREF_ENGINE",
  __dataType = "EngineData"
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
