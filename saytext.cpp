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
// saytext.cpp
//
// implementation of CHudSayText class
//
#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "draw_util.h"
#include "com_weapons.h"
//#include "vgui_TeamFortressViewport.h"
#include "cs_localize.h"

extern float* GetClientColor(int clientIndex);

#define MAX_LINES	5
#define MAX_CHARS_PER_LINE	1024  /* it can be less than this, depending on char size */

// allow 20 pixels on either side of the text
#define MAX_LINE_WIDTH  ( ScreenWidth - 40 )
#define LINE_START  10

static char g_szLineBuffer[MAX_LINES + 1][MAX_CHARS_PER_LINE];
static float* g_pflNameColors[MAX_LINES + 1];
static int g_iNameLengths[MAX_LINES + 1];
static float flScrollTime = 0;  // the time at which the lines next scroll up

static int Y_START = 0;
static int line_height = 0;

DECLARE_MESSAGE(m_SayText, SayText);

int CHudSayText::Init(void)
{
	gHUD.AddHudElem(this);

	HOOK_MESSAGE(SayText);

	InitHUDData();

	m_HUD_saytext = gEngfuncs.pfnRegisterVariable("hud_saytext", "1", 0);
	m_HUD_saytext_time = gEngfuncs.pfnRegisterVariable("hud_saytext_time", "5", 0);

	m_iFlags |= HUD_INTERMISSION; // is always drawn during an intermission

	return 1;
}


void CHudSayText::InitHUDData(void)
{
	memset(g_szLineBuffer, 0, sizeof g_szLineBuffer);
	memset(g_pflNameColors, 0, sizeof g_pflNameColors);
	memset(g_iNameLengths, 0, sizeof g_iNameLengths);
}

int CHudSayText::VidInit(void)
{
	return 1;
}


int ScrollTextUp(void)
{
	g_szLineBuffer[MAX_LINES][0] = 0;
	memmove(g_szLineBuffer[0], g_szLineBuffer[1], sizeof(g_szLineBuffer) - sizeof(g_szLineBuffer[0])); // overwrite the first line // -V512
	memmove(&g_pflNameColors[0], &g_pflNameColors[1], sizeof(g_pflNameColors) - sizeof(g_pflNameColors[0]));
	memmove(&g_iNameLengths[0], &g_iNameLengths[1], sizeof(g_iNameLengths) - sizeof(g_iNameLengths[0]));
	g_szLineBuffer[MAX_LINES - 1][0] = 0;

	if (g_szLineBuffer[0][0] == ' ') // also scroll up following lines
	{
		g_szLineBuffer[0][0] = 2;
		return 1 + ScrollTextUp();
	}

	return 1;
}

int CHudSayText::Draw(float flTime)
{
	int y = Y_START;

	//if ( ( gViewPort && gViewPort->AllowedToPrintText() == FALSE) || !m_HUD_saytext->value )
		//return 1;

	// make sure the scrolltime is within reasonable bounds,  to guard against the clock being reset
	flScrollTime = min(flScrollTime, flTime + m_HUD_saytext_time->value);

	// make sure the scrolltime is within reasonable bounds,  to guard against the clock being reset
	flScrollTime = min(flScrollTime, flTime + m_HUD_saytext_time->value);

	if (flScrollTime <= flTime)
	{
		if (*g_szLineBuffer[0])
		{
			flScrollTime = flTime + m_HUD_saytext_time->value;
			// push the console up
			ScrollTextUp();
		}
		else
		{ // buffer is empty,  just disable drawing of this section
			m_iFlags &= ~HUD_DRAW;
		}
	}

	for (int i = 0; i < MAX_LINES; i++)
	{
		if (!g_szLineBuffer[i][0]) // skip empty string
			continue;

		int current_x = LINE_START;
		const char* text = g_szLineBuffer[i];
		size_t length = strlen(text);

		// default color if not set
		DrawUtils::SetConsoleTextColor(g_ColorYellow[0], g_ColorYellow[1], g_ColorYellow[2]);

		// buffer for accumulating characters of the same color
		char color_buffer[256] = { 0 };
		size_t buffer_pos = 0;

for (size_t c = 0; c < length; c++)
		{
			// Επαναφορά στο κίτρινο ακριβώς στο σημείο που τελειώνει το όνομα (απαραίτητο για το \x02)
			if (c == g_iNameLengths[i] && g_pflNameColors[i])
			{
				if (buffer_pos > 0)
				{
					color_buffer[buffer_pos] = '\x00';
					current_x = DrawUtils::DrawConsoleString(current_x, y, color_buffer);
					buffer_pos = 0;
				}
				DrawUtils::SetConsoleTextColor(g_ColorYellow[0], g_ColorYellow[1], g_ColorYellow[2]);
			}

			// Αναγνώριση όλων των κωδικών χρώματος (Προστέθηκε το \x02)
			if (text[c] == '\x01' || text[c] == '\x02' || text[c] == '\x03' || text[c] == '\x04')
			{
				if (buffer_pos > 0)
				{
					color_buffer[buffer_pos] = '\x00';
					current_x = DrawUtils::DrawConsoleString(current_x, y, color_buffer);
					buffer_pos = 0;
				}

				switch (text[c])
				{
				case '\x01': // Κίτρινο (Κανονικό)
					DrawUtils::SetConsoleTextColor(g_ColorYellow[0], g_ColorYellow[1], g_ColorYellow[2]);
					break;
				case '\x02': // STX - Χρώμα ομάδας μέχρι να τελειώσει το όνομα
				case '\x03': // ETX - Χρώμα ομάδας μόνιμα
					if (g_pflNameColors[i])
					{
						DrawUtils::SetConsoleTextColor(g_pflNameColors[i][0], g_pflNameColors[i][1], g_pflNameColors[i][2]);
					}
					break;
				case '\x04': // Πράσινο (Τοποθεσία)
					DrawUtils::SetConsoleTextColor(g_ColorGreen[0], g_ColorGreen[1], g_ColorGreen[2]);
					break;
				}
				continue;
			}

			// Προσθήκη χαρακτήρα στον buffer
			if (buffer_pos < sizeof(color_buffer) - 1)
			{
				color_buffer[buffer_pos++] = text[c];
			}
		}

		// draw the remaining characters
		if (buffer_pos > 0)
		{
			color_buffer[buffer_pos] = '\x00';
			DrawUtils::DrawConsoleString(current_x, y, color_buffer);
		}

		y += line_height;
	}

	return 1;
}

