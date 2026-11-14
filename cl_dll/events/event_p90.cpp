#include "events.h"
#include "wpn_shared.h"


static const char *SOUNDS_NAME = "weapons/p90-1.wav";

void EV_FireP90(event_args_s *args)
{
	vec3_t ShellVelocity, ShellOrigin;
	Vector vecSrc, vecAiming;

	int idx = args->entindex;
	Vector origin(args->origin);
	Vector angles(
		args->iparam1 / 100.0f + args->angles[0],
		args->iparam2 / 100.0f + args->angles[1],
		args->angles[2]);
	Vector velocity(args->velocity);
	Vector forward, right, up;

	AngleVectors(angles, forward, right, up);

	if ( EV_IsLocal( args->entindex ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();

		//Play a random shoot animation P90_SHOOT1, P90_SHOOT2 or P90_SHOOT3 from enumeration structure p90_e in wpn_shared.h
		gEngfuncs.pEventAPI->EV_WeaponAnimation(Com_RandomLong(P90_SHOOT1, P90_SHOOT3), 2);
	
		if( !gHUD.cl_righthand->value )
		{
			//Local player (Left hand): We pass '1' as the last argument to EV_GetDefaultShellInfo.
			//Note: The engine's original function signature used a boolean (bReverseDirection) here. 
			//This was modified to accept an integer (attachmentIndex) in ev_common.cpp to dynamically fetch the 
			//precise ejection coordinates directly from the v_ (view) model's attachments.
			//Setting -right for left-hand, the empty shells will eject in the left side
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, -right, up, 35.0, -16.0, -22.0, 1);
		}
		else
		{
			//Local player (Right hand): Use attachment index 1 and ejection to the right side
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 35.0, -16.0, -22.0, 1);
		}
	}
	else
	{
		//Third-person players (p_ models): Attachments are not defined by default, 
		//so we pass index 0 (or -1) to force the engine to use the hardcoded mathematical offsets
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
	EV_EjectBrass(ShellOrigin, ShellVelocity, shellAngles, g_iRShell, TE_BOUNCE_SHELL);

	PLAY_EVENT_SOUND(SOUNDS_NAME);

	EV_GetGunPosition(args, vecSrc, origin);
	VectorCopy(forward, vecAiming);

	Vector vSpread(args->fparam1, args->fparam2, 0.0f);
	EV_HLDM_FireBullets(idx, forward, right, up, 1, vecSrc, vecAiming, vSpread, 8192.0, BULLET_PLAYER_57MM, 2);
}
