
#include "cbase.h"
#include "c_clientpartialragdoll.h"
#include "c_gstring_util.h"

#include "jigglebones.h"
#include "viewrender.h"


C_ClientPartialRagdoll::C_ClientPartialRagdoll()
	: BaseClass( false )
	, m_bShrinking( true )
	, m_bIsPartial( false )
{
}

CStudioHdr *C_ClientPartialRagdoll::OnNewModel()
{
	CStudioHdr *ret = BaseClass::OnNewModel();

	return ret;
}

void C_ClientPartialRagdoll::SetShrinkingEnabled( bool bEnable )
{
	m_bShrinking = bEnable;
}

bool C_ClientPartialRagdoll::InitAsClientRagdoll( const matrix3x4_t *pDeltaBones0, const matrix3x4_t *pDeltaBones1,
		const matrix3x4_t *pCurrentBonePosition, float boneDt, bool bFixedConstraints, ragdollparams_partial_t *pPartialParams )
{
	if ( pPartialParams != NULL )
	{
		BuildPartial( *pPartialParams );

		m_bIsPartial = true;
	}

	return BaseClass::InitAsClientRagdoll( pDeltaBones0, pDeltaBones1, pCurrentBonePosition, boneDt, bFixedConstraints, pPartialParams );
}

void C_ClientPartialRagdoll::BuildPartial( ragdollparams_partial_t &params )
{
	CStudioHdr *pHdr = GetModelPtr();

	if ( pHdr != NULL )
	{
		FOR_EACH_VEC( params.branchBones, b )
		{
			const char *pszBranch = params.branchBones[ b ].Get();

			for ( int i = 0; i < pHdr->numbones(); i++ )
			{
				const char *pszBone = pHdr->pBone( i )->pszName();

				if ( !BoneHasParent( pHdr, pszBone, pszBranch )
					&& Q_stricmp( pszBone, pszBranch ) != 0 )
				{
					if ( !params.cutBones.HasElement( pszBone ) )
					{
						params.cutBones.AddToTail( pszBone );
					}
				}
				else
				{
					m_normalBones.Set( LookupBone( pszBone ) );
				}
			}
		}

		FOR_EACH_VEC( params.trunkBones, t )
		{
			const char *pszTrunk = params.trunkBones[ t ].Get();

			for ( int i = 0; i < pHdr->numbones(); i++ )
			{
				const char *pszBone = pHdr->pBone( i )->pszName();

				if ( BoneParentDepth( pHdr, pszBone, pszTrunk ) >= 0 )
				{
					if ( !params.cutBones.HasElement( pszBone ) )
					{
						params.cutBones.AddToTail( pszBone );

						m_trunkBones.Set( LookupBone( pszBone ) );
					}
				}
				else
				{
					m_normalBones.Set( LookupBone( pszBone ) );
				}
			}
		}
	}

	Assert( m_normalBones.FindNextSetBit( 0 ) >= 0 );
}

