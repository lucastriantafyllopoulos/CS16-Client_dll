#include "events.h"
#include "wpn_shared.h"

static const char *SOUNDS_NAME = "weapons/elite_fire.wav";

//isRight: false = left pistol, true = right pistol
void EV_FireElite(event_args_s *args, bool isRight)
{
	vec3_t ShellVelocity, ShellOrigin;
	Vector vecSrc;

	int idx = args->entindex;
	Vector origin(args->origin);
	Vector angles(args->angles);

	Vector velocity(args->velocity);
	Vector forward, right, up;

	int	sequence;
	float flTimeDiff = args->fparam1;
	int iBulletsLeft = args->iparam2;

	AngleVectors(angles, forward, right, up);

	if(EV_IsLocal(idx))
	{
		++g_iShotsFired;
		EV_MuzzleFlash();

		//Select the appropriate firing animation sequence based on remaining ammo and fire rate
		if( !iBulletsLeft ) sequence = ELITE_SHOOTLEFTLAST;
		else if( flTimeDiff >= 0.5 ) sequence = ELITE_SHOOTLEFT5;
		else if( flTimeDiff >= 0.4 ) sequence = ELITE_SHOOTLEFT4;
		else if( flTimeDiff >= 0.3 ) sequence = ELITE_SHOOTLEFT3;
		else if( flTimeDiff >= 0.2 ) sequence = ELITE_SHOOTLEFT2;
		else sequence = ELITE_SHOOTLEFT1;

		if(isRight)
		{
			sequence += (ELITE_SHOOTRIGHT1 - ELITE_SHOOTLEFT1);

			if (!gHUD.cl_righthand->value)
			{
				//Local player (Left hand): Mirrored right pistol appears on the left -> eject left (16.0), attachment 3
				EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 35.0, -11.0, 16.0, 3);
			}
			else
			{
				//Local player (Right hand): Right pistol appears on the right -> eject right (-16.0), attachment 3
				EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 35.0, -11.0, -16.0, 3);
			}
		}
		else
		{
			if (!gHUD.cl_righthand->value)
			{
				//Local player (Left hand): Mirrored left pistol appears on the right -> eject right (-16.0), attachment 2
				EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 35.0, -11.0, -16.0, 2);
			}
			else
			{
				//Local player (Right hand): Left pistol appears on the left -> eject left (16.0), attachment 2
				EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 35.0, -11.0, 16.0, 2);
			}
		}

		gEngfuncs.pEventAPI->EV_WeaponAnimation(sequence, 2);
	}
	else
	{
		//Third-person players (p_ models): Attachments are not defined by default,
		//so we pass index 0 to force the engine to use the hardcoded mathematical offsets.
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20.0, -12.0, 4.0, 0);
	}

	//Construct a full 3D Vector for the shell's initial rotation.
	//This ensures the shell aligns with the weapon's barrel direction across all axes, 
	//while introducing a controlled random variance to simulate natural ejection physics.
	Vector shellAngles;

	//PITCH (Up/Down): Invert the player's pitch to match the shell model's origin orientation, 
	//and add a random variance for visual realism.
	shellAngles[0] = -angles[0] + gEngfuncs.pfnRandomFloat(-60.0f, 60.0f); 

	//YAW (Left/Right): Inherit the player's facing direction with minor random variance
	shellAngles[1] = angles[1] + gEngfuncs.pfnRandomFloat(-30.0f, 30.0f); 

	//ROLL (Tilt): Inherit the player's roll with variance to simulate tumbling
	shellAngles[2] = angles[2]  + gEngfuncs.pfnRandomFloat(-60.0f, 60.0f); 
	
	//Pass the complete Vector to the overloaded EV_EjectBrass function
	EV_EjectBrass(ShellOrigin, ShellVelocity, shellAngles, g_iPShell, TE_BOUNCE_SHELL);

	PLAY_EVENT_SOUND(SOUNDS_NAME);

	EV_GetGunPosition(args, vecSrc, origin);
	
	Vector vSpread;
	vSpread.x = args->fparam2;
	vSpread.y = args->iparam1 / 100.0f;

	//Offset the bullet trace origin slightly to the right or left depending on which pistol fired
	if(isRight)
	{
		vecSrc = vecSrc + right * 5;
	}
	else
	{
		vecSrc = vecSrc - right * 5;
	}

	EV_HLDM_FireBullets(idx, forward, right, up, 1, vecSrc, forward, vSpread, 8192.0, BULLET_PLAYER_9MM, 2);
}

void EV_FireEliteLeft(event_args_s *args)
{
	EV_FireElite(args, false);
}

void EV_FireEliteRight(event_args_s *args)
{
	EV_FireElite(args, true);
}
