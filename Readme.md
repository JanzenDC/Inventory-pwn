# open.mp Server - Inventory System

Credits: **Habibi / Janzzzz**

Reusable textdraw inventory for open.mp. Same style as a modular player grid (model previews + Use / Drop / Close), packaged as includes under `qawno/include`.

## Online (MySQL) test

`gamemodes/inventory_test_online.pwn` uses the same inventory UI, but saves/loads slots through MySQL (`a_mysql.inc`).

### Setup

1. Put the MySQL plugin DLL in `plugins/` and enable it in `config.json`:

```json
"pawn": {
    "legacy_plugins": ["mysql"],
    "main_scripts": ["inventory_test_online 1"]
}
```

2. Edit the login defines at the top of `inventory_test_online.pwn`:

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

5. Start `omp-server.exe` and join with a normal player name (inventory is keyed by **player name**).

### How to use in-game

| Command | What it does |
|---------|----------------|
| `/giveitem` | Add test items, then auto-save to DB |
| `/dropfront` | Drop a Medkit bag in front of you |
| `/inv` | Open inventory UI |
| `/saveinv` | Force-save inventory to DB right now |
| Use / move / drop items | Triggers `Inv_OnInventoryChanged` -> queued save |

Flow:

1. Connect -> `InvDB_Load` runs `SELECT` for your name and fills slots with `Inventory_SetSlot`.
2. Change inventory (add, use, move, drop) -> `Inv_OnInventoryChanged` queues `InvDB_Save` after ~1.2 seconds.
3. Disconnect or `/saveinv` -> save runs immediately.

### How delete works in the database

Saving does **not** update rows one-by-one. It uses a wipe-and-rewrite:

1. `DELETE FROM player_inventory WHERE owner_name = 'YourName'`
   - Removes **all** stored slots for that player.
2. `INSERT` only the slots that still have items.

So if you:

- **Use** an item until qty is 0
- **Drop** it to the ground bag
- **Move** it out of inventory
- Clear a slot any other way

…that slot is **gone from memory**, and on the next save it is **not inserted again**. Because step 1 already deleted the old rows, that item **disappears from the database**.

Examples:

| Action | DB result after save |
|--------|----------------------|
| Had Bread x3, used 1 -> Bread x2 | Old rows deleted; one INSERT for Bread x2 |
| Had Pistol, dropped it to ground | Old rows deleted; Pistol row not inserted (no longer in inventory) |
| Emptied whole inventory | `DELETE` only - no INSERTs; player has zero rows |
| `/giveitem` then disconnect | `DELETE` then INSERTs for current slots |

Important:

- Ground bags / nearby drops are **not** saved to MySQL (memory only).
- Only the **left inventory** slots are stored in `player_inventory`.
- Key column is `owner_name` (player name). Renaming the account = different inventory rows.

Table shape:

```sql
player_inventory
  id, owner_name, slot, invItem, invModel, invQuantity
```

---

## Layout

| Path | Purpose |
|------|---------|
| `qawno/include/inventory_defs.inc` | Early API - defines + forwards |
| `qawno/include/inventory.inc` | Full implementation - slots, ops, textdraw UI |
| `gamemodes/inventory_test.pwn` | Offline test (no database) |
| `gamemodes/inventory_test_online.pwn` | Online test (MySQL save/load) |
| `scriptfiles/inventory_online.sql` | Optional SQL schema |
| `compile_inventory.bat` | Compile offline test |
| `compile_inventory_online.bat` | Compile online test |
| `config.json` | `main_scripts` + `legacy_plugins` |

## Quick start

### Compile

```bat
qawno\pawncc.exe gamemodes\inventory_test.pwn -iqawno\include -ogamemodes\inventory_test.amx
```

### Enable in `config.json`

```json
"main_scripts": [
    "inventory_test 1"
]
```

### In-game

| Command / action | Result |
|------------------|--------|
| `/giveitem` | Add 3x Bread + 1x Pistol |
| `/dropfront` | Spawn 2x Medkit on the ground in front of you |
| `/inv` | Open inventory (left bag / right nearby drops) |
| Click left slot | Select item (click again to cancel) |
| Click right slot | Select bag item, or place selected item there |
| Click 2nd slot | Move / swap / stack onto that target |
| **Use** | Fires `Inv_OnItemUse` |
| **Drop** | Drop item in front of you (shows on right panel) |
| **Close** / ESC | Close UI |

