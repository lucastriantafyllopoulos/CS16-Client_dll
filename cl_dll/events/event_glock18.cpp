#include "events.h"
#include "wpn_shared.h"

bool g_bGlockBurstMode = false;

static const char *SOUNDS_NAME[] =
{
	"weapons/glock18-1.wav", 
	"weapons/glock18-2.wav"
};

void EV_Fireglock18(event_args_t *args)
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

		int seq;
		bool isBurst = (g_iWeaponFlags & WPNSTATE_GLOCK18_BURST_MODE) != 0 || g_bGlockBurstMode;

		if(!args->bparam1)
		{
			if(g_bHoldingShield)
			{
				//If player is holding the shield, play a random shoot animation between GLOCK18_SHIELD_SHOOT and GLOCK18_SHIELD_SHOOT2 from enumeration structure glock18_shield_e in wpn_shared.h
				seq = Com_RandomLong(GLOCK18_SHIELD_SHOOT, GLOCK18_SHIELD_SHOOT2);
			}
			else
			{
				//If player is using burst mode, play the animation GLOCK18_SHOOT else use GLOCK18_SHOOT3 from enumeration structure glock18_e in wpn_shared.h
				if(isBurst)
				{
					seq = GLOCK18_SHOOT;
				}
				else
				{
					seq = GLOCK18_SHOOT3;
				}
			}
		}
		else
		{
			if(g_bHoldingShield)
			{
				//If player is holding the shield and glock is dry, play the animation GLOCK18_SHIELD_SHOOT_EMPTY from enumeration structure glock18_shield_e in wpn_shared.h
				seq = GLOCK18_SHIELD_SHOOT_EMPTY;
			}
			else
			{
				//If player is just holding the glock and it's dry, play the animation GLOCK18_SHOOT_EMPTY from enumeration structure glock18_e in wpn_shared.h
				seq = GLOCK18_SHOOT_EMPTY;
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

	//Pass the complete Vector to the overloaded EV_EjectBrass function
	EV_EjectBrass(ShellOrigin, ShellVelocity, shellAngles, g_iPShell, TE_BOUNCE_SHELL);

	bool isBurst = (g_iWeaponFlags & WPNSTATE_GLOCK18_BURST_MODE) != 0 || g_bGlockBurstMode;

	//If player has burst mode on/off or holds/doesn't hold a shield, play the corresponding sound
	PLAY_EVENT_SOUND(isBurst && !g_bHoldingShield ? SOUNDS_NAME[0] : SOUNDS_NAME[1]);

	EV_GetGunPosition(args, vecSrc, origin);

	Vector vSpread(args->fparam1, args->fparam2, 0.0f);
	EV_HLDM_FireBullets(idx, forward, right, up, 1, vecSrc, forward, vSpread, 4096.0, BULLET_PLAYER_9MM, 2);
}
