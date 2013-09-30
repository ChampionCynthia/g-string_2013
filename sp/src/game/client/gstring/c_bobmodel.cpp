
#include "cbase.h"
#include "c_bobmodel.h"


C_BobModel::C_BobModel()
	: m_bDirty( false )
	, m_angIdle( vec3_angle )
	, m_posIdle( vec3_origin )
	, m_bInvalid( false )
	, m_iBoneFollow( -1 )
	, m_iAttachmentFollow( 1 )
	, m_flScale( 1.0f )
	, m_flReferenceCycle( 0.0f )
	, m_angOrientation( vec3_angle )
	, m_angOffset( vec3_angle )
{
}

CStudioHdr *C_BobModel::OnNewModel()
{
	m_bDirty = true;

	return BaseClass::OnNewModel();
}

bool C_BobModel::IsDirty() const
{
	return m_bDirty;
}

void C_BobModel::SetDirty( bool bDirty )
{
	m_bDirty = bDirty;
}

bool C_BobModel::IsInvalid() const
{
	return m_bInvalid;
}

void C_BobModel::UpdateDefaultTransforms()
{
	int iSequence = SelectWeightedSequence( ACT_VM_IDLE );

	if ( iSequence < 0 )
	{
		m_angIdle = vec3_angle;
		m_posIdle = vec3_origin;

		m_bInvalid = true;
		return;
	}

	SetSequence( iSequence );
	SetCycle( m_flReferenceCycle );

	Vector vecPos;
	QAngle ang;

	if ( GetTransforms( vecPos, ang ) )
	{
		m_posIdle = vecPos;
		m_angIdle = ang;
		m_bInvalid = false;
	}
	else
	{
		m_bInvalid = true;
	}
}

void C_BobModel::GetDeltaTransforms( QAngle &angDelta )
{
	Assert( !m_bInvalid );

	Vector pos;

	GetTransforms( pos, angDelta );

	for ( int i = 0; i < 3; i++ )
	{
		angDelta[ i ] = AngleDiff( angDelta[ i ], m_angIdle[ i ] );
	}

	angDelta *= m_flScale;
}

bool C_BobModel::GetTransforms( Vector &pos, QAngle &ang )
{
	matrix3x4_t mat;

	if ( m_iBoneFollow >= 0 )
	{
		if ( !GetModelPtr()
			|| GetModelPtr()->numbones() <= m_iBoneFollow )
			return false;

		SetupBones( NULL, -1, BONE_USED_BY_ANYTHING, gpGlobals->curtime );

		mat = GetBone( m_iBoneFollow );
	}
	else
	{
		if ( !GetModelPtr()
			|| GetModelPtr()->GetNumAttachments() < m_iAttachmentFollow )
			return false;

		if ( !GetAttachment( m_iAttachmentFollow, mat ) )
			return false;
	}

	Vector f, r, u;
	MatrixAngles( mat, ang, pos );
	AngleVectors( ang, &f, &r, &u );

	matrix3x4_t rot, tmp;
	MatrixBuildRotationAboutAxis( f, m_angOrientation.x, rot );
	ConcatTransforms( rot, mat, tmp );
	MatrixBuildRotationAboutAxis( r, m_angOrientation.y, rot );
	ConcatTransforms( rot, tmp, mat );
	MatrixBuildRotationAboutAxis( u, m_angOrientation.z, rot );
	ConcatTransforms( rot, mat, tmp );

	MatrixAngles( tmp, ang, pos );
	return true;
}

void C_BobModel::SetAttachmentInfo( const FileWeaponInfo_t &info )
{
	m_iBoneFollow = LookupBone( info.szCameraBoneName );
	m_iAttachmentFollow = LookupAttachment( info.szCameraAttachmentName );

	if ( m_iBoneFollow < 0
		&& m_iAttachmentFollow <= 0 )
	{
		m_iAttachmentFollow = 1;
	}

	m_flScale = info.flCameraMovementScale;
	m_flReferenceCycle = info.flCameraMovementReferenceCycle;

	m_angOrientation = info.angCameraMovementOrientation;
	m_angOffset = info.angCameraMovementOffset;
}