---

## Using in your gamemode

### 1) Include

```pawn
#include <open.mp>
#include <inventory>   // also pulls inventory_defs.inc
```

If another module only needs the API (forwards/defines) before the full UI is included:

```pawn
#include <inventory_defs>
```

Optional config **before** `#include <inventory>`:

```pawn
#define MAX_INVENTORY     (20)
#define INV_DROP_OBJECT_MODEL (2919)
#include <inventory>
```

### 2) Wire required callbacks

These must exist in your gamemode so the UI and lifecycle work:

```pawn
public OnPlayerConnect(playerid)
{
    Inventory_OnPlayerConnect(playerid); // clears slots for this player
    return 1;
}

public OnPlayerDisconnect(playerid, reason)
{
    #pragma unused reason
    Inventory_OnPlayerDisconnect(playerid); // hides UI + clears
    return 1;
}

public OnPlayerClickPlayerTextDraw(playerid, PlayerText:playertextid)
{
    // Slot clicks, Use / Drop / Close, click-to-move
    if (Inv_ClickPlayerTD(playerid, playertextid))
        return 1;
    return 0;
}

public OnPlayerClickTextDraw(playerid, Text:clickedid)
{
    // ESC while selecting textdraws closes inventory
    if (clickedid == Text:INVALID_TEXT_DRAW)
        Inv_HandleEscClose(playerid);
    return 0;
}

public OnDialogResponse(playerid, dialogid, response, listitem, inputtext[])
{
    // Drop-amount dialog (when qty > 1)
    if (Inventory_OnDialogResponse(playerid, dialogid, response, listitem, inputtext))
        return 1;
    return 0;
}
```

### 3) Define your items (recommended)

Keep one table instead of many `#define` pairs:

```pawn
enum
{
    ITEM_BREAD,
    ITEM_PISTOL,
    ITEM_MEDKIT
}

enum E_ITEM_INFO
{
    itemName[24],
    itemModel
}

new const g_Items[][E_ITEM_INFO] =
{
    { "Bread",  2670  }, // ITEM_BREAD
    { "Pistol", 346   }, // ITEM_PISTOL
    { "Medkit", 11738 }  // ITEM_MEDKIT
};

stock bool:IsItem(const item[], itemid)
{
    return bool:(itemid >= 0 && itemid < sizeof(g_Items)
        && !strcmp(item, g_Items[itemid][itemName], true));
}

stock GiveItem(playerid, itemid, quantity = 1)
{
    if (itemid < 0 || itemid >= sizeof(g_Items))
        return 0;
    return Inventory_Add(playerid,
        g_Items[itemid][itemName],
        g_Items[itemid][itemModel],
        quantity);
}
```

Add a new item = one enum value + one row in `g_Items`.

### 4) Give items and open UI

```pawn
// Give by helper
GiveItem(playerid, ITEM_BREAD, 3);
GiveItem(playerid, ITEM_PISTOL, 1);

// Or give by name + model directly
Inventory_Add(playerid, "Bread", 2670, 3);

// Open the textdraw inventory
Inventory_Show(playerid);

// Close
Inventory_Hide(playerid);
```

Example command:

```pawn
public OnPlayerCommandText(playerid, cmdtext[])
{
    if (!strcmp(cmdtext, "/inv", true))
    {
        Inventory_Show(playerid);
        return 1;
    }
    if (!strcmp(cmdtext, "/giveitem", true))
    {
        GiveItem(playerid, ITEM_BREAD, 3);
        GiveItem(playerid, ITEM_PISTOL, 1);
        return 1;
    }
    return 0;
}
```

### 5) Handle Use / Drop / Pickup

