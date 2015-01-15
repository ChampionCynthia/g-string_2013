
#include "cbase.h"
#include "holo_ship_comm.h"
#include "holo_utilities.h"
#include "gstring/c_gstring_util.h"

#include "vgui_controls/Label.h"
#include "vgui/ILocalize.h"
#include "materialsystem/imaterialvar.h"
#include "c_user_message_register.h"
#include "filesystem.h"

using namespace vgui;

DECLARE_HOLO_MESSAGE( HoloMessage, CHoloShipComm );

CHoloShipComm::CHoloShipComm( ISpacecraftData *pSpacecraftData ) :
	m_flResetTimer( 0.0f ),
	m_flFlashTimer( 0.0f ),
	m_flWaveVisibilityAmount( 0.0f ),
	m_bWaveFormActive( false ),
	m_iWaveFormReadStart( 0 )
{
	m_pLabelHeader = new Label( this, "", "#holo_gui_comm_online" );

	SetAngles( QAngle( 0, 160, 0 ) );

	m_flScale = 0.018f;

	ADD_HOLO_MESSAGE( HoloMessage );
}

CHoloShipComm::~CHoloShipComm()
{
	REMOVE_HOLO_MESSAGE( HoloMessage );
}

void CHoloShipComm::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pLabelHeader->SetContentAlignment( Label::a_east );
	m_pLabelHeader->SetFont( m_FontLarge );
	m_pLabelHeader->SetFgColor( Color( HOLO_COLOR255_DEFAULT, 127 ) );
	m_pLabelHeader->SizeToContents();
	m_pLabelHeader->SetWide( 400 );

	SetBounds( 0, 0, 400, m_pLabelHeader->GetTall() );
}

void CHoloShipComm::MsgFunc_HoloMessage( bf_read &msg )
{
	char szDisplayName[ 128 ];
	char szWaveName[ 128 ];
	msg.ReadString( szDisplayName, sizeof( szDisplayName ) );
	msg.ReadString( szWaveName, sizeof( szWaveName ) );
	const float flDuration = msg.ReadFloat();

	//Msg( "Holo message received: %s, %s, %f\n", szDisplayName, szWaveName, flDuration );

	wchar_t wszFormatted[ 64 ];
	g_pVGuiLocalize->ConstructString( wszFormatted, sizeof( wszFormatted ),
		SafeLocalizeInline( "#holo_gui_comm_incoming" ).Get(),
		1, SafeLocalizeInline( szDisplayName ).Get() );
	m_pLabelHeader->SetText( wszFormatted );

	m_flResetTimer = flDuration;
	m_flFlashTimer = 0.5f;

	ResetWaveForm( szWaveName );
}

void CHoloShipComm::Draw( IMatRenderContext *pRenderContext )
{
	m_vecPanelWorldOffset.x = -m_flWidth * m_flScale;

	BaseClass::Draw( pRenderContext );

	GetColorVar()->SetVecValue( HOLO_COLOR_HIGHLIGHT );
	GetAlphaVar()->SetFloatValue( 1.0f );

	const float flTriangleStart = -0.5f;
	const float flTriangleHeight = 0.03f;
	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, 0, 0, GetMaterial() );
	CreateSlantedRect( pMesh, 0, flTriangleStart, m_flWidth * -m_flScale, flTriangleHeight );

	//CMeshBuilder builder;
	//builder.Begin( pMesh, MATERIAL_TRIANGLES, 1 );

	//builder.Position3f( 0, 0, flTriangleStart );
	//builder.AdvanceVertex();

	//builder.Position3f( 0, 0, flTriangleStart + flTriangleHeight );
	//builder.AdvanceVertex();

	//builder.Position3f( 0, -m_flWidth * m_flScale, flTriangleStart );
	//builder.AdvanceVertex();

	//builder.End();
	pMesh->Draw();

	if ( m_flWaveVisibilityAmount > 0.0f )
	{
		DrawWaveForm( pRenderContext );
	}
}

