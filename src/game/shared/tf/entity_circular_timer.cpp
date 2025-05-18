//========= Copyright LOLOLOL, All rights reserved. ============//
//
// Purpose: Circular timer.
//
//=============================================================================//
#include "cbase.h"
#include "entity_circular_timer.h"

#ifdef CLIENT_DLL

#include "view.h"

ConVar cl_timer_return_size( "cl_timer_return_size", "20", FCVAR_CHEAT );
ConVar cl_timer_color_red("cl_timer_color_red", "200", FCVAR_CHEAT);
ConVar cl_timer_color_green("cl_timer_color_green", "200", FCVAR_CHEAT);
ConVar cl_timer_color_blue("cl_timer_color_blue", "200", FCVAR_CHEAT);
ConVar cl_timer_color_alpha("cl_timer_color_alpha", "255", FCVAR_CHEAT);

#endif //CLIENT_DLL

LINK_ENTITY_TO_CLASS(item_circular_timer, CCircularTimer);

IMPLEMENT_NETWORKCLASS_ALIASED(CircularTimer, DT_CircularTimer)

BEGIN_NETWORK_TABLE(CCircularTimer, DT_CircularTimer)
END_NETWORK_TABLE()

CCircularTimer::CCircularTimer()
{
#ifdef CLIENT_DLL

	m_pTimerProgressMaterial_Empty = NULL;
	m_pTimerProgressMaterial_Full = NULL;

#endif //CLIENT_DLL
}

#ifdef GAME_DLL

void CCircularTimer::Spawn(void)
{
	BaseClass::Spawn();

	UTIL_SetSize(this, Vector(-8, -8, -8), Vector(8, 8, 8));

	CollisionProp()->SetCollisionBounds(Vector(-50, -50, -50), Vector(50, 50, 50));
}