```pawn
public Inv_OnItemUse(playerid, slot, const item[], model, quantity)
{
    #pragma unused model, quantity

    // Consume bread
    if (IsItem(item, ITEM_BREAD))
    {
        Inventory_RemoveEx(playerid, slot, 1);
        SendClientMessage(playerid, -1, "You ate some bread.");
        return 1;
    }

    // Heal with medkit
    if (IsItem(item, ITEM_MEDKIT))
    {
        Inventory_RemoveEx(playerid, slot, 1);
        SetPlayerHealth(playerid, 100.0);
        return 1;
    }

    // Default: just notify
    new msg[72];
    format(msg, sizeof(msg), "You used: %s", item);
    SendClientMessage(playerid, -1, msg);
    return 1;
}

public Inv_OnItemDrop(playerid, slot, const item[], model, quantity)
{
    #pragma unused slot, model
    // Library already:
    //  - removed qty from inventory
    //  - put it into the nearest/front ground bag (object 2919 + 3D label)
    new msg[72];
    format(msg, sizeof(msg), "Dropped %dx %s", quantity, item);
    SendClientMessage(playerid, -1, msg);
    return 1;
}

public Inv_OnItemPickup(playerid, pileid, const item[], model, quantity)
{
    #pragma unused pileid, model
    // Fired when player moves an item from the right bag into inventory
    new msg[72];
    format(msg, sizeof(msg), "Picked up %dx %s", quantity, item);
    SendClientMessage(playerid, -1, msg);
    return 1;
}
```

### 6) Click-to-move (built in)

Textdraws cannot true mouse-drag. The include uses **click source, then click target**:

1. Click an item on the **left** (inventory) or **right** (nearest bag).
2. Click the target slot.
3. Result:
   - left -> right: deposit into that bag slot (creates bag in front if needed)
   - right -> left: take into that inventory slot
   - same panel: move / swap / stack
4. Click the same slot again to cancel.

`Use` / `Drop` / `Close` still work on the selected left item.

### 7) Ground bags (optional API)

```pawn
// Spawn / merge into bag in front of player
DropItemInFront(playerid, "Medkit", 11738, 1);

// Spawn / merge at exact coords (same bag if within INV_DROP_MERGE_RANGE)
DropItem("Bread", 2670, 2, x, y, z, interior, world);

// Nearest bag id (-1 if none)
new pileid = Item_Nearest(playerid);

// Take one slot from a bag into inventory
Inventory_PickupDropped(playerid, pileid, slot);
```

Right panel shows **only the nearest bag**. Multiple drops near each other merge into **one** object (`INV_DROP_OBJECT_MODEL`, default 2919).

### 8) MySQL online (optional)

Offline `inventory.inc` has no database. For persistence, use the pattern in `inventory_test_online.pwn`:

```pawn
#include <a_mysql>
#include <inventory>

public OnPlayerConnect(playerid)
{
    Inventory_OnPlayerConnect(playerid);
    InvDB_Load(playerid); // your SELECT -> Inventory_SetSlot(...)
    return 1;
}

public OnPlayerDisconnect(playerid, reason)
{
    InvDB_Save(playerid); // DELETE all rows for name, then INSERT current slots
    Inventory_OnPlayerDisconnect(playerid);
    return 1;
}

// Fired after add/remove/move - queue a delayed save
public Inv_OnInventoryChanged(playerid)
{
    InvDB_QueueSave(playerid);
    return 1;
}
```

Read slots for saving:

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

How DB delete works on save:

1. `DELETE FROM player_inventory WHERE owner_name = 'Name'`
2. `INSERT` only slots that still exist in memory

Used / dropped / emptied items are not re-inserted, so they are removed from MySQL automatically.

### 9) Minimal full example

