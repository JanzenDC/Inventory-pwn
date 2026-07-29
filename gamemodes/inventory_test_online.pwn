/*
    Inventory Test Online (MySQL)
    Same inventory UI as inventory_test.pwn, with player inventory saved to database.

    Credits: Habibi / Janzzzz

    Requires:
      - MySQL plugin (R39) in plugins/ (config.json -> pawn.legacy_plugins)
      - a_mysql.inc (already in qawno/include)
      - Edit MYSQL_* defines below to match your server

    Commands:
      /giveitem   - give Bread + Medkit
      /givegun    - give a Deagle in your hand (for /placegun)
      /placegun   - store held gun + ammo into inventory
      /dropfront  - drop Medkit in front
      /inv        - open inventory
      /saveinv    - force-save inventory now
*/

#include <open.mp>
#include <a_mysql>
#include <inventory>

// ---------------------------------------------------------------------------
// MySQL login - change these
// ---------------------------------------------------------------------------
#define MYSQL_HOST     "127.0.0.1"
#define MYSQL_USER     "root"
#define MYSQL_PASS     ""
#define MYSQL_DB       "inventory_test"
#define MYSQL_PORT     (3306)

new g_SQL;

// One row per item - add new items here only.
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

new g_InvSaveTimer[MAX_PLAYERS];
new bool:g_InvLoaded[MAX_PLAYERS];

stock bool:IsItem(const item[], itemid)
{
    return bool:(itemid >= 0 && itemid < sizeof(g_Items) && !strcmp(item, g_Items[itemid][itemName], true));
}

stock GiveItem(playerid, itemid, quantity = 1)
{
    if (itemid < 0 || itemid >= sizeof(g_Items))
        return 0;
    return Inventory_Add(playerid, g_Items[itemid][itemName], g_Items[itemid][itemModel], quantity);
}

stock DropFront(playerid, itemid, quantity = 1)
{
    if (itemid < 0 || itemid >= sizeof(g_Items))
        return -1;
    return DropItemInFront(playerid, g_Items[itemid][itemName], g_Items[itemid][itemModel], quantity);
}

// ---------------------------------------------------------------------------
// Database
// ---------------------------------------------------------------------------

stock InvDB_Init()
{
    mysql_log(LOG_ERROR | LOG_WARNING);
    g_SQL = mysql_connect(MYSQL_HOST, MYSQL_USER, MYSQL_DB, MYSQL_PASS, MYSQL_PORT);

    if (mysql_errno(g_SQL) != 0)
    {
        printf("[inventory_test_online] MySQL connect FAILED (errno %d). Check MYSQL_* defines.", mysql_errno(g_SQL));
        return 0;
    }

    print("[inventory_test_online] MySQL connected.");

    mysql_tquery(g_SQL,
        "CREATE TABLE IF NOT EXISTS `player_inventory` (\
            `id` INT NOT NULL AUTO_INCREMENT,\
            `owner_name` VARCHAR(24) NOT NULL,\
            `slot` TINYINT NOT NULL,\
            `invItem` VARCHAR(64) NOT NULL,\
            `invModel` INT NOT NULL,\
            `invQuantity` INT NOT NULL,\
            PRIMARY KEY (`id`),\
            UNIQUE KEY `owner_slot` (`owner_name`, `slot`)\
        ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");
    return 1;
}

stock InvDB_Load(playerid)
{
    if (g_SQL < 1)
        return 0;

    new name[MAX_PLAYER_NAME], query[160];
    GetPlayerName(playerid, name, sizeof(name));
    g_InvLoaded[playerid] = false;

    mysql_format(g_SQL, query, sizeof(query),
        "SELECT `slot`, `invItem`, `invModel`, `invQuantity` FROM `player_inventory` WHERE `owner_name` = '%e'",
        name);
    mysql_tquery(g_SQL, query, "OnInventoryDBLoad", "d", playerid);
    return 1;
}

