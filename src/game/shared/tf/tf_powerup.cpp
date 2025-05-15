//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF AmmoPack.
//
//=============================================================================//
#include "cbase.h"
#include "tf_powerup.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#include "engine/IEngineSound.h"
#include "filesystem.h"

#ifdef GAME_DLL

#include "items.h"
#include "tf_player.h"
#include "tf_team.h"
#include "bot/tf_bot.h"

ConVar cl_timer_return_height("cl_timer_return_height", "20", FCVAR_CHEAT);

#else

#include "view.h"

ConVar cl_timer_return_size("cl_timer_return_size", "20", FCVAR_CHEAT);
ConVar cl_timer_color_red("cl_timer_color_red", "200", FCVAR_CHEAT);
ConVar cl_timer_color_green("cl_timer_color_green", "200", FCVAR_CHEAT);
ConVar cl_timer_color_blue("cl_timer_color_blue", "200", FCVAR_CHEAT);
ConVar cl_timer_color_alpha("cl_timer_color_alpha", "255", FCVAR_CHEAT);

#endif

//=============================================================================
//
// Main CTF Powerup code
//

#ifdef GAME_DLL

//=============================================================================
float PackRatios[POWERUP_SIZES] =
{
	0.2,	// SMALL
	0.5,	// MEDIUM
	1.0,	// FULL
};

//=============================================================================
//
// CTF Powerup tables.
//

BEGIN_DATADESC( CTFPowerup )

	// Keyfields.
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),
	DEFINE_KEYFIELD( m_iszModel, FIELD_STRING, "powerup_model" ),
	DEFINE_KEYFIELD( m_bAutoMaterialize, FIELD_BOOLEAN, "AutoMaterialize" ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),

	// Outputs.

END_DATADESC();