int CCircularTimer::UpdateTransmitState( void )
{
	return SetTransmitState( FL_EDICT_ALWAYS );
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
RenderGroup_t CCircularTimer::GetRenderGroup( void ) 
{	
	return RENDER_GROUP_TRANSLUCENT_ENTITY;	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCircularTimer::GetRenderBounds( Vector& theMins, Vector& theMaxs )
{
	theMins.Init( -20, -20, -20 );
	theMaxs.Init(  20,  20,  20 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CCircularTimer::DrawModel( int flags )
{
	int nRetVal = BaseClass::DrawModel( flags );
	
	DrawTimerProgressBar();

	return nRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: Draw progress bar above the flag indicating when it will return
//-----------------------------------------------------------------------------
void CCircularTimer::DrawTimerProgressBar( void )
{
	if ( !m_pTimerProgressMaterial_Full )
	{
		m_pTimerProgressMaterial_Full = materials->FindMaterial( "VGUI/flagtime_empty", TEXTURE_GROUP_VGUI );
	}

	if ( !m_pTimerProgressMaterial_Empty )
	{
		m_pTimerProgressMaterial_Empty = materials->FindMaterial( "VGUI/flagtime_full", TEXTURE_GROUP_VGUI );
	}

	if ( !m_pTimerProgressMaterial_Full || !m_pTimerProgressMaterial_Empty )
	{
		return;
	}

	CMatRenderContextPtr pRenderContext( materials );

	Vector vOrigin = GetAbsOrigin();
	QAngle vAngle = vec3_angle;

	// Align it towards the viewer
	Vector vUp = CurrentViewUp();
	Vector vRight = CurrentViewRight();
	if ( fabs( vRight.z ) > 0.95 )	// don't draw it edge-on
		return;

	vRight.z = 0;
	VectorNormalize( vRight );

	float flSize = cl_timer_return_size.GetFloat();

	unsigned char ubColor[4];
	ubColor[3] = cl_timer_color_alpha.GetFloat();

	ubColor[0] = cl_timer_color_red.GetFloat();
	ubColor[1] = cl_timer_color_green.GetFloat();
	ubColor[2] = cl_timer_color_blue.GetFloat();

	// First we draw a quad of a complete icon, background
	CMeshBuilder meshBuilder;

	pRenderContext->Bind( m_pTimerProgressMaterial_Empty );
	IMesh *pMesh = pRenderContext->GetDynamicMesh();

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0,0,0 );
	meshBuilder.Position3fv( (vOrigin + (vRight * -flSize) + (vUp * flSize)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0,1,0 );
	meshBuilder.Position3fv( (vOrigin + (vRight * flSize) + (vUp * flSize)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0,1,1 );
	meshBuilder.Position3fv( (vOrigin + (vRight * flSize) + (vUp * -flSize)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0,0,1 );
	meshBuilder.Position3fv( (vOrigin + (vRight * -flSize) + (vUp * -flSize)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();

	pMesh->Draw();

	//float flProgress = pFlag->GetReturnProgress();
	
	//NOTE: we want to change this to get the value of FlItemRespawnTime

	if ( m_flSimStartTime <= 0.0f )
	m_flSimStartTime = gpGlobals->curtime;
	
	float flElapsed = gpGlobals->curtime - m_flSimStartTime;
	float flProgress = clamp(flElapsed / m_flSimDuration, 0.0f, 1.0f);

	pRenderContext->Bind( m_pTimerProgressMaterial_Full );
	pMesh = pRenderContext->GetDynamicMesh();

	vRight *= flSize * 2;
	vUp *= flSize * -2;

	// Next we're drawing the circular progress bar, in 8 segments
	// For each segment, we calculate the vertex position that will draw
	// the slice.
	int i;
	for ( i=0;i<8;i++ )
	{
		if ( flProgress < Segments[i].maxProgress )
		{
			CMeshBuilder meshBuilder_Full;

			meshBuilder_Full.Begin( pMesh, MATERIAL_TRIANGLES, 3 );

			// vert 0 is ( 0.5, 0.5 )
			meshBuilder_Full.Color4ubv( ubColor );
			meshBuilder_Full.TexCoord2f( 0, 0.5, 0.5 );
			meshBuilder_Full.Position3fv( vOrigin.Base() );
			meshBuilder_Full.AdvanceVertex();

			// Internal progress is the progress through this particular slice
			float internalProgress = RemapVal( flProgress, Segments[i].maxProgress - 0.125, Segments[i].maxProgress, 0.0, 1.0 );
			internalProgress = clamp( internalProgress, 0.0f, 1.0f );

			// Calculate the x,y of the moving vertex based on internal progress
			float swipe_x = Segments[i].vert2x - ( 1.0 - internalProgress ) * 0.5 * Segments[i].swipe_dir_x;
			float swipe_y = Segments[i].vert2y - ( 1.0 - internalProgress ) * 0.5 * Segments[i].swipe_dir_y;

			// vert 1 is calculated from progress
			meshBuilder_Full.Color4ubv( ubColor );
			meshBuilder_Full.TexCoord2f( 0, swipe_x, swipe_y );
			meshBuilder_Full.Position3fv( (vOrigin + (vRight * ( swipe_x - 0.5 ) ) + (vUp *( swipe_y - 0.5 ) ) ).Base() );
			meshBuilder_Full.AdvanceVertex();

			// vert 2 is ( Segments[i].vert1x, Segments[i].vert1y )
			meshBuilder_Full.Color4ubv( ubColor );
			meshBuilder_Full.TexCoord2f( 0, Segments[i].vert2x, Segments[i].vert2y );
			meshBuilder_Full.Position3fv( (vOrigin + (vRight * ( Segments[i].vert2x - 0.5 ) ) + (vUp *( Segments[i].vert2y - 0.5 ) ) ).Base() );
			meshBuilder_Full.AdvanceVertex();

			meshBuilder_Full.End();

			pMesh->Draw();
		}
	}
}

#endif //CLIENT_DLL