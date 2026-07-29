/*
 * inventory.inc - Reusable textdraw inventory for open.mp / SA-MP
 *
 * Credits: Habibi / Janzzzz
 *
 * Left panel  = player inventory (5x4 model previews)
 * Right panel = dynamic context:
 *               INV_RIGHT_DROPS    = nearest ground bag
 *               INV_RIGHT_PLAYER   = another player's inventory (/loot)
 *               INV_RIGHT_STORAGE  = external buffer (house, vehicle, etc.)
 * Actions     = Use / Drop / Close
 * Move items  = click source slot, then click target slot (left <-> right or rearrange)
 *
 *   #include <open.mp>
 *   #include <inventory>
 */

#if defined _INC_INVENTORY
	#endinput
#endif
#define _INC_INVENTORY

#include <inventory_defs>

#if !defined TEXT_DRAW_FONT_1
	#define TEXT_DRAW_FONT_1 (0)
#endif
#if !defined TEXT_DRAW_FONT_MODEL_PREVIEW
	#define TEXT_DRAW_FONT_MODEL_PREVIEW (5)
#endif
#if !defined TEXT_DRAW_ALIGN_LEFT
	#define TEXT_DRAW_ALIGN_LEFT (1)
#endif
#if !defined TEXT_DRAW_ALIGN_CENTRE
	#define TEXT_DRAW_ALIGN_CENTRE (2)
#endif

// ---------------------------------------------------------------------------
// Player inventory data
// ---------------------------------------------------------------------------

enum inventoryData
{
	bool:invExists,
	invItem[MAX_INVENTORY_ITEM_NAME],
	invModel,
	invQuantity
};

static InventoryData[MAX_PLAYERS][MAX_INVENTORY][inventoryData];

// Weapon id -> object model (for inventory preview / placegun)
new const Inv_WeaponModels[] =
{
	0, 331, 333, 334, 335, 336, 337, 338, 339, 341, 321, 322, 323, 324,
	325, 326, 342, 343, 344, 0, 0, 0, 346, 347, 348, 349, 350, 351, 352,
	353, 355, 356, 372, 357, 358, 359, 360, 361, 362, 363, 364, 365, 366,
	367, 368, 368, 371
};

#define INV_GUN_PREFIX "Gun: "

// ---------------------------------------------------------------------------
// World drop piles (one bag object can hold many item slots)
// ---------------------------------------------------------------------------

enum E_DROP_PILE
{
	bool:dropExists,
	Float:dropX,
	Float:dropY,
	Float:dropZ,
	dropInterior,
	dropWorld,
	dropObject,
	Text3D:dropLabel
};

static DropPiles[MAX_DROPPED_ITEMS][E_DROP_PILE];
static DropPileSlots[MAX_DROPPED_ITEMS][MAX_INVENTORY][inventoryData];

// Active nearest pile for each player (-1 = none) — used in DROPS mode
static g_InvNearPile[MAX_PLAYERS];

// Right panel context
static g_InvRightMode[MAX_PLAYERS];
static g_InvRightId[MAX_PLAYERS];
static g_InvRightTitle[MAX_PLAYERS][MAX_INV_RIGHT_TITLE];
static g_InvRightSlots[MAX_PLAYERS][MAX_INVENTORY][inventoryData];

// ---------------------------------------------------------------------------
// Textdraws
// ---------------------------------------------------------------------------

static PlayerText:g_InvBg[MAX_PLAYERS][2];
static PlayerText:g_InvTitle[MAX_PLAYERS];
static PlayerText:g_InvWeight[MAX_PLAYERS];
static PlayerText:g_InvPreview[MAX_PLAYERS][INV_UI_SLOTS];
static PlayerText:g_InvAccent[MAX_PLAYERS][INV_UI_SLOTS];
static PlayerText:g_InvName[MAX_PLAYERS][INV_UI_SLOTS];
static PlayerText:g_InvQty[MAX_PLAYERS][INV_UI_SLOTS];
static PlayerText:g_InvBtn[MAX_PLAYERS][3];

static PlayerText:g_Inv2Bg[MAX_PLAYERS];
static PlayerText:g_Inv2Title[MAX_PLAYERS];
static PlayerText:g_Inv2Preview[MAX_PLAYERS][INV_UI_SLOTS];
static PlayerText:g_Inv2Accent[MAX_PLAYERS][INV_UI_SLOTS];
static PlayerText:g_Inv2Name[MAX_PLAYERS][INV_UI_SLOTS];
static PlayerText:g_Inv2Qty[MAX_PLAYERS][INV_UI_SLOTS];

static bool:g_InvOpen[MAX_PLAYERS];
static g_InvSelected[MAX_PLAYERS];
static g_InvSelected2[MAX_PLAYERS];
static g_InvDragPanel[MAX_PLAYERS]; // 0 none, 1 left, 2 right
static g_InvDragSlot[MAX_PLAYERS];
static bool:g_InvBuilt[MAX_PLAYERS];
static bool:g_InvIgnoreEsc[MAX_PLAYERS];

// Item add/remove toast queue (up to INV_NOTIFY_MAX side by side)
static PlayerText:g_InvNotifPreview[MAX_PLAYERS][INV_NOTIFY_MAX];
static PlayerText:g_InvNotifTitle[MAX_PLAYERS][INV_NOTIFY_MAX];
static PlayerText:g_InvNotifName[MAX_PLAYERS][INV_NOTIFY_MAX];
static g_InvNotifExpire[MAX_PLAYERS][INV_NOTIFY_MAX];
static g_InvNotifModel[MAX_PLAYERS][INV_NOTIFY_MAX];
static g_InvNotifTitleStr[MAX_PLAYERS][INV_NOTIFY_MAX][16];
static g_InvNotifLabel[MAX_PLAYERS][INV_NOTIFY_MAX][MAX_INVENTORY_ITEM_NAME + 16];
static g_InvNotifCount[MAX_PLAYERS];
static g_InvNotifTimer[MAX_PLAYERS];

#define INV_PANEL_NONE  (0)
#define INV_PANEL_LEFT  (1)
#define INV_PANEL_RIGHT (2)

// ---------------------------------------------------------------------------
// Item notifications (Received / Removed) — queue shifts left as oldest expires
// ---------------------------------------------------------------------------

stock Inv_NotifDestroyBox(playerid, slot)
{
	if (slot < 0 || slot >= INV_NOTIFY_MAX)
		return 0;
	if (g_InvNotifPreview[playerid][slot] != PlayerText:0)
	{
		PlayerTextDrawDestroy(playerid, g_InvNotifPreview[playerid][slot]);
		g_InvNotifPreview[playerid][slot] = PlayerText:0;
	}
	if (g_InvNotifTitle[playerid][slot] != PlayerText:0)
	{
		PlayerTextDrawDestroy(playerid, g_InvNotifTitle[playerid][slot]);
		g_InvNotifTitle[playerid][slot] = PlayerText:0;
	}
	if (g_InvNotifName[playerid][slot] != PlayerText:0)
	{
		PlayerTextDrawDestroy(playerid, g_InvNotifName[playerid][slot]);
		g_InvNotifName[playerid][slot] = PlayerText:0;
	}
	return 1;
}

stock Inv_NotifCreateBox(playerid, slot, Float:baseX, const title[], const label[], model)
{
	new Float:y = INV_NOTIFY_Y;

	g_InvNotifPreview[playerid][slot] = CreatePlayerTextDraw(playerid, baseX, y, "Preview_Model");
	PlayerTextDrawFont(playerid, g_InvNotifPreview[playerid][slot], TEXT_DRAW_FONT_MODEL_PREVIEW);
	PlayerTextDrawLetterSize(playerid, g_InvNotifPreview[playerid][slot], 0.600000, 2.000000);
	PlayerTextDrawTextSize(playerid, g_InvNotifPreview[playerid][slot], 32.000000, 35.000000);
	PlayerTextDrawSetOutline(playerid, g_InvNotifPreview[playerid][slot], 0);
	PlayerTextDrawSetShadow(playerid, g_InvNotifPreview[playerid][slot], 0);
	PlayerTextDrawAlignment(playerid, g_InvNotifPreview[playerid][slot], TEXT_DRAW_ALIGN_LEFT);
	PlayerTextDrawColour(playerid, g_InvNotifPreview[playerid][slot], -1);
	PlayerTextDrawBackgroundColour(playerid, g_InvNotifPreview[playerid][slot], 54);
	PlayerTextDrawBoxColour(playerid, g_InvNotifPreview[playerid][slot], 255);
	PlayerTextDrawUseBox(playerid, g_InvNotifPreview[playerid][slot], false);
	PlayerTextDrawSetProportional(playerid, g_InvNotifPreview[playerid][slot], true);
	PlayerTextDrawSetSelectable(playerid, g_InvNotifPreview[playerid][slot], false);
	PlayerTextDrawSetPreviewModel(playerid, g_InvNotifPreview[playerid][slot], model);
	PlayerTextDrawSetPreviewRot(playerid, g_InvNotifPreview[playerid][slot], -10.000000, 0.000000, -20.000000, 1.000000);
	PlayerTextDrawSetPreviewVehicleColours(playerid, g_InvNotifPreview[playerid][slot], 1, 1);

	g_InvNotifTitle[playerid][slot] = CreatePlayerTextDraw(playerid, baseX + 1.0, y + 1.0, title);
	PlayerTextDrawFont(playerid, g_InvNotifTitle[playerid][slot], TEXT_DRAW_FONT_1);
	PlayerTextDrawLetterSize(playerid, g_InvNotifTitle[playerid][slot], 0.125000, 0.650000);
	PlayerTextDrawTextSize(playerid, g_InvNotifTitle[playerid][slot], baseX + 30.0, 17.000000);
	PlayerTextDrawSetOutline(playerid, g_InvNotifTitle[playerid][slot], 0);
	PlayerTextDrawSetShadow(playerid, g_InvNotifTitle[playerid][slot], 0);
	PlayerTextDrawAlignment(playerid, g_InvNotifTitle[playerid][slot], TEXT_DRAW_ALIGN_LEFT);
	PlayerTextDrawColour(playerid, g_InvNotifTitle[playerid][slot], -1);
	PlayerTextDrawBackgroundColour(playerid, g_InvNotifTitle[playerid][slot], 255);
	PlayerTextDrawBoxColour(playerid, g_InvNotifTitle[playerid][slot], 50);
	PlayerTextDrawUseBox(playerid, g_InvNotifTitle[playerid][slot], false);
	PlayerTextDrawSetProportional(playerid, g_InvNotifTitle[playerid][slot], true);
	PlayerTextDrawSetSelectable(playerid, g_InvNotifTitle[playerid][slot], false);

	g_InvNotifName[playerid][slot] = CreatePlayerTextDraw(playerid, baseX + 16.0, y + 27.0, label);
	PlayerTextDrawFont(playerid, g_InvNotifName[playerid][slot], TEXT_DRAW_FONT_1);
	PlayerTextDrawLetterSize(playerid, g_InvNotifName[playerid][slot], 0.125000, 0.650000);
	PlayerTextDrawTextSize(playerid, g_InvNotifName[playerid][slot], baseX + 31.5, 29.500000);
	PlayerTextDrawSetOutline(playerid, g_InvNotifName[playerid][slot], 0);
	PlayerTextDrawSetShadow(playerid, g_InvNotifName[playerid][slot], 0);
	PlayerTextDrawAlignment(playerid, g_InvNotifName[playerid][slot], TEXT_DRAW_ALIGN_CENTRE);
	PlayerTextDrawColour(playerid, g_InvNotifName[playerid][slot], -1);
	PlayerTextDrawBackgroundColour(playerid, g_InvNotifName[playerid][slot], 255);
	PlayerTextDrawBoxColour(playerid, g_InvNotifName[playerid][slot], 50);
	PlayerTextDrawUseBox(playerid, g_InvNotifName[playerid][slot], true);
	PlayerTextDrawSetProportional(playerid, g_InvNotifName[playerid][slot], true);
	PlayerTextDrawSetSelectable(playerid, g_InvNotifName[playerid][slot], false);

	PlayerTextDrawShow(playerid, g_InvNotifPreview[playerid][slot]);
	PlayerTextDrawShow(playerid, g_InvNotifTitle[playerid][slot]);
	PlayerTextDrawShow(playerid, g_InvNotifName[playerid][slot]);
	return 1;
}

