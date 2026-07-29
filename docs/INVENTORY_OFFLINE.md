# Inventory - Offline docs

Credits: **Habibi / Janzzzz**

Back to: [Readme.md](../Readme.md) | [Docs hub](INVENTORY.md) | [Online docs](INVENTORY_ONLINE.md)

**Mode:** no database. Inventory lives in memory only.  
**Demo:** `gamemodes/inventory_test.pwn`  
**Compile:** `compile_inventory.bat`

---

## Table of contents

- [Quick start](#quick-start)
- [In-game commands](#in-game-commands)
- [Using in your gamemode](#using-in-your-gamemode)
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
| `/givegun` | Give Deagle in hand (for `/placegun`) |
| `/placegun` | Store held gun + ammo into inventory |
| `/dropfront` | Spawn Medkit bag in front of you |
| `/inv` | Open inventory UI |
| Click left slot | Select item (click again to cancel) |
| Click right slot | Select bag item, or place selected item there |
| Click 2nd slot | Move / swap / stack onto that target |
| **Use** | Use item, or equip gun |
| **Drop** | Drop into ground bag (right panel) |
| **Close** / ESC | Close UI |

---

## Using in your gamemode

### 1) Include

```pawn
#include <open.mp>
#include <inventory>   // also pulls inventory_defs.inc
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

Guns are handled by the include: **Use** on a `Gun: ...` slot calls `Inventory_EquipGun` automatically.

### 5b) Place / equip guns

Must be **holding** the weapon first:

```pawn
Inventory_PlaceGun(playerid);   // hand -> inventory (with ammo)
Inventory_EquipGun(playerid, slot); // optional manual equip
```

### 6) Click-to-move

1. Click source slot (left or right)
2. Click target slot
3. Same slot again = cancel

- left -> right: deposit to nearest/front bag  
- right -> left: take into inventory  
- same panel: move / swap / stack  

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
| `Inventory_Add` / `Remove` / `RemoveEx` | Add / remove items |
| `Inventory_GetSlot` / `SetSlot` | Read / write one slot |
| `Inventory_Count` / `HasItem` / `GetItemID` / `GetFreeID` | Queries |
| `Inventory_Clear` / `Items` | Wipe / used-slot count |
| `Inventory_Show` / `Hide` / `IsOpen` | Textdraw UI |
| `Inventory_PlaceGun` / `EquipGun` | Hand weapon <-> inventory |
| `DropItem` / `DropItemInFront` | Ground bag |
| `Item_Nearest` / `Inventory_PickupDropped` | Find / take from bag |
| `Inv_ClickPlayerTD` / `Inv_HandleEscClose` | UI wiring |
| `Inventory_OnDialogResponse` | Drop amount dialog |

### Callbacks (offline)

| Callback | When |
|----------|------|
| `Inv_OnItemUse` | Use on non-gun item |
| `Inv_OnItemDrop` | Dropped to ground bag |
| `Inv_OnItemPickup` | Taken from ground bag |
| `Inv_OnInventoryChanged` | Optional (unused offline unless you hook it) |

---

## Screenshots

<img width="1419" height="695" alt="image" src="https://github.com/user-attachments/assets/2d99c29b-f077-4785-abed-e14c6af5832a" />
<img width="1503" height="669" alt="image" src="https://github.com/user-attachments/assets/4fc75a7a-1daa-4dcb-a594-d3f755cec1ef" />
<img width="1486" height="695" alt="image" src="https://github.com/user-attachments/assets/c90cae20-6da9-4593-8c10-43c585cc7ed2" />
