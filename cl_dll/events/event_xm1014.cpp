#include "events.h"
#include "wpn_shared.h"

static const char *SOUNDS_NAME = "weapons/xm1014-1.wav";

void EV_FireXM1014(event_args_s *args)
{
	vec3_t ShellVelocity, ShellOrigin;
	Vector vecSrc;

	int idx = args->entindex;
	Vector origin(args->origin);
	Vector angles(
		//Calculate the exact firing angles including recoil (punchangle).
		//To optimize network bandwidth, the server transmits the recoil pitch and yaw 
		//as integers (multiplied by 100). We divide by 100.0f here to decode them 
		//back into floats before adding them to the player's current view angles.
		args->iparam1 / 100.0f + args->angles[0],
		args->iparam2 / 100.0f + args->angles[1],
		args->angles[2]);
	Vector velocity(args->velocity);
	Vector forward, right, up;

	AngleVectors(angles, forward, right, up);

	if(EV_IsLocal(idx))
	{
		++g_iShotsFired;
		EV_MuzzleFlash();

		//Play a random shoot animation XM1014_FIRE1 or XM1014_FIRE2 from enumeration structure xm1014_e in wpn_shared.h
		gEngfuncs.pEventAPI->EV_WeaponAnimation(Com_RandomLong(XM1014_FIRE1, XM1014_FIRE2), 2);

		if(!gHUD.cl_righthand->value)
		{
			//Local player (Left hand): We pass '1' as the last argument to EV_GetDefaultShellInfo.
			//Note: The engine's original function signature used a boolean (bReverseDirection) here. 
			//This was modified to accept an integer (attachmentIndex) in ev_common.cpp to dynamically fetch the 
			//precise ejection coordinates directly from the v_ (view) model's attachments.
			EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 22.0, -9.0, -11.0, 1);
		}
		else
		{
			//Local player (Right hand): Use attachment index 1
			EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 22.0, -9.0, 11.0, 1);
		}
	}
	else
	{	
		//Third-person players (p_ models): Attachments are not defined by default, 
		//so we pass index 0 (or -1) to force the engine to use the hardcoded mathematical offsets
		EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20.0, -12.0, 4.0, 0);
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
	EV_EjectBrass(ShellOrigin, ShellVelocity, shellAngles, g_iShotgunShell, TE_BOUNCE_SHOTSHELL);

	PLAY_EVENT_SOUND(SOUNDS_NAME);

	EV_GetGunPosition(args, vecSrc, origin);
	
	//VSpread vector was removed and XM1014_CONE_VECTOR from wpn_shared.h took its place.
	//Fifth parameter changed value from "1" to "6", which means they will, maximum, 6 holes on the wall from buckshot
	EV_HLDM_FireBullets(idx, forward, right, up, 6, vecSrc, forward, XM1014_CONE_VECTOR, 8192.0, BULLET_PLAYER_BUCKSHOT, 2);
}
