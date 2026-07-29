/*
 * inventory_defs.inc - Early inventory API (defines + forwards)
 *
 * Credits: Habibi / Janzzzz
 *
 * Include this BEFORE other modules that call Inventory_* so they can see
 * the API without pulling the full textdraw implementation:
 *
 *   #include <inventory_defs>
 *
 * Or just #include <inventory> (it pulls this file automatically).
 */

#if defined _INC_INVENTORY_DEFS
	#endinput
#endif
#define _INC_INVENTORY_DEFS

#if !defined MAX_INVENTORY
	#define MAX_INVENTORY           (20)
#endif
#if !defined INV_UI_SLOTS
	#define INV_UI_SLOTS            (MAX_INVENTORY)
#endif
#if !defined MAX_INVENTORY_ITEM_NAME
	#define MAX_INVENTORY_ITEM_NAME (64)
#endif
#if !defined MAX_DROPPED_ITEMS
	#define MAX_DROPPED_ITEMS       (100)
#endif
#if !defined INV_DROP_RANGE
	#define INV_DROP_RANGE          (3.0)
#endif
#if !defined INV_DROP_MERGE_RANGE
	#define INV_DROP_MERGE_RANGE    (2.0)
#endif
#if !defined INV_DROP_FORWARD
	#define INV_DROP_FORWARD        (1.5)
#endif
#if !defined DIALOG_INV_AMOUNT
	#define DIALOG_INV_AMOUNT       (2200)
#endif
#if !defined INV_EMPTY_MODEL
	#define INV_EMPTY_MODEL         (19300)
#endif
#if !defined INV_SELECT_COLOUR
	#define INV_SELECT_COLOUR       (0xFFb05748)
#endif
#if !defined INV_DROP_OBJECT_MODEL
	#define INV_DROP_OBJECT_MODEL   (2919)
#endif

forward Inventory_Add(playerid, const item[], model, quantity = 1);
forward Inventory_Remove(playerid, const item[], quantity = 1);
forward Inventory_RemoveEx(playerid, slot, quantity = 1);
forward Inventory_Count(playerid, const item[]);
forward Inventory_HasItem(playerid, const item[]);
forward Inventory_GetFreeID(playerid);
forward Inventory_GetItemID(playerid, const item[]);
forward Inventory_Items(playerid);
forward Inventory_Clear(playerid);
forward Inventory_OnPlayerConnect(playerid);
forward Inventory_OnPlayerDisconnect(playerid);
forward Inventory_Show(playerid);
forward Inventory_Hide(playerid);
forward Inventory_IsOpen(playerid);
forward Inv_ClickPlayerTD(playerid, PlayerText:playertextid);
forward Inv_HandleEscClose(playerid);
forward Inventory_OnDialogResponse(playerid, dialogid, response, listitem, const inputtext[]);

forward DropItem(const item[], model, quantity, Float:x, Float:y, Float:z, interior = 0, world = 0);
forward DropItemInFront(playerid, const item[], model, quantity = 1);
forward DestroyDropPile(pileid);
forward Item_Nearest(playerid, Float:range = INV_DROP_RANGE);
forward Inventory_PickupDropped(playerid, pileid, slot);

// Gamemode implements these to react to UI actions.
forward Inv_OnItemUse(playerid, slot, const item[], model, quantity);
forward Inv_OnItemDrop(playerid, slot, const item[], model, quantity);
forward Inv_OnItemPickup(playerid, pileid, const item[], model, quantity);