stock Inv_NotifRedraw(playerid)
{
	new i, count = g_InvNotifCount[playerid];
	new Float:startX, Float:baseX;

	for (i = 0; i < INV_NOTIFY_MAX; i++)
		Inv_NotifDestroyBox(playerid, i);

	if (count < 1)
		return 0;

	startX = INV_NOTIFY_CENTER_X - (float(count) * INV_NOTIFY_SPACING) / 2.0;
	for (i = 0; i < count; i++)
	{
		baseX = startX + float(i) * INV_NOTIFY_SPACING;
		Inv_NotifCreateBox(playerid, i, baseX,
			g_InvNotifTitleStr[playerid][i],
			g_InvNotifLabel[playerid][i],
			g_InvNotifModel[playerid][i]);
	}
	return 1;
}

stock Inv_NotifSchedule(playerid)
{
	if (g_InvNotifTimer[playerid])
	{
		KillTimer(g_InvNotifTimer[playerid]);
		g_InvNotifTimer[playerid] = 0;
	}
	if (g_InvNotifCount[playerid] < 1)
		return 0;

	new wait = g_InvNotifExpire[playerid][0] - GetTickCount();
	if (wait < 1)
		wait = 1;
	g_InvNotifTimer[playerid] = SetTimerEx("Inv_NotifOnExpire", wait, false, "i", playerid);
	return 1;
}

stock Inv_NotifPopFront(playerid)
{
	new i, count = g_InvNotifCount[playerid];
	if (count < 1)
		return 0;

	for (i = 0; i < count - 1; i++)
	{
		g_InvNotifExpire[playerid][i] = g_InvNotifExpire[playerid][i + 1];
		g_InvNotifModel[playerid][i] = g_InvNotifModel[playerid][i + 1];
		format(g_InvNotifTitleStr[playerid][i], 16, "%s", g_InvNotifTitleStr[playerid][i + 1]);
		format(g_InvNotifLabel[playerid][i], MAX_INVENTORY_ITEM_NAME + 16, "%s", g_InvNotifLabel[playerid][i + 1]);
	}
	g_InvNotifCount[playerid]--;
	Inv_NotifRedraw(playerid);
	Inv_NotifSchedule(playerid);
	return 1;
}

public Inv_NotifOnExpire(playerid)
{
	g_InvNotifTimer[playerid] = 0;
	if (!IsPlayerConnected(playerid))
		return 0;
	while (g_InvNotifCount[playerid] > 0 && g_InvNotifExpire[playerid][0] <= GetTickCount())
		Inv_NotifPopFront(playerid);
	return 1;
}

stock Inv_HideItemNotification(playerid)
{
	if (g_InvNotifTimer[playerid])
	{
		KillTimer(g_InvNotifTimer[playerid]);
		g_InvNotifTimer[playerid] = 0;
	}
	for (new i = 0; i < INV_NOTIFY_MAX; i++)
		Inv_NotifDestroyBox(playerid, i);
	g_InvNotifCount[playerid] = 0;
	return 1;
}

stock Inventory_ShowNotification(playerid, const title[], const item[], model, quantity = 1)
{
#if !INV_NOTIFY_ENABLE
	#pragma unused playerid, title, item, model, quantity
	return 0;
#else
	if (!IsPlayerConnected(playerid) || !item[0] || model <= 0)
		return 0;

	new label[MAX_INVENTORY_ITEM_NAME + 16];
	if (quantity > 1)
		format(label, sizeof(label), "%s x%d", item, quantity);
	else
		format(label, sizeof(label), "%s", item);

	// Queue full → drop oldest so the row shifts left
	if (g_InvNotifCount[playerid] >= INV_NOTIFY_MAX)
		Inv_NotifPopFront(playerid);

	new slot = g_InvNotifCount[playerid];
	g_InvNotifExpire[playerid][slot] = GetTickCount() + INV_NOTIFY_DURATION;
	g_InvNotifModel[playerid][slot] = model;
	format(g_InvNotifTitleStr[playerid][slot], 16, "%s", title);
	format(g_InvNotifLabel[playerid][slot], MAX_INVENTORY_ITEM_NAME + 16, "%s", label);
	g_InvNotifCount[playerid]++;

	Inv_NotifRedraw(playerid);
	Inv_NotifSchedule(playerid);
	return 1;
#endif
}

stock Inventory_NotifyReceived(playerid, const item[], model, quantity = 1)
{
	return Inventory_ShowNotification(playerid, "Received", item, model, quantity);
}

stock Inventory_NotifyRemoved(playerid, const item[], model, quantity = 1)
{
	return Inventory_ShowNotification(playerid, "Removed", item, model, quantity);
}

// ---------------------------------------------------------------------------
// Core inventory ops
// ---------------------------------------------------------------------------

stock Inventory_Clear(playerid)
{
	for (new slot = 0; slot < MAX_INVENTORY; slot++)
	{
		InventoryData[playerid][slot][invExists] = false;
		InventoryData[playerid][slot][invItem][0] = EOS;
		InventoryData[playerid][slot][invModel] = 0;
		InventoryData[playerid][slot][invQuantity] = 0;
	}
	g_InvSelected[playerid] = -1;
	g_InvSelected2[playerid] = -1;
	g_InvDragPanel[playerid] = INV_PANEL_NONE;
	g_InvDragSlot[playerid] = -1;
	return 1;
}

stock Inventory_OnPlayerConnect(playerid)
{
	Inventory_Clear(playerid);
	g_InvOpen[playerid] = false;
	g_InvBuilt[playerid] = false;
	g_InvIgnoreEsc[playerid] = false;
	g_InvNearPile[playerid] = -1;
	g_InvDragPanel[playerid] = INV_PANEL_NONE;
	g_InvDragSlot[playerid] = -1;
	g_InvRightMode[playerid] = INV_RIGHT_DROPS;
	g_InvRightId[playerid] = -1;
	g_InvRightTitle[playerid][0] = EOS;
	Inventory_Right_Clear(playerid);
	g_InvNotifCount[playerid] = 0;
	g_InvNotifTimer[playerid] = 0;
	for (new i = 0; i < INV_NOTIFY_MAX; i++)
	{
		g_InvNotifPreview[playerid][i] = PlayerText:0;
		g_InvNotifTitle[playerid][i] = PlayerText:0;
		g_InvNotifName[playerid][i] = PlayerText:0;
		g_InvNotifExpire[playerid][i] = 0;
		g_InvNotifModel[playerid][i] = 0;
		g_InvNotifTitleStr[playerid][i][0] = EOS;
		g_InvNotifLabel[playerid][i][0] = EOS;
	}
	return 1;
}

stock Inventory_OnPlayerDisconnect(playerid)
{
	Inv_HideItemNotification(playerid);
	if (g_InvOpen[playerid])
		Inventory_Hide(playerid);

	// Close anyone who was looting this player
	for (new i = 0; i < MAX_PLAYERS; i++)
	{
		if (i == playerid || !IsPlayerConnected(i))
			continue;
		if (g_InvRightMode[i] == INV_RIGHT_PLAYER && g_InvRightId[i] == playerid)
		{
			if (g_InvOpen[i])
				Inventory_Hide(i);
			Inventory_ClearRightContext(i);
			SendClientMessage(i, 0xFFFF00AA, "Loot target disconnected.");
		}
	}

	Inventory_Clear(playerid);
	Inventory_ClearRightContext(playerid);
	return 1;
}

stock Inventory_GetFreeID(playerid)
{
	for (new slot = 0; slot < MAX_INVENTORY; slot++)
		if (!InventoryData[playerid][slot][invExists])
			return slot;
	return -1;
}

stock Inventory_GetItemID(playerid, const item[])
{
	for (new slot = 0; slot < MAX_INVENTORY; slot++)
	{
		if (InventoryData[playerid][slot][invExists]
			&& !strcmp(InventoryData[playerid][slot][invItem], item, true))
			return slot;
	}
	return -1;
}

stock Inventory_Items(playerid)
{
	new count = 0;
	for (new slot = 0; slot < MAX_INVENTORY; slot++)
		if (InventoryData[playerid][slot][invExists] && InventoryData[playerid][slot][invQuantity] > 0)
			count++;
	return count;
}

stock Inventory_Count(playerid, const item[])
{
	new slot = Inventory_GetItemID(playerid, item);
	if (slot == -1)
		return 0;
	return InventoryData[playerid][slot][invQuantity];
}

stock Inventory_HasItem(playerid, const item[])
{
	return Inventory_Count(playerid, item) > 0;
}

stock Inventory_IsOpen(playerid)
{
	return _:g_InvOpen[playerid];
}

stock Inventory_GetSlot(playerid, slot, item[], itemLen, &model, &quantity)
{
	if (slot < 0 || slot >= MAX_INVENTORY || !InventoryData[playerid][slot][invExists])
	{
		item[0] = EOS;
		model = 0;
		quantity = 0;
		return 0;
	}
	format(item, itemLen, "%s", InventoryData[playerid][slot][invItem]);
	model = InventoryData[playerid][slot][invModel];
	quantity = InventoryData[playerid][slot][invQuantity];
	return 1;
}

// Direct slot write (used by DB load). quantity <= 0 clears the slot.
stock Inventory_SetSlot(playerid, slot, const item[], model, quantity)
{
	if (slot < 0 || slot >= MAX_INVENTORY)
		return 0;

	if (quantity <= 0 || !item[0])
	{
		InventoryData[playerid][slot][invExists] = false;
		InventoryData[playerid][slot][invItem][0] = EOS;
		InventoryData[playerid][slot][invModel] = 0;
		InventoryData[playerid][slot][invQuantity] = 0;
	}
	else
	{
		InventoryData[playerid][slot][invExists] = true;
		InventoryData[playerid][slot][invModel] = model;
		InventoryData[playerid][slot][invQuantity] = quantity;
		format(InventoryData[playerid][slot][invItem], MAX_INVENTORY_ITEM_NAME, "%s", item);
	}

	if (g_InvOpen[playerid])
		Inv_RefreshSlot(playerid, slot);
	return 1;
}

stock Inv_NotifyChanged(playerid)
{
	CallLocalFunction("Inv_OnInventoryChanged", "d", playerid);
	return 1;
}

// ---------------------------------------------------------------------------
// Guns (place from hand / use to equip)
// ---------------------------------------------------------------------------

stock Inv_ModelForWeaponId(weaponid)
{
	if (weaponid < 0 || weaponid >= sizeof(Inv_WeaponModels))
		return 0;
	return Inv_WeaponModels[weaponid];
}

stock Inv_WeaponIdFromModel(model)
{
	if (!model)
		return 0;
	for (new w = 1; w < sizeof(Inv_WeaponModels); w++)
		if (Inv_WeaponModels[w] == model)
			return w;
	return 0;
}

stock Inv_IsGunItem(const item[])
{
	return bool:(strfind(item, INV_GUN_PREFIX, true) == 0);
}

stock Inv_IsPlaceableWeapon(weaponid)
{
	// Must be a real held weapon with a preview model (not fist / invalid)
	return (weaponid >= 1 && weaponid <= 46 && Inv_ModelForWeaponId(weaponid) != 0);
}

stock Inv_FormatGunItemName(weaponid, dest[], destLen = sizeof(dest))
{
	new wname[32];
	GetWeaponName(WEAPON:weaponid, wname, sizeof(wname));
	if (!wname[0])
		format(wname, sizeof(wname), "Weapon %d", weaponid);
	format(dest, destLen, "%s%s", INV_GUN_PREFIX, wname);
	return 1;
}