struct
{
	const char key[32];
	bool allowDead;
	bool replaceFirstArgToName;
} sayTextFmt[] =
{
	{ "#Cstrike_Chat_CT", true, true },
	{ "#Cstrike_Chat_T", true, true },
	{ "#Cstrike_Chat_CT_Dead", false, true },
	{ "#Cstrike_Chat_T_Dead", false, true },
	{ "#Cstrike_Chat_Spec", false, true },
	{ "#Cstrike_Chat_All", true, true },
	{ "#Cstrike_Chat_AllDead", false, true },
	{ "#Cstrike_Chat_AllSpec", false, true },
	{ "#Cstrike_Name_Change", true, false },
	{ "#Cstrike_Chat_T_Loc", true, true },
	{ "#Cstrike_Chat_CT_Loc", true, true },
	{ "#Spec_PlayerItem", true, false },
};

int CHudSayText::MsgFunc_SayText(const char* pszName, int iSize, void* pbuf)
{
	BufferReader reader(pszName, pbuf, iSize);
	int client_index = reader.ReadByte();

	char fmt[256];
	strncpy(fmt, reader.ReadString(), sizeof(fmt) - 1);
	fmt[sizeof(fmt) - 1] = '\0';

	const char* argv[4] = { nullptr, nullptr, nullptr, nullptr };
	char argsBuffer[4][256];
	int argc = 0;

	// Διαβάζουμε τα ορίσματα (όνομα, μήνυμα, τοποθεσία)
	for (argc = 0; argc < 4; argc++)
	{
		const char* arg = reader.ReadString();
		if (!arg[0] && !reader.Valid())
			break;
		strncpy(argsBuffer[argc], arg, sizeof(argsBuffer[argc]) - 1);
		argsBuffer[argc][sizeof(argsBuffer[argc]) - 1] = '\0';
		argv[argc] = argsBuffer[argc];
	}

	bool allowDead = true;
	bool replaceFirstArgToName = false;

	// Ελέγχουμε μόνο τους κανόνες (αν επιτρέπεται νεκρός κλπ.)
	if (fmt[0] == '#')
	{
		for (int i = 0; i < sizeof(sayTextFmt) / sizeof(sayTextFmt[0]); i++)
		{
			if (!strcmp(fmt, sayTextFmt[i].key))
			{
				allowDead = sayTextFmt[i].allowDead;
				replaceFirstArgToName = sayTextFmt[i].replaceFirstArgToName;
				break;
			}
		}
	}

	if (!allowDead && !CL_IsDead() && !g_iUser1)
	{
		return 1;
	}

	// [ΔΙΟΡΘΩΣΗ]
	// Αν ο server (mp.dll) έστειλε κενό το argv[0], ΠΡΕΠΕΙ να το γεμίσουμε με το όνομα του παίκτη!
	// Αυτό καλύπτει και τις περιπτώσεις που το μήνυμα δεν ξεκινάει με "#".
	if (client_index > 0 && (replaceFirstArgToName || !argv[0] || argv[0][0] == '\0'))
	{
		GetPlayerInfo(client_index, &g_PlayerInfoList[client_index]);
		argv[0] = g_PlayerInfoList[client_index].name;
	}

char dst[1024];
	
	const char* localizedFormat = fmt;
	if (fmt[0] == '#')
	{
		localizedFormat = CS16_Localize(fmt);
	}
	else
	{
		// Προσπέρασε τυχόν κενά ή μη εκτυπώσιμους χαρακτήρες στην αρχή
		// Αυτό λύνει το πρόβλημα με τα κενά στο cstrike_english.txt
		while (*localizedFormat && (*localizedFormat == ' ' || (unsigned char)*localizedFormat < 0x20) && *localizedFormat != '\x01' && *localizedFormat != '\x02' && *localizedFormat != '\x03' && *localizedFormat != '\x04')
		{
			localizedFormat++;
		}
	}

	CS16_LocalizeFormat(dst, sizeof(dst), localizedFormat, argv, argc);

	SayTextPrint(dst, strlen(dst), client_index);

	return 1;
}