forward OnInventoryDBLoad(playerid);
public OnInventoryDBLoad(playerid)
{
    if (!IsPlayerConnected(playerid))
        return 0;

    Inventory_Clear(playerid);

    new rows = cache_get_row_count();
    for (new i = 0; i < rows; i++)
    {
        new slot = cache_get_field_content_int(i, "slot");
        new model = cache_get_field_content_int(i, "invModel");
        new qty = cache_get_field_content_int(i, "invQuantity");
        new item[MAX_INVENTORY_ITEM_NAME];
        cache_get_field_content(i, "invItem", item, g_SQL, sizeof(item));

        if (slot < 0 || slot >= MAX_INVENTORY || qty < 1)
            continue;
        Inventory_SetSlot(playerid, slot, item, model, qty);
    }

    g_InvLoaded[playerid] = true;
    SendClientMessage(playerid, 0x33CCFFFF, "Inventory loaded from database.");
    return 1;
}

forward InvDB_Save(playerid);
public InvDB_Save(playerid)
{
    g_InvSaveTimer[playerid] = 0;
    if (!IsPlayerConnected(playerid) || !g_InvLoaded[playerid] || g_SQL < 1)
        return 0;

    new name[MAX_PLAYER_NAME], query[320], item[MAX_INVENTORY_ITEM_NAME], model, qty;
    GetPlayerName(playerid, name, sizeof(name));

    // Wipe then rewrite (sync so order is safe)
    mysql_format(g_SQL, query, sizeof(query), "DELETE FROM `player_inventory` WHERE `owner_name` = '%e'", name);
    mysql_query(g_SQL, query, false);

    for (new slot = 0; slot < MAX_INVENTORY; slot++)
    {
        if (!Inventory_GetSlot(playerid, slot, item, sizeof(item), model, qty))
            continue;

        mysql_format(g_SQL, query, sizeof(query),
            "INSERT INTO `player_inventory` (`owner_name`, `slot`, `invItem`, `invModel`, `invQuantity`) \
             VALUES ('%e', %d, '%e', %d, %d)",
            name, slot, item, model, qty);
        mysql_query(g_SQL, query, false);
    }
    return 1;
}

stock InvDB_QueueSave(playerid)
{
    if (!g_InvLoaded[playerid])
        return 0;
    if (g_InvSaveTimer[playerid])
        KillTimer(g_InvSaveTimer[playerid]);
    g_InvSaveTimer[playerid] = SetTimerEx("InvDB_Save", 1200, false, "i", playerid);
    return 1;
}

// ---------------------------------------------------------------------------
// Gamemode
// ---------------------------------------------------------------------------

main()
{
    print("----------");
    print("Inventory Test Online loaded.");
    print("Credits: Habibi / Janzzzz");
    print("----------");
}

public OnGameModeInit()
{
    SetGameModeText("Inventory Online");
    AddPlayerClass(0, 1958.3783, 1343.1572, 15.3746, 269.1425, WEAPON:0, 0, WEAPON:0, 0, WEAPON:0, 0);
    InvDB_Init();
    return 1;
}

public OnGameModeExit()
{
    if (g_SQL >= 1)
        mysql_close(g_SQL);
    return 1;
}

public OnPlayerConnect(playerid)
{
    g_InvSaveTimer[playerid] = 0;
    g_InvLoaded[playerid] = false;
    Inventory_OnPlayerConnect(playerid);
    InvDB_Load(playerid);

    SendClientMessage(playerid, 0xFFFFFFFF, "Welcome! MySQL inventory. /giveitem /givegun /placegun /dropfront /inv /saveinv");
    SendClientMessage(playerid, 0xAAAAAAFF, "Hold a gun + /placegun to store. In /inv press Use to equip. Guns auto-save ammo.");
    return 1;
}

