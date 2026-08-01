# Inventory - Offline docs

Credits: **Habibi / Janzzzz**

Back to: [Readme.md](../Readme.md) | [Docs hub](INVENTORY.md) | [Easy examples](INVENTORY_EXAMPLES.md) | [Online docs](INVENTORY_ONLINE.md)

**Mode:** no database. Inventory lives in memory only.  
**Demo:** `gamemodes/inventory_test.pwn`  
**Compile:** `compile_inventory.bat`

> **Add an item step by step:** [INVENTORY_ADD_ITEMS.md](INVENTORY_ADD_ITEMS.md)  
> **Weapons on the panel:** [INVENTORY_WEAPONS.md](INVENTORY_WEAPONS.md)  
> **Right panel (drops / loot / house):** [INVENTORY_EXAMPLES.md](INVENTORY_EXAMPLES.md)

---

## Table of contents

- [Quick start](#quick-start)
- [In-game commands](#in-game-commands)
- [Using in your gamemode](#using-in-your-gamemode)
- [Right panel contexts](#right-panel-contexts) → [EXAMPLES](INVENTORY_EXAMPLES.md) · [ADD ITEMS](INVENTORY_ADD_ITEMS.md)
- [Configuration](#configuration)
- [API](#api)
- [Screenshots](#screenshots)

---

## Quick start

```bat
compile_inventory.bat
```

In `config.json`:

```json
"main_scripts": [
    "inventory_test 1"
]
```

No MySQL plugin needed.

---

## In-game commands

| Command / action | Result |
|------------------|--------|
| `/giveitem` | Add Bread + Medkit |
| `/givegun` | Give Deagle in hand |
| `/placegun` | Store held gun in inventory |
| `/dropfront` | Drop Medkit on ground |
| `/inv` | Open inventory (right = nearby drops) |
| `/loot` `[id]` | Loot nearest/other player (right = their inv) |
| `/houseinv` | Demo house storage on the right panel |
| Click left slot | Select item (click again to cancel) |
| Click right slot | Select / place into right context |
| Click 2nd slot | Move / swap / stack onto that target |
| Stack move (`qty > 1`) | Dialog asks how many to transfer |
| **Use** | Use item, or equip gun |
| **Drop** | Ground bag, or deposit into loot/storage (asks amount if stack) |
| **Close** / ESC | Close UI |

---

## Transfer amount dialog

When you **Drop** or click-move a **non-gun** stack with quantity greater than 1, a dialog opens:

**Transfer amount** → `Enter amount to transfer (1 - X):`

| Item | Dialog? |
|------|---------|
| Medkit x3, Bread x5, etc. | Yes - type how many (1 to max) |
| Gun (`Gun: ...`) | No - whole weapon + ammo moves as one unit |
| Quantity `1` | No - moves immediately |

Works for:
- **Drop** onto ground bag
- **Drop** into house / loot / storage
- Click-move onto an empty slot or same-item stack

`OnDialogResponse` must call `Inventory_OnDialogResponse` (dialog id `DIALOG_INV_AMOUNT`, default `2200`).

---

## Using in your gamemode

### 1) Include

```pawn
#include <open.mp>
#include <textdraw-streamer>  // optional but recommended (remove PlayerTextDraw limit)
#include <inventory>          // also pulls inventory_defs.inc
```

API-only (early):

```pawn
#include <inventory_defs>
```

Optional config **before** `#include <inventory>`:

```pawn
#define MAX_INVENTORY     (20)
#define INV_DROP_OBJECT_MODEL (2919)
#include <inventory>
```

Enable the plugin in `config.json`:

```json
"legacy_plugins": ["textdraw-streamer"]
```

### 2) Wire required callbacks

```pawn
public OnPlayerConnect(playerid)
{
    Inventory_OnPlayerConnect(playerid);
    return 1;
}

public OnPlayerDisconnect(playerid, reason)
{
    #pragma unused reason
    Inventory_OnPlayerDisconnect(playerid);
    return 1;
}

// With textdraw-streamer (demo gamemodes):
public OnClickDynamicPlayerTextDraw(playerid, PlayerText:textid)
{
    if (Inv_ClickPlayerTD(playerid, textid))
        return 1;
    return 0;
}

public OnCancelDynamicTextDraw(playerid)
{
    Inv_HandleEscClose(playerid);
    return 0;
}

// Without textdraw-streamer, use stock callbacks instead:
// public OnPlayerClickPlayerTextDraw(playerid, PlayerText:playertextid)
// public OnPlayerClickTextDraw(playerid, Text:clickedid)  // ESC = INVALID_TEXT_DRAW

public OnDialogResponse(playerid, dialogid, response, listitem, inputtext[])
{
    if (Inventory_OnDialogResponse(playerid, dialogid, response, listitem, inputtext))
        return 1;
    return 0;
}
```

### 3) Define your items

```pawn
enum
{
    ITEM_BREAD,
    ITEM_MEDKIT
}

enum E_ITEM_INFO
{
    itemName[24],
    itemModel
}

new const g_Items[][E_ITEM_INFO] =
{
    { "Bread",  2670  },
    { "Medkit", 11738 }
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

### 4) Give items and open UI

```pawn
GiveItem(playerid, ITEM_BREAD, 3);
Inventory_Add(playerid, "Bread", 2670, 3);
Inventory_Show(playerid);
Inventory_Hide(playerid);
```

### 5) Handle Use / Drop / Pickup

```pawn
public Inv_OnItemUse(playerid, slot, const item[], model, quantity)
{
    #pragma unused model, quantity
    if (IsItem(item, ITEM_BREAD))
    {
        Inventory_RemoveEx(playerid, slot, 1);
        SendClientMessage(playerid, -1, "You ate some bread.");
        return 1;
    }
    if (IsItem(item, ITEM_MEDKIT))
    {
        Inventory_RemoveEx(playerid, slot, 1);
        SetPlayerHealth(playerid, 100.0);
        return 1;
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

### 5b) Place / equip guns

Full starter guide: **[INVENTORY_WEAPONS.md](INVENTORY_WEAPONS.md)**

Must be **holding** the weapon first:

```pawn
Inventory_PlaceGun(playerid);   // hand -> inventory (with ammo)
Inventory_EquipGun(playerid, slot); // optional manual equip
```

Guns are handled by the include: **Use** on a `Gun: ...` slot calls `Inventory_EquipGun` automatically.

### 6) Click-to-move

1. Click source slot (left or right)
2. Click target slot
3. Same slot again = cancel

- left -> right: deposit to nearest/front bag  
- right -> left: take into inventory  
- same panel: move / swap / stack  
- **stack `qty > 1` (non-gun):** dialog asks how many to transfer  
- **gun:** always moves as one whole slot (ammo stays with it)  

### 7) Ground bags

```pawn
DropItemInFront(playerid, "Medkit", 11738, 1);
DropItem("Bread", 2670, 2, x, y, z, interior, world);
new pileid = Item_Nearest(playerid);
Inventory_PickupDropped(playerid, pileid, slot);
```

Right panel = **nearest bag only**. Nearby drops merge into one object.

### 8) Minimal full example

```pawn
#include <open.mp>
#include <textdraw-streamer>
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

public OnClickDynamicPlayerTextDraw(playerid, PlayerText:textid)
{
    if (Inv_ClickPlayerTD(playerid, textid))
        return 1;
    return 0;
}

public OnCancelDynamicTextDraw(playerid)
{
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
    if (!strcmp(cmdtext, "/placegun", true))
        return Inventory_PlaceGun(playerid), 1;
    return 0;
}

public Inv_OnItemUse(playerid, slot, const item[], model, quantity)
{
    #pragma unused model, quantity
    if (!strcmp(item, "Bread", true))
        Inventory_RemoveEx(playerid, slot, 1);
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

Need MySQL save/load? See [INVENTORY_ONLINE.md](INVENTORY_ONLINE.md).

---

## Configuration

Define **before** `#include <inventory>`:

| Define | Default | Meaning |
|--------|---------|---------|
| `MAX_INVENTORY` | `20` | Slots per player |
| `INV_UI_SLOTS` | same as `MAX_INVENTORY` | Visible grid slots |
| `MAX_INVENTORY_ITEM_NAME` | `64` | Max item name length |
| `DIALOG_INV_AMOUNT` | `2200` | Transfer-amount dialog ID (Drop / click-move stacks) |
| `INV_SELECT_COLOUR` | `0xFFb05748` | Textdraw hover colour |
| `INV_DROP_OBJECT_MODEL` | `2919` | Ground bag object |
| `INV_DROP_RANGE` | `3.0` | Nearest-bag panel range |
| `INV_DROP_MERGE_RANGE` | `2.0` | Merge drops into same bag |
| `INV_DROP_FORWARD` | `1.5` | Drop distance in front of player |
| `INV_NOTIFY_ENABLE` | `1` | Show Received/Removed toast on add/remove |
| `INV_NOTIFY_DURATION` | `2000` | Toast lifetime (ms) |
| `INV_NOTIFY_MAX` | `3` | Max toasts shown side-by-side (oldest shifts off) |
| `INV_LOOT_RANGE` | `5.0` | Default range for `Inventory_FindLootTarget` |

---

## Right panel contexts

Full beginner examples (drops / loot / house / trunk):  
**[INVENTORY_EXAMPLES.md](INVENTORY_EXAMPLES.md)**

Short reference:

| Mode | Constant | Right side shows |
|------|----------|------------------|
| Drops | `INV_RIGHT_DROPS` | Nearest ground bag (default `/inv`) |
| Player | `INV_RIGHT_PLAYER` | Another player's inventory (`/loot`) |
| Storage | `INV_RIGHT_STORAGE` | External buffer (house, vehicle, …) |

```pawn
Inventory_OpenLoot(playerid, targetid);

Inventory_Right_Clear(playerid);
Inventory_Right_SetSlot(playerid, 0, "Bread", 2670, 5);
Inventory_OpenStorage(playerid, houseid, "House Storage");

Inventory_ClearRightContext(playerid); // back to drops
```

On move/close in storage mode, `Inv_OnRightStorageChanged` fires — copy slots with `Inventory_Right_GetSlot` and save.

---

## API

| Function | Description |
|----------|-------------|
| `Inventory_Add` / `Remove` / `RemoveEx` | Add / remove items (toast by default; pass `false` as last arg to silence) |
| `Inventory_NotifyReceived` / `NotifyRemoved` / `ShowNotification` | Manual toast |
| `Inventory_GetSlot` / `SetSlot` | Read / write one slot (`SetSlot` is silent — use for DB load) |
| `Inventory_Count` / `HasItem` / `GetItemID` / `GetFreeID` | Queries |
| `Inventory_Clear` / `Items` | Wipe / used-slot count |
| `Inventory_Show` / `Hide` / `IsOpen` | Textdraw UI |
| `Inventory_SetRightContext` / `ClearRightContext` / `GetRightContext` | Right panel mode |
| `Inventory_Right_Clear` / `SetSlot` / `GetSlot` | External storage buffer |
| `Inventory_OpenLoot` / `OpenStorage` / `FindLootTarget` | Convenience openers |
| `Inventory_PlaceGun` / `EquipGun` | Hand weapon <-> inventory |
| `DropItem` / `DropItemInFront` | Ground bag |
| `Item_Nearest` / `Inventory_PickupDropped` | Find / take from bag |
| `Inv_ClickPlayerTD` / `Inv_HandleEscClose` | UI wiring |
| `Inventory_OnDialogResponse` | Transfer amount dialog (Drop / click-move) |

### Callbacks (offline)

| Callback | When |
|----------|------|
| `Inv_OnItemUse` | Use on non-gun item |
| `Inv_OnItemDrop` | Dropped to ground bag |
| `Inv_OnItemPickup` | Taken from ground bag |
| `Inv_OnLootTransfer` | Moved to/from another player's inventory |
| `Inv_OnRightStorageChanged` | House/storage buffer changed (or UI closed) |
| `Inv_OnInventoryChanged` | Optional (unused offline unless you hook it) |

---

## Screenshots

<img width="1419" height="695" alt="image" src="https://github.com/user-attachments/assets/2d99c29b-f077-4785-abed-e14c6af5832a" />
<img width="1503" height="669" alt="image" src="https://github.com/user-attachments/assets/4fc75a7a-1daa-4dcb-a594-d3f755cec1ef" />
<img width="1486" height="695" alt="image" src="https://github.com/user-attachments/assets/c90cae20-6da9-4593-8c10-43c585cc7ed2" />