// Put the currently held weapon + ammo into inventory. Fails if not holding a gun.
stock Inventory_PlaceGun(playerid)
{
	if (!IsPlayerConnected(playerid))
		return 0;

	new weaponid = _:GetPlayerWeapon(playerid);
	if (!Inv_IsPlaceableWeapon(weaponid))
	{
		SendClientMessage(playerid, 0xFFFF00AA, "You must be holding a gun to place it.");
		return 0;
	}

	new ammo = GetPlayerAmmo(playerid);
	if (ammo < 1)
		ammo = 1;

	new model = Inv_ModelForWeaponId(weaponid);
	new itemName[MAX_INVENTORY_ITEM_NAME];
	Inv_FormatGunItemName(weaponid, itemName, sizeof(itemName));

	if (!Inventory_Add(playerid, itemName, model, ammo))
		return 0;

	RemovePlayerWeapon(playerid, WEAPON:weaponid);
	SendClientMessage(playerid, 0x33CCFFFF, "Gun placed in your inventory. Open /inv and press Use to equip it.");
	return 1;
}

// Give the gun from an inventory slot into the player's hands, then remove the slot.
stock Inventory_EquipGun(playerid, slot)
{
	if (!IsPlayerConnected(playerid) || slot < 0 || slot >= MAX_INVENTORY)
		return 0;
	if (!InventoryData[playerid][slot][invExists])
		return 0;
	if (!Inv_IsGunItem(InventoryData[playerid][slot][invItem]))
		return 0;

	new weaponid = Inv_WeaponIdFromModel(InventoryData[playerid][slot][invModel]);
	if (!Inv_IsPlaceableWeapon(weaponid))
		return 0;

	new ammo = InventoryData[playerid][slot][invQuantity];
	if (ammo < 1)
		ammo = 1;

	new itemName[MAX_INVENTORY_ITEM_NAME];
	new model = InventoryData[playerid][slot][invModel];
	format(itemName, sizeof(itemName), "%s", InventoryData[playerid][slot][invItem]);

	GivePlayerWeapon(playerid, WEAPON:weaponid, ammo);
	SetPlayerArmedWeapon(playerid, WEAPON:weaponid);

	// Clear whole gun slot (quantity is ammo, not stack count of items)
	InventoryData[playerid][slot][invExists] = false;
	InventoryData[playerid][slot][invItem][0] = EOS;
	InventoryData[playerid][slot][invModel] = 0;
	InventoryData[playerid][slot][invQuantity] = 0;
	if (g_InvSelected[playerid] == slot)
		g_InvSelected[playerid] = -1;
	if (g_InvOpen[playerid])
		Inv_RefreshSlot(playerid, slot);
	Inv_NotifyChanged(playerid);
	Inventory_NotifyRemoved(playerid, itemName, model, 1);

	SendClientMessage(playerid, 0x33CCFFFF, "Gun equipped.");
	return 1;
}

stock Inventory_Add(playerid, const item[], model, quantity = 1, bool:notify = true)
{
	if (!IsPlayerConnected(playerid) || quantity < 1 || !item[0])
		return 0;

	new slot = Inventory_GetItemID(playerid, item);
	if (slot != -1)
	{
		InventoryData[playerid][slot][invQuantity] += quantity;
		if (g_InvOpen[playerid])
			Inv_RefreshSlot(playerid, slot);
		Inv_NotifyChanged(playerid);
		if (notify)
			Inventory_NotifyReceived(playerid, item, model, quantity);
		return 1;
	}

	slot = Inventory_GetFreeID(playerid);
	if (slot == -1)
	{
		SendClientMessage(playerid, 0xFFFF00AA, "Your inventory is full.");
		return 0;
	}

	InventoryData[playerid][slot][invExists] = true;
	InventoryData[playerid][slot][invModel] = model;
	InventoryData[playerid][slot][invQuantity] = quantity;
	format(InventoryData[playerid][slot][invItem], MAX_INVENTORY_ITEM_NAME, "%s", item);

	if (g_InvOpen[playerid])
		Inv_RefreshSlot(playerid, slot);
	Inv_NotifyChanged(playerid);
	if (notify)
		Inventory_NotifyReceived(playerid, item, model, quantity);
	return 1;
}

stock Inventory_RemoveEx(playerid, slot, quantity = 1, bool:notify = true)
{
	if (!IsPlayerConnected(playerid) || slot < 0 || slot >= MAX_INVENTORY)
		return 0;
	if (!InventoryData[playerid][slot][invExists] || quantity < 1)
		return 0;
	if (InventoryData[playerid][slot][invQuantity] < quantity)
		return 0;

	new itemName[MAX_INVENTORY_ITEM_NAME];
	new model = InventoryData[playerid][slot][invModel];
	format(itemName, sizeof(itemName), "%s", InventoryData[playerid][slot][invItem]);

	InventoryData[playerid][slot][invQuantity] -= quantity;
	if (InventoryData[playerid][slot][invQuantity] <= 0)
	{
		InventoryData[playerid][slot][invExists] = false;
		InventoryData[playerid][slot][invItem][0] = EOS;
		InventoryData[playerid][slot][invModel] = 0;
		InventoryData[playerid][slot][invQuantity] = 0;
		if (g_InvSelected[playerid] == slot)
			g_InvSelected[playerid] = -1;
	}

	if (g_InvOpen[playerid])
		Inv_RefreshSlot(playerid, slot);
	Inv_NotifyChanged(playerid);
	if (notify)
		Inventory_NotifyRemoved(playerid, itemName, model, quantity);
	return 1;
}

stock Inventory_Remove(playerid, const item[], quantity = 1, bool:notify = true)
{
	new slot = Inventory_GetItemID(playerid, item);
	if (slot == -1)
		return 0;
	return Inventory_RemoveEx(playerid, slot, quantity, notify);
}

// ---------------------------------------------------------------------------
// Drop piles (one ground object = many items)
// ---------------------------------------------------------------------------

stock Float:Inv_Dist3D(Float:x1, Float:y1, Float:z1, Float:x2, Float:y2, Float:z2)
{
	return floatsqroot((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2) + (z1 - z2) * (z1 - z2));
}

stock DropPile_CountItems(pileid)
{
	if (pileid < 0 || pileid >= MAX_DROPPED_ITEMS || !DropPiles[pileid][dropExists])
		return 0;

	new count = 0;
	for (new s = 0; s < MAX_INVENTORY; s++)
		if (DropPileSlots[pileid][s][invExists] && DropPileSlots[pileid][s][invQuantity] > 0)
			count++;
	return count;
}

stock DropPile_FindFreeSlot(pileid)
{
	for (new s = 0; s < MAX_INVENTORY; s++)
		if (!DropPileSlots[pileid][s][invExists])
			return s;
	return -1;
}

stock DropPile_FindItemSlot(pileid, const item[])
{
	for (new s = 0; s < MAX_INVENTORY; s++)
	{
		if (DropPileSlots[pileid][s][invExists]
			&& !strcmp(DropPileSlots[pileid][s][invItem], item, true))
			return s;
	}
	return -1;
}

stock DropPile_UpdateLabel(pileid)
{
	if (pileid < 0 || pileid >= MAX_DROPPED_ITEMS || !DropPiles[pileid][dropExists])
		return 0;

	new count = DropPile_CountItems(pileid);
	new label[96];
	if (count <= 0)
		format(label, sizeof(label), "{AAAAAA}Empty bag");
	else if (count == 1)
	{
		for (new s = 0; s < MAX_INVENTORY; s++)
		{
			if (!DropPileSlots[pileid][s][invExists])
				continue;
			format(label, sizeof(label), "{33CCFF}%s\n{FFFFFF}x%d",
				DropPileSlots[pileid][s][invItem], DropPileSlots[pileid][s][invQuantity]);
			break;
		}
	}
	else
		format(label, sizeof(label), "{33CCFF}Dropped bag\n{FFFFFF}%d items", count);

	Delete3DTextLabel(DropPiles[pileid][dropLabel]);
	DropPiles[pileid][dropLabel] = Create3DTextLabel(label, 0xFFFFFFFF,
		DropPiles[pileid][dropX], DropPiles[pileid][dropY], DropPiles[pileid][dropZ] - 0.35,
		INV_DROP_RANGE + 2.0, DropPiles[pileid][dropWorld], false);
	return 1;
}

stock DropPile_ClearSlots(pileid)
{
	for (new s = 0; s < MAX_INVENTORY; s++)
	{
		DropPileSlots[pileid][s][invExists] = false;
		DropPileSlots[pileid][s][invItem][0] = EOS;
		DropPileSlots[pileid][s][invModel] = 0;
		DropPileSlots[pileid][s][invQuantity] = 0;
	}
	return 1;
}

stock DropPile_AddItem(pileid, const item[], model, quantity)
{
	if (pileid < 0 || pileid >= MAX_DROPPED_ITEMS || !DropPiles[pileid][dropExists] || quantity < 1)
		return 0;

	new slot = DropPile_FindItemSlot(pileid, item);
	if (slot != -1)
	{
		DropPileSlots[pileid][slot][invQuantity] += quantity;
		DropPile_UpdateLabel(pileid);
		return 1;
	}

	slot = DropPile_FindFreeSlot(pileid);
	if (slot == -1)
		return 0;

	DropPileSlots[pileid][slot][invExists] = true;
	DropPileSlots[pileid][slot][invModel] = model;
	DropPileSlots[pileid][slot][invQuantity] = quantity;
	format(DropPileSlots[pileid][slot][invItem], MAX_INVENTORY_ITEM_NAME, "%s", item);
	DropPile_UpdateLabel(pileid);
	return 1;
}

stock DropPile_FindAt(Float:x, Float:y, Float:z, interior, world, Float:range = INV_DROP_MERGE_RANGE)
{
	new nearest = -1;
	new Float:best = range + 1.0, Float:dist;

	for (new i = 0; i < MAX_DROPPED_ITEMS; i++)
	{
		if (!DropPiles[i][dropExists])
			continue;
		if (DropPiles[i][dropInterior] != interior || DropPiles[i][dropWorld] != world)
			continue;

		dist = Inv_Dist3D(x, y, z, DropPiles[i][dropX], DropPiles[i][dropY], DropPiles[i][dropZ]);
		if (dist <= range && dist < best)
		{
			best = dist;
			nearest = i;
		}
	}
	return nearest;
}

stock DropPile_Create(Float:x, Float:y, Float:z, interior, world)
{
	new id = -1;
	for (new i = 0; i < MAX_DROPPED_ITEMS; i++)
	{
		if (!DropPiles[i][dropExists])
		{
			id = i;
			break;
		}
	}
	if (id == -1)
		return -1;

	DropPile_ClearSlots(id);
	DropPiles[id][dropExists] = true;
	DropPiles[id][dropX] = x;
	DropPiles[id][dropY] = y;
	DropPiles[id][dropZ] = z;
	DropPiles[id][dropInterior] = interior;
	DropPiles[id][dropWorld] = world;
	DropPiles[id][dropObject] = CreateObject(INV_DROP_OBJECT_MODEL, x, y, z - 0.95, 0.0, 0.0, 0.0, 80.0);
	DropPiles[id][dropLabel] = Create3DTextLabel(" ", 0xFFFFFFFF, x, y, z - 0.35, INV_DROP_RANGE + 2.0, world, false);
	#pragma unused interior
	return id;
}

stock DestroyDropPile(pileid)
{
	if (pileid < 0 || pileid >= MAX_DROPPED_ITEMS || !DropPiles[pileid][dropExists])
		return 0;

	if (IsValidObject(DropPiles[pileid][dropObject]))
		DestroyObject(DropPiles[pileid][dropObject]);
	Delete3DTextLabel(DropPiles[pileid][dropLabel]);

	DropPile_ClearSlots(pileid);
	DropPiles[pileid][dropExists] = false;
	DropPiles[pileid][dropObject] = INVALID_OBJECT_ID;
	return 1;
}