void CHudSayText::SayTextPrint(const char* pszBuf, int iBufSize, int clientIndex)
{
	// find an empty string slot
	int i;
	for (i = 0; i < MAX_LINES; i++)
	{
		if (!*g_szLineBuffer[i])
			break;
	}
	if (i == MAX_LINES)
	{
		// force scroll buffer up
		ScrollTextUp();
		i = MAX_LINES - 1;
	}

	g_iNameLengths[i] = 0;
	g_pflNameColors[i] = NULL;

#if 1
	// if it's a say message, search for the players name in the string
	if (clientIndex > 0)
	{
		GetPlayerInfo(clientIndex, &g_PlayerInfoList[clientIndex]);
		const char* pName = g_PlayerInfoList[clientIndex].name;
		g_pflNameColors[i] = GetClientColor(clientIndex);

		if (pName)
		{
			const char* nameInString = strstr(pszBuf, pName);

			if (nameInString)
			{
				g_iNameLengths[i] = strlen(pName) + (nameInString - pszBuf);
			}
		}
	}
#endif


	strncpy(g_szLineBuffer[i], pszBuf, max(iBufSize - 1, MAX_CHARS_PER_LINE - 1));

	// make sure the text fits in one line
	EnsureTextFitsInOneLineAndWrapIfHaveTo(i);

	// Set scroll time
	if (i == 0)
	{
		flScrollTime = gHUD.m_flTime + m_HUD_saytext_time->value;
	}

	m_iFlags |= HUD_DRAW;
	//PlaySound("misc/talk.wav", 1);

	if (!g_iUser1)
	{
		Y_START = ScreenHeight - 60;
	}
	else
	{
		Y_START = ScreenHeight * 4 / 5;
	}
	Y_START -= (line_height * (MAX_LINES + 1));

}

void CHudSayText::EnsureTextFitsInOneLineAndWrapIfHaveTo(int line)
{
	int line_width = 0;
	DrawUtils::ConsoleStringSize(g_szLineBuffer[line], &line_width, &line_height);

	if ((line_width + LINE_START) > MAX_LINE_WIDTH)
	{ // string is too long to fit on line
		// scan the string until we find what word is too long,  and wrap the end of the sentence after the word
		int length = LINE_START;
		int tmp_len = 0;
		char* last_break = NULL;
		for (char* x = g_szLineBuffer[line]; *x != 0; x++)
		{
			// check for a color change, if so skip past it
			if (x[0] == '/' && x[1] == '(')
			{
				x += 2;
				// skip forward until past mode specifier
				while (*x != 0 && *x != ')')
					x++;

				if (*x != 0)
					x++;

				if (*x == 0)
					break;
			}

			char buf[2];
			buf[1] = 0;

			if (*x == ' ' && x != g_szLineBuffer[line])  // store each line break,  except for the very first character
				last_break = x;

			buf[0] = *x;  // get the length of the current character
			DrawUtils::ConsoleStringSize(buf, &tmp_len, &line_height);
			length += tmp_len;

			if (length > MAX_LINE_WIDTH)
			{  // needs to be broken up
				if (!last_break)
					last_break = x - 1;

				x = last_break;

				// find an empty string slot
				int j;
				do
				{
					for (j = 0; j < MAX_LINES; j++)
					{
						if (!*g_szLineBuffer[j])
							break;
					}
					if (j == MAX_LINES)
					{
						// need to make more room to display text, scroll stuff up then fix the pointers
						int linesmoved = ScrollTextUp();
						line -= linesmoved;
						last_break = last_break - (sizeof(g_szLineBuffer[0]) * linesmoved);
					}
				} while (j == MAX_LINES);

				// copy remaining string into next buffer,  making sure it starts with a space character
				if ((char)*last_break == (char)' ')
				{
					int linelen = strlen(g_szLineBuffer[j]);
					int remaininglen = strlen(last_break);

					if ((linelen - remaininglen) <= MAX_CHARS_PER_LINE)
						strcat(g_szLineBuffer[j], last_break);
				}
				else
				{
					if ((strlen(g_szLineBuffer[j]) - strlen(last_break) - 2) < MAX_CHARS_PER_LINE)
					{
						strcat(g_szLineBuffer[j], " ");
						strcat(g_szLineBuffer[j], last_break);
					}
				}

				*last_break = 0; // cut off the last string

				EnsureTextFitsInOneLineAndWrapIfHaveTo(j);
				break;
			}
		}
	}
}
