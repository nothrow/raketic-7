# Raketic Engine - Průvodce vytvářením nových typů entit

## Přehled

Tento dokument popisuje kroky potřebné pro přidání nového typu entity do Raketic Engine.

## Checklist

### 1. Registrace typu v `types.h`

Soubor: `engine/src/entity/types.h`

1. Přidat novou hodnotu do `enum entity_type`:
   ```c
   enum entity_type {
     ENTITY_TYPE_ANY = 0,
     // ... existující typy ...
     ENTITY_TYPE_NOVA_ENTITA,  // <-- přidat před ENTITY_TYPE_COUNT
     ENTITY_TYPE_COUNT,
   };
   ```

2. Přidat statickou typovou referenci (pokud je potřeba z jiných modulů):
   ```c
   static entity_type_t ENTITY_TYPEREF_NOVA_ENTITA = { ENTITY_TYPE_NOVA_ENTITA };
   ```

### 2. Vytvoření souborů entity

Vytvořit v `engine/src/entity/`:

**Header (`nova_entita.h`):**
```c
#pragma once

#include "entity_internal.h"

void nova_entita_entity_initialize(void);
```

**Implementace (`nova_entita.c`):**
```c
#include "platform/platform.h"
#include "platform/math.h"

#include "entity_internal.h"
#include "entity.h"
#include "nova_entita.h"

static void _nova_entita_dispatch(entity_id_t id, message_t msg) {
  switch (msg.message) {
    // Zpracování zpráv specifických pro tento typ
    case MESSAGE_BROADCAST_120HZ_BEFORE_PHYSICS:
      // Update logika
      break;
  }
}

void nova_entita_entity_initialize(void) {
  entity_manager_vtables[ENTITY_TYPE_NOVA_ENTITA].dispatch_message = _nova_entita_dispatch;
}
```

### 3. Registrace v entity manageru

Soubor: `engine/src/entity/entity.c`

1. Přidat include:
   ```c
   #include "nova_entita.h"
   ```

2. Volat inicializaci v `_entity_manager_types_initialize()`:
   ```c
   static void _entity_manager_types_initialize(void) {
     // ... existující inicializace ...
     nova_entita_entity_initialize();
   }
   ```

### 4. Přidání SVG modelu (pokud má vizuální reprezentaci)

Soubor: `models/nova_entita.svg`

- Vytvořit SVG soubor s wireframe grafikou
- Použít bílé čáry (`stroke="white"`) bez výplně (`fill="none"`)
- Střed modelu bude automaticky vypočítán, nebo lze použít `<circle id="center" cx="X" cy="Y" r="1"/>` pro explicitní střed
- Pro sloty (např. engine) použít atribut `slot="engine"` na elementu

**Příklad struktury:**
```xml
<?xml version="1.0" encoding="UTF-8"?>
<svg width="100" height="100" viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg">
  <circle id="center" cx="50" cy="50" r="1" />
  <path d="..." fill="none" stroke="white" stroke-width="2"/>
</svg>
```

### 5. Regenerace kódu

Po přidání SVG modelu spustit model generátor:

```powershell
cd raketic.modelgen
dotnet run
```

Tím se vygeneruje:
- `engine/generated/renderer.gen.h` - konstanty `MODEL_*_IDX`
- `engine/generated/models.gen.c` - vykreslovací funkce

### 6. Spawn entity

Pro vytvoření instance entity použít:

```c
entity_id_t id = _generate_entity_by_model(ENTITY_TYPEREF_NOVA_ENTITA, MODEL_NOVA_ENTITA_IDX);
```

### 7. Přidání do vcxproj (Visual Studio)

Soubor: `engine/engine.vcxproj`

Přidat nové .c a .h soubory do příslušných `<ItemGroup>` sekcí.

---

## Typy entit

| Kategorie | Popis | Příklady |
|-----------|-------|----------|
| **Objects** | Hlavní entity s fyzikou | ship, planet |
| **Parts** | Komponenty připojené k objektům | engine |
| **Particles** | Krátkodobé efekty | exhaust, spark |

## Messaging systém

Entity komunikují přes message bus. Broadcast zprávy:
- `MESSAGE_BROADCAST_120HZ_BEFORE_PHYSICS` - před fyzikou
- `MESSAGE_BROADCAST_120HZ_AFTER_PHYSICS` - po fyzice

Specifické zprávy definovat v `messaging/messaging.h`.