// Adds into nearest bag in merge range, or creates one bag object.
stock DropItem(const item[], model, quantity, Float:x, Float:y, Float:z, interior = 0, world = 0)
{
	if (!item[0] || quantity < 1)
		return -1;

	new pileid = DropPile_FindAt(x, y, z, interior, world, INV_DROP_MERGE_RANGE);
	if (pileid == -1)
	{
		pileid = DropPile_Create(x, y, z, interior, world);
		if (pileid == -1)
			return -1;
	}

	if (!DropPile_AddItem(pileid, item, model, quantity))
	{
		// Bag full - only destroy if we just created an empty pile
		if (DropPile_CountItems(pileid) <= 0)
			DestroyDropPile(pileid);
		return -1;
	}
	return pileid;
}

stock DropItemInFront(playerid, const item[], model, quantity = 1)
{
	if (!IsPlayerConnected(playerid) || quantity < 1)
		return -1;

	new Float:x, Float:y, Float:z, Float:a;
	GetPlayerPos(playerid, x, y, z);
	GetPlayerFacingAngle(playerid, a);

	x += INV_DROP_FORWARD * floatsin(-a, degrees);
	y += INV_DROP_FORWARD * floatcos(-a, degrees);

	return DropItem(item, model, quantity, x, y, z, GetPlayerInterior(playerid), GetPlayerVirtualWorld(playerid));
}

stock Item_Nearest(playerid, Float:range = INV_DROP_RANGE)
{
	new Float:px, Float:py, Float:pz;
	GetPlayerPos(playerid, px, py, pz);
	return DropPile_FindAt(px, py, pz, GetPlayerInterior(playerid), GetPlayerVirtualWorld(playerid), range);
}

stock Inventory_PickupDropped(playerid, pileid, slot)
{
	if (!IsPlayerConnected(playerid) || pileid < 0 || pileid >= MAX_DROPPED_ITEMS)
		return 0;
	if (!DropPiles[pileid][dropExists])
		return 0;
	if (slot < 0 || slot >= MAX_INVENTORY || !DropPileSlots[pileid][slot][invExists])
		return 0;

	new itemName[MAX_INVENTORY_ITEM_NAME];
	format(itemName, sizeof(itemName), "%s", DropPileSlots[pileid][slot][invItem]);
	new model = DropPileSlots[pileid][slot][invModel];
	new qty = DropPileSlots[pileid][slot][invQuantity];

	if (!Inventory_Add(playerid, itemName, model, qty))
		return 0;

	CallLocalFunction("Inv_OnItemPickup", "ddsii", playerid, pileid, itemName, model, qty);

	DropPileSlots[pileid][slot][invExists] = false;
	DropPileSlots[pileid][slot][invItem][0] = EOS;
	DropPileSlots[pileid][slot][invModel] = 0;
	DropPileSlots[pileid][slot][invQuantity] = 0;

	if (DropPile_CountItems(pileid) <= 0)
		DestroyDropPile(pileid);
	else
		DropPile_UpdateLabel(pileid);

	if (g_InvOpen[playerid])
		Inv_RefreshRightPanel(playerid);
	return 1;
}

// ---------------------------------------------------------------------------
// Right panel context (drops / player loot / external storage)
// ---------------------------------------------------------------------------

stock Inventory_Right_Clear(playerid)
{
	for (new slot = 0; slot < MAX_INVENTORY; slot++)
	{
		g_InvRightSlots[playerid][slot][invExists] = false;
		g_InvRightSlots[playerid][slot][invItem][0] = EOS;
		g_InvRightSlots[playerid][slot][invModel] = 0;
		g_InvRightSlots[playerid][slot][invQuantity] = 0;
	}
	return 1;
}

stock Inventory_Right_SetSlot(playerid, slot, const item[], model, quantity)
{
	if (slot < 0 || slot >= MAX_INVENTORY)
		return 0;

	if (quantity <= 0 || !item[0])
	{
		g_InvRightSlots[playerid][slot][invExists] = false;
		g_InvRightSlots[playerid][slot][invItem][0] = EOS;
		g_InvRightSlots[playerid][slot][invModel] = 0;
		g_InvRightSlots[playerid][slot][invQuantity] = 0;
	}
	else
	{
		g_InvRightSlots[playerid][slot][invExists] = true;
		g_InvRightSlots[playerid][slot][invModel] = model;
		g_InvRightSlots[playerid][slot][invQuantity] = quantity;
		format(g_InvRightSlots[playerid][slot][invItem], MAX_INVENTORY_ITEM_NAME, "%s", item);
	}

	if (g_InvOpen[playerid] && g_InvRightMode[playerid] == INV_RIGHT_STORAGE)
		Inv_RefreshRightSlot(playerid, slot);
	return 1;
}

stock Inventory_Right_GetSlot(playerid, slot, item[], itemLen, &model, &quantity)
{
	if (slot < 0 || slot >= MAX_INVENTORY || !g_InvRightSlots[playerid][slot][invExists])
	{
		item[0] = EOS;
		model = 0;
		quantity = 0;
		return 0;
	}
	format(item, itemLen, "%s", g_InvRightSlots[playerid][slot][invItem]);
	model = g_InvRightSlots[playerid][slot][invModel];
	quantity = g_InvRightSlots[playerid][slot][invQuantity];
	return 1;
}

stock Inventory_SetRightContext(playerid, mode, id = -1, const title[] = "")
{
	if (!IsPlayerConnected(playerid))
		return 0;
	if (mode != INV_RIGHT_DROPS && mode != INV_RIGHT_PLAYER && mode != INV_RIGHT_STORAGE)
		return 0;

	if (mode == INV_RIGHT_PLAYER)
	{
		if (id < 0 || id == playerid || !IsPlayerConnected(id))
			return 0;
	}

	g_InvRightMode[playerid] = mode;
	g_InvRightId[playerid] = id;

	if (title[0])
		format(g_InvRightTitle[playerid], MAX_INV_RIGHT_TITLE, "%s", title);
	else
	{
		switch (mode)
		{
			case INV_RIGHT_PLAYER:
			{
				new name[MAX_PLAYER_NAME];
				GetPlayerName(id, name, sizeof(name));
				format(g_InvRightTitle[playerid], MAX_INV_RIGHT_TITLE, "%s's Loot", name);
			}
			case INV_RIGHT_STORAGE:
				format(g_InvRightTitle[playerid], MAX_INV_RIGHT_TITLE, "Storage");
			default:
				format(g_InvRightTitle[playerid], MAX_INV_RIGHT_TITLE, "Nearby Drops");
		}
	}

	Inv_ClearDrag(playerid);
	if (g_InvOpen[playerid])
		Inv_RefreshRightPanel(playerid);
	return 1;
}

stock Inventory_ClearRightContext(playerid)
{
	g_InvRightMode[playerid] = INV_RIGHT_DROPS;
	g_InvRightId[playerid] = -1;
	g_InvRightTitle[playerid][0] = EOS;
	if (g_InvOpen[playerid])
		Inv_RefreshRightPanel(playerid);
	return 1;
}

stock Inventory_GetRightContext(playerid, &mode, &id)
{
	mode = g_InvRightMode[playerid];
	id = g_InvRightId[playerid];
	return 1;
}

stock Inventory_FindLootTarget(playerid, Float:range = INV_LOOT_RANGE)
{
	if (!IsPlayerConnected(playerid))
		return INVALID_PLAYER_ID;

	new Float:px, Float:py, Float:pz;
	GetPlayerPos(playerid, px, py, pz);
	new interior = GetPlayerInterior(playerid);
	new world = GetPlayerVirtualWorld(playerid);

	new best = INVALID_PLAYER_ID;
	new Float:bestDist = range + 1.0;

	for (new i = 0; i < MAX_PLAYERS; i++)
	{
		if (i == playerid || !IsPlayerConnected(i))
			continue;
		if (GetPlayerInterior(i) != interior || GetPlayerVirtualWorld(i) != world)
			continue;

		new Float:x, Float:y, Float:z;
		GetPlayerPos(i, x, y, z);
		new Float:dist = Inv_Dist3D(px, py, pz, x, y, z);
		if (dist <= range && dist < bestDist)
		{
			bestDist = dist;
			best = i;
		}
	}
	return best;
}

stock Inventory_OpenLoot(playerid, targetid)
{
	if (!Inventory_SetRightContext(playerid, INV_RIGHT_PLAYER, targetid))
		return 0;
	return Inventory_Show(playerid);
}

stock Inventory_OpenStorage(playerid, storageid, const title[] = "Storage")
{
	if (!Inventory_SetRightContext(playerid, INV_RIGHT_STORAGE, storageid, title))
		return 0;
	return Inventory_Show(playerid);
}

stock Inv_RightEnsureReady(playerid)
{
	switch (g_InvRightMode[playerid])
	{
		case INV_RIGHT_PLAYER:
		{
			new tid = g_InvRightId[playerid];
			if (tid < 0 || !IsPlayerConnected(tid))
			{
				Inventory_ClearRightContext(playerid);
				return 0;
			}
			return 1;
		}
		case INV_RIGHT_STORAGE:
			return 1;
		default:
		{
			Inv_GatherNearbyDrops(playerid);
			return 1;
		}
	}
}

stock bool:Inv_RightSlotExists(playerid, slot)
{
	if (slot < 0 || slot >= MAX_INVENTORY)
		return false;

	switch (g_InvRightMode[playerid])
	{
		case INV_RIGHT_PLAYER:
		{
			new tid = g_InvRightId[playerid];
			return (tid >= 0 && IsPlayerConnected(tid) && InventoryData[tid][slot][invExists]
				&& InventoryData[tid][slot][invQuantity] > 0);
		}
		case INV_RIGHT_STORAGE:
			return (g_InvRightSlots[playerid][slot][invExists] && g_InvRightSlots[playerid][slot][invQuantity] > 0);
		default:
		{
			new pileid = g_InvNearPile[playerid];
			return (pileid != -1 && DropPiles[pileid][dropExists]
				&& DropPileSlots[pileid][slot][invExists] && DropPileSlots[pileid][slot][invQuantity] > 0);
		}
	}
}

stock Inv_RightReadSlot(playerid, slot, &bool:exists, &model, &qty, name[], nameLen)
{
	exists = false;
	model = 0;
	qty = 0;
	name[0] = EOS;

	if (slot < 0 || slot >= MAX_INVENTORY)
		return 0;

	switch (g_InvRightMode[playerid])
	{
		case INV_RIGHT_PLAYER:
		{
			new tid = g_InvRightId[playerid];
			if (tid < 0 || !IsPlayerConnected(tid) || !InventoryData[tid][slot][invExists])
				return 0;
			exists = true;
			model = InventoryData[tid][slot][invModel];
			qty = InventoryData[tid][slot][invQuantity];
			format(name, nameLen, "%s", InventoryData[tid][slot][invItem]);
			return 1;
		}
		case INV_RIGHT_STORAGE:
		{
			if (!g_InvRightSlots[playerid][slot][invExists])
				return 0;
			exists = true;
			model = g_InvRightSlots[playerid][slot][invModel];
			qty = g_InvRightSlots[playerid][slot][invQuantity];
			format(name, nameLen, "%s", g_InvRightSlots[playerid][slot][invItem]);
			return 1;
		}
		default:
		{
			new pileid = g_InvNearPile[playerid];
			if (pileid < 0 || !DropPiles[pileid][dropExists] || !DropPileSlots[pileid][slot][invExists])
				return 0;
			exists = true;
			model = DropPileSlots[pileid][slot][invModel];
			qty = DropPileSlots[pileid][slot][invQuantity];
			format(name, nameLen, "%s", DropPileSlots[pileid][slot][invItem]);
			return 1;
		}
	}
}

