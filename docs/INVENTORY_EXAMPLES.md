# Inventory — Easy examples (right panels)

Credits: **Habibi / Janzzzz**

Back to: [Readme.md](../Readme.md) | [Docs hub](INVENTORY.md) | [Add items](INVENTORY_ADD_ITEMS.md) | [Weapons](INVENTORY_WEAPONS.md) | [Offline](INVENTORY_OFFLINE.md) | [Online](INVENTORY_ONLINE.md)

This page is the **simple copy/paste guide** for right panels (drops / loot / house).  
New item not showing? Start with **[INVENTORY_ADD_ITEMS.md](INVENTORY_ADD_ITEMS.md)**.  
Guns / placegun? Start with **[INVENTORY_WEAPONS.md](INVENTORY_WEAPONS.md)**.  
For full API tables see [INVENTORY_OFFLINE.md](INVENTORY_OFFLINE.md).

---

## How the UI works

```
┌─────────────────┐     Use / Drop / Close     ┌─────────────────┐
│  LEFT = YOU     │                            │  RIGHT = CONTEXT│
│  Your items     │◄──── click to move ───────►│  Changes mode   │
└─────────────────┘                            └─────────────────┘
```

| Side | Always shows |
|------|----------------|
| **Left** | Your personal inventory |
| **Right** | Whatever you opened: ground bag, another player, house, trunk, … |

You do **not** make a new UI for house or loot. You only tell the right panel *what* to show.

---

## Try it in 1 minute (offline demo)

1. Run `compile_inventory.bat`
2. In `config.json` set: `"main_scripts": ["inventory_test 1"]`
3. Start the server and join
4. Try these commands:

| Command | What you see on the RIGHT |
|---------|---------------------------|
| `/giveitem` then `/inv` | Nearby ground bag (drops) |
| `/dropfront` then `/inv` | Items on the ground bag |
| `/loot` or `/loot [id]` | That player's inventory |
| `/houseinv` | Demo house storage |

**How to move items:** click one slot, then click another slot (left ↔ right).

---

## Minimal setup in YOUR gamemode

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