void CHoloShipComm::Think( float frametime )
{
	BaseClass::Think( frametime );

	if ( m_flResetTimer > 0.0f )
	{
		m_flResetTimer -= frametime;
		if ( m_flResetTimer <= 0.0f )
		{
			m_flResetTimer = 0.0f;
			m_pLabelHeader->SetText( "#holo_gui_comm_online" );
			m_flFlashTimer = 0.25f;
		}
	}

	if ( m_flFlashTimer > 0.0f )
	{
		float alpha;
		m_flFlashTimer -= frametime;
		if ( m_flFlashTimer <= 0.0f )
		{
			m_flFlashTimer = 0.0f;
			alpha = 0.5f;
		}
		else
		{
			alpha = sin( m_flFlashTimer * M_PI * 8.0f ) * 0.5f + 0.5f;
		}
		m_pLabelHeader->SetFgColor( Color( HOLO_COLOR255_DEFAULT, 255 * alpha ) );
	}

	const float flDesiredVisibility = m_flResetTimer > 0.0f ? 1.0f : 0.0f;
	m_flWaveVisibilityAmount = Approach( flDesiredVisibility, m_flWaveVisibilityAmount, frametime * 3.0f );

	if ( m_bWaveFormActive )
	{
		UpdateWaveForm( frametime );
	}
}

void CHoloShipComm::PerformLayout3D( int width, int height, bool useVR )
{
	float flYPos = -11.0f;
	if ( !useVR )
	{
		const float flAspectRatio = width / float( height );
		if ( flAspectRatio < 16.0f / 9.0f )
		{
			flYPos += 6.25f * ( 16.0f / 9.0f - flAspectRatio );
		}
	}
	SetOrigin( Vector( -10.0f, flYPos, 5.9f ) );
}

void CHoloShipComm::DrawWaveForm( IMatRenderContext *pRenderContext )
{
	const float flAlpha = m_flWaveVisibilityAmount < 0.4f ? sin( m_flWaveVisibilityAmount * 20.0f ) * 0.5f + 0.5f : 1.0f;

	GetColorVar()->SetVecValue( HOLO_COLOR_DEFAULT );
	GetAlphaVar()->SetFloatValue( flAlpha );

	const float flBarrierFraction = RemapValClamped( m_flWaveVisibilityAmount, 0.4f, 1, 0, 1 );
	const float flYCenter = m_vecPanelWorldOffset.x * 0.5f;
	const float flCenterSpacing = 0.1f;
	const float flOverlap = 0.16f;
	const float flLineWidth = 0.03f;
	const float flHeight = 0.8f;
	const float flYPosLeft = flYCenter - flCenterSpacing + ( flYCenter + flCenterSpacing ) * flBarrierFraction;
	const float flYPosRight = flYCenter + flCenterSpacing - ( flYCenter + flCenterSpacing ) * flBarrierFraction;

	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, 0, 0, GetMaterial() );
	CMeshBuilder builder;

	builder.Begin( pMesh, MATERIAL_QUADS, 4 );

	CreateSlantedRect( builder, flYPosLeft, -0.6f, flLineWidth, -flHeight + flLineWidth * 2.0f );
	CreateSlantedRect( builder, flYPosLeft + flLineWidth, -0.6f, flOverlap, -flLineWidth );

	CreateSlantedRect( builder, flYPosRight, -0.6f - flLineWidth * 2.0f, -flLineWidth, -flHeight + flLineWidth * 2.0f );
	CreateSlantedRect( builder, flYPosRight - flLineWidth, -0.6f - flHeight, -flOverlap, flLineWidth );

	builder.End();

	pMesh->Draw();

	const float flWavePadding = 0.08f;
	if ( flYPosRight - flYPosLeft > flWavePadding * 2.0f )
	{
		pRenderContext->ClearBuffers( false, false, true );
		pRenderContext->SetStencilEnable( true );
		pRenderContext->SetStencilWriteMask( 1 );
		pRenderContext->SetStencilTestMask( 1 );
		pRenderContext->SetStencilReferenceValue( 1 );
		pRenderContext->SetStencilCompareFunction( STENCILCOMPARISONFUNCTION_NEVER );
		pRenderContext->SetStencilFailOperation( STENCILOPERATION_REPLACE );
		pRenderContext->SetStencilPassOperation( STENCILOPERATION_KEEP );
		pRenderContext->SetStencilZFailOperation( STENCILOPERATION_KEEP );

		// Draw clip geometry
		pMesh = pRenderContext->GetDynamicMesh( true, 0, 0, GetMaterial() );
		builder.Begin( pMesh, MATERIAL_QUADS, 1 );
		CreateSlantedRect( builder, flYPosLeft + flWavePadding, -0.6f, flYPosRight - flYPosLeft - flWavePadding * 2.0f, -1.0f );
		builder.End();
		pMesh->Draw();

		pRenderContext->SetStencilCompareFunction( STENCILCOMPARISONFUNCTION_EQUAL );

		pRenderContext->SetStencilWriteMask( 0 );

		// Draw clipped waveform
		GetAlphaVar()->SetFloatValue( 0.2f );
		pMesh = pRenderContext->GetDynamicMesh( true, 0, 0, GetMaterial() );
		builder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, WAVEFORM_LENGTH * 2 );

		const float flStartX = m_vecPanelWorldOffset.x + flWavePadding;
		const float flWidth = abs( m_vecPanelWorldOffset.x ) - flWavePadding * 2.0f;
		const float flStepSize = WAVEFORM_LENGTH - 1;

		builder.Position3f( 0.0f, flStartX, -1 );
		builder.AdvanceVertex();

		builder.Position3f( 0.0f, flStartX, -1.05f );
		builder.AdvanceVertex();

		for ( int i = 0; i < WAVEFORM_LENGTH; ++i )
		{
			float xpos = flStartX + flWidth * ( i / flStepSize );
			float yOffset = m_flWaveFormData[ ( m_iWaveFormReadStart + i ) % WAVEFORM_LENGTH ] * 0.4f;

			builder.Position3f( 0.0f, xpos, -1 + yOffset );
			builder.AdvanceVertex();

			builder.Position3f( 0.0f, xpos, -1.05f + yOffset );
			builder.AdvanceVertex();
		}

		builder.End();
		pMesh->Draw();

		pRenderContext->SetStencilEnable( false );
	}
}

