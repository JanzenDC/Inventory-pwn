# Inventory docs

Credits: **Habibi / Janzzzz**

Back to: [Readme.md](../Readme.md)

The inventory library is the same include for both modes. Choose the guide that matches how you run it:

| Mode | Gamemode | Database | Guide |
|------|----------|----------|-------|
| **Offline** | `inventory_test.pwn` | None (RAM only) | [INVENTORY_OFFLINE.md](INVENTORY_OFFLINE.md) |
| **Online** | `inventory_test_online.pwn` | MySQL via `a_mysql.inc` | [INVENTORY_ONLINE.md](INVENTORY_ONLINE.md) |

## Shared files

| Path | Role |
|------|------|
| `qawno/include/inventory_defs.inc` | Early API |
| `qawno/include/inventory.inc` | UI + inventory logic (used by both) |

## What is the same

- Textdraw UI (left bag / right nearest bag)
- Click-to-move, Use / Drop / Close
- Item table pattern, placegun / equip gun
- Ground bag object + label

## What is different

| | Offline | Online |
|-|---------|--------|
| Persistence | Lost on disconnect / restart | Saved to MySQL by player name |
| Extra plugin | No | MySQL plugin + `a_mysql.inc` |
| Compile | `compile_inventory.bat` | `compile_inventory_online.bat` |
| Extra commands | `/giveitem` `/givegun` `/placegun` `/dropfront` `/inv` | Same + `/saveinv` |
| Callback for save | Not required | `Inv_OnInventoryChanged` queues DB save |
| Ground bags | Memory only | Memory only (still not in DB) |