// Example: give an item
public OnPlayerCommandText(playerid, cmdtext[])
{
    if (!strcmp(cmdtext, "/inv", true))
    {
        Inventory_Show(playerid);
        return 1;
    }
    if (!strcmp(cmdtext, "/bread", true))
    {
        Inventory_Add(playerid, "Bread", 2670, 1); // name, model id, qty
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
```

More wiring: [Using in your gamemode](INVENTORY_OFFLINE.md#using-in-your-gamemode).

---

## Right panel modes

| Mode | Constant | Example |
|------|----------|---------|
| Ground bag | `INV_RIGHT_DROPS` | Default `/inv` |
| Other player | `INV_RIGHT_PLAYER` | `/loot` |
| Your storage | `INV_RIGHT_STORAGE` | House, trunk, fridge |

---

## Example A — Nearby drops (default)

**When:** Player presses `/inv` while walking around.

**What happens:** Left = their bag. Right = nearest dropped bag on the ground.

```pawn
if (!strcmp(cmdtext, "/inv", true))
{
    Inventory_ClearRightContext(playerid); // make sure right = drops
    Inventory_Show(playerid);
    return 1;
}
```

Optional callbacks:

```pawn
public Inv_OnItemDrop(playerid, slot, const item[], model, quantity)
{
    // player dropped something to the ground
    return 1;
}

public Inv_OnItemPickup(playerid, pileid, const item[], model, quantity)
{
    // player took something from the ground bag
    return 1;
}
```

Demo command: `/inv` in `inventory_test.pwn`.

---

## Example B — Loot another player

**When:** Player is near a downed / dead / cuffed player and types `/loot`.

**What happens:** Left = looter. Right = **target's** inventory. Moving right → left takes the item.

```pawn
if (!strcmp(cmdtext, "/loot", true, 5))
{
    new targetid;

    // /loot 3  → loot player id 3
    // /loot    → nearest player in range
    if (cmdtext[5] == ' ' && cmdtext[6] != EOS)
        targetid = strval(cmdtext[6]);
    else
        targetid = Inventory_FindLootTarget(playerid);

    if (targetid == INVALID_PLAYER_ID || !IsPlayerConnected(targetid))
    {
        SendClientMessage(playerid, -1, "No player to loot. Usage: /loot [id]");
        return 1;
    }

    // YOUR RULES HERE, for example:
    // if (!IsPlayerDowned(targetid)) return SendClientMessage(...);

    Inventory_OpenLoot(playerid, targetid);
    return 1;
}

public Inv_OnLootTransfer(playerid, targetid, bool:taking, const item[], model, quantity)
{
    #pragma unused model
    new name[MAX_PLAYER_NAME], msg[96];
    GetPlayerName(targetid, name, sizeof(name));

    if (taking)
        format(msg, sizeof(msg), "You took %dx %s from %s.", quantity, item, name);
    else
        format(msg, sizeof(msg), "You put %dx %s into %s.", quantity, item, name);

    SendClientMessage(playerid, -1, msg);
    return 1;
}
```

Same idea without the helper:

```pawn
Inventory_SetRightContext(playerid, INV_RIGHT_PLAYER, targetid, "Loot");
Inventory_Show(playerid);
```

Demo command: `/loot` or `/loot [id]` in `inventory_test.pwn`.

---

## Example C — House storage

**When:** Player is inside their house and opens the stash.

**Steps:**

1. **Clear** the right buffer  
2. **Load** house items into the buffer  
3. **Open** storage mode  
4. **Save** when something changes (`Inv_OnRightStorageChanged`)

```pawn
// Your house data (example — use MySQL in a real server)
new bool:g_HouseHasItem[MAX_INVENTORY];
new g_HouseItemName[MAX_INVENTORY][64];
new g_HouseItemModel[MAX_INVENTORY];
new g_HouseItemQty[MAX_INVENTORY];

stock OpenMyHouse(playerid, houseid)
{
    #pragma unused houseid

    // 1) empty right panel buffer
    Inventory_Right_Clear(playerid);

    // 2) copy house → right panel
    for (new slot = 0; slot < MAX_INVENTORY; slot++)
    {
        if (g_HouseHasItem[slot])
            Inventory_Right_SetSlot(playerid, slot,
                g_HouseItemName[slot],
                g_HouseItemModel[slot],
                g_HouseItemQty[slot]);
    }

    // 3) open UI (right title = "House Storage")
    Inventory_OpenStorage(playerid, houseid, "House Storage");
    return 1;
}

// 4) save right panel → house (called on move + on close)
public Inv_OnRightStorageChanged(playerid, storageid)
{
    #pragma unused storageid

    for (new slot = 0; slot < MAX_INVENTORY; slot++)
    {
        new item[64], model, qty;
        if (Inventory_Right_GetSlot(playerid, slot, item, sizeof(item), model, qty))
        {
            g_HouseHasItem[slot] = true;
            format(g_HouseItemName[slot], sizeof(g_HouseItemName[]), "%s", item);
            g_HouseItemModel[slot] = model;
            g_HouseItemQty[slot] = qty;
        }
        else
        {
            g_HouseHasItem[slot] = false;
            g_HouseItemName[slot][0] = EOS;
            g_HouseItemModel[slot] = 0;
            g_HouseItemQty[slot] = 0;
        }
    }

    // Here you would also SAVE TO MYSQL if online
    SendClientMessage(playerid, -1, "House storage saved.");
    return 1;
}

// Command example
if (!strcmp(cmdtext, "/houseinv", true))
{
    OpenMyHouse(playerid, 0);
    return 1;
}
```

**Vehicle trunk** = same pattern. Change the title and use `vehicleid`:

```pawn
Inventory_OpenStorage(playerid, vehicleid, "Trunk");
```

Demo command: `/houseinv` in `inventory_test.pwn`.

---

## Example D — Back to normal drops

```pawn
Inventory_ClearRightContext(playerid);
Inventory_Show(playerid);
```

Closing with **Close** or **ESC** also resets the right panel to nearby drops.

---

## How players use the UI

| They do this | Result |
|--------------|--------|
| Click item, click empty/other slot | Move / swap / stack |
| Move a stack (`qty > 1`, not a gun) | **Transfer amount** dialog - type how many |
| **Use** | Use food/item, or equip a gun from inventory |
| **Drop** while right = drops | Put item on the ground bag (asks amount if stack) |
| **Drop** while right = loot/house | Put item into the right panel (asks amount if stack) |
| **Drop** / move a gun | Whole gun + ammo, no amount dialog |
| **Close** / ESC | Close UI |

### Transfer amount

Dialog title: **Transfer amount**  
Body: `Enter amount to transfer (1 - X):`

Wire it in your gamemode:

```pawn
public OnDialogResponse(playerid, dialogid, response, listitem, inputtext[])
{
    if (Inventory_OnDialogResponse(playerid, dialogid, response, listitem, inputtext))
        return 1;
    return 0;
}
```

Default dialog id: `DIALOG_INV_AMOUNT` (`2200`). Override before `#include <inventory>` if needed.

---

## Common questions

**Q: Do I need a new textdraw UI for house?**  
No. Same UI. Only the right panel data changes.

**Q: Can I use loot + house + drops together?**  
Yes - one at a time. Each open sets one right mode.

**Q: Why no amount dialog for my Deagle?**  
Guns use quantity as **ammo**. They always move as one unit. See [INVENTORY_WEAPONS.md](INVENTORY_WEAPONS.md).

**Q: Where do toasts (Received / Removed) come from?**  
Automatic on add/remove. See [offline config](INVENTORY_OFFLINE.md#configuration) for `INV_NOTIFY_*`.

**Q: Online MySQL?**  
Player inventory save: [INVENTORY_ONLINE.md](INVENTORY_ONLINE.md).  
House storage save is still your job inside `Inv_OnRightStorageChanged` (Example C).

---

## Related docs

| Doc | Content |
|-----|---------|
| [INVENTORY_ADD_ITEMS.md](INVENTORY_ADD_ITEMS.md) | Add items step by step |
| [INVENTORY_WEAPONS.md](INVENTORY_WEAPONS.md) | Place / equip guns |
| [INVENTORY.md](INVENTORY.md) | Hub / offline vs online |
| [INVENTORY_OFFLINE.md](INVENTORY_OFFLINE.md) | Full offline API + config |
| [INVENTORY_ONLINE.md](INVENTORY_ONLINE.md) | MySQL save / delete |
