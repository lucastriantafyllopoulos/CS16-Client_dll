#include "hud.h"
#include "parsemsg.h"
#include "cl_util.h"
#include "r_efx.h"
#include "event_api.h"
#include "com_model.h"
#include "draw_util.h"
#include <string.h>
#include <math.h> 

DECLARE_MESSAGE(m_Scenario, Scenario)

static char g_szScenarioSprite[32] = {0}; 

int CHudScenario::Init( )
{
	HOOK_MESSAGE( Scenario );
	m_iFlags = HUD_ACTIVE; 
	gHUD.AddHudElem( this );

	return 1;
}

int CHudScenario::VidInit( )
{
	return 1;
}

void CHudScenario::Reset()
{
	m_sprite.spr = 0;
	m_iFlags &= ~HUD_DRAW; 
}

int CHudScenario::MsgFunc_Scenario(const char *pszName, int iSize, void *buf)
{
	BufferReader reader( pszName, buf, iSize );

	bool active = reader.ReadByte();
	if( !active )
	{
		m_iFlags &= ~HUD_DRAW;
		return 1;
	}

	m_iFlags |= HUD_DRAW; 

	const char *spriteName = reader.ReadString();
	
	// Αποθηκεύουμε το όνομα του sprite ("bombticking" ή "hostage1" κλπ)
	strncpy(g_szScenarioSprite, spriteName, sizeof(g_szScenarioSprite)-1);
	
	// Φορτώνουμε ακριβώς το sprite που ζήτησε ο server
	m_sprite.SetSpriteByName(spriteName);

	m_iFlashAlpha = reader.ReadByte();
	m_fFlashRate = reader.ReadShort() * 0.01f;
	float fOffset = reader.ReadShort() * 0.01f; 

	// Αποθηκεύουμε την τρέχουσα στιγμή για το fade out
	m_fNextFlash = gHUD.m_flTime;

	return 1;
}

int CHudScenario::Draw(float flTime)
{
	if( g_iUser1 || gEngfuncs.IsSpectateOnly() || !m_sprite.spr )
		return 1;

	int r = 255, g = 255, b = 255; 
	int x = (ScreenWidth / 2) + 90; 
	
	// Δηλώνουμε το y εδώ, αλλά του δίνουμε τιμή ξεχωριστά παρακάτω!
	int y; 

	if (!strcmp(g_szScenarioSprite, "bombticking"))
	{
		// Θέση για το C4 (Βόμβα)
		y = ScreenHeight - 38;

		// ΛΟΓΙΚΗ ΓΙΑ THN ΤΟΠΟΘΕΤΗΜΕΝΗ ΒΟΜΒΑ (C4) - Μόνο Fade-out, χωρίς αλλαγή sprite
		if (m_fFlashRate > 0.0f)
		{
			float timePassed = flTime - m_fNextFlash;
			float timeSinceBeep = (float)fmod(timePassed, m_fFlashRate);
			float cycle = timeSinceBeep / m_fFlashRate; 
			
			// Το Alpha ξεκινάει από το 255 (φωτεινό) και σβήνει σταδιακά μέχρι το 55 (αχνό)
			m_iAlpha = 255 - (int)(cycle * 200.0f);
			if (m_iAlpha < MIN_ALPHA) m_iAlpha = MIN_ALPHA;
		}
		else
		{
			m_iAlpha = 255;
		}
	}
	else
	{
		// Θέση για τους Ομήρους (Hostages)
		y = ScreenHeight - 35;

		// ΛΟΓΙΚΗ ΓΙΑ ΤΟΥΣ ΟΜΗΡΟΥΣ (Σταθερό λευκό)
		m_iAlpha = 255;
	}

	DrawUtils::ScaleColors( r, g, b, m_iAlpha );
	SPR_Set( m_sprite.spr, r, g, b );
	SPR_DrawAdditive( 0, x, y, &m_sprite.rect );
	
	return 1;
}