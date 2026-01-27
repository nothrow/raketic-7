local shipDefaults = {
  type   = "ENTITY_TYPEREF_SHIP",
  dataType = "EntityWithSlotsData"
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
