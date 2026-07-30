# Inventory — Weapons (starter)

Credits: **Habibi / Janzzzz**

Back to: [Readme.md](../Readme.md) | [Docs hub](INVENTORY.md) | [Add items](INVENTORY_ADD_ITEMS.md) | [Examples](INVENTORY_EXAMPLES.md)

This page is only about **guns** on the inventory panel.

---

## How guns work (important)

Guns are **not** normal table items like Bread.

| Normal item | Gun |
|-------------|-----|
| You pick a name + model | System builds `"Gun: Deagle"` from the weapon |
| `quantity` = stack count | `quantity` = **ammo** |
| `Inventory_Add(...)` | Prefer `Inventory_PlaceGun(playerid)` |

```
Hold weapon in hand  →  /placegun  →  left panel shows "Gun: ..." + A:50
Select slot + Use    →  gun back in hand, slot removed
```

---

## Step by step — store a gun on the panel

### Step 1 — Give the player a weapon in hand

```pawn
GivePlayerWeapon(playerid, WEAPON_DEAGLE, 50);
SetPlayerArmedWeapon(playerid, WEAPON_DEAGLE);
```

Or demo command from the test gamemode: `/givegun`

### Step 2 — Player must be **holding** that gun

`Inventory_PlaceGun` reads the **currently armed** weapon.  
If they are on fists, it fails with a message.

### Step 3 — Place into inventory

```pawn
if (!strcmp(cmdtext, "/placegun", true))
{
    Inventory_PlaceGun(playerid);
    return 1;
}
```

What happens inside:

1. Reads weapon id + ammo  
2. Adds `"Gun: Desert Eagle"` (name depends on weapon)  
3. Sets quantity = ammo  
4. Removes the weapon from their hands  
5. Shows **Received** toast  

### Step 4 — Open the panel

```pawn
Inventory_Show(playerid); // or /inv
```

You should see:

- Preview of the gun model  
- Name like `Gun: Desert Eagle`  
- Qty text like `A:50` (ammo, not stack)

---

## Step by step — equip from the panel

1. `/inv`  
2. Click the gun slot (select it)  
3. Press **Use**

The include detects `Gun: ...` and calls `Inventory_EquipGun` for you.

Manual call (optional):

```pawn
Inventory_EquipGun(playerid, slot);
```

That:

- Gives the weapon + ammo back  
- Clears the inventory slot  
- Shows **Removed** toast  

---

## Full copy/paste example

```pawn
#include <open.mp>
#include <inventory>

// ... normal OnPlayerConnect / click / dialog wiring ...

public OnPlayerCommandText(playerid, cmdtext[])
{
    if (!strcmp(cmdtext, "/givegun", true))
    {
        GivePlayerWeapon(playerid, WEAPON_DEAGLE, 50);
        SetPlayerArmedWeapon(playerid, WEAPON_DEAGLE);
        SendClientMessage(playerid, -1, "Deagle in hand. Type /placegun then /inv");
        return 1;
    }

    if (!strcmp(cmdtext, "/placegun", true))
    {
        Inventory_PlaceGun(playerid);
        return 1;
    }

    if (!strcmp(cmdtext, "/inv", true))
    {
        Inventory_Show(playerid);
        return 1;
    }
    return 0;
}

// Use on normal items only — guns are handled by the include
public Inv_OnItemUse(playerid, slot, const item[], model, quantity)
{
    #pragma unused slot, model, quantity
    // If item starts with "Gun: ", inventory.inc already equipped it
    // before this callback for Use button — actually Use calls EquipGun
    // directly and does NOT call Inv_OnItemUse for guns.
    SendClientMessage(playerid, -1, item);
    return 1;
}
```

In the include, **Use** on a gun slot calls `Inventory_EquipGun` and returns — `Inv_OnItemUse` is **not** used for guns.

---

## Do NOT do this for guns

```pawn
// Bad — missing Gun: prefix, ammo rules, weapon id mapping
Inventory_Add(playerid, "Deagle", 348, 50);
```

Prefer:

```pawn
Inventory_PlaceGun(playerid); // correct name, model, ammo
```

If you must create a gun slot from script/DB without the player holding it, use the same format the system uses:

```pawn
// Advanced / DB restore only
Inventory_Add(playerid, "Gun: Desert Eagle", 348, 50);
// name must start with "Gun: "
// model must match Inv_WeaponModels for that weapon
// quantity = ammo
```

Safer for DB load: `Inventory_SetSlot` with the same fields (silent, no toast).

---

## Drop / move guns

| Action | Result |
|--------|--------|
| **Drop** on a gun | Whole gun + ammo goes to ground bag (no amount dialog) |
| Click-move to right | Same — moves as one slot |
| Pickup from bag | Gun returns to left panel with ammo |

---

## Checklist (gun not showing?)

| Problem | Fix |
|---------|-----|
| `/placegun` says must hold a gun | Arm the weapon (`SetPlayerArmedWeapon`) |
| Fist / invalid weapon | Only weapons with a preview model work |
| Inventory full | Free a slot |
| Added with wrong name | Must start with `Gun: ` for Use→equip |
| Qty looks wrong | For guns, number is **ammo** (`A:50`) |

---

## Demo commands (offline test)

| Command | What it does |
|---------|----------------|
| `/givegun` | Puts Deagle (50 ammo) in hand |
| `/placegun` | Stores held gun on left panel |
| `/inv` | Open UI — Use to equip again |

---

## Related

| Doc | Content |
|-----|---------|
| [INVENTORY_ADD_ITEMS.md](INVENTORY_ADD_ITEMS.md) | Normal items (Bread, Water, …) |
| [INVENTORY_EXAMPLES.md](INVENTORY_EXAMPLES.md) | Right panels |
| [INVENTORY_OFFLINE.md](INVENTORY_OFFLINE.md) | Full API |
| [INVENTORY_ONLINE.md](INVENTORY_ONLINE.md) | MySQL — gun ammo saved as `invQuantity` |