void CHoloShipComm::ResetWaveForm( const char *pszWaveFile )
{
	m_iWaveFormReadStart = 0;
	m_flWaveFormSampleTime = 0.0f;
	m_iWaveFormSamples = 0;
	m_iRawSamplesRead = 0;
	Plat_FastMemset( m_flWaveFormData, 0, sizeof( m_flWaveFormData ) );
	m_WaveBuffer.Clear();
	m_bWaveFormActive = g_pFullFileSystem->ReadFile( VarArgs( "sound/%s", pszWaveFile ), "GAME", m_WaveBuffer );

	if ( m_bWaveFormActive )
	{
		//m_WaveBuffer.GetObjects( &m_RawWaveHeader );
		m_RawWaveHeader.chunk_id[ 0 ] = m_WaveBuffer.GetChar();
		m_RawWaveHeader.chunk_id[ 1 ] = m_WaveBuffer.GetChar();
		m_RawWaveHeader.chunk_id[ 2 ] = m_WaveBuffer.GetChar();
		m_RawWaveHeader.chunk_id[ 3 ] = m_WaveBuffer.GetChar();
		m_RawWaveHeader.chunk_size = m_WaveBuffer.GetInt();
		m_RawWaveHeader.format[ 0 ] = m_WaveBuffer.GetChar();
		m_RawWaveHeader.format[ 1 ] = m_WaveBuffer.GetChar();
		m_RawWaveHeader.format[ 2 ] = m_WaveBuffer.GetChar();
		m_RawWaveHeader.format[ 3 ] = m_WaveBuffer.GetChar();
		m_RawWaveHeader.subchunk1_id[ 0 ] = m_WaveBuffer.GetChar();
		m_RawWaveHeader.subchunk1_id[ 1 ] = m_WaveBuffer.GetChar();
		m_RawWaveHeader.subchunk1_id[ 2 ] = m_WaveBuffer.GetChar();
		m_RawWaveHeader.subchunk1_id[ 3 ] = m_WaveBuffer.GetChar();
		m_RawWaveHeader.subchunk1_size = m_WaveBuffer.GetInt();
		m_RawWaveHeader.audio_format = m_WaveBuffer.GetShort();
		m_RawWaveHeader.num_channels = m_WaveBuffer.GetShort();
		m_RawWaveHeader.sample_rate = m_WaveBuffer.GetInt();
		m_RawWaveHeader.byte_rate = m_WaveBuffer.GetInt();
		m_RawWaveHeader.block_align = m_WaveBuffer.GetShort();
		m_RawWaveHeader.bits_per_sample = m_WaveBuffer.GetShort();

		if ( m_RawWaveHeader.subchunk1_size > 16 )
		{
			m_WaveBuffer.SeekGet( CUtlBuffer::SEEK_CURRENT, m_RawWaveHeader.subchunk1_size - 16 );
		}

		m_RawWaveHeader.subchunk2_id[ 0 ] = m_WaveBuffer.GetChar();
		m_RawWaveHeader.subchunk2_id[ 1 ] = m_WaveBuffer.GetChar();
		m_RawWaveHeader.subchunk2_id[ 2 ] = m_WaveBuffer.GetChar();
		m_RawWaveHeader.subchunk2_id[ 3 ] = m_WaveBuffer.GetChar();
		m_RawWaveHeader.subchunk2_size = m_WaveBuffer.GetInt();

		if ( m_RawWaveHeader.num_channels != 1 )
		{
			Error( VarArgs( "%s has %i channels, must have only 1.", pszWaveFile, m_RawWaveHeader.num_channels ) );
		}

		if ( m_RawWaveHeader.audio_format != 1 )
		{
			Error( VarArgs( "%s does not use PCM format (%i).", pszWaveFile, m_RawWaveHeader.audio_format ) );
		}
	}
}