```pawn
#include <open.mp>
#include <inventory>

public OnPlayerConnect(playerid)
{
    Inventory_OnPlayerConnect(playerid);
    return 1;
}

public OnPlayerDisconnect(playerid, reason)
{
    Inventory_OnPlayerDisconnect(playerid);
    return 1;
}

public OnPlayerClickPlayerTextDraw(playerid, PlayerText:playertextid)
{
    if (Inv_ClickPlayerTD(playerid, playertextid))
        return 1;
    return 0;
}

public OnPlayerClickTextDraw(playerid, Text:clickedid)
{
    if (clickedid == Text:INVALID_TEXT_DRAW)
        Inv_HandleEscClose(playerid);
    return 0;
}

public OnDialogResponse(playerid, dialogid, response, listitem, inputtext[])
{
    if (Inventory_OnDialogResponse(playerid, dialogid, response, listitem, inputtext))
        return 1;
    return 0;
}

public OnPlayerCommandText(playerid, cmdtext[])
{
    if (!strcmp(cmdtext, "/inv", true))
        return Inventory_Show(playerid), 1;

    if (!strcmp(cmdtext, "/bread", true))
    {
        Inventory_Add(playerid, "Bread", 2670, 1);
        return 1;
    }
    return 0;
}

public Inv_OnItemUse(playerid, slot, const item[], model, quantity)
{
    #pragma unused model, quantity
    if (!strcmp(item, "Bread", true))
    {
        Inventory_RemoveEx(playerid, slot, 1);
        SendClientMessage(playerid, -1, "You ate bread.");
    }
    return 1;
}

public Inv_OnItemDrop(playerid, slot, const item[], model, quantity)
{
    #pragma unused slot, model, item, quantity
    return 1;
}

public Inv_OnItemPickup(playerid, pileid, const item[], model, quantity)
{
    #pragma unused pileid, model, item, quantity
    return 1;
}
```

See also:

- Offline demo: `gamemodes/inventory_test.pwn`
- Online demo: `gamemodes/inventory_test_online.pwn`

---

## Configuration

Define **before** `#include <inventory>`:

| Define | Default | Meaning |
|--------|---------|---------|
| `MAX_INVENTORY` | `20` | Slots per player |
| `INV_UI_SLOTS` | same as `MAX_INVENTORY` | Visible grid slots |
| `MAX_INVENTORY_ITEM_NAME` | `64` | Max item name length |
| `DIALOG_INV_AMOUNT` | `2200` | Drop-amount dialog ID |
| `INV_SELECT_COLOUR` | `0xFFb05748` | Textdraw hover colour |
| `INV_DROP_OBJECT_MODEL` | `2919` | Ground bag object |
| `INV_DROP_RANGE` | `3.0` | Nearest-bag panel range |
| `INV_DROP_MERGE_RANGE` | `2.0` | Merge drops into same bag |
| `INV_DROP_FORWARD` | `1.5` | Drop distance in front of player |

---

## API

| Function | Description |
|----------|-------------|
| `Inventory_Add(playerid, item[], model, qty)` | Add / stack by name |
| `Inventory_Remove(playerid, item[], qty)` | Remove by name |
| `Inventory_RemoveEx(playerid, slot, qty)` | Remove by slot |
| `Inventory_GetSlot` / `Inventory_SetSlot` | Read / write one slot (DB load/save) |
| `Inventory_Count` / `HasItem` / `GetItemID` / `GetFreeID` | Queries |
| `Inventory_Clear` / `Items` | Wipe / used-slot count |
| `Inventory_Show` / `Hide` / `IsOpen` | Textdraw UI |
| `DropItem` / `DropItemInFront` | Create / merge ground bag |
| `Item_Nearest` / `Inventory_PickupDropped` | Find / take from bag |
| `Inv_ClickPlayerTD` | Wire from player TD clicks |
| `Inv_HandleEscClose` | Wire from ESC cancel select |
| `Inventory_OnDialogResponse` | Drop amount dialog |

### Callbacks

| Callback | When |
|----------|------|
| `Inv_OnItemUse(playerid, slot, item[], model, quantity)` | Use button |
| `Inv_OnItemDrop(playerid, slot, item[], model, quantity)` | Dropped to ground bag |
| `Inv_OnItemPickup(playerid, pileid, item[], model, quantity)` | Taken from ground bag |
| `Inv_OnInventoryChanged(playerid)` | Inventory slots changed (hook for MySQL save) |

---

## Notes

- Self-contained core - no `PlayerInfo` required.
- MySQL is optional (use `inventory_test_online.pwn` pattern).
- Include-guarded - safe to include from multiple files.
- UI matches modular grid style (5x4 model previews + side actions), not a classic list dialog.


<img width="1419" height="695" alt="image" src="https://github.com/user-attachments/assets/2d99c29b-f077-4785-abed-e14c6af5832a" />
<img width="1503" height="669" alt="image" src="https://github.com/user-attachments/assets/4fc75a7a-1daa-4dcb-a594-d3f755cec1ef" />
<img width="1486" height="695" alt="image" src="https://github.com/user-attachments/assets/c90cae20-6da9-4593-8c10-43c585cc7ed2" />

