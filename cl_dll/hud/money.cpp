/*
money.cpp -- Money HUD Widget
Copyright (C) 2015-2016 a1batross

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

In addition, as a special exception, the author gives permission to
link the code of this program with the Half-Life Game Engine ("HL
Engine") and Modified Game Libraries ("MODs") developed by Valve,
L.L.C ("Valve").  You must obey the GNU General Public License in all
respects for all of the code used other than the HL Engine and MODs
from Valve.  If you modify this file, you may extend this exception
to your version of the file, but you are not obligated to do so.  If
you do not wish to do so, delete this exception statement from your
version.
*/


#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>
#include "draw_util.h"

DECLARE_MESSAGE(m_Money, Money)
DECLARE_MESSAGE(m_Money, BlinkAcct)

int CHudMoney::Init( )
{
	HOOK_MESSAGE( Money );
	HOOK_MESSAGE( BlinkAcct );
	gHUD.AddHudElem(this);
	m_fFade = 0;
	m_iFlags = 0;
	return 1;
}

int CHudMoney::VidInit()
{
	m_hDollar.SetSpriteByName("dollar");
	m_hMinus.SetSpriteByName("minus");
	m_hPlus.SetSpriteByName("plus");

	return 1;
}

int CHudMoney::Draw(float flTime)
{
	if(( gHUD.m_iHideHUDDisplay & ( HIDEHUD_HEALTH ) ))
		return 1;

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT))))
		return 1;

	m_fFade -= gHUD.m_flTimeDelta;
	if( m_fFade < 0)
	{
		m_fFade = 0.0f;
		m_iDelta = 0;
	}

	int iDollarWidth = m_hDollar.rect.Width();
	int x = ScreenWidth - iDollarWidth * 7;
	int y = MONEY_YPOS;

	// --- ΥΠΟΛΟΓΙΣΜΟΣ OFFSET ΓΙΑ ΚΥΡΙΩΣ ΠΟΣΟ (Δεξιά στοίχιση 5 ψηφίων) ---
	int moneyVal = m_iMoneyCount;
	int moneyDigits = 1;
	if (moneyVal >= 100000) moneyDigits = 6;
	else if (moneyVal >= 10000) moneyDigits = 5;
	else if (moneyVal >= 1000) moneyDigits = 4;
	else if (moneyVal >= 100) moneyDigits = 3;
	else if (moneyVal >= 10) moneyDigits = 2;

	int moneyOffset = (5 - moneyDigits) * gHUD.m_iFontWidth;
	if (moneyOffset < 0) moneyOffset = 0;

	// --- 1. ΖΩΓΡΑΦΙΚΗ ΠΡΟΣΘΑΦΑΙΡΕΣΗΣ (+/-) ΜΕ FADE OUT ---
	if( m_iDelta != 0 )
	{
		int iDeltaR, iDeltaG, iDeltaB;
		int iDollarHeight = m_hDollar.rect.Height();
		
		float interpolate = ( 5.0f - m_fFade ) / 5.0f;
		int iDeltaAlpha = 255 - (int)(interpolate * 255.0f);
		if (iDeltaAlpha < 0) iDeltaAlpha = 0;

		if (m_iDelta > 0) {
			iDeltaR = 0; iDeltaG = 255; iDeltaB = 0;
		} else {
			iDeltaR = 255; iDeltaG = 0; iDeltaB = 0;
		}

		DrawUtils::ScaleColors(iDeltaR, iDeltaG, iDeltaB, iDeltaAlpha);

		// Το +/- τοποθετείται πάντα σταθερά πάνω από το $
		if( m_iDelta > 0 )
		{
			SPR_Set(m_hPlus.spr, iDeltaR, iDeltaG, iDeltaB );
			SPR_DrawAdditive(0, x, y - iDollarHeight * 1.5, &m_hPlus.rect );
		}
		else
		{
			SPR_Set(m_hMinus.spr, iDeltaR, iDeltaG, iDeltaB );
			SPR_DrawAdditive(0, x, y - iDollarHeight * 1.5, &m_hMinus.rect );
		}

		// Υπολογισμός ψηφίων και Offset για το ποσό που αυξομειώθηκε (+/-)
		int deltaVal = m_iDelta < 0 ? -m_iDelta : m_iDelta;
		int deltaDigits = 1;
		if (deltaVal >= 100000) deltaDigits = 6;
		else if (deltaVal >= 10000) deltaDigits = 5;
		else if (deltaVal >= 1000) deltaDigits = 4;
		else if (deltaVal >= 100) deltaDigits = 3;
		else if (deltaVal >= 10) deltaDigits = 2;

		int deltaOffset = (5 - deltaDigits) * gHUD.m_iFontWidth;
		if (deltaOffset < 0) deltaOffset = 0;

		// Προσθέτουμε το deltaOffset ώστε να στοιχιστεί απόλυτα στα δεξιά
		DrawUtils::DrawHudNumber2( x + iDollarWidth + deltaOffset, y - iDollarHeight * 1.5 , false, 5,
								   deltaVal,
								   iDeltaR, iDeltaG, iDeltaB);
	}

	// --- 2. ΖΩΓΡΑΦΙΚΗ ΚΥΡΙΟΥ ΠΟΣΟΥ (ΧΡΩΜΑΤΙΚΟ FADE ΠΡΟΣ ΛΕΥΚΟ) ---
	int r = 255;
	int g = 255;
	int b = 255;

	if (m_iDelta != 0 && m_fFade > 0.0f)
	{
		float interpolate = (5.0f - m_fFade) / 5.0f;
		int blend = (int)(interpolate * 255.0f);
		
		if (blend < 0) blend = 0;
		if (blend > 255) blend = 255;

		if (m_iDelta > 0) 
		{
			r = blend;
			g = 255;
			b = blend;
		}
		else 
		{
			r = 255;
			g = blend;
			b = blend;
		}
	}

	SPR_Set(m_hDollar.spr, r, g, b);
	SPR_DrawAdditive(0, x, y, &m_hDollar.rect);

	// Προσθέτουμε το moneyOffset για να είναι κι αυτό στοιχισμένο δεξιά
	DrawUtils::DrawHudNumber2( x + iDollarWidth + moneyOffset, y, false, 5, m_iMoneyCount, r, g, b );
	
	FillRGBA(x + iDollarWidth / 4, y + gHUD.m_iFontHeight / 4, 2, 2, r, g, b, 255 );

	return 1;
}

int CHudMoney::MsgFunc_Money(const char *pszName, int iSize, void *pbuf)
{
	BufferReader buf( pszName, pbuf, iSize );
	int iOldCount = m_iMoneyCount;
	m_iMoneyCount = buf.ReadLong();
	gEngfuncs.Cvar_SetValue( gHUD.cscl_currentmoney->name, m_iMoneyCount );
	m_iDelta = m_iMoneyCount - iOldCount;
	m_fFade = 5.0f; //fade for 5 seconds
	m_iFlags |= HUD_DRAW;
	return 1;
}

int CHudMoney::MsgFunc_BlinkAcct(const char *pszName, int iSize, void *pbuf)
{
	BufferReader buf( pszName, pbuf, iSize );

	m_iBlinkAmt = buf.ReadByte();
	m_fBlinkTime = 0;
	return 1;
}
