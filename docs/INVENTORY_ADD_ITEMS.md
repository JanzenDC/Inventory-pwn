# Inventory — How to add an item (starter)

Credits: **Habibi / Janzzzz**

Back to: [Readme.md](../Readme.md) | [Docs hub](INVENTORY.md) | [Easy examples](INVENTORY_EXAMPLES.md) | [Offline](INVENTORY_OFFLINE.md)

This is a **step-by-step** guide: create an item → give it to a player → see it on the left panel.

---

## What you need for one item

Every inventory item is just **3 things**:

| Piece | Example | Meaning |
|-------|---------|---------|
| **Name** | `"Water"` | Text under the preview on the panel |
| **Model** | `1484` | Object model shown in the slot |
| **Quantity** | `1` | How many (or ammo for guns) |

The include does **not** have a fixed item list.  
**Your gamemode** decides names + models, then calls `Inventory_Add`.

```
Inventory_Add(playerid, "Water", 1484, 1)
        │              │         │     │
        │              │         │     └─ quantity
        │              │         └─────── 3D preview model
        │              └───────────────── label on the panel
        └──────────────────────────────── who receives it
```

Open `/inv` → the item appears on the **left** panel.

---

## Fastest path (1 item, no table)

### Step 1 — Wire inventory (once)

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
```

`OnDialogResponse` is required for the **Transfer amount** dialog when dropping/moving stacks (`qty > 1`).

### Step 2 — Give the item

```pawn
public OnPlayerCommandText(playerid, cmdtext[])
{
    if (!strcmp(cmdtext, "/givewater", true))
    {
        // name        model   qty
        Inventory_Add(playerid, "Water", 1484, 1);
        SendClientMessage(playerid, -1, "You got Water. Type /inv");
        return 1;
    }

    if (!strcmp(cmdtext, "/inv", true))
    {
        Inventory_Show(playerid);
        return 1;
    }
    return 0;
}
```

### Step 3 — See it on the panel

1. Compile and join  
2. Type `/givewater`  
3. Type `/inv`  
4. Left panel shows **Water** with model `1484`

You should also see a short **Received** toast.

Stacks: `Inventory_Add(playerid, "Water", 1484, 5)` gives 5 Water. Drop or click-move asks **how many** to transfer. Details: [INVENTORY_OFFLINE.md - Transfer amount](INVENTORY_OFFLINE.md#transfer-amount-dialog).

---

## Recommended path (item table — add many items)

Use one table so every item has a name + model in one place (same style as `inventory_test.pwn`).

### Step 1 — Add an ID in the enum

```pawn
enum
{
    ITEM_BREAD,
    ITEM_MEDKIT,
    ITEM_WATER   // ← new
}
```

### Step 2 — Add name + model in the table

**Order must match the enum** (first row = first enum value).

```pawn
enum E_ITEM_INFO
{
    itemName[24],
    itemModel
}

new const g_Items[][E_ITEM_INFO] =
{
    { "Bread",  2670  }, // ITEM_BREAD
    { "Medkit", 11738 }, // ITEM_MEDKIT
    { "Water",  1484  }  // ITEM_WATER  ← new
};
```

### Step 3 — Keep these helpers (copy once)

```pawn
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

### Step 4 — Give it and open `/inv`

```pawn
GiveItem(playerid, ITEM_WATER, 2);  // 2x Water
Inventory_Show(playerid);
```

Or with a command:

```pawn
if (!strcmp(cmdtext, "/givewater", true))
{
    GiveItem(playerid, ITEM_WATER, 1);
    SendClientMessage(playerid, -1, "Gave 1x Water. Open /inv");
    return 1;
}
```

### Step 5 — (Optional) Make **Use** do something

```pawn
public Inv_OnItemUse(playerid, slot, const item[], model, quantity)
{
    #pragma unused model, quantity

    if (IsItem(item, ITEM_WATER))
    {
        Inventory_RemoveEx(playerid, slot, 1); // remove 1 from panel
        SendClientMessage(playerid, -1, "You drank water.");
        return 1;
    }

    // ... other items
    return 1;
}
```

Without this callback, the item still **shows** on the panel — Use just won’t have custom behavior.

---

## Checklist (item not showing?)

| Check | Fix |
|-------|-----|
| Did you call `Inventory_Add` / `GiveItem`? | Item only appears after add |
| Did you open `/inv`? | Call `Inventory_Show(playerid)` |
| Is the model id valid? | Wrong model = empty / invisible preview |
| Is inventory full (20 slots)? | Free a slot or raise `MAX_INVENTORY` |
| Is `Inventory_OnPlayerConnect` called? | Required so data is ready |
| Are click callbacks wired? | Needed to open/use the UI |
| Enum row ≠ table row? | Same order in enum and `g_Items` |

---

## Useful model ids (examples)

| Item idea | Name | Model |
|-----------|------|-------|
| Bread | `"Bread"` | `2670` |
| Medkit | `"Medkit"` | `11738` |
| Water / bottle | `"Water"` | `1484` |
| Money stack (prop) | `"Cash"` | `1212` |
| Phone | `"Phone"` | `330` |

Find more object models in any SA object browser (e.g. objects.gta3 / open.mp docs).

---

## Guns (special case)

Guns are different from Bread/Water. Full walkthrough:

**[INVENTORY_WEAPONS.md](INVENTORY_WEAPONS.md)**

Short version — hold weapon, then:

```pawn
Inventory_PlaceGun(playerid); // hand → left panel as "Gun: Deagle" + ammo
```

In `/inv`, select it and press **Use** to equip again.

---

## Put item on the ground (right panel)

```pawn
DropItemInFront(playerid, "Water", 1484, 1);
Inventory_Show(playerid); // right = nearest bag
```

Or with the table:

```pawn
DropItemInFront(playerid, g_Items[ITEM_WATER][itemName], g_Items[ITEM_WATER][itemModel], 1);
```

---

## Online (MySQL) note

Same steps. After `Inventory_Add`, online demo auto-saves via `Inv_OnInventoryChanged`.  
You still add items the same way — only persistence is different. See [INVENTORY_ONLINE.md](INVENTORY_ONLINE.md).

---

## Next reads

| Doc | Why |
|-----|-----|
| [INVENTORY_ADD_ITEMS.md](INVENTORY_ADD_ITEMS.md) | Normal items (Bread, Water, …) |
| [INVENTORY_WEAPONS.md](INVENTORY_WEAPONS.md) | Place / equip guns |
| [INVENTORY_EXAMPLES.md](INVENTORY_EXAMPLES.md) | Right panels |
| [INVENTORY_OFFLINE.md](INVENTORY_OFFLINE.md) | Full API |
| [INVENTORY_ONLINE.md](INVENTORY_ONLINE.md) | MySQL save |