stock Inv_RightWriteSlot(playerid, slot, bool:exists, model, qty, const name[])
{
	if (slot < 0 || slot >= MAX_INVENTORY)
		return 0;

	switch (g_InvRightMode[playerid])
	{
		case INV_RIGHT_PLAYER:
		{
			new tid = g_InvRightId[playerid];
			if (tid < 0 || !IsPlayerConnected(tid))
				return 0;
			if (!exists || qty <= 0 || !name[0])
			{
				InventoryData[tid][slot][invExists] = false;
				InventoryData[tid][slot][invItem][0] = EOS;
				InventoryData[tid][slot][invModel] = 0;
				InventoryData[tid][slot][invQuantity] = 0;
			}
			else
			{
				InventoryData[tid][slot][invExists] = true;
				InventoryData[tid][slot][invModel] = model;
				InventoryData[tid][slot][invQuantity] = qty;
				format(InventoryData[tid][slot][invItem], MAX_INVENTORY_ITEM_NAME, "%s", name);
			}
			if (g_InvOpen[tid])
				Inv_RefreshSlot(tid, slot);
			CallLocalFunction("Inv_OnInventoryChanged", "d", tid);
			return 1;
		}
		case INV_RIGHT_STORAGE:
		{
			if (!exists || qty <= 0 || !name[0])
			{
				g_InvRightSlots[playerid][slot][invExists] = false;
				g_InvRightSlots[playerid][slot][invItem][0] = EOS;
				g_InvRightSlots[playerid][slot][invModel] = 0;
				g_InvRightSlots[playerid][slot][invQuantity] = 0;
			}
			else
			{
				g_InvRightSlots[playerid][slot][invExists] = true;
				g_InvRightSlots[playerid][slot][invModel] = model;
				g_InvRightSlots[playerid][slot][invQuantity] = qty;
				format(g_InvRightSlots[playerid][slot][invItem], MAX_INVENTORY_ITEM_NAME, "%s", name);
			}
			return 1;
		}
		default:
		{
			new pileid = g_InvNearPile[playerid];
			if (pileid < 0 || !DropPiles[pileid][dropExists])
				return 0;
			if (!exists || qty <= 0 || !name[0])
			{
				DropPileSlots[pileid][slot][invExists] = false;
				DropPileSlots[pileid][slot][invItem][0] = EOS;
				DropPileSlots[pileid][slot][invModel] = 0;
				DropPileSlots[pileid][slot][invQuantity] = 0;
			}
			else
			{
				DropPileSlots[pileid][slot][invExists] = true;
				DropPileSlots[pileid][slot][invModel] = model;
				DropPileSlots[pileid][slot][invQuantity] = qty;
				format(DropPileSlots[pileid][slot][invItem], MAX_INVENTORY_ITEM_NAME, "%s", name);
			}
			return 1;
		}
	}
}

stock Inv_RightFindFree(playerid)
{
	for (new slot = 0; slot < MAX_INVENTORY; slot++)
		if (!Inv_RightSlotExists(playerid, slot))
			return slot;
	return -1;
}

// Right panel shows content for the active context mode.
stock Inv_GatherNearbyDrops(playerid)
{
	g_InvNearPile[playerid] = Item_Nearest(playerid, INV_DROP_RANGE);
	return (g_InvNearPile[playerid] != -1) ? DropPile_CountItems(g_InvNearPile[playerid]) : 0;
}

stock Inv_ClearDrag(playerid)
{
	new prevL = g_InvSelected[playerid];
	new prevR = g_InvSelected2[playerid];
	g_InvDragPanel[playerid] = INV_PANEL_NONE;
	g_InvDragSlot[playerid] = -1;
	g_InvSelected[playerid] = -1;
	g_InvSelected2[playerid] = -1;
	if (g_InvOpen[playerid])
	{
		if (prevL != -1)
			Inv_RefreshSlot(playerid, prevL);
		if (prevR != -1)
			Inv_RefreshRightSlot(playerid, prevR);
	}
	return 1;
}

stock Inv_SetDrag(playerid, panel, slot)
{
	new prevL = g_InvSelected[playerid];
	new prevR = g_InvSelected2[playerid];

	g_InvDragPanel[playerid] = panel;
	g_InvDragSlot[playerid] = slot;
	g_InvSelected[playerid] = (panel == INV_PANEL_LEFT) ? slot : -1;
	g_InvSelected2[playerid] = (panel == INV_PANEL_RIGHT) ? slot : -1;

	if (prevL != -1)
		Inv_RefreshSlot(playerid, prevL);
	if (prevR != -1)
		Inv_RefreshRightSlot(playerid, prevR);
	if (panel == INV_PANEL_LEFT)
		Inv_RefreshSlot(playerid, slot);
	else if (panel == INV_PANEL_RIGHT)
		Inv_RefreshRightSlot(playerid, slot);

	SendClientMessage(playerid, 0xAAAAAAFF, "Selected - click another slot to move.");
	return 1;
}

stock DropPile_GetOrCreateNearPlayer(playerid)
{
	new pileid = Item_Nearest(playerid, INV_DROP_RANGE);
	if (pileid != -1)
		return pileid;

	new Float:x, Float:y, Float:z, Float:a;
	GetPlayerPos(playerid, x, y, z);
	GetPlayerFacingAngle(playerid, a);
	x += INV_DROP_FORWARD * floatsin(-a, degrees);
	y += INV_DROP_FORWARD * floatcos(-a, degrees);
	return DropPile_Create(x, y, z, GetPlayerInterior(playerid), GetPlayerVirtualWorld(playerid));
}

// Move / swap / stack between any two slots (left inventory or right context).
stock Inv_MoveBetweenSlots(playerid, fromPanel, fromSlot, toPanel, toSlot)
{
	if (fromPanel == toPanel && fromSlot == toSlot)
		return 0;

	if (!Inv_RightEnsureReady(playerid) && (fromPanel == INV_PANEL_RIGHT || toPanel == INV_PANEL_RIGHT))
		return 0;

	new bool:fromExists, fromModel, fromQty, fromName[MAX_INVENTORY_ITEM_NAME];
	new bool:toExists, toModel, toQty, toName[MAX_INVENTORY_ITEM_NAME];
	new mode = g_InvRightMode[playerid];
	new pileid = g_InvNearPile[playerid];

	if (mode == INV_RIGHT_DROPS && (fromPanel == INV_PANEL_RIGHT || toPanel == INV_PANEL_RIGHT))
	{
		if (toPanel == INV_PANEL_RIGHT && (pileid < 0 || !DropPiles[pileid][dropExists]))
		{
			pileid = DropPile_GetOrCreateNearPlayer(playerid);
			if (pileid == -1)
				return 0;
			g_InvNearPile[playerid] = pileid;
		}
		if (fromPanel == INV_PANEL_RIGHT && (pileid < 0 || !DropPiles[pileid][dropExists]))
			return 0;
	}

	if (fromPanel == INV_PANEL_LEFT)
	{
		if (fromSlot < 0 || fromSlot >= MAX_INVENTORY || !InventoryData[playerid][fromSlot][invExists])
			return 0;
		fromExists = true;
		fromModel = InventoryData[playerid][fromSlot][invModel];
		fromQty = InventoryData[playerid][fromSlot][invQuantity];
		format(fromName, sizeof(fromName), "%s", InventoryData[playerid][fromSlot][invItem]);
	}
	else
	{
		if (!Inv_RightReadSlot(playerid, fromSlot, fromExists, fromModel, fromQty, fromName, sizeof(fromName)))
			return 0;
	}

	if (toPanel == INV_PANEL_LEFT)
	{
		toExists = InventoryData[playerid][toSlot][invExists];
		toModel = InventoryData[playerid][toSlot][invModel];
		toQty = InventoryData[playerid][toSlot][invQuantity];
		format(toName, sizeof(toName), "%s", InventoryData[playerid][toSlot][invItem]);
	}
	else
	{
		Inv_RightReadSlot(playerid, toSlot, toExists, toModel, toQty, toName, sizeof(toName));
	}

	// Stack same items onto target
	if (toExists && !strcmp(fromName, toName, true) && fromModel == toModel)
	{
		toQty += fromQty;
		fromExists = false;
		fromQty = 0;
		fromModel = 0;
		fromName[0] = EOS;
	}
	else
	{
		// Swap (or move into empty)
		new bool:tmpExists = toExists, tmpModel = toModel, tmpQty = toQty, tmpName[MAX_INVENTORY_ITEM_NAME];
		format(tmpName, sizeof(tmpName), "%s", toName);

		toExists = fromExists;
		toModel = fromModel;
		toQty = fromQty;
		format(toName, sizeof(toName), "%s", fromName);

		fromExists = tmpExists;
		fromModel = tmpModel;
		fromQty = tmpQty;
		format(fromName, sizeof(fromName), "%s", tmpName);
	}

	if (fromPanel == INV_PANEL_LEFT)
	{
		InventoryData[playerid][fromSlot][invExists] = fromExists;
		InventoryData[playerid][fromSlot][invModel] = fromModel;
		InventoryData[playerid][fromSlot][invQuantity] = fromQty;
		format(InventoryData[playerid][fromSlot][invItem], MAX_INVENTORY_ITEM_NAME, "%s", fromName);
	}
	else if (!Inv_RightWriteSlot(playerid, fromSlot, fromExists, fromModel, fromQty, fromName))
		return 0;

	if (toPanel == INV_PANEL_LEFT)
	{
		InventoryData[playerid][toSlot][invExists] = toExists;
		InventoryData[playerid][toSlot][invModel] = toModel;
		InventoryData[playerid][toSlot][invQuantity] = toQty;
		format(InventoryData[playerid][toSlot][invItem], MAX_INVENTORY_ITEM_NAME, "%s", toName);
	}
	else if (!Inv_RightWriteSlot(playerid, toSlot, toExists, toModel, toQty, toName))
		return 0;

	if (fromPanel == INV_PANEL_LEFT && toPanel == INV_PANEL_RIGHT)
	{
		if (mode == INV_RIGHT_DROPS)
		{
			CallLocalFunction("Inv_OnItemDrop", "ddsii", playerid, fromSlot, toName, toModel, toQty);
			Inventory_NotifyRemoved(playerid, toName, toModel, toQty);
		}
		else if (mode == INV_RIGHT_PLAYER)
		{
			CallLocalFunction("Inv_OnLootTransfer", "ddisii", playerid, g_InvRightId[playerid], false, toName, toModel, toQty);
			Inventory_NotifyRemoved(playerid, toName, toModel, toQty);
		}
		else
		{
			CallLocalFunction("Inv_OnRightStorageChanged", "dd", playerid, g_InvRightId[playerid]);
			Inventory_NotifyRemoved(playerid, toName, toModel, toQty);
		}
	}
	else if (fromPanel == INV_PANEL_RIGHT && toPanel == INV_PANEL_LEFT)
	{
		if (mode == INV_RIGHT_DROPS)
		{
			CallLocalFunction("Inv_OnItemPickup", "ddsii", playerid, pileid, toName, toModel, toQty);
			Inventory_NotifyReceived(playerid, toName, toModel, toQty);
		}
		else if (mode == INV_RIGHT_PLAYER)
		{
			CallLocalFunction("Inv_OnLootTransfer", "ddisii", playerid, g_InvRightId[playerid], true, toName, toModel, toQty);
			Inventory_NotifyReceived(playerid, toName, toModel, toQty);
		}
		else
		{
			CallLocalFunction("Inv_OnRightStorageChanged", "dd", playerid, g_InvRightId[playerid]);
			Inventory_NotifyReceived(playerid, toName, toModel, toQty);
		}
	}
	else if (mode == INV_RIGHT_STORAGE && (fromPanel == INV_PANEL_RIGHT || toPanel == INV_PANEL_RIGHT))
	{
		CallLocalFunction("Inv_OnRightStorageChanged", "dd", playerid, g_InvRightId[playerid]);
	}
	else if (mode == INV_RIGHT_PLAYER && (fromPanel == INV_PANEL_RIGHT || toPanel == INV_PANEL_RIGHT))
	{
		CallLocalFunction("Inv_OnInventoryChanged", "d", g_InvRightId[playerid]);
	}

	if (mode == INV_RIGHT_DROPS && pileid != -1 && DropPiles[pileid][dropExists])
	{
		if (DropPile_CountItems(pileid) <= 0)
		{
			DestroyDropPile(pileid);
			g_InvNearPile[playerid] = -1;
		}
		else
			DropPile_UpdateLabel(pileid);
	}

	Inv_ClearDrag(playerid);
	Inv_RefreshPanel(playerid);
	Inv_RefreshRightPanel(playerid);
	Inv_NotifyChanged(playerid);
	return 1;
}

