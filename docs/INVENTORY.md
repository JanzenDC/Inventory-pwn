# Inventory docs

Credits: **Habibi / Janzzzz**

Back to: [Readme.md](../Readme.md)

The inventory library is the same include for both modes. Choose the guide that matches how you run it:

| Guide | When to use |
|-------|-------------|
| **[INVENTORY_EXAMPLES.md](INVENTORY_EXAMPLES.md)** | **Start here** — easy copy/paste for drops, loot, house, trunk |
| [INVENTORY_OFFLINE.md](INVENTORY_OFFLINE.md) | Full offline API, guns, config |
| [INVENTORY_ONLINE.md](INVENTORY_ONLINE.md) | MySQL save / delete |

| Mode | Gamemode | Database |
|------|----------|----------|
| **Offline** | `inventory_test.pwn` | None (RAM only) |
| **Online** | `inventory_test_online.pwn` | MySQL via `a_mysql.inc` |

## Shared files

| Path | Role |
|------|------|
| `qawno/include/inventory_defs.inc` | Early API |
| `qawno/include/inventory.inc` | UI + inventory logic (used by both) |

## What is the same

- Textdraw UI (left = your bag, right = **dynamic context**)
- Right modes: nearby drops / player loot / house-storage buffer
- Click-to-move, Use / Drop / Close
- Item table pattern, placegun / equip gun
- Ground bag object + label

Right panel examples: [INVENTORY_EXAMPLES.md](INVENTORY_EXAMPLES.md)

## What is different

| | Offline | Online |
|-|---------|--------|
| Persistence | Lost on disconnect / restart | Saved to MySQL by player name |
| Extra plugin | No | MySQL plugin + `a_mysql.inc` |
| Compile | `compile_inventory.bat` | `compile_inventory_online.bat` |
| Extra commands | `/giveitem` `/givegun` `/placegun` `/dropfront` `/inv` `/loot` `/houseinv` | Same core + `/saveinv` (loot/house optional) |
| Callback for save | Not required | `Inv_OnInventoryChanged` queues DB save |
| Ground bags | Memory only | Memory only (still not in DB) |