public OnPlayerDisconnect(playerid, reason)
{
    #pragma unused reason
    if (g_InvSaveTimer[playerid])
    {
        KillTimer(g_InvSaveTimer[playerid]);
        g_InvSaveTimer[playerid] = 0;
    }
    InvDB_Save(playerid);
    Inventory_OnPlayerDisconnect(playerid);
    g_InvLoaded[playerid] = false;
    return 1;
}

public OnPlayerSpawn(playerid)
{
    DropFront(playerid, ITEM_MEDKIT, 1);
    return 1;
}

public OnPlayerCommandText(playerid, cmdtext[])
{
    if (!strcmp(cmdtext, "/giveitem", true))
    {
        GiveItem(playerid, ITEM_BREAD, 3);
        GiveItem(playerid, ITEM_MEDKIT, 1);
        SendClientMessage(playerid, 0xFFFFFFFF, "Gave you 3x Bread and 1x Medkit (will auto-save).");
        return 1;
    }

    if (!strcmp(cmdtext, "/givegun", true))
    {
        GivePlayerWeapon(playerid, WEAPON_DEAGLE, 50);
        SetPlayerArmedWeapon(playerid, WEAPON_DEAGLE);
        SendClientMessage(playerid, 0xFFFFFFFF, "Gave you a Desert Eagle (50 ammo). Use /placegun while holding it.");
        return 1;
    }

    if (!strcmp(cmdtext, "/placegun", true))
    {
        Inventory_PlaceGun(playerid);
        return 1;
    }

    if (!strcmp(cmdtext, "/dropfront", true))
    {
        if (DropFront(playerid, ITEM_MEDKIT, 2) == -1)
            SendClientMessage(playerid, 0xFFFF00AA, "Could not create a dropped item.");
        else
            SendClientMessage(playerid, 0x33CCFFFF, "Dropped 2x Medkit in front of you.");
        return 1;
    }

    if (!strcmp(cmdtext, "/inv", true))
    {
        Inventory_Show(playerid);
        return 1;
    }

    if (!strcmp(cmdtext, "/saveinv", true))
    {
        InvDB_Save(playerid);
        SendClientMessage(playerid, 0x33CCFFFF, "Inventory saved.");
        return 1;
    }

    return 0;
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

public Inv_OnInventoryChanged(playerid)
{
    InvDB_QueueSave(playerid);
    return 1;
}

public Inv_OnItemUse(playerid, slot, const item[], model, quantity)
{
    #pragma unused model, quantity
    if (IsItem(item, ITEM_BREAD))
    {
        Inventory_RemoveEx(playerid, slot, 1);
        SendClientMessage(playerid, 0xFFFFFFFF, "You ate some bread.");
        return 1;
    }
    if (IsItem(item, ITEM_MEDKIT))
    {
        Inventory_RemoveEx(playerid, slot, 1);
        SetPlayerHealth(playerid, 100.0);
        SendClientMessage(playerid, 0xFFFFFFFF, "You used a Medkit.");
        return 1;
    }

    new msg[72];
    format(msg, sizeof(msg), "You used: %s", item);
    SendClientMessage(playerid, 0xFFFFFFFF, msg);
    return 1;
}

public Inv_OnItemDrop(playerid, slot, const item[], model, quantity)
{
    #pragma unused slot, model
    new msg[72];
    format(msg, sizeof(msg), "Dropped %dx %s (bag / right panel).", quantity, item);
    SendClientMessage(playerid, 0xFFFFFFFF, msg);
    return 1;
}

public Inv_OnItemPickup(playerid, pileid, const item[], model, quantity)
{
    #pragma unused pileid, model
    new msg[72];
    format(msg, sizeof(msg), "Picked up %dx %s", quantity, item);
    SendClientMessage(playerid, 0xFFFFFFFF, msg);
    return 1;
}

public OnQueryError(errorid, error[], callback[], query[], connectionHandle)
{
    #pragma unused connectionHandle
    printf("[MySQL] Error %d in %s: %s | Query: %s", errorid, callback, error, query);
    return 1;
}