void C_ClientPartialRagdoll::BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion *q,
		const matrix3x4_t &cameraTransform, int boneMask, CBoneBitList &boneComputed )
{
	if ( !hdr )
		return;

	matrix3x4_t bonematrix;
	bool boneSimulated[MAXSTUDIOBONES];

	// no bones have been simulated
	memset( boneSimulated, 0, sizeof(boneSimulated) );
	mstudiobone_t *pbones = hdr->pBone( 0 );

	if ( m_pRagdoll )
	{
		// simulate bones and update flags
		int oldWritableBones = m_BoneAccessor.GetWritableBones();
		int oldReadableBones = m_BoneAccessor.GetReadableBones();
		m_BoneAccessor.SetWritableBones( BONE_USED_BY_ANYTHING );
		m_BoneAccessor.SetReadableBones( BONE_USED_BY_ANYTHING );
		
#if defined( REPLAY_ENABLED )
		// If we're playing back a demo, override the ragdoll bones with cached version if available - otherwise, simulate.
		if ( ( !engine->IsPlayingDemo() && !engine->IsPlayingTimeDemo() ) ||
				!CReplayRagdollCache::Instance().IsInitialized() ||
				!CReplayRagdollCache::Instance().GetFrame( this, engine->GetDemoPlaybackTick(), boneSimulated, &m_BoneAccessor ) )
#endif
		{
			m_pRagdoll->RagdollBone( this, pbones, hdr->numbones(), boneSimulated, m_BoneAccessor );
		}
		
		m_BoneAccessor.SetWritableBones( oldWritableBones );
		m_BoneAccessor.SetReadableBones( oldReadableBones );
	}

	// For EF_BONEMERGE entities, copy the bone matrices for any bones that have matching names.
	bool boneMerge = IsEffectActive(EF_BONEMERGE);
	if ( boneMerge || m_pBoneMergeCache )
	{
		if ( boneMerge )
		{
			if ( !m_pBoneMergeCache )
			{
				m_pBoneMergeCache = new CBoneMergeCache;
				m_pBoneMergeCache->Init( this );
			}
			m_pBoneMergeCache->MergeMatchingBones( boneMask );
		}
		else
		{
			delete m_pBoneMergeCache;
			m_pBoneMergeCache = NULL;
		}
	}

	matrix3x4_t rootBone;
	bool bHasRoot = false;
	bool bIsShrinking = m_bShrinking && m_bIsPartial;

	for (int i = 0; i < hdr->numbones(); i++) 
	{
		// Only update bones reference by the bone mask.
		if ( ( hdr->boneFlags( i ) & boneMask ) == 0 )
		{
			continue;
		}

		if ( bIsShrinking )
		{
			if ( !m_normalBones.IsBitSet( i )
				&& !m_trunkBones.IsBitSet( i ) )
			{
				SetIdentityMatrix( GetBoneForWrite( i ) );
				continue;
			}

			if ( !bHasRoot )
			{
				rootBone = GetBone( i );
				bHasRoot = true;
			}
		}

		if ( m_pBoneMergeCache && m_pBoneMergeCache->IsBoneMerged( i ) )
			continue;

		// animate all non-simulated bones
		if ( boneSimulated[i] || CalcProceduralBone( hdr, i, m_BoneAccessor ))
		{
			continue;
		}
		// skip bones that the IK has already setup
		else if (boneComputed.IsBoneMarked( i ))
		{
			// dummy operation, just used to verify in debug that this should have happened
			GetBoneForWrite( i );
		}
		else
		{
			QuaternionMatrix( q[i], pos[i], bonematrix );

			Assert( fabs( pos[i].x ) < 100000 );
			Assert( fabs( pos[i].y ) < 100000 );
			Assert( fabs( pos[i].z ) < 100000 );

			if ( (hdr->boneFlags( i ) & BONE_ALWAYS_PROCEDURAL) && 
					(hdr->pBone( i )->proctype & STUDIO_PROC_JIGGLE) )
			{
				//
				// Physics-based "jiggle" bone
				// Bone is assumed to be along the Z axis
				// Pitch around X, yaw around Y
				//

				// compute desired bone orientation
				matrix3x4_t goalMX;

				if (pbones[i].parent == -1) 
				{
					ConcatTransforms( cameraTransform, bonematrix, goalMX );
				} 
				else 
				{
					ConcatTransforms( GetBone( pbones[i].parent ), bonematrix, goalMX );
				}

				// get jiggle properties from QC data
				mstudiojigglebone_t *jiggleInfo = (mstudiojigglebone_t *)pbones[i].pProcedure( );

				if (!m_pJiggleBones)
				{
					m_pJiggleBones = new CJiggleBones;
				}

				// do jiggle physics
				m_pJiggleBones->BuildJiggleTransformations( i, gpGlobals->realtime, jiggleInfo, goalMX, GetBoneForWrite( i ) );

			}
			else if (hdr->boneParent(i) == -1) 
			{
				ConcatTransforms( cameraTransform, bonematrix, GetBoneForWrite( i ) );
			} 
			else 
			{
				ConcatTransforms( GetBone( hdr->boneParent(i) ), bonematrix, GetBoneForWrite( i ) );
			}
		}

		if ( hdr->boneParent(i) >= 0
			&& !m_normalBones.IsBitSet( i )
			&& bIsShrinking )
		{
			MatrixScaleBy( 0.01f, GetBoneForWrite( i ) );
		}
	}

	if ( bIsShrinking )
	{
		for ( int i = 0; i < hdr->numbones(); i++ )
		{
			if ( ( hdr->boneFlags( i ) & boneMask ) == 0 )
			{
				continue;
			}

			if ( !m_normalBones.IsBitSet( i )
				&& !m_trunkBones.IsBitSet( i )
				&& bHasRoot )
			{
				GetBoneForWrite( i ) = rootBone;
				MatrixScaleBy( 0.01f, GetBoneForWrite( i ) );
			}
		}
	}
}
