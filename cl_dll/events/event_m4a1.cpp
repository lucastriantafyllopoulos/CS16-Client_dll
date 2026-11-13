#include "events.h"
#include "wpn_shared.h"


static const char *SOUNDS_NAME[] =
{
	"weapons/m4a1-1.wav",
	"weapons/m4a1_unsil-1.wav",
	"weapons/m4a1_unsil-2.wav"
};

// bparam1: 1-> silencer on, 0-> silencer off
void EV_FireM4A1( event_args_t *args )
{
	vec3_t ShellVelocity, ShellOrigin;
	Vector vecSrc, vecAiming;

	int sequence, idx = args->entindex;

	Vector origin(args->origin);
	Vector angles(
		args->iparam1 / 100.0f + args->angles[0],
		args->iparam2 / 100.0f + args->angles[1],
		args->angles[2]);
	Vector velocity(args->velocity);
	Vector forward, right, up;

	AngleVectors(angles, forward, right, up);

	if  (EV_IsLocal(idx))
	{
		++g_iShotsFired;
		EV_MuzzleFlash();

		if(args->bparam1)
		{
			//Play a random shoot animation M4A1_SHOOT1, M4A1_SHOOT2, or M4A1_SHOOT3 from enumeration structure m4a1_e in wpn_shared.h when silencer is on.
			sequence = Com_RandomLong(M4A1_SHOOT1, M4A1_SHOOT3);
		}
		else
		{
			//Play a random shoot animation M4A1_UNSIL_SHOOT1, M4A1_UNSIL_SHOOT2, or M4A1_UNSIL_SHOOT3 from enumeration structure m4a1_e in wpn_shared.h when silencer is off.
			sequence = Com_RandomLong(M4A1_UNSIL_SHOOT1, M4A1_UNSIL_SHOOT3);
		}

		//Play weapon animations from bparam1.
		gEngfuncs.pEventAPI->EV_WeaponAnimation(sequence, 2);
		if(!gHUD.cl_righthand->value)
		{
			//Local player (Left hand): We pass '1' as the last argument to EV_GetDefaultShellInfo.
			//Note: The engine's original function signature used a boolean (bReverseDirection) here. 
			//This was modified to accept an integer (attachmentIndex) in ev_common.cpp to dynamically fetch the 
			//precise ejection coordinates directly from the v_ (view) model's attachments.
			EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20.0, -8.0, -10.0, 1);
		}
		else
		{
			//Local player (Right hand): Use attachment index 1
			EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20.0, -8.0, 10.0, 1);
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
	EV_EjectBrass(ShellOrigin, ShellVelocity, shellAngles, g_iRShell, TE_BOUNCE_SHELL);

	//If player has silencer on/off, play the corresponding sound
	PLAY_EVENT_SOUND(args->bparam1 ? SOUNDS_NAME[0] : SOUNDS_NAME[Com_RandomLong(1, 2)]);

	EV_GetGunPosition(args, vecSrc, origin);
	VectorCopy(forward, vecAiming);

	Vector vSpread(args->fparam1, args->fparam2, 0.0f);
	EV_HLDM_FireBullets(idx, forward, right, up, 1, vecSrc, vecAiming, vSpread, 8192.0, BULLET_PLAYER_556MM, 2);
}