stock Inv_DoDropFromSlot(playerid, slot, amount)
{
	if (slot < 0 || !InventoryData[playerid][slot][invExists])
		return 0;
	if (amount < 1 || amount > InventoryData[playerid][slot][invQuantity])
		return 0;

	new itemName[MAX_INVENTORY_ITEM_NAME];
	format(itemName, sizeof(itemName), "%s", InventoryData[playerid][slot][invItem]);
	new model = InventoryData[playerid][slot][invModel];

	CallLocalFunction("Inv_OnItemDrop", "ddsii", playerid, slot, itemName, model, amount);
	Inventory_RemoveEx(playerid, slot, amount);
	DropItemInFront(playerid, itemName, model, amount);

	if (g_InvOpen[playerid])
		Inv_RefreshRightPanel(playerid);
	return 1;
}

// ---------------------------------------------------------------------------
// Left panel refresh
// ---------------------------------------------------------------------------

stock Inv_RefreshSlot(playerid, slot)
{
	if (!g_InvOpen[playerid] || slot < 0 || slot >= INV_UI_SLOTS)
		return 0;

	new string[16];
	new bool:selected = (g_InvSelected[playerid] == slot);

	if (!InventoryData[playerid][slot][invExists] || InventoryData[playerid][slot][invQuantity] < 1)
	{
		PlayerTextDrawSetPreviewModel(playerid, g_InvPreview[playerid][slot], INV_EMPTY_MODEL);
		PlayerTextDrawBackgroundColour(playerid, g_InvPreview[playerid][slot], 30);
		PlayerTextDrawSetString(playerid, g_InvName[playerid][slot], " ");
		PlayerTextDrawSetString(playerid, g_InvQty[playerid][slot], " ");
		PlayerTextDrawUseBox(playerid, g_InvAccent[playerid][slot], false);
		PlayerTextDrawHide(playerid, g_InvAccent[playerid][slot]);
		PlayerTextDrawHide(playerid, g_InvPreview[playerid][slot]);
		PlayerTextDrawShow(playerid, g_InvPreview[playerid][slot]);
	}
	else
	{
		PlayerTextDrawSetPreviewModel(playerid, g_InvPreview[playerid][slot], InventoryData[playerid][slot][invModel]);
		PlayerTextDrawBackgroundColour(playerid, g_InvPreview[playerid][slot], selected ? (-1523963217) : (30));
		PlayerTextDrawSetString(playerid, g_InvName[playerid][slot], InventoryData[playerid][slot][invItem]);
		if (Inv_IsGunItem(InventoryData[playerid][slot][invItem]))
			format(string, sizeof(string), "A:%i", InventoryData[playerid][slot][invQuantity]);
		else
			format(string, sizeof(string), "%i", InventoryData[playerid][slot][invQuantity]);
		PlayerTextDrawSetString(playerid, g_InvQty[playerid][slot], string);
		PlayerTextDrawUseBox(playerid, g_InvAccent[playerid][slot], true);
		PlayerTextDrawShow(playerid, g_InvName[playerid][slot]);
		PlayerTextDrawHide(playerid, g_InvQty[playerid][slot]);
		PlayerTextDrawShow(playerid, g_InvQty[playerid][slot]);
		PlayerTextDrawHide(playerid, g_InvPreview[playerid][slot]);
		PlayerTextDrawShow(playerid, g_InvPreview[playerid][slot]);
		PlayerTextDrawShow(playerid, g_InvAccent[playerid][slot]);
	}

	format(string, sizeof(string), "%i / %i", Inventory_Items(playerid), MAX_INVENTORY);
	PlayerTextDrawSetString(playerid, g_InvWeight[playerid], string);
	return 1;
}

stock Inv_RefreshPanel(playerid)
{
	if (!g_InvOpen[playerid])
		return 0;

	for (new slot = 0; slot < INV_UI_SLOTS; slot++)
	{
		Inv_RefreshSlot(playerid, slot);
		PlayerTextDrawShow(playerid, g_InvPreview[playerid][slot]);
		PlayerTextDrawShow(playerid, g_InvName[playerid][slot]);
		PlayerTextDrawShow(playerid, g_InvQty[playerid][slot]);
	}
	return 1;
}

stock Inv_RefreshRightSlot(playerid, ui)
{
	if (!g_InvOpen[playerid] || ui < 0 || ui >= INV_UI_SLOTS)
		return 0;

	new string[16];
	new bool:selected = (g_InvSelected2[playerid] == ui);
	new bool:exists, model, qty, itemName[MAX_INVENTORY_ITEM_NAME];
	new bool:hasItem = false;

	if (Inv_RightReadSlot(playerid, ui, exists, model, qty, itemName, sizeof(itemName)) && exists && qty > 0)
		hasItem = true;

	if (!hasItem)
	{
		PlayerTextDrawSetPreviewModel(playerid, g_Inv2Preview[playerid][ui], INV_EMPTY_MODEL);
		PlayerTextDrawBackgroundColour(playerid, g_Inv2Preview[playerid][ui], 30);
		PlayerTextDrawSetString(playerid, g_Inv2Name[playerid][ui], " ");
		PlayerTextDrawSetString(playerid, g_Inv2Qty[playerid][ui], " ");
		PlayerTextDrawUseBox(playerid, g_Inv2Accent[playerid][ui], false);
		PlayerTextDrawHide(playerid, g_Inv2Accent[playerid][ui]);
		PlayerTextDrawHide(playerid, g_Inv2Preview[playerid][ui]);
		PlayerTextDrawShow(playerid, g_Inv2Preview[playerid][ui]);
	}
	else
	{
		PlayerTextDrawSetPreviewModel(playerid, g_Inv2Preview[playerid][ui], model);
		PlayerTextDrawBackgroundColour(playerid, g_Inv2Preview[playerid][ui], selected ? (-1523963217) : (30));
		PlayerTextDrawSetString(playerid, g_Inv2Name[playerid][ui], itemName);
		if (Inv_IsGunItem(itemName))
			format(string, sizeof(string), "A:%i", qty);
		else
			format(string, sizeof(string), "%i", qty);
		PlayerTextDrawSetString(playerid, g_Inv2Qty[playerid][ui], string);
		PlayerTextDrawUseBox(playerid, g_Inv2Accent[playerid][ui], true);
		PlayerTextDrawShow(playerid, g_Inv2Name[playerid][ui]);
		PlayerTextDrawHide(playerid, g_Inv2Qty[playerid][ui]);
		PlayerTextDrawShow(playerid, g_Inv2Qty[playerid][ui]);
		PlayerTextDrawHide(playerid, g_Inv2Preview[playerid][ui]);
		PlayerTextDrawShow(playerid, g_Inv2Preview[playerid][ui]);
		PlayerTextDrawShow(playerid, g_Inv2Accent[playerid][ui]);
	}
	return 1;
}

stock Inv_RefreshRightPanel(playerid)
{
	if (!g_InvOpen[playerid])
		return 0;

	Inv_RightEnsureReady(playerid);
	g_InvSelected2[playerid] = -1;

	if (g_InvRightTitle[playerid][0])
		PlayerTextDrawSetString(playerid, g_Inv2Title[playerid], g_InvRightTitle[playerid]);
	else
	{
		switch (g_InvRightMode[playerid])
		{
			case INV_RIGHT_PLAYER:
				PlayerTextDrawSetString(playerid, g_Inv2Title[playerid], "Player Loot");
			case INV_RIGHT_STORAGE:
				PlayerTextDrawSetString(playerid, g_Inv2Title[playerid], "Storage");
			default:
			{
				if (g_InvNearPile[playerid] == -1)
					PlayerTextDrawSetString(playerid, g_Inv2Title[playerid], "Nearby Drops");
				else
					PlayerTextDrawSetString(playerid, g_Inv2Title[playerid], "Nearest Bag");
			}
		}
	}

	for (new ui = 0; ui < INV_UI_SLOTS; ui++)
	{
		Inv_RefreshRightSlot(playerid, ui);
		PlayerTextDrawShow(playerid, g_Inv2Preview[playerid][ui]);
		PlayerTextDrawShow(playerid, g_Inv2Name[playerid][ui]);
		PlayerTextDrawShow(playerid, g_Inv2Qty[playerid][ui]);
	}
	return 1;
}

// ---------------------------------------------------------------------------
// UI create / destroy
// ---------------------------------------------------------------------------

stock Inv_BuildActionButtons(playerid)
{
	g_InvBtn[playerid][0] = CreatePlayerTextDraw(playerid, 337.000000, 200.000000, "Use");
	PlayerTextDrawFont(playerid, g_InvBtn[playerid][0], TEXT_DRAW_FONT_1);
	PlayerTextDrawLetterSize(playerid, g_InvBtn[playerid][0], 0.245833, 1.150001);
	PlayerTextDrawTextSize(playerid, g_InvBtn[playerid][0], 6.500000, 42.000000);
	PlayerTextDrawSetOutline(playerid, g_InvBtn[playerid][0], 0);
	PlayerTextDrawSetShadow(playerid, g_InvBtn[playerid][0], 0);
	PlayerTextDrawAlignment(playerid, g_InvBtn[playerid][0], TEXT_DRAW_ALIGN_CENTRE);
	PlayerTextDrawColour(playerid, g_InvBtn[playerid][0], -1);
	PlayerTextDrawBackgroundColour(playerid, g_InvBtn[playerid][0], 255);
	PlayerTextDrawBoxColour(playerid, g_InvBtn[playerid][0], 49);
	PlayerTextDrawUseBox(playerid, g_InvBtn[playerid][0], true);
	PlayerTextDrawSetProportional(playerid, g_InvBtn[playerid][0], true);
	PlayerTextDrawSetSelectable(playerid, g_InvBtn[playerid][0], true);

	g_InvBtn[playerid][1] = CreatePlayerTextDraw(playerid, 337.000000, 220.000000, "Drop");
	PlayerTextDrawFont(playerid, g_InvBtn[playerid][1], TEXT_DRAW_FONT_1);
	PlayerTextDrawLetterSize(playerid, g_InvBtn[playerid][1], 0.245833, 1.150001);
	PlayerTextDrawTextSize(playerid, g_InvBtn[playerid][1], 6.500000, 42.000000);
	PlayerTextDrawSetOutline(playerid, g_InvBtn[playerid][1], 0);
	PlayerTextDrawSetShadow(playerid, g_InvBtn[playerid][1], 0);
	PlayerTextDrawAlignment(playerid, g_InvBtn[playerid][1], TEXT_DRAW_ALIGN_CENTRE);
	PlayerTextDrawColour(playerid, g_InvBtn[playerid][1], -1);
	PlayerTextDrawBackgroundColour(playerid, g_InvBtn[playerid][1], 255);
	PlayerTextDrawBoxColour(playerid, g_InvBtn[playerid][1], 49);
	PlayerTextDrawUseBox(playerid, g_InvBtn[playerid][1], true);
	PlayerTextDrawSetProportional(playerid, g_InvBtn[playerid][1], true);
	PlayerTextDrawSetSelectable(playerid, g_InvBtn[playerid][1], true);

	g_InvBtn[playerid][2] = CreatePlayerTextDraw(playerid, 337.000000, 240.000000, "Close");
	PlayerTextDrawFont(playerid, g_InvBtn[playerid][2], TEXT_DRAW_FONT_1);
	PlayerTextDrawLetterSize(playerid, g_InvBtn[playerid][2], 0.245833, 1.150001);
	PlayerTextDrawTextSize(playerid, g_InvBtn[playerid][2], 6.000000, 41.500000);
	PlayerTextDrawSetOutline(playerid, g_InvBtn[playerid][2], 0);
	PlayerTextDrawSetShadow(playerid, g_InvBtn[playerid][2], 0);
	PlayerTextDrawAlignment(playerid, g_InvBtn[playerid][2], TEXT_DRAW_ALIGN_CENTRE);
	PlayerTextDrawColour(playerid, g_InvBtn[playerid][2], -1);
	PlayerTextDrawBackgroundColour(playerid, g_InvBtn[playerid][2], 255);
	PlayerTextDrawBoxColour(playerid, g_InvBtn[playerid][2], 49);
	PlayerTextDrawUseBox(playerid, g_InvBtn[playerid][2], true);
	PlayerTextDrawSetProportional(playerid, g_InvBtn[playerid][2], true);
	PlayerTextDrawSetSelectable(playerid, g_InvBtn[playerid][2], true);
}