//=============================================================================
//
// CTF Powerup functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPowerup::CTFPowerup()
{
	m_bDisabled = false;
	m_bRespawning = false;
	m_bAutoMaterialize = true;

	m_iszModel = NULL_STRING;

	m_flThrowerTouchTime = -1;

	UseClientSideAnimation();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPowerup::Spawn( void )
{
	Precache();
	SetModel( GetPowerupModel() );

	BaseClass::Spawn();

	BaseClass::SetOriginalSpawnOrigin( GetAbsOrigin() );
	BaseClass::SetOriginalSpawnAngles( GetAbsAngles() );

	VPhysicsDestroyObject();
	SetMoveType( MOVETYPE_NONE );
	SetSolidFlags( FSOLID_NOT_SOLID | FSOLID_TRIGGER );

	if ( m_bDisabled )
	{
		SetDisabled( true );
	}

	m_bRespawning = false;

	ResetSequence( LookupSequence("idle") );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPowerup::Precache()
{
	PrecacheModel( GetPowerupModel() );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity* CTFPowerup::Respawn( void )
{
	m_bRespawning = true;
	CBaseEntity *pReturn = BaseClass::Respawn();

	// Override the respawn time
	SetNextThink( gpGlobals->curtime + GetRespawnDelay() );

	return pReturn;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPowerup::Materialize( void )
{
	if ( !m_bAutoMaterialize )
	{
		return;
	}

	Materialize_Internal();

	DestroyTimerIcon();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPowerup::Materialize_Internal( void )
{
	if ( !m_bDisabled && IsEffectActive( EF_NODRAW ) )
	{
		// changing from invisible state to visible.
		EmitSound( "Item.Materialize" );
		RemoveEffects( EF_NODRAW );
	}

	m_bRespawning = false;
	SetTouch( &CItem::ItemTouch );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPowerup::ValidTouch( CBasePlayer *pPlayer )
{
	// Is the item enabled?
	if ( IsDisabled() )
	{
		return false;
	}

	// Only touch a live player.
	if ( !pPlayer || !pPlayer->IsPlayer() || !pPlayer->IsAlive() )
	{
		return false;
	}

	// Team number and does it match?
	int iTeam = GetTeamNumber();
	if ( iTeam && ( pPlayer->GetTeamNumber() != iTeam ) )
	{
		return false;
	}

	// enemies in mann vs machine can't pick up any powerups
	if ( TFGameRules()->IsMannVsMachineMode() && pPlayer->GetTeamNumber() == TF_TEAM_PVE_INVADERS )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPowerup::MyTouch( CBasePlayer *pPlayer )
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPowerup::DropSingleInstance( Vector &vecLaunchVel, CBaseCombatCharacter *pThrower, float flThrowerTouchDelay, float flResetTime /*= 0.1f*/ )
{
//	SetSize( Vector(-8,-8,-8), Vector(8,8,8) );
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetAbsVelocity( vecLaunchVel );	
	SetSolid( SOLID_BBOX );
	if ( flResetTime )
	{
		ActivateWhenAtRest( flResetTime );
	}

	m_bThrownSingleInstance = true;
	AddSpawnFlags( SF_NORESPAWN );

	SetOwnerEntity( pThrower );
	m_flThrowerTouchTime = gpGlobals->curtime + flThrowerTouchDelay;

	// Remove ourselves after some time
	SetContextThink( &CBaseEntity::SUB_Remove, gpGlobals->curtime + GetLifeTime(), "PowerupRemoveThink" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPowerup::InputEnable( inputdata_t &inputdata )
{
	SetDisabled( false );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPowerup::InputDisable( inputdata_t &inputdata )
{
	SetDisabled( true );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPowerup::IsDisabled( void )
{
	return m_bDisabled;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPowerup::InputToggle( inputdata_t &inputdata )
{
	if ( m_bDisabled )
	{
		SetDisabled( false );
	}
	else
	{
		SetDisabled( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPowerup::SetDisabled( bool bDisabled )
{
	m_bDisabled = bDisabled;

	if ( bDisabled )
	{
		AddEffects( EF_NODRAW );
	}
	else
	{
		// only turn it back on if we're not in the middle of respawning
		if ( !m_bRespawning )
		{
            RemoveEffects( EF_NODRAW );
		}
		else if ( !m_bAutoMaterialize )
		{
			// We wait for a set-enabled to re-materialize if we were 
			// set to not auto-materialize
			Materialize_Internal();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CTFPowerup::GetPowerupModel( void )
{
	if ( m_iszModel != NULL_STRING )
	{
		if ( g_pFullFileSystem->FileExists( STRING( m_iszModel ), "GAME" ) )
		{
			return ( STRING( m_iszModel ) );
		}
	}

	return GetDefaultPowerupModel();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPowerup::ItemCanBeTouchedByPlayer( CBasePlayer *pPlayer )
{
	if ( pPlayer == GetOwnerEntity() )
	{
		if ( ( m_flThrowerTouchTime > 0 ) && ( gpGlobals->curtime < m_flThrowerTouchTime ) )
		{
			return false;
		}
	}

	return BaseClass::ItemCanBeTouchedByPlayer( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: Create circular timer
//-----------------------------------------------------------------------------
void CTFPowerup::CreateTimerIcon(void)
{
	if (m_hTimerIcon.Get())
		return;

	CBaseEntity* pTimerIcon = CBaseEntity::Create("item_circular_timer", GetAbsOrigin() + Vector(0, 0, cl_timer_return_height.GetFloat()), vec3_angle, this);
	//CBaseEntity* pTimerIcon = CBaseEntity::Create("item_circular_timer", GetAbsOrigin() + Vector(0, 0, 20), vec3_angle, this);
	if (pTimerIcon)
	{
		m_hTimerIcon = pTimerIcon;
		m_hTimerIcon->SetParent(this);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destroy current circular timer
//-----------------------------------------------------------------------------

void CTFPowerup::DestroyTimerIcon(void)
{
	if (!m_hTimerIcon.Get())
		return;

	UTIL_Remove(m_hTimerIcon);
	m_hTimerIcon = NULL;
}

#endif //GAME_DLL

//=============================================================================
//
// Pickup timer code
//

LINK_ENTITY_TO_CLASS(item_circular_timer, CCircularTimer);

IMPLEMENT_NETWORKCLASS_ALIASED(CircularTimer, DT_CircularTimer)

BEGIN_NETWORK_TABLE(CCircularTimer, DT_CircularTimer)
END_NETWORK_TABLE()

CCircularTimer::CCircularTimer()
{
#ifdef CLIENT_DLL
	m_pTimerProgressMaterial_Empty = NULL;
	m_pTimerProgressMaterial_Full = NULL;
#endif
}

#ifdef GAME_DLL

void CCircularTimer::Spawn(void)
{
	BaseClass::Spawn();

	UTIL_SetSize(this, Vector(-8, -8, -8), Vector(8, 8, 8));

	CollisionProp()->SetCollisionBounds(Vector(-50, -50, -50), Vector(50, 50, 50));
}

int CCircularTimer::UpdateTransmitState(void)
{
	return SetTransmitState(FL_EDICT_ALWAYS);
}

#endif // GAME_DLL


#ifdef CLIENT_DLL

typedef struct
{
	float maxProgress;

	float vert1x;
	float vert1y;
	float vert2x;
	float vert2y;

	int swipe_dir_x;
	int swipe_dir_y;
} timer_progress_segment_t;


// This defines the properties of the 8 circle segments
// in the circular progress bar.
timer_progress_segment_t Segments[8] =
{
	{ 0.125, 0.5, 0.0, 1.0, 0.0, 1, 0 },
	{ 0.25,	 1.0, 0.0, 1.0, 0.5, 0, 1 },
	{ 0.375, 1.0, 0.5, 1.0, 1.0, 0, 1 },
	{ 0.50,	 1.0, 1.0, 0.5, 1.0, -1, 0 },
	{ 0.625, 0.5, 1.0, 0.0, 1.0, -1, 0 },
	{ 0.75,	 0.0, 1.0, 0.0, 0.5, 0, -1 },
	{ 0.875, 0.0, 0.5, 0.0, 0.0, 0, -1 },
	{ 1.0,	 0.0, 0.0, 0.5, 0.0, 1, 0 },
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
RenderGroup_t CCircularTimer::GetRenderGroup(void)
{
	return RENDER_GROUP_TRANSLUCENT_ENTITY;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCircularTimer::GetRenderBounds(Vector& theMins, Vector& theMaxs)
{
	theMins.Init(-20, -20, -20);
	theMaxs.Init(20, 20, 20);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CCircularTimer::DrawModel(int flags)
{
	int nRetVal = BaseClass::DrawModel(flags);

	DrawTimerProgressBar();

	return nRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: Draw progress bar above the flag indicating when it will return
//-----------------------------------------------------------------------------
void CCircularTimer::DrawTimerProgressBar(void)
{
	/* CCaptureFlag *pFlag = dynamic_cast< CCaptureFlag * > ( GetOwnerEntity() );

	if ( !pFlag )
		return;

	// Don't draw if this flag is not going to reset
	if ( pFlag->GetMaxResetTime() <= 0 )
		return;

	if ( !TFGameRules()->FlagsMayBeCapped() )
		return; */

	if (!m_pTimerProgressMaterial_Full)
	{
		m_pTimerProgressMaterial_Full = materials->FindMaterial("VGUI/flagtime_empty", TEXTURE_GROUP_VGUI);
	}

	if (!m_pTimerProgressMaterial_Empty)
	{
		m_pTimerProgressMaterial_Empty = materials->FindMaterial("VGUI/flagtime_full", TEXTURE_GROUP_VGUI);
	}

	if (!m_pTimerProgressMaterial_Full || !m_pTimerProgressMaterial_Empty)
	{
		return;
	}

	CMatRenderContextPtr pRenderContext(materials);

	Vector vOrigin = GetAbsOrigin();
	QAngle vAngle = vec3_angle;

	// Align it towards the viewer
	Vector vUp = CurrentViewUp();
	Vector vRight = CurrentViewRight();
	if (fabs(vRight.z) > 0.95)	// don't draw it edge-on
		return;

	vRight.z = 0;
	VectorNormalize(vRight);

	float flSize = cl_timer_return_size.GetFloat();

	unsigned char ubColor[4];
	ubColor[3] = cl_timer_color_alpha.GetFloat();

	/* 	switch( pFlag->GetTeamNumber() )
		{
		case TF_TEAM_RED:
			ubColor[0] = 255;
			ubColor[1] = 0;
			ubColor[2] = 0;
			break;
		case TF_TEAM_BLUE:
			ubColor[0] = 0;
			ubColor[1] = 0;
			ubColor[2] = 255;
			break;
		default:
			ubColor[0] = 200;
			ubColor[1] = 200;
			ubColor[2] = 200;
			break;
		} */

	ubColor[0] = cl_timer_color_red.GetFloat();
	ubColor[1] = cl_timer_color_green.GetFloat();
	ubColor[2] = cl_timer_color_blue.GetFloat();

	// First we draw a quad of a complete icon, background
	CMeshBuilder meshBuilder;

	pRenderContext->Bind(m_pTimerProgressMaterial_Empty);
	IMesh* pMesh = pRenderContext->GetDynamicMesh();

	meshBuilder.Begin(pMesh, MATERIAL_QUADS, 1);

	meshBuilder.Color4ubv(ubColor);
	meshBuilder.TexCoord2f(0, 0, 0);
	meshBuilder.Position3fv((vOrigin + (vRight * -flSize) + (vUp * flSize)).Base());
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv(ubColor);
	meshBuilder.TexCoord2f(0, 1, 0);
	meshBuilder.Position3fv((vOrigin + (vRight * flSize) + (vUp * flSize)).Base());
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv(ubColor);
	meshBuilder.TexCoord2f(0, 1, 1);
	meshBuilder.Position3fv((vOrigin + (vRight * flSize) + (vUp * -flSize)).Base());
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv(ubColor);
	meshBuilder.TexCoord2f(0, 0, 1);
	meshBuilder.Position3fv((vOrigin + (vRight * -flSize) + (vUp * -flSize)).Base());
	meshBuilder.AdvanceVertex();

	meshBuilder.End();

	pMesh->Draw();

	//float flProgress = pFlag->GetReturnProgress();

	if (m_flSimStartTime <= 0.0f)
		m_flSimStartTime = gpGlobals->curtime;

	float flElapsed = gpGlobals->curtime - m_flSimStartTime;
	float flProgress = clamp(flElapsed / m_flSimDuration, 0.0f, 1.0f);

	pRenderContext->Bind(m_pTimerProgressMaterial_Full);
	pMesh = pRenderContext->GetDynamicMesh();

	vRight *= flSize * 2;
	vUp *= flSize * -2;

	// Next we're drawing the circular progress bar, in 8 segments
	// For each segment, we calculate the vertex position that will draw
	// the slice.
	int i;
	for (i = 0; i < 8; i++)
	{
		if (flProgress < Segments[i].maxProgress)
		{
			CMeshBuilder meshBuilder_Full;

			meshBuilder_Full.Begin(pMesh, MATERIAL_TRIANGLES, 3);

			// vert 0 is ( 0.5, 0.5 )
			meshBuilder_Full.Color4ubv(ubColor);
			meshBuilder_Full.TexCoord2f(0, 0.5, 0.5);
			meshBuilder_Full.Position3fv(vOrigin.Base());
			meshBuilder_Full.AdvanceVertex();

			// Internal progress is the progress through this particular slice
			float internalProgress = RemapVal(flProgress, Segments[i].maxProgress - 0.125, Segments[i].maxProgress, 0.0, 1.0);
			internalProgress = clamp(internalProgress, 0.0f, 1.0f);

			// Calculate the x,y of the moving vertex based on internal progress
			float swipe_x = Segments[i].vert2x - (1.0 - internalProgress) * 0.5 * Segments[i].swipe_dir_x;
			float swipe_y = Segments[i].vert2y - (1.0 - internalProgress) * 0.5 * Segments[i].swipe_dir_y;

			// vert 1 is calculated from progress
			meshBuilder_Full.Color4ubv(ubColor);
			meshBuilder_Full.TexCoord2f(0, swipe_x, swipe_y);
			meshBuilder_Full.Position3fv((vOrigin + (vRight * (swipe_x - 0.5)) + (vUp * (swipe_y - 0.5))).Base());
			meshBuilder_Full.AdvanceVertex();

			// vert 2 is ( Segments[i].vert1x, Segments[i].vert1y )
			meshBuilder_Full.Color4ubv(ubColor);
			meshBuilder_Full.TexCoord2f(0, Segments[i].vert2x, Segments[i].vert2y);
			meshBuilder_Full.Position3fv((vOrigin + (vRight * (Segments[i].vert2x - 0.5)) + (vUp * (Segments[i].vert2y - 0.5))).Base());
			meshBuilder_Full.AdvanceVertex();

			meshBuilder_Full.End();

			pMesh->Draw();
		}
	}
}

#endif //CLIENT_DLL