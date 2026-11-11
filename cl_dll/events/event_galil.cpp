#include "events.h"
#include "wpn_shared.h"


static const char *SOUNDS_NAME[] =
{
	"weapons/galil-1.wav", 
	"weapons/galil-2.wav"
};


void EV_FireGALIL( event_args_t *args )
{
	Vector ShellVelocity;
	Vector ShellOrigin;
	Vector vecSrc, vecAiming;
	int idx = args->entindex;
	Vector origin( args->origin );
	Vector angles(
		args->iparam1 / 10000000.0f + args->angles[0],
		args->iparam2 / 10000000.0f + args->angles[1],
		args->angles[2]
		);
	Vector velocity( args->velocity );
	Vector forward, right, up;


	AngleVectors( angles, forward, right, up );

	if ( EV_IsLocal( idx ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();

		//Play a random shoot animation GALIL_SHOOT1, GALIL_SHOOT2, or GALIL_SHOOT3 from enumeration structure galil_e in wpn_shared.h
		gEngfuncs.pEventAPI->EV_WeaponAnimation(GALIL_SHOOT1 + Com_RandomLong(0,2), 2);

		if( !gHUD.cl_righthand->value )
		{
			//Local player (Left hand): Use attachment index 1 for shell ejection
			//The fallback offsets (0.0, 0.0, 0.0) are ignored since the model attachment is utilized
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 0.0, 0.0, 0.0, 1);
		}
		else
		{
			//Local player (Right hand): Use attachment index 1
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 0.0, 0.0, 0.0, 1);
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

	PLAY_EVENT_SOUND( SOUNDS_NAME[Com_RandomLong( 0, 1 )] );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );
	Vector vSpread;

	vSpread.x = args->fparam1;
	vSpread.y = args->fparam2;
	EV_HLDM_FireBullets(idx, forward, right, up, 1, vecSrc, vecAiming, vSpread, 8192.0, BULLET_PLAYER_556MM, 2);
}
