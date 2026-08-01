/*
    Inventory Test Gamemode
    Textdraw inventory + dynamic right panel (drops / loot / house storage)

    Credits: Habibi / Janzzzz

    Commands:
      /giveitem   - give Bread + Medkit into inventory
      /givegun    - give a Deagle in your hand (for testing /placegun)
      /placegun   - put the gun you are holding into inventory (with ammo)
      /dropfront  - spawn a Medkit on the ground in front of you
      /inv        - open inventory (right = nearby drops)
      /loot       - loot nearest player (or /loot [id]) — right = their inventory
      /houseinv   - open demo house storage on the right panel
*/

#include <open.mp>
#include <textdraw-streamer>
#include <inventory>

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
    { "Bread",  2670  }, // ITEM_BREAD
    { "Medkit", 11738 }  // ITEM_MEDKIT
};

// Demo house storage (id 0) — replace with your house DB later
new bool:g_HouseExists[MAX_INVENTORY];
new g_HouseItem[MAX_INVENTORY][MAX_INVENTORY_ITEM_NAME];
new g_HouseModel[MAX_INVENTORY];
new g_HouseQty[MAX_INVENTORY];

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

stock House_InitDemo()
{
    for (new s = 0; s < MAX_INVENTORY; s++)
    {
        g_HouseExists[s] = false;
        g_HouseItem[s][0] = EOS;
        g_HouseModel[s] = 0;
        g_HouseQty[s] = 0;
    }
    // Seed a few items in the demo house
    g_HouseExists[0] = true;
    format(g_HouseItem[0], MAX_INVENTORY_ITEM_NAME, "Bread");
    g_HouseModel[0] = 2670;
    g_HouseQty[0] = 5;

    g_HouseExists[1] = true;
    format(g_HouseItem[1], MAX_INVENTORY_ITEM_NAME, "Medkit");
    g_HouseModel[1] = 11738;
    g_HouseQty[1] = 2;
}

stock House_LoadToRight(playerid)
{
    Inventory_Right_Clear(playerid);
    for (new s = 0; s < MAX_INVENTORY; s++)
    {
        if (g_HouseExists[s])
            Inventory_Right_SetSlot(playerid, s, g_HouseItem[s], g_HouseModel[s], g_HouseQty[s]);
    }
}

stock House_SaveFromRight(playerid)
{
    for (new s = 0; s < MAX_INVENTORY; s++)
    {
        new item[MAX_INVENTORY_ITEM_NAME], model, qty;
        if (Inventory_Right_GetSlot(playerid, s, item, sizeof(item), model, qty))
        {
            g_HouseExists[s] = true;
            format(g_HouseItem[s], MAX_INVENTORY_ITEM_NAME, "%s", item);
            g_HouseModel[s] = model;
            g_HouseQty[s] = qty;
        }
        else
        {
            g_HouseExists[s] = false;
            g_HouseItem[s][0] = EOS;
            g_HouseModel[s] = 0;
            g_HouseQty[s] = 0;
        }
    }
}

main()
{
    print("----------");
    print("Inventory Test loaded.");
    print("Credits: Habibi / Janzzzz");
    print("----------");
}

public OnGameModeInit()
{
    SetGameModeText("Inventory Test");
    AddPlayerClass(0, 1958.3783, 1343.1572, 15.3746, 269.1425, WEAPON:0, 0, WEAPON:0, 0, WEAPON:0, 0);
    House_InitDemo();
    return 1;
}

public OnGameModeExit()
{
    return 1;
}

public OnPlayerConnect(playerid)
{
    Inventory_OnPlayerConnect(playerid);
    SendClientMessage(playerid, 0xFFFFFFFF, "Welcome! /giveitem /givegun /placegun /dropfront /inv /loot /houseinv");
    SendClientMessage(playerid, 0xAAAAAAFF, "Hold a gun, /placegun to store it. In /inv press Use to equip it again.");
    return 1;
}

public OnPlayerDisconnect(playerid, reason)
{
    Inventory_OnPlayerDisconnect(playerid);
    return 1;
}

public OnPlayerSpawn(playerid)
{
    DropFront(playerid, ITEM_MEDKIT, 1);
    SendClientMessage(playerid, 0x33CCFFFF, "A Medkit was dropped in front of you - open /inv and check the right panel.");
    return 1;
}

public OnPlayerCommandText(playerid, cmdtext[])
{
    if (!strcmp(cmdtext, "/giveitem", true))
    {
        GiveItem(playerid, ITEM_BREAD, 3);
        GiveItem(playerid, ITEM_MEDKIT, 1);
        SendClientMessage(playerid, 0xFFFFFFFF, "Gave you 3x Bread and 1x Medkit.");
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
            SendClientMessage(playerid, 0xFFFF00AA, "Could not create a dropped item (pool full?).");
        else
            SendClientMessage(playerid, 0x33CCFFFF, "Dropped 2x Medkit in front of you. Open /inv to see it on the right.");
        return 1;
    }

    if (!strcmp(cmdtext, "/inv", true))
    {
        Inventory_ClearRightContext(playerid);
        Inventory_Show(playerid);
        return 1;
    }

    if (!strcmp(cmdtext, "/loot", true, 5))
    {
        new targetid = INVALID_PLAYER_ID;
        if (cmdtext[5] == ' ' && cmdtext[6] != EOS)
            targetid = strval(cmdtext[6]);
        else
            targetid = Inventory_FindLootTarget(playerid);

        if (targetid == INVALID_PLAYER_ID || !IsPlayerConnected(targetid))
        {
            SendClientMessage(playerid, 0xFFFF00AA, "Usage: /loot [id] — or stand near another player.");
            return 1;
        }
        if (targetid == playerid)
        {
            SendClientMessage(playerid, 0xFFFF00AA, "You cannot loot yourself.");
            return 1;
        }

        if (!Inventory_OpenLoot(playerid, targetid))
            SendClientMessage(playerid, 0xFFFF00AA, "Could not open loot.");
        else
            SendClientMessage(playerid, 0x33CCFFFF, "Loot open — move items from the right panel into your inventory.");
        return 1;
    }

    if (!strcmp(cmdtext, "/houseinv", true))
    {
        House_LoadToRight(playerid);
        if (!Inventory_OpenStorage(playerid, 0, "House Storage"))
            SendClientMessage(playerid, 0xFFFF00AA, "Could not open house storage.");
        else
            SendClientMessage(playerid, 0x33CCFFFF, "House storage open — deposit/take via click-to-move.");
        return 1;
    }

    return 0;
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
    format(msg, sizeof(msg), "Dropped %dx %s in front of you (right panel).", quantity, item);
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

public Inv_OnRightStorageChanged(playerid, storageid)
{
    #pragma unused storageid
    House_SaveFromRight(playerid);
    SendClientMessage(playerid, 0xAAAAAAFF, "House storage saved.");
    return 1;
}

public Inv_OnLootTransfer(playerid, targetid, bool:taking, const item[], model, quantity)
{
    #pragma unused model
    new tname[MAX_PLAYER_NAME], msg[96];
    GetPlayerName(targetid, tname, sizeof(tname));
    if (taking)
        format(msg, sizeof(msg), "Looted %dx %s from %s.", quantity, item, tname);
    else
        format(msg, sizeof(msg), "Put %dx %s into %s's inventory.", quantity, item, tname);
    SendClientMessage(playerid, 0xFFFFFFFF, msg);
    return 1;
}
