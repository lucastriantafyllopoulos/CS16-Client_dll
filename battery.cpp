/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// battery.cpp
//
// implementation of CHudBattery class
//
#include "hud.h"
#include "parsemsg.h"
#include "cl_util.h"
#include "draw_util.h"

DECLARE_MESSAGE(m_Battery, Battery);
DECLARE_MESSAGE(m_Battery, ArmorType);

int CHudBattery::Init(void)
{
	m_iBat = 0;
	m_fFade = 0;
	m_iFlags = 0;
	m_enArmorType = Vest;

	HOOK_MESSAGE(Battery);
	HOOK_MESSAGE(ArmorType);
	gHUD.AddHudElem(this);

	return 1;
}

int CHudBattery::VidInit(void)
{
	m_hEmpty[Vest].SetSpriteByName("suit_empty");
	m_hFull[Vest].SetSpriteByName("suit_full");
	m_hEmpty[VestHelm].SetSpriteByName("suithelmet_empty");
	m_hFull[VestHelm].SetSpriteByName("suithelmet_full");

	m_iHeight = m_hFull[Vest].rect.Height();
	m_fFade = 0;

	return 1;
}


int CHudBattery::MsgFunc_Battery(const char* pszName, int iSize, void* pbuf)
{
	BufferReader reader(pszName, pbuf, iSize);

	m_iFlags |= HUD_DRAW;
	int x = reader.ReadLong(); //Changing from ReadByte to ReadLong, for showing great armor value

	if (x != m_iBat)
	{
		m_fFade = FADE_TIME;
		m_iBat = x;
		if (m_iBat < 0)
			m_enArmorType = Vest;
	}

	return 1;
}

int CHudBattery::Draw(float flTime)
{
	if (g_iUser1)
		return 1;

	if (gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH)
		return 1;

	if (!(gHUD.m_iWeaponBits & (1 << (WEAPON_SUIT))))
		return 1;

	int r, g, b, x, y;
	wrect_t rc;

	rc = m_hEmpty[m_enArmorType].rect;

	// battery can go from 0 to 100 so * 0.01 goes from 0 to 1
	rc.top += m_iHeight * ((float)(100 - (min(100, m_iBat))) * 0.01f);

	// Lock color to solid white with no transparency (alpha transparency)
	r = 255; 
	g = 255; 
	b = 255;

	y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
	
	// Find the pixel width of a single digit sprite
	int digitWidth = gHUD.GetSpriteRect(gHUD.m_HUD_number_0).Width();
	
	//Calculate the starting X position for the Armor icon so it never overlaps with Health:
	// ~40 pixels reserved for the Health cross icon
	// + Space reserved for up to 6 Health digits (digitWidth * 6)
	// + 20 pixels static gap between the Health and Armor blocks
	x = 40 + (digitWidth * 6) + 20; 

	// Make sure we have the right sprite handles
	SPR_Set(m_hFull[m_enArmorType].spr, r, g, b);
	SPR_DrawAdditive(0, x, y, &m_hFull[m_enArmorType].rect);

	if (rc.bottom > rc.top)
	{
		SPR_Set(m_hEmpty[m_enArmorType].spr, r, g, b);
		SPR_DrawAdditive(0, x, y + (rc.top - m_hEmpty[m_enArmorType].rect.top), &rc);
	}

	//Move X forward by the width of the armor icon
	x += (m_hEmpty[m_enArmorType].rect.Width());

	//Add a small 5-pixel gap between the armor icon and its numbers
	x += 5;
	
	//Draw the armor value compactly aligned to the left
	DrawUtils::DrawHudNumberLeft(x, y, m_iBat, r, g, b);

	return 1;
}

int CHudBattery::MsgFunc_ArmorType(const char* pszName, int iSize, void* pbuf)
{
	BufferReader reader(pszName, pbuf, iSize);

	m_enArmorType = (armortype_t)reader.ReadByte();

	return 1;
}
