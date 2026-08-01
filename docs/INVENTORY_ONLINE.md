# Inventory - Online (MySQL) docs

Credits: **Habibi / Janzzzz**

Back to: [Readme.md](../Readme.md) | [Docs hub](INVENTORY.md) | [Easy examples](INVENTORY_EXAMPLES.md) | [Weapons](INVENTORY_WEAPONS.md) | [Offline docs](INVENTORY_OFFLINE.md)

**Mode:** MySQL persistence via `a_mysql.inc`.  
**Demo:** `gamemodes/inventory_test_online.pwn`  
**Compile:** `compile_inventory_online.bat`  
**Schema:** `scriptfiles/inventory_online.sql`

UI, guns, click-to-move, and ground bags work the same as offline. This page covers **database setup, save, and delete**.

For UI / API / item tables, see [INVENTORY_OFFLINE.md](INVENTORY_OFFLINE.md).  
To add items so they show on the panel, see [INVENTORY_ADD_ITEMS.md](INVENTORY_ADD_ITEMS.md).  
For weapons online/offline, see [INVENTORY_WEAPONS.md](INVENTORY_WEAPONS.md#online-mysql--weapon-database).  
For loot / house right-panel examples, see [INVENTORY_EXAMPLES.md](INVENTORY_EXAMPLES.md).

---

## Table of contents

- [Setup](#setup)
- [In-game commands](#in-game-commands)
- [Save flow](#save-flow)
- [How delete works in the database](#how-delete-works-in-the-database)
- [Weapons in MySQL](#weapons-in-mysql)
- [Wire MySQL in your gamemode](#wire-mysql-in-your-gamemode)
- [Table shape](#table-shape)
- [Differences from offline](#differences-from-offline)

---

## Setup

1. Put the MySQL plugin DLL in `plugins/` and enable it in `config.json`:

```json
"pawn": {
    "legacy_plugins": ["mysql", "textdraw-streamer"],
    "main_scripts": ["inventory_test_online 1"]
}
```

2. Edit login defines at the top of `inventory_test_online.pwn`:

```pawn
#define MYSQL_HOST     "127.0.0.1"
#define MYSQL_USER     "root"
#define MYSQL_PASS     ""
#define MYSQL_DB       "inventory_test"
#define MYSQL_PORT     (3306)
```

3. Create the database (optional - the gamemode also auto-creates the table):

```bat
mysql -u root < scriptfiles\inventory_online.sql
```

4. Compile:

```bat
compile_inventory_online.bat
```

5. Start the server and join. Inventory is keyed by **player name**.

---

## In-game commands

| Command | What it does |
|---------|----------------|
| `/giveitem` | Add test items, then auto-save to DB |
| `/givegun` | Give Deagle in hand |
| `/placegun` | Store held gun + ammo (saved as quantity) |
| `/dropfront` | Drop Medkit bag in front (not saved to DB) |
| `/inv` | Open inventory UI |
| `/saveinv` | Force-save inventory to DB now |
| Use / move / drop inventory items | Triggers `Inv_OnInventoryChanged` -> queued save |

---

## Save flow

1. **Connect** -> `InvDB_Load` runs `SELECT` for your name and fills slots with `Inventory_SetSlot`.
2. **Change inventory** (add, use, move, placegun, drop from left bag) -> `Inv_OnInventoryChanged` queues `InvDB_Save` after ~1.2 seconds.
3. **Disconnect** or `/saveinv` -> save runs immediately.

Ground bags on the map are **not** written to MySQL (memory only), same as offline.

---

## How delete works in the database

Saving does **not** update rows one-by-one. It uses wipe-and-rewrite:

1. `DELETE FROM player_inventory WHERE owner_name = 'YourName'`  
   - Removes **all** stored slots for that player.
2. `INSERT` only the slots that still have items in memory.

So if you:

- **Use** an item until qty is 0  
- **Drop** it to the ground bag  
- **Move** it out of inventory  
- Clear a slot any other way  

…that slot is gone from memory, and on the next save it is **not inserted again**. After step 1 deleted the old rows, that item **disappears from the database**.

Examples:

| Action | DB result after save |
|--------|----------------------|
| Had Bread x3, used 1 -> Bread x2 | Old rows deleted; one INSERT for Bread x2 |
| Had gun, used it (equipped) | Gun row not inserted |
| Had item, dropped to ground bag | Item row not inserted (left inventory empty of it) |
| Emptied whole inventory | `DELETE` only - no INSERTs |
| `/giveitem` then disconnect | `DELETE` then INSERTs for current slots |

Important:

- Only **left inventory** slots are stored.
- Key column is `owner_name` (player name). Renaming = different rows.
- Gun ammo is stored in `invQuantity`.

---

## Weapons in MySQL

Guns are **normal inventory rows**. No extra weapons table needed.

| Field | Stores |
|-------|--------|
| `invItem` | `Gun: Desert Eagle` (prefix required) |
| `invModel` | Weapon object model |
| `invQuantity` | **Ammo** |

```
/placegun → save row with ammo
reconnect → SetSlot → gun on panel
Use → EquipGun → ammo back in hand → row removed on next save
```

Step-by-step + advanced notes: [INVENTORY_WEAPONS.md — Online](INVENTORY_WEAPONS.md#online-mysql--weapon-database)

---

## Wire MySQL in your gamemode

Same UI wiring as offline, plus load/save:

```pawn
#include <open.mp>
#include <a_mysql>
#include <textdraw-streamer>
#include <inventory>

public OnPlayerConnect(playerid)
{
    Inventory_OnPlayerConnect(playerid);
    InvDB_Load(playerid); // SELECT -> Inventory_SetSlot(...)
    return 1;
}

public OnPlayerDisconnect(playerid, reason)
{
    #pragma unused reason
    InvDB_Save(playerid); // DELETE all for name, then INSERT current slots
    Inventory_OnPlayerDisconnect(playerid);
    return 1;
}

// Fired after add/remove/move/placegun/equip - queue delayed save
public Inv_OnInventoryChanged(playerid)
{
    InvDB_QueueSave(playerid);
    return 1;
}
```

Read a slot for saving:

```pawn
new item[64], model, qty;
if (Inventory_GetSlot(playerid, slot, item, sizeof(item), model, qty))
{
    // INSERT this slot
}
```

Load a slot from DB:

```pawn
Inventory_SetSlot(playerid, slot, "Bread", 2670, 3);
// quantity <= 0 clears the slot
```

Copy the `InvDB_*` helpers from `inventory_test_online.pwn`, or keep using that gamemode as a template.

UI callbacks (`Inv_OnItemUse`, click handlers, etc.) stay the same as [offline](INVENTORY_OFFLINE.md#using-in-your-gamemode).

---

## Table shape

```sql
player_inventory
  id, owner_name, slot, invItem, invModel, invQuantity
```

`invQuantity` = stack count for normal items, **ammo** for `Gun: ...` items.

---

## Differences from offline

| | Offline | Online |
|-|---------|--------|
| File | `inventory_test.pwn` | `inventory_test_online.pwn` |
| Plugin | none | `mysql` in `legacy_plugins` |
| Include | `<inventory>` | `<a_mysql>` + `<inventory>` |
| On connect | clear only | clear + load from DB |
| On change | memory only | queue MySQL save |
| On disconnect | clear | save then clear |
| Extra command | - | `/saveinv` |
| Docs | [INVENTORY_OFFLINE.md](INVENTORY_OFFLINE.md) | this file |
