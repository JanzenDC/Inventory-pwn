/*
    Inventory Test Gamemode
    Textdraw inventory + right panel for nearest dropped bag

    Credits: Habibi / Janzzzz

    Commands:
      /giveitem   - give Bread + Pistol into inventory
      /dropfront  - spawn a Medkit on the ground in front of you
      /inv        - open inventory (left = bag, right = nearest drops)
*/

#include <open.mp>
#include <inventory>

// One row per item - add new items here only.
enum
{
    ITEM_BREAD,
    ITEM_PISTOL,
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
    { "Pistol", 346   }, // ITEM_PISTOL
    { "Medkit", 11738 }  // ITEM_MEDKIT
};

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
    return 1;
}

public OnGameModeExit()
{
    return 1;
}

public OnPlayerConnect(playerid)
{
    Inventory_OnPlayerConnect(playerid);
    SendClientMessage(playerid, 0xFFFFFFFF, "Welcome! /giveitem, /dropfront, then /inv.");
    SendClientMessage(playerid, 0xAAAAAAFF, "Click an item, then click another slot to move (left <-> right).");
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
        GiveItem(playerid, ITEM_PISTOL, 1);
        SendClientMessage(playerid, 0xFFFFFFFF, "Gave you 3x Bread and 1x Pistol.");
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
        Inventory_Show(playerid);
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

