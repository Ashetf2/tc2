//========= Copyright LOLOLOL, All rights reserved. ============//
//
// Purpose: CTF Flag.
//
//=============================================================================//
#ifndef ENTITY_CIRCULAR_TIMER_H
#define ENTITY_CIRCULAR_TIMER_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
	#define CCircularTimer C_CircularTimer
	#define CBaseAnimating C_BaseAnimating
#endif

class CCircularTimer: public CBaseAnimating
{
public:
	DECLARE_CLASS( CCircularTimer, CBaseEntity );
	DECLARE_NETWORKCLASS();

	CCircularTimer();

#ifdef CLIENT_DLL

	virtual int		DrawModel( int flags );
	void			DrawTimerProgressBar( void );

	virtual RenderGroup_t GetRenderGroup( void );
	virtual bool	ShouldDraw( void ) { return true; }

	virtual void GetRenderBounds( Vector& theMins, Vector& theMaxs );

private:

	IMaterial	*m_pTimerProgressMaterial_Empty;
	IMaterial	*m_pTimerProgressMaterial_Full;

#else
public:
	virtual void Spawn( void );
	virtual int UpdateTransmitState( void );

#endif

private:

	float m_flSimStartTime = 0.0f;
	float m_flSimDuration = 10.0f;
};

#endif // ENTITY_CIRCULAR_TIMER_H
