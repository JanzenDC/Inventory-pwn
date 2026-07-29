# open.mp Server - Inventory System

Credits: **Habibi / Janzzzz**

Reusable textdraw inventory for open.mp (`qawno/include/inventory.inc`).

---

## Choose your mode

| Mode | Guide | Demo gamemode |
|------|-------|----------------|
| **Offline** (no database) | [docs/INVENTORY_OFFLINE.md](docs/INVENTORY_OFFLINE.md) | `inventory_test.pwn` |
| **Online** (MySQL) | [docs/INVENTORY_ONLINE.md](docs/INVENTORY_ONLINE.md) | `inventory_test_online.pwn` |

Hub / comparison: [docs/INVENTORY.md](docs/INVENTORY.md)

---

## Structure

| Path | Purpose |
|------|---------|
| [qawno/include/inventory_defs.inc](qawno/include/inventory_defs.inc) | Early API (defines + forwards) |
| [qawno/include/inventory.inc](qawno/include/inventory.inc) | Full inventory + textdraw UI |
| [gamemodes/inventory_test.pwn](gamemodes/inventory_test.pwn) | Offline test |
| [gamemodes/inventory_test_online.pwn](gamemodes/inventory_test_online.pwn) | Online test (MySQL) |
| [scriptfiles/inventory_online.sql](scriptfiles/inventory_online.sql) | Optional SQL schema |
| [compile_inventory.bat](compile_inventory.bat) | Compile offline |
| [compile_inventory_online.bat](compile_inventory_online.bat) | Compile online |
| [config.json](config.json) | `main_scripts` + `legacy_plugins` |
| [docs/INVENTORY.md](docs/INVENTORY.md) | Docs hub |
| [docs/INVENTORY_OFFLINE.md](docs/INVENTORY_OFFLINE.md) | Offline documentation |
| [docs/INVENTORY_ONLINE.md](docs/INVENTORY_ONLINE.md) | Online / MySQL documentation |

---

## Docs index

### Offline

| Topic | Link |
|-------|------|
| Quick start | [Offline quick start](docs/INVENTORY_OFFLINE.md#quick-start) |
| Using in your gamemode | [Using in your gamemode](docs/INVENTORY_OFFLINE.md#using-in-your-gamemode) |
| Place / equip guns | [Place / equip guns](docs/INVENTORY_OFFLINE.md#5b-place--equip-guns) |
| Click-to-move | [Click-to-move](docs/INVENTORY_OFFLINE.md#6-click-to-move) |
| Configuration + API | [Configuration](docs/INVENTORY_OFFLINE.md#configuration) / [API](docs/INVENTORY_OFFLINE.md#api) |

### Online (MySQL)

| Topic | Link |
|-------|------|
| Setup | [Online setup](docs/INVENTORY_ONLINE.md#setup) |
| Save flow | [Save flow](docs/INVENTORY_ONLINE.md#save-flow) |
| How DB delete works | [How delete works](docs/INVENTORY_ONLINE.md#how-delete-works-in-the-database) |
| Wire MySQL in gamemode | [Wire MySQL](docs/INVENTORY_ONLINE.md#wire-mysql-in-your-gamemode) |
| Offline vs online | [Differences](docs/INVENTORY_ONLINE.md#differences-from-offline) |

---

## Fast start

**Offline**

```bat
compile_inventory.bat
```

`config.json`: `"main_scripts": ["inventory_test 1"]`  
Guide: [INVENTORY_OFFLINE.md](docs/INVENTORY_OFFLINE.md)

**Online (MySQL)**

```bat
compile_inventory_online.bat
```

1. `"legacy_plugins": ["mysql"]`
2. Edit `MYSQL_*` in `inventory_test_online.pwn`
3. `"main_scripts": ["inventory_test_online 1"]`  

Guide: [INVENTORY_ONLINE.md](docs/INVENTORY_ONLINE.md)

<img width="1436" height="558" alt="image" src="https://github.com/user-attachments/assets/b87f3b14-228a-4652-be50-a67e94e9f4d9" />