stock Inv_CreateSlotGrid(playerid, bool:right)
{
	new Float:x = right ? 355.000000 : 107.000000;
	new Float:y = 141.000000;
	new Float:startX = right ? 385.000000 : 137.000000;

	for (new i = 0, idx = 0; i < INV_UI_SLOTS; i++)
	{
		if (idx > 0 && (idx % 5) == 0)
		{
			x = startX;
			y += 43.0;
		}
		else
			x += 30.0;
		idx++;

		new PlayerText:accent, PlayerText:nameTD, PlayerText:qtyTD, PlayerText:preview;

		accent = CreatePlayerTextDraw(playerid, x + 14.0, y + 28.0, "_");
		PlayerTextDrawFont(playerid, accent, TEXT_DRAW_FONT_1);
		PlayerTextDrawLetterSize(playerid, accent, 0.600000, 0.500002);
		PlayerTextDrawTextSize(playerid, accent, 0.000000, 24.700000);
		PlayerTextDrawSetOutline(playerid, accent, 1);
		PlayerTextDrawSetShadow(playerid, accent, 0);
		PlayerTextDrawAlignment(playerid, accent, TEXT_DRAW_ALIGN_CENTRE);
		PlayerTextDrawColour(playerid, accent, -1);
		PlayerTextDrawBackgroundColour(playerid, accent, 255);
		PlayerTextDrawBoxColour(playerid, accent, 1687547391);
		PlayerTextDrawUseBox(playerid, accent, true);
		PlayerTextDrawSetProportional(playerid, accent, true);
		PlayerTextDrawSetSelectable(playerid, accent, false);

		nameTD = CreatePlayerTextDraw(playerid, x + 14.0, y + 26.0, " ");
		PlayerTextDrawFont(playerid, nameTD, TEXT_DRAW_FONT_1);
		PlayerTextDrawLetterSize(playerid, nameTD, 0.087499, 0.600000);
		PlayerTextDrawTextSize(playerid, nameTD, 385.000000, 25.000000);
		PlayerTextDrawSetOutline(playerid, nameTD, 0);
		PlayerTextDrawSetShadow(playerid, nameTD, 0);
		PlayerTextDrawAlignment(playerid, nameTD, TEXT_DRAW_ALIGN_CENTRE);
		PlayerTextDrawColour(playerid, nameTD, -1);
		PlayerTextDrawBackgroundColour(playerid, nameTD, 255);
		PlayerTextDrawBoxColour(playerid, nameTD, 9109759);
		PlayerTextDrawUseBox(playerid, nameTD, false);
		PlayerTextDrawSetProportional(playerid, nameTD, true);
		PlayerTextDrawSetSelectable(playerid, nameTD, false);

		qtyTD = CreatePlayerTextDraw(playerid, x + 1.0, y + 1.0, " ");
		PlayerTextDrawFont(playerid, qtyTD, TEXT_DRAW_FONT_1);
		PlayerTextDrawLetterSize(playerid, qtyTD, 0.124999, 0.650000);
		PlayerTextDrawTextSize(playerid, qtyTD, 163.000000, 25.000000);
		PlayerTextDrawSetOutline(playerid, qtyTD, 0);
		PlayerTextDrawSetShadow(playerid, qtyTD, 0);
		PlayerTextDrawAlignment(playerid, qtyTD, TEXT_DRAW_ALIGN_LEFT);
		PlayerTextDrawColour(playerid, qtyTD, -1);
		PlayerTextDrawBackgroundColour(playerid, qtyTD, 255);
		PlayerTextDrawBoxColour(playerid, qtyTD, 9109759);
		PlayerTextDrawUseBox(playerid, qtyTD, false);
		PlayerTextDrawSetProportional(playerid, qtyTD, true);
		PlayerTextDrawSetSelectable(playerid, qtyTD, false);

		preview = CreatePlayerTextDraw(playerid, x, y, "Preview_Model");
		PlayerTextDrawFont(playerid, preview, TEXT_DRAW_FONT_MODEL_PREVIEW);
		PlayerTextDrawLetterSize(playerid, preview, 0.600000, 2.000000);
		PlayerTextDrawTextSize(playerid, preview, 27.500000, 34.000000);
		PlayerTextDrawSetOutline(playerid, preview, 0);
		PlayerTextDrawSetShadow(playerid, preview, 0);
		PlayerTextDrawAlignment(playerid, preview, TEXT_DRAW_ALIGN_LEFT);
		PlayerTextDrawColour(playerid, preview, -1);
		PlayerTextDrawBackgroundColour(playerid, preview, 30);
		PlayerTextDrawBoxColour(playerid, preview, 255);
		PlayerTextDrawUseBox(playerid, preview, false);
		PlayerTextDrawSetProportional(playerid, preview, true);
		PlayerTextDrawSetSelectable(playerid, preview, true);
		PlayerTextDrawSetPreviewModel(playerid, preview, INV_EMPTY_MODEL);
		PlayerTextDrawSetPreviewRot(playerid, preview, -85.000000, 0.000000, -11.000000, 1.000000);
		PlayerTextDrawSetPreviewVehicleColours(playerid, preview, 1, 1);

		if (right)
		{
			g_Inv2Accent[playerid][i] = accent;
			g_Inv2Name[playerid][i] = nameTD;
			g_Inv2Qty[playerid][i] = qtyTD;
			g_Inv2Preview[playerid][i] = preview;
		}
		else
		{
			g_InvAccent[playerid][i] = accent;
			g_InvName[playerid][i] = nameTD;
			g_InvQty[playerid][i] = qtyTD;
			g_InvPreview[playerid][i] = preview;
		}
	}
}

stock Inv_CreateUI(playerid)
{
	g_InvBg[playerid][0] = CreatePlayerTextDraw(playerid, 211.000000, 126.000000, "_");
	PlayerTextDrawFont(playerid, g_InvBg[playerid][0], TEXT_DRAW_FONT_1);
	PlayerTextDrawLetterSize(playerid, g_InvBg[playerid][0], 0.600000, 20.250005);
	PlayerTextDrawTextSize(playerid, g_InvBg[playerid][0], 298.500000, 151.500000);
	PlayerTextDrawSetOutline(playerid, g_InvBg[playerid][0], 1);
	PlayerTextDrawSetShadow(playerid, g_InvBg[playerid][0], 0);
	PlayerTextDrawAlignment(playerid, g_InvBg[playerid][0], TEXT_DRAW_ALIGN_CENTRE);
	PlayerTextDrawColour(playerid, g_InvBg[playerid][0], -1);
	PlayerTextDrawBackgroundColour(playerid, g_InvBg[playerid][0], 16777215);
	PlayerTextDrawBoxColour(playerid, g_InvBg[playerid][0], 1296911764);
	PlayerTextDrawUseBox(playerid, g_InvBg[playerid][0], true);
	PlayerTextDrawSetSelectable(playerid, g_InvBg[playerid][0], false);

	g_InvBg[playerid][1] = CreatePlayerTextDraw(playerid, 337.000000, 187.000000, "_");
	PlayerTextDrawFont(playerid, g_InvBg[playerid][1], TEXT_DRAW_FONT_1);
	PlayerTextDrawLetterSize(playerid, g_InvBg[playerid][1], 0.600000, 8.50000);
	PlayerTextDrawTextSize(playerid, g_InvBg[playerid][1], 298.500000, 47.000000);
	PlayerTextDrawSetOutline(playerid, g_InvBg[playerid][1], 1);
	PlayerTextDrawSetShadow(playerid, g_InvBg[playerid][1], 0);
	PlayerTextDrawAlignment(playerid, g_InvBg[playerid][1], TEXT_DRAW_ALIGN_CENTRE);
	PlayerTextDrawColour(playerid, g_InvBg[playerid][1], -1);
	PlayerTextDrawBackgroundColour(playerid, g_InvBg[playerid][1], 16777215);
	PlayerTextDrawBoxColour(playerid, g_InvBg[playerid][1], 1296911764);
	PlayerTextDrawUseBox(playerid, g_InvBg[playerid][1], true);
	PlayerTextDrawSetSelectable(playerid, g_InvBg[playerid][1], false);

	Inv_BuildActionButtons(playerid);

	g_InvTitle[playerid] = CreatePlayerTextDraw(playerid, 135.000000, 125.000000, "Inventory");
	PlayerTextDrawFont(playerid, g_InvTitle[playerid], TEXT_DRAW_FONT_1);
	PlayerTextDrawLetterSize(playerid, g_InvTitle[playerid], 0.158333, 0.900000);
	PlayerTextDrawTextSize(playerid, g_InvTitle[playerid], 400.000000, 17.000000);
	PlayerTextDrawSetOutline(playerid, g_InvTitle[playerid], 0);
	PlayerTextDrawSetShadow(playerid, g_InvTitle[playerid], 0);
	PlayerTextDrawAlignment(playerid, g_InvTitle[playerid], TEXT_DRAW_ALIGN_LEFT);
	PlayerTextDrawColour(playerid, g_InvTitle[playerid], -1);
	PlayerTextDrawBackgroundColour(playerid, g_InvTitle[playerid], 255);
	PlayerTextDrawBoxColour(playerid, g_InvTitle[playerid], 50);
	PlayerTextDrawUseBox(playerid, g_InvTitle[playerid], false);
	PlayerTextDrawSetProportional(playerid, g_InvTitle[playerid], true);
	PlayerTextDrawSetSelectable(playerid, g_InvTitle[playerid], false);

	g_InvWeight[playerid] = CreatePlayerTextDraw(playerid, 135.000000, 133.000000, "0 / 20");
	PlayerTextDrawFont(playerid, g_InvWeight[playerid], TEXT_DRAW_FONT_1);
	PlayerTextDrawLetterSize(playerid, g_InvWeight[playerid], 0.137500, 0.650000);
	PlayerTextDrawTextSize(playerid, g_InvWeight[playerid], 400.000000, 17.000000);
	PlayerTextDrawSetOutline(playerid, g_InvWeight[playerid], 0);
	PlayerTextDrawSetShadow(playerid, g_InvWeight[playerid], 0);
	PlayerTextDrawAlignment(playerid, g_InvWeight[playerid], TEXT_DRAW_ALIGN_LEFT);
	PlayerTextDrawColour(playerid, g_InvWeight[playerid], -1);
	PlayerTextDrawBackgroundColour(playerid, g_InvWeight[playerid], 255);
	PlayerTextDrawBoxColour(playerid, g_InvWeight[playerid], 50);
	PlayerTextDrawUseBox(playerid, g_InvWeight[playerid], false);
	PlayerTextDrawSetProportional(playerid, g_InvWeight[playerid], true);
	PlayerTextDrawSetSelectable(playerid, g_InvWeight[playerid], false);

	Inv_CreateSlotGrid(playerid, false);

	// Right panel - nearby ground drops
	g_Inv2Bg[playerid] = CreatePlayerTextDraw(playerid, 459.000000, 126.000000, "_");
	PlayerTextDrawFont(playerid, g_Inv2Bg[playerid], TEXT_DRAW_FONT_1);
	PlayerTextDrawLetterSize(playerid, g_Inv2Bg[playerid], 0.600000, 20.250005);
	PlayerTextDrawTextSize(playerid, g_Inv2Bg[playerid], 298.500000, 151.500000);
	PlayerTextDrawSetOutline(playerid, g_Inv2Bg[playerid], 1);
	PlayerTextDrawSetShadow(playerid, g_Inv2Bg[playerid], 0);
	PlayerTextDrawAlignment(playerid, g_Inv2Bg[playerid], TEXT_DRAW_ALIGN_CENTRE);
	PlayerTextDrawColour(playerid, g_Inv2Bg[playerid], -1);
	PlayerTextDrawBackgroundColour(playerid, g_Inv2Bg[playerid], 16777215);
	PlayerTextDrawBoxColour(playerid, g_Inv2Bg[playerid], 1296911764);
	PlayerTextDrawUseBox(playerid, g_Inv2Bg[playerid], true);
	PlayerTextDrawSetSelectable(playerid, g_Inv2Bg[playerid], false);

	g_Inv2Title[playerid] = CreatePlayerTextDraw(playerid, 386.000000, 125.000000, "Nearby Drops");
	PlayerTextDrawFont(playerid, g_Inv2Title[playerid], TEXT_DRAW_FONT_1);
	PlayerTextDrawLetterSize(playerid, g_Inv2Title[playerid], 0.158333, 0.900000);
	PlayerTextDrawTextSize(playerid, g_Inv2Title[playerid], 532.500000, 17.000000);
	PlayerTextDrawSetOutline(playerid, g_Inv2Title[playerid], 0);
	PlayerTextDrawSetShadow(playerid, g_Inv2Title[playerid], 0);
	PlayerTextDrawAlignment(playerid, g_Inv2Title[playerid], TEXT_DRAW_ALIGN_LEFT);
	PlayerTextDrawColour(playerid, g_Inv2Title[playerid], -1);
	PlayerTextDrawBackgroundColour(playerid, g_Inv2Title[playerid], 255);
	PlayerTextDrawBoxColour(playerid, g_Inv2Title[playerid], 50);
	PlayerTextDrawUseBox(playerid, g_Inv2Title[playerid], false);
	PlayerTextDrawSetProportional(playerid, g_Inv2Title[playerid], true);
	PlayerTextDrawSetSelectable(playerid, g_Inv2Title[playerid], false);

	Inv_CreateSlotGrid(playerid, true);

	g_InvBuilt[playerid] = true;
	return 1;
}

