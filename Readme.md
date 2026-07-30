# Inventory System (open.mp)

Credits: **Habibi / Janzzzz**

Reusable textdraw inventory (`qawno/include/inventory.inc`).

```
LEFT = your bag          RIGHT = drops / loot / house / trunk
```

---

## Start here

| I want… | Open this |
|---------|-----------|
| **Add an item** so it shows on the panel | **[docs/INVENTORY_ADD_ITEMS.md](docs/INVENTORY_ADD_ITEMS.md)** |
| Easy examples (drops, loot, house) | [docs/INVENTORY_EXAMPLES.md](docs/INVENTORY_EXAMPLES.md) |
| Offline full guide (API, guns, config) | [docs/INVENTORY_OFFLINE.md](docs/INVENTORY_OFFLINE.md) |
| Online MySQL save | [docs/INVENTORY_ONLINE.md](docs/INVENTORY_ONLINE.md) |
| Offline vs online | [docs/INVENTORY.md](docs/INVENTORY.md) |

---

## Try the demo (1 minute)

```bat
compile_inventory.bat
```

`config.json`: `"main_scripts": ["inventory_test 1"]`

| Command | Right panel |
|---------|-------------|
| `/inv` | Nearby drops |
| `/loot` `[id]` | Other player |
| `/houseinv` | Demo house storage |

---

## Files

| Path | Purpose |
|------|---------|
| [qawno/include/inventory.inc](qawno/include/inventory.inc) | Inventory + UI |
| [qawno/include/inventory_defs.inc](qawno/include/inventory_defs.inc) | Defines + forwards |
| [gamemodes/inventory_test.pwn](gamemodes/inventory_test.pwn) | Offline demo |
| [gamemodes/inventory_test_online.pwn](gamemodes/inventory_test_online.pwn) | MySQL demo |
| [docs/INVENTORY_ADD_ITEMS.md](docs/INVENTORY_ADD_ITEMS.md) | Starter: add items |
| [docs/INVENTORY_EXAMPLES.md](docs/INVENTORY_EXAMPLES.md) | Easy examples |
| [docs/INVENTORY_OFFLINE.md](docs/INVENTORY_OFFLINE.md) | Offline docs |
| [docs/INVENTORY_ONLINE.md](docs/INVENTORY_ONLINE.md) | Online docs |
| [compile_inventory.bat](compile_inventory.bat) | Compile offline |
| [compile_inventory_online.bat](compile_inventory_online.bat) | Compile online |

---

## Fast compile (MySQL)

```bat
compile_inventory_online.bat
```

1. `"legacy_plugins": ["mysql"]`  
2. Edit `MYSQL_*` in `inventory_test_online.pwn`  
3. `"main_scripts": ["inventory_test_online 1"]`  

Guide: [docs/INVENTORY_ONLINE.md](docs/INVENTORY_ONLINE.md)

<img width="1436" height="558" alt="image" src="https://github.com/user-attachments/assets/b87f3b14-228a-4652-be50-a67e94e9f4d9" />
