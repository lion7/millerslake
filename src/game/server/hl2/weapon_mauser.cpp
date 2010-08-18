//-----------------------------------------------------------------------------
// Purpose:	Millers Lake Mauser - Hunting Rifle
//-----------------------------------------------------------------------------

#include "cbase.h"
#include "NPCEvent.h"
#include "basehlcombatweapon.h"
#include "basecombatcharacter.h"
#include "AI_BaseNPC.h"
#include "player.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "soundent.h"
#include "game.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "te_effect_dispatch.h"
#include "gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// CWeaponMauser
//-----------------------------------------------------------------------------

class CWeaponMauser : public CBaseHLCombatWeapon
{
	DECLARE_CLASS( CWeaponMauser, CBaseHLCombatWeapon );
public:

	CWeaponMauser( void );

	void	PrimaryAttack( void );
	void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	float	WeaponAutoAimScale()	{ return 0.6f; }

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( weapon_Mauser, CWeaponMauser );

PRECACHE_WEAPON_REGISTER( weapon_Mauser );

IMPLEMENT_SERVERCLASS_ST( CWeaponMauser, DT_WeaponMauser )
END_SEND_TABLE()

BEGIN_DATADESC( CWeaponMauser )
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponMauser::CWeaponMauser( void )
{
	m_bReloadsSingly	= false; // Millers Lake - Problems - 18-08-2010: Have to find out if it uses a clip or if you can just insert 1 shell at a time.
	m_bFiresUnderwater	= false; // Millers Lake - Problems - 18-08-2010: Water logged :P
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponMauser::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	switch( pEvent->event )
	{
		case EVENT_WEAPON_RELOAD:
			{
				CEffectData data;

				// Emit six shells on reload
				for ( int i = 0; i < 6; i++ ) // Millers Lake - Problems - 18-08-2010: How many bullets can the gun hold? Ask blob? 
				{
					data.m_vOrigin = pOwner->WorldSpaceCenter() + RandomVector( -4, 4 );
					data.m_vAngles = QAngle( 90, random->RandomInt( 0, 360 ), 0 );
					data.m_nEntIndex = entindex();

					DispatchEffect( "ShellEject", data );
				}

				break;
			}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponMauser::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast.
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( !pPlayer )	//If we aren't a player, exit the function.
	{
		return;				
	}

	if ( m_iClip1 <= 0 )
	{
		if ( !m_bFireOnEmpty )
		{
			Reload();
		}
		else
		{
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = 0.15;
		}
	
		return;
	}

	m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );

	WeaponSound( SINGLE );
	pPlayer->DoMuzzleFlash();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.75; // Millers Lake - Problems - 18-08-2010: find out the Mausers fire rate.
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.75; // Millers Lake - Problems - 18-08-2010: Don't have a secondary fire but may as well set it up. Hope it doesn't conflict with the zoom (Uses +attack_2).

	m_iClip1--;

	Vector vecSrc		= pPlayer->Weapon_ShootPosition();
	Vector vecAiming	= pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );	

	pPlayer->FireBullets( 1, vecSrc, vecAiming, vec3_origin, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0 );

	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );

	//Disorient the player
	QAngle angles = pPlayer->GetLocalAngles();

	angles.x += random->RandomInt( -1, 1 );
	angles.y += random->RandomInt( -1, 1 );
	angles.z = 0;

	pPlayer->SnapEyeAngles( angles );

	pPlayer->ViewPunch( QAngle( -8, random->RandomFloat( -2, 2 ), 0 ) );

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 600, 0.2, GetOwner() );

	if ( !m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 ); 
	}
}
