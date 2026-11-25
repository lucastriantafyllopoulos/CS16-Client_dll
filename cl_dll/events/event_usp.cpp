#include "events.h"
#include "wpn_shared.h"

static const char *SOUNDS_NAME[] =
{
	"weapons/usp1.wav",
	"weapons/usp2.wav",
	"weapons/usp_unsil-1.wav",
};

void EV_FireUSP(event_args_t *args)
{
	vec3_t ShellVelocity, ShellOrigin;
	Vector vecSrc;

	bool silencer_on = !args->bparam2;
	bool empty = args->bparam1;

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
		int seq;

		if(g_bHoldingShield)
		{
			if(!args->bparam1)
			{
				//If player is holding the shield, play a random shoot animation between USP_SHIELD_SHOOT1 and USP_SHIELD_SHOOT2 from enumeration structure usp_shield_e in wpn_shared.h
				seq = Com_RandomLong(USP_SHIELD_SHOOT1, USP_SHIELD_SHOOT2);
			}
			else 
			{
				//If player is holding the shield and usp is dry, play the animation USP_SHIELD_SHOOT_EMPTY from enumeration structure usp_shield_e in wpn_shared.h
				seq = USP_SHIELD_SHOOT_EMPTY;
			}
		}
		else if(silencer_on)
		{
			if(!args->bparam1)
			{
				//If player has the silencer detached to usp, play a random shoot animation between USP_UNSIL_SHOOT1, USP_UNSIL_SHOOT2 or USP_UNSIL_SHOOT3 from enumeration structure usp_e in wpn_shared.h
				seq = Com_RandomLong(USP_UNSIL_SHOOT1, USP_UNSIL_SHOOT3);
			}
			else 
			{
				//If player has the silencer detached to usp and it's dry, play the animation USP_UNSIL_SHOOT_EMPTY from enumeration structure usp_e in wpn_shared.h
				seq = USP_UNSIL_SHOOT_EMPTY;
			}
		}
		else
		{
			EV_MuzzleFlash();
			if(!args->bparam1)
			{
				//If player has the silencer attached to usp, play a random shoot animation between USP_SHOOT1, USP_SHOOT2 or USP_SHOOT3 from enumeration structure usp_e in wpn_shared.h
				seq = Com_RandomLong(USP_SHOOT1, USP_SHOOT3);
			}
			else 
			{
				//If player has the silencer attached to usp and it's dry, play the animation  USP_SHOOT_EMPTY from enumeration structure usp_e in wpn_shared.h
				seq = USP_SHOOT_EMPTY;
			}
		}

		gEngfuncs.pEventAPI->EV_WeaponAnimation(seq, 2);

		if(!gHUD.cl_righthand->value)
		{
			//Local player (Left hand): We pass '1' as the last argument to EV_GetDefaultShellInfo.
			//Note: The engine's original function signature used a boolean (bReverseDirection) here. 
			//This was modified to accept an integer (attachmentIndex) in ev_common.cpp to dynamically fetch the 
			//precise ejection coordinates directly from the v_ (view) model's attachments.
			EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 36.0, -14.0, -14.0, 1);
		}
		else
		{
			//Local player (Right hand): Use attachment index 1
			EV_GetDefaultShellInfo(args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 36.0, -14.0, 14.0, 1);
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
	shellAngles[0] = -angles[0]; 

	//YAW (Left/Right): Inherit the player's facing direction with minor random variance
	shellAngles[1] = angles[1] + gEngfuncs.pfnRandomFloat(-15.0f, 15.0f); 

	//ROLL (Tilt): Inherit the player's roll with variance to simulate tumbling
	shellAngles[2] = angles[2]; 

	EV_EjectBrass(ShellOrigin, ShellVelocity, shellAngles, g_iPShell, TE_BOUNCE_SHELL);

	PLAY_EVENT_SOUND(silencer_on? SOUNDS_NAME[2] : SOUNDS_NAME[Com_RandomLong(0, 1)]);

	EV_GetGunPosition(args, vecSrc, origin);

	Vector vSpread(args->fparam1, args->fparam2, 0.0f);
	EV_HLDM_FireBullets(idx, forward, right, up, 1, vecSrc, forward, vSpread, 8192.0, BULLET_PLAYER_45ACP, 2);
}