void CHoloShipComm::UpdateWaveForm( float frametime )
{
	m_flWaveFormSampleTime += frametime;
	const int iExpectedSamples = m_RawWaveHeader.sample_rate * 0.5f * m_flWaveFormSampleTime;

	const int iExpectedRawSamples = m_RawWaveHeader.sample_rate * m_flWaveFormSampleTime;
	const int iSampleSize = m_RawWaveHeader.bits_per_sample / 8;
	const int iTotalSampleOffset = ( iExpectedRawSamples - m_iRawSamplesRead );

	int iOffsetBytesApplied = 0;
	int iOffsetIncrementsApplied = 0;
	const int iSampleSteps = iExpectedSamples - m_iWaveFormSamples;

	while ( m_iWaveFormSamples < iExpectedSamples )
	{
		if ( m_WaveBuffer.GetBytesRemaining() > 4 )
		{
			const float flSample = m_RawWaveHeader.bits_per_sample == 8 ? ((unsigned char)(m_WaveBuffer.GetChar()) - 128.0f ) / 128.0f : m_WaveBuffer.GetShort() / 32768.0f;
			m_flWaveFormData[ m_iWaveFormReadStart ] = clamp( flSample * 2.0f, -1, 1 );
		}
		else
		{
			m_flWaveFormData[ m_iWaveFormReadStart ] = 0.0f;
		}

		++iOffsetIncrementsApplied;

		int iDesiredOffset = iTotalSampleOffset * iOffsetIncrementsApplied / (float)iSampleSteps - iSampleSize;
		iDesiredOffset = MAX( 0, iDesiredOffset ) * iSampleSize;
		m_WaveBuffer.SeekGet( CUtlBuffer::SEEK_CURRENT, iDesiredOffset - iOffsetBytesApplied );
		iOffsetBytesApplied = iDesiredOffset + iSampleSize;

		m_iWaveFormReadStart = ( m_iWaveFormReadStart + 1 ) % WAVEFORM_LENGTH;
		++m_iWaveFormSamples;
	}

	m_iRawSamplesRead = iExpectedRawSamples;

	if ( m_flWaveVisibilityAmount <= 0.0f )
	{
		m_bWaveFormActive = false;
	}
}
