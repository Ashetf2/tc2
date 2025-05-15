//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF AmmoPack.
//
//=============================================================================//
#ifndef TF_POWERUP_H
#define TF_POWERUP_H

#ifdef _WIN32
#pragma once
#endif

#ifdef GAME_DLL
#include "items.h"

#define TF_POWERUP_LIFETIME		30.0f		// normal powerup timeout

enum powerupsize_t
{
	POWERUP_SMALL,
	POWERUP_MEDIUM,
	POWERUP_FULL,

	POWERUP_SIZES,
};

extern float PackRatios[POWERUP_SIZES];

//=============================================================================
//
// CTF Powerup class.
//

class CTFPowerup : public CItem
{
public:
	DECLARE_CLASS( CTFPowerup, CItem );

	CTFPowerup();

	void			Spawn( void );
	CBaseEntity*	Respawn( void );
	virtual void	Precache();
	void			Materialize( void );
	virtual bool	ValidTouch( CBasePlayer *pPlayer );
	virtual bool	MyTouch( CBasePlayer *pPlayer );

	void			DropSingleInstance( Vector &vecLaunchVel, CBaseCombatCharacter *pThrower, float flThrowerTouchDelay, float flResetTime = 0.1f );

	bool			IsDisabled( void );
	void			SetDisabled( bool bDisabled );

	virtual float	GetRespawnDelay( void ) { return g_pGameRules->FlItemRespawnTime( this ); }

	// Input handlers
	void			InputEnable( inputdata_t &inputdata );
	void			InputDisable( inputdata_t &inputdata );
	void			InputToggle( inputdata_t &inputdata );

	virtual powerupsize_t	GetPowerupSize( void ) { return POWERUP_FULL; }

	virtual const char *GetPowerupModel( void );
	virtual const char *GetDefaultPowerupModel( void ) = 0;

	virtual bool	ItemCanBeTouchedByPlayer( CBasePlayer *pPlayer );

	virtual float	GetLifeTime() { return TF_POWERUP_LIFETIME; }

	void			CreateTimerIcon(void);
	void			DestroyTimerIcon(void);

private:
	EHANDLE		m_hTimerIcon;

protected:
	void			Materialize_Internal( void );

	bool			m_bDisabled;
	bool			m_bRespawning;
	bool			m_bThrownSingleInstance;
	bool			m_bAutoMaterialize;

	string_t		m_iszModel;

	float			m_flThrowerTouchTime;

	DECLARE_DATADESC();
};

#endif // GAME_DLL

//=============================================================================
//
// Pickup timer declare
//

#ifdef CLIENT_DLL
#define CCircularTimer C_CircularTimer
#define CBaseAnimating C_BaseAnimating
#endif

class CCircularTimer : public CBaseAnimating
{
public:
	DECLARE_CLASS(CCircularTimer, CBaseEntity);
	DECLARE_NETWORKCLASS();

	CCircularTimer();

#ifdef CLIENT_DLL

	virtual int		DrawModel(int flags);
	void			DrawTimerProgressBar(void);

	virtual RenderGroup_t GetRenderGroup(void);
	virtual bool	ShouldDraw(void) { return true; }

	virtual void GetRenderBounds(Vector& theMins, Vector& theMaxs);

private:

	IMaterial* m_pTimerProgressMaterial_Empty;
	IMaterial* m_pTimerProgressMaterial_Full;

#else
public:
	virtual void Spawn(void);
	virtual int UpdateTransmitState(void);

#endif

private:

	float m_flSimStartTime = 0.0f;
	float m_flSimDuration = 10.0f;
};

#endif // TF_POWERUP_H


