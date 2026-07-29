# open.mp Server - Inventory System

Credits: **Habibi / Janzzzz**

Reusable textdraw inventory for open.mp. Same style as a modular player grid (model previews + Use / Drop / Close), packaged as includes under `qawno/include`.

## Layout

| Path | Purpose |
|------|---------|
| `qawno/include/inventory_defs.inc` | Early API - defines + forwards (safe to include from other modules first) |
| `qawno/include/inventory.inc` | Full implementation - slots, ops, textdraw UI |
| `gamemodes/inventory_test.pwn` | Test gamemode |
| `config.json` | Set `"inventory_test 1"` under `pawn.main_scripts` to run it |

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
| Click left slot | Select bag item |
| Click right slot | Take that item from the nearest bag |
| **Use** | Fires `Inv_OnItemUse` |
| **Drop** | Drop item in front of you (shows on right panel) |
| **Close** / ESC | Close UI |

---

## Using in your gamemode

```pawn
#include <open.mp>
#include <inventory>   // also pulls inventory_defs.inc

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

// Add by name + model (same style as modular inventories)
Inventory_Add(playerid, "Bread", 2670, 3);
Inventory_Show(playerid);

public Inv_OnItemUse(playerid, slot, const item[], model, quantity)
{
    if (!strcmp(item, "Bread", true))
        Inventory_RemoveEx(playerid, slot, 1);
    return 1;
}

public Inv_OnItemDrop(playerid, slot, const item[], model, quantity)
{
    // World drop logic goes here; library already removed the qty
    return 1;
}
```

Modules that only need the API (not the UI body) can include defs early:

```pawn
#include <inventory_defs>
```

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

---

## API

| Function | Description |
|----------|-------------|
| `Inventory_Add(playerid, item[], model, qty)` | Add / stack by name |
| `Inventory_Remove(playerid, item[], qty)` | Remove by name |
| `Inventory_RemoveEx(playerid, slot, qty)` | Remove by slot |
| `Inventory_Count` / `HasItem` / `GetItemID` / `GetFreeID` | Queries |
| `Inventory_Clear` / `Items` | Wipe / used-slot count |
| `Inventory_Show` / `Hide` / `IsOpen` | Textdraw UI |
| `Inv_ClickPlayerTD` | Wire from player TD clicks |
| `Inv_HandleEscClose` | Wire from ESC cancel select |
| `Inventory_OnDialogResponse` | Drop amount dialog |

### Callbacks

| Callback | When |
|----------|------|
| `Inv_OnItemUse(playerid, slot, item[], model, quantity)` | Use button |
| `Inv_OnItemDrop(playerid, slot, item[], model, quantity)` | Drop (qty already removed after) |

---

## Notes

- Self-contained - no `PlayerInfo`, MySQL, or gamemode-specific items.
- Include-guarded - safe to include from multiple files.
- UI matches the modular grid style (5x4 model previews + side actions), not a classic list dialog.


<img width="1419" height="695" alt="image" src="https://github.com/user-attachments/assets/2d99c29b-f077-4785-abed-e14c6af5832a" />
<img width="1503" height="669" alt="image" src="https://github.com/user-attachments/assets/4fc75a7a-1daa-4dcb-a594-d3f755cec1ef" />
<img width="1486" height="695" alt="image" src="https://github.com/user-attachments/assets/c90cae20-6da9-4593-8c10-43c585cc7ed2" />