stock Inv_DestroyUI(playerid)
{
	if (!g_InvBuilt[playerid])
		return 0;

	for (new i = 0; i < INV_UI_SLOTS; i++)
	{
		PlayerTextDrawDestroy(playerid, g_InvPreview[playerid][i]);
		PlayerTextDrawDestroy(playerid, g_InvAccent[playerid][i]);
		PlayerTextDrawDestroy(playerid, g_InvName[playerid][i]);
		PlayerTextDrawDestroy(playerid, g_InvQty[playerid][i]);
		PlayerTextDrawDestroy(playerid, g_Inv2Preview[playerid][i]);
		PlayerTextDrawDestroy(playerid, g_Inv2Accent[playerid][i]);
		PlayerTextDrawDestroy(playerid, g_Inv2Name[playerid][i]);
		PlayerTextDrawDestroy(playerid, g_Inv2Qty[playerid][i]);
	}

	PlayerTextDrawDestroy(playerid, g_InvBg[playerid][0]);
	PlayerTextDrawDestroy(playerid, g_InvBg[playerid][1]);
	PlayerTextDrawDestroy(playerid, g_InvTitle[playerid]);
	PlayerTextDrawDestroy(playerid, g_InvWeight[playerid]);
	PlayerTextDrawDestroy(playerid, g_InvBtn[playerid][0]);
	PlayerTextDrawDestroy(playerid, g_InvBtn[playerid][1]);
	PlayerTextDrawDestroy(playerid, g_InvBtn[playerid][2]);
	PlayerTextDrawDestroy(playerid, g_Inv2Bg[playerid]);
	PlayerTextDrawDestroy(playerid, g_Inv2Title[playerid]);

	g_InvBuilt[playerid] = false;
	return 1;
}

stock Inventory_Show(playerid)
{
	if (!IsPlayerConnected(playerid))
		return 0;
	if (g_InvOpen[playerid])
		Inventory_Hide(playerid, false);

	g_InvSelected[playerid] = -1;
	g_InvSelected2[playerid] = -1;
	g_InvDragPanel[playerid] = INV_PANEL_NONE;
	g_InvDragSlot[playerid] = -1;
	Inv_CreateUI(playerid);

	new name[MAX_PLAYER_NAME], title[48];
	GetPlayerName(playerid, name, sizeof(name));
	format(title, sizeof(title), "%s's Inventory", name);
	PlayerTextDrawSetString(playerid, g_InvTitle[playerid], title);

	g_InvOpen[playerid] = true;
	SelectTextDraw(playerid, INV_SELECT_COLOUR);

	PlayerTextDrawShow(playerid, g_InvBg[playerid][0]);
	PlayerTextDrawShow(playerid, g_InvBg[playerid][1]);
	PlayerTextDrawShow(playerid, g_InvTitle[playerid]);
	PlayerTextDrawShow(playerid, g_InvWeight[playerid]);
	PlayerTextDrawShow(playerid, g_InvBtn[playerid][0]);
	PlayerTextDrawShow(playerid, g_InvBtn[playerid][1]);
	PlayerTextDrawShow(playerid, g_InvBtn[playerid][2]);
	PlayerTextDrawShow(playerid, g_Inv2Bg[playerid]);
	PlayerTextDrawShow(playerid, g_Inv2Title[playerid]);

	Inv_RefreshPanel(playerid);
	Inv_RefreshRightPanel(playerid);
	return 1;
}

stock Inventory_Hide(playerid, bool:resetContext = true)
{
	if (!g_InvOpen[playerid])
		return 0;

	new mode = g_InvRightMode[playerid];
	new sid = g_InvRightId[playerid];

	g_InvOpen[playerid] = false;
	g_InvSelected[playerid] = -1;
	g_InvSelected2[playerid] = -1;
	g_InvDragPanel[playerid] = INV_PANEL_NONE;
	g_InvDragSlot[playerid] = -1;
	Inv_DestroyUI(playerid);
	g_InvIgnoreEsc[playerid] = true;
	CancelSelectTextDraw(playerid);

	if (resetContext)
	{
		if (mode == INV_RIGHT_STORAGE)
			CallLocalFunction("Inv_OnRightStorageChanged", "dd", playerid, sid);
		g_InvRightMode[playerid] = INV_RIGHT_DROPS;
		g_InvRightId[playerid] = -1;
		g_InvRightTitle[playerid][0] = EOS;
	}
	return 1;
}

stock Inv_HandleEscClose(playerid)
{
	if (g_InvIgnoreEsc[playerid])
	{
		g_InvIgnoreEsc[playerid] = false;
		return 1;
	}
	if (g_InvOpen[playerid])
		return Inventory_Hide(playerid);
	return 0;
}

stock Inv_ClickPlayerTD(playerid, PlayerText:playertextid)
{
	if (playertextid == PlayerText:INVALID_TEXT_DRAW || !g_InvOpen[playerid])
		return 0;

	// Left slots - select or move/swap onto this slot
	for (new slot = 0; slot < INV_UI_SLOTS; slot++)
	{
		if (playertextid != g_InvPreview[playerid][slot])
			continue;

		if (g_InvDragPanel[playerid] == INV_PANEL_NONE)
		{
			if (!InventoryData[playerid][slot][invExists])
				return 1;
			Inv_SetDrag(playerid, INV_PANEL_LEFT, slot);
			return 1;
		}

		if (g_InvDragPanel[playerid] == INV_PANEL_LEFT && g_InvDragSlot[playerid] == slot)
		{
			Inv_ClearDrag(playerid);
			SendClientMessage(playerid, 0xAAAAAAFF, "Move cancelled.");
			return 1;
		}

		if (!Inv_MoveBetweenSlots(playerid, g_InvDragPanel[playerid], g_InvDragSlot[playerid], INV_PANEL_LEFT, slot))
			SendClientMessage(playerid, 0xFFFF00AA, "Could not move that item.");
		return 1;
	}

	// Right slots - select or move onto right context slot
	for (new ui = 0; ui < INV_UI_SLOTS; ui++)
	{
		if (playertextid != g_Inv2Preview[playerid][ui])
			continue;

		Inv_RightEnsureReady(playerid);
		new bool:hasItem = Inv_RightSlotExists(playerid, ui);

		if (g_InvDragPanel[playerid] == INV_PANEL_NONE)
		{
			if (!hasItem)
				return 1;
			Inv_SetDrag(playerid, INV_PANEL_RIGHT, ui);
			return 1;
		}

		if (g_InvDragPanel[playerid] == INV_PANEL_RIGHT && g_InvDragSlot[playerid] == ui)
		{
			Inv_ClearDrag(playerid);
			SendClientMessage(playerid, 0xAAAAAAFF, "Move cancelled.");
			return 1;
		}

		if (!Inv_MoveBetweenSlots(playerid, g_InvDragPanel[playerid], g_InvDragSlot[playerid], INV_PANEL_RIGHT, ui))
			SendClientMessage(playerid, 0xFFFF00AA, "Could not move that item.");
		return 1;
	}

	// Use
	if (playertextid == g_InvBtn[playerid][0])
	{
		new slot = g_InvSelected[playerid];
		if (slot < 0 || !InventoryData[playerid][slot][invExists])
			return SendClientMessage(playerid, 0xAAAAAAFF, "Select an inventory item first."), 1;

		new itemName[MAX_INVENTORY_ITEM_NAME];
		format(itemName, sizeof(itemName), "%s", InventoryData[playerid][slot][invItem]);

		// Guns: equip to hand automatically
		if (Inv_IsGunItem(itemName))
		{
			Inventory_EquipGun(playerid, slot);
			return 1;
		}

		CallLocalFunction("Inv_OnItemUse", "ddsii",
			playerid, slot, itemName,
			InventoryData[playerid][slot][invModel],
			InventoryData[playerid][slot][invQuantity]);
		return 1;
	}

	// Drop button (still drops in front / into nearest bag freely)
	if (playertextid == g_InvBtn[playerid][1])
	{
		new slot = g_InvSelected[playerid];
		if (slot < 0 || !InventoryData[playerid][slot][invExists])
			return SendClientMessage(playerid, 0xAAAAAAFF, "Select an inventory item first."), 1;

		// In loot/storage mode, Drop moves into the right panel
		if (g_InvRightMode[playerid] != INV_RIGHT_DROPS)
		{
			Inv_RightEnsureReady(playerid);
			new dest = Inv_RightFindFree(playerid);
			if (dest == -1)
				return SendClientMessage(playerid, 0xFFFF00AA, "Right panel is full."), 1;
			if (!Inv_MoveBetweenSlots(playerid, INV_PANEL_LEFT, slot, INV_PANEL_RIGHT, dest))
				SendClientMessage(playerid, 0xFFFF00AA, "Could not move that item.");
			return 1;
		}

		new qty = InventoryData[playerid][slot][invQuantity];
		// Guns always drop as one whole weapon (ammo stays with it)
		if (Inv_IsGunItem(InventoryData[playerid][slot][invItem]) || qty <= 1)
		{
			Inv_DoDropFromSlot(playerid, slot, qty);
			Inv_ClearDrag(playerid);
			return 1;
		}

		ShowPlayerDialog(playerid, DIALOG_INV_AMOUNT, DIALOG_STYLE_INPUT,
			"Drop amount", "Enter how many to drop in front of you:", "Drop", "Cancel");
		return 1;
	}

	if (playertextid == g_InvBtn[playerid][2])
	{
		Inventory_Hide(playerid);
		return 1;
	}

	return 0;
}

stock Inventory_OnDialogResponse(playerid, dialogid, response, listitem, const inputtext[])
{
	#pragma unused listitem
	if (dialogid != DIALOG_INV_AMOUNT)
		return 0;
	if (!response)
		return 1;

	new slot = g_InvSelected[playerid];
	if (slot < 0 || !InventoryData[playerid][slot][invExists])
		return 1;

	new amount = strval(inputtext);
	if (amount < 1)
		amount = 1;
	if (amount > InventoryData[playerid][slot][invQuantity])
		amount = InventoryData[playerid][slot][invQuantity];

	Inv_DoDropFromSlot(playerid, slot, amount);
	return 1;
}
