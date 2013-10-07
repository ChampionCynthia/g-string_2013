
#include "cbase.h"
#include "c_clientpartialragdoll.h"
#include "c_gstring_util.h"
#include "c_gibconfig.h"

#include "jigglebones.h"
#include "viewrender.h"
#include "gamestringpool.h"


BEGIN_DATADESC( C_ClientPartialRagdoll )

	DEFINE_FIELD( m_bShrinking, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIsPartial, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_strRecursiveParent, FIELD_STRING ),
	DEFINE_ARRAY( m_iBoneFlagStorage, FIELD_INTEGER, 12 ),
	DEFINE_FIELD( m_iBranchRootBone, FIELD_INTEGER ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( client_ragdoll_partial, C_ClientPartialRagdoll );


C_ClientPartialRagdoll::C_ClientPartialRagdoll( bool bRestoring )
	: BaseClass( bRestoring )
	, m_bShrinking( true )
	, m_bIsPartial( false )
	, m_iBranchRootBone( -1 )
{
	SetClassname( "client_ragdoll_partial" );

	Q_memset( m_iBoneFlagStorage, 0, sizeof( m_iBoneFlagStorage ) );
}

void C_ClientPartialRagdoll::SetShrinkingEnabled( bool bEnable )
{
	m_bShrinking = bEnable;
}

void C_ClientPartialRagdoll::SetRecursiveGibData( const char *pszParentName )
{
	Assert( pszParentName && *pszParentName );

	m_strRecursiveParent = AllocPooledString( pszParentName );
}

void C_ClientPartialRagdoll::OnRestore()
{
	BaseClass::OnRestore();

	Q_memcpy( m_normalBones.Base(), m_iBoneFlagStorage, sizeof( int ) * 4 );
	Q_memcpy( m_trunkBones.Base(), m_iBoneFlagStorage + 4, sizeof( int ) * 4 );
	Q_memcpy( m_jointBones.Base(), m_iBoneFlagStorage + 8, sizeof( int ) * 4 );
}

void C_ClientPartialRagdoll::ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName )
{
	BaseClass::ImpactTrace( pTrace, iDamageType, pCustomImpactName );

	if ( m_bReleaseRagdoll )
		return;

	int iStudioBone = ConvertPhysBoneToStudioBone( this, pTrace->physicsbone );

	if ( iStudioBone >= 0
		&& !m_jointBones.IsBitSet( iStudioBone ) )
	{
		matrix3x4_t boneDelta0[MAXSTUDIOBONES];
		matrix3x4_t boneDelta1[MAXSTUDIOBONES];
		matrix3x4_t currentBones[MAXSTUDIOBONES];
		const float boneDt = 0.1f;

		SetShrinkingEnabled( false );
		GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
		SetShrinkingEnabled( true );

		CUtlVector< ragdollparams_partial_t > gibModels;

		GibbingParamsRecursive_t params;
		params.pHdr = GetModelPtr();
		params.pszHitBone = GetModelPtr()->pBone( iStudioBone )->pszName();
		params.pszParentName = STRING( m_strRecursiveParent );

		const char *pszParentSplitBone;

		if ( C_GibConfig::GetInstance()->GetGibsForGroup( params, gibModels, &pszParentSplitBone ) )
		{
			if ( m_jointBones.IsBitSet( Studio_BoneIndexByName( GetModelPtr(), pszParentSplitBone ) ) )
				return;

			FOR_EACH_VEC( gibModels, i )
			{
				ragdollparams_partial_t &partial = gibModels[ i ];

				// add old trunk joints
				for ( int iBit = m_jointBones.FindNextSetBit( 0 );
					iBit >= 0; iBit = m_jointBones.FindNextSetBit( iBit + 1 ) )
				{
					if ( m_iBranchRootBone == iBit )
						continue;

					const char *pszName = GetModelPtr()->pBone( iBit )->pszName();

					partial.trunkBones.AddToTail( pszName );
				}

				// if we create a trunk from an existing branch
				// we have to propagate the branch root bone
				if ( partial.branchBones.Count() == 0
					&& m_iBranchRootBone >= 0 )
				{
					partial.branchBones.AddToTail( GetModelPtr()->pBone( m_iBranchRootBone )->pszName() );
				}

				C_BaseAnimating *pGib = CreateRagdollCopy( i == 0 );

				C_ClientPartialRagdoll *pRecursiveRagdoll = dynamic_cast< C_ClientPartialRagdoll* >( pGib );

				Assert( pRecursiveRagdoll );

				if ( pRecursiveRagdoll != NULL )
				{
					pRecursiveRagdoll->SetRecursiveGibData( STRING( m_strRecursiveParent ) );
					pRecursiveRagdoll->SetShrinkingEnabled( true );

					pRecursiveRagdoll->m_vecForce = ( pTrace->endpos - pTrace->startpos ).Normalized() * RandomFloat( 25000.0f, 50000.0f );
					pRecursiveRagdoll->m_nForceBone = pTrace->physicsbone;
				}

				pGib->InitAsClientRagdoll( boneDelta0, boneDelta1, currentBones, boneDt, false, &partial );
			}

			m_bReleaseRagdoll = true;
			SetNextClientThink( CLIENT_THINK_ALWAYS );
		}
	}
}

bool C_ClientPartialRagdoll::TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr )
{
	if ( !m_bIsPartial )
		return BaseClass::TestHitboxes( ray, fContentsMask, tr );

	MDLCACHE_CRITICAL_SECTION();

	CStudioHdr *pStudioHdr = GetModelPtr();
	if (!pStudioHdr)
		return false;

	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( m_nHitboxSet );
	if ( !set || !set->numhitboxes )
		return false;

	// Use vcollide for box traces.
	if ( !ray.m_IsRay )
		return false;

	// This *has* to be true for the existing code to function correctly.
	Assert( ray.m_StartOffset == vec3_origin );

	CBoneCache *pCache = GetBoneCache( pStudioHdr );
	matrix3x4_t *hitboxbones[MAXSTUDIOBONES];
	pCache->ReadCachedBonePointers( hitboxbones, pStudioHdr->numbones() );

	// we need to mask hidden hitboxes out of the studio data here :(
	struct hitboxsetoverride_t : public mstudiohitboxset_t
	{
		mstudiobbox_t hitboxes[ MAXSTUDIOBONES ];
	};

	hitboxsetoverride_t partialSet;
	partialSet.hitboxindex = offsetof( hitboxsetoverride_t, hitboxes );
	partialSet.numhitboxes = 0;

	for ( int h = 0; h < set->numhitboxes; h++ )
	{
		if ( m_normalBones.IsBitSet( set->pHitbox( h )->bone ) )
		{
			// some data here becomes invalid and could potentially lead to crashes
			partialSet.hitboxes[ partialSet.numhitboxes++ ] = *set->pHitbox( h );

			//Msg( "adding hitbox: %s (%i)\n", pStudioHdr->pBone( set->pHitbox( h )->bone )->pszName(), set->pHitbox( h )->bone );
		}
	}

	if ( TraceToStudio( physprops, ray, pStudioHdr, &partialSet, hitboxbones, fContentsMask, GetRenderOrigin(), GetModelScale(), tr ) )
	//if ( TraceToStudio( physprops, ray, pStudioHdr, set, hitboxbones, fContentsMask, GetRenderOrigin(), GetModelScale(), tr ) )
	{
		mstudiobbox_t *pbox = partialSet.pHitbox( tr.hitbox );
		mstudiobone_t *pBone = pStudioHdr->pBone(pbox->bone);

		//Msg( "hit: %s\n", pBone->pszName() );

		// better safe than sorry
		if ( !m_normalBones.IsBitSet( pbox->bone ) )
		{
			return false;
		}

		tr.surface.name = "**studio**";
		tr.surface.flags = SURF_HITBOX;
		tr.surface.surfaceProps = physprops->GetSurfaceIndex( pBone->pszSurfaceProp() );
		if ( IsRagdoll() )
		{
			IPhysicsObject *pReplace = m_pRagdoll->GetElement( tr.physicsbone );
			if ( pReplace )
			{
				VPhysicsSetObject( NULL );
				VPhysicsSetObject( pReplace );
			}
		}
	}

	return true;
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

	Assert( pHdr );

	if ( pHdr != NULL )
	{
		m_iBranchRootBone = -1;

		// cut all bones that aren't part of this 'branch'
		// i.e. spine is not part of arm branch
		CUtlVector< int > tempNormalBones;

		Assert( params.branchBones.Count() <= 1 );

		FOR_EACH_VEC( params.branchBones, b )
		{
			const char *pszBranch = params.branchBones[ b ].Get();

			int iBone = LookupBone( pszBranch );

			if ( m_iBranchRootBone < 0 )
			{
				m_iBranchRootBone = iBone;
			}

			m_jointBones.Set( iBone );

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
					tempNormalBones.AddToTail( LookupBone( pszBone ) );
				}
			}
		}

		// cut all bones that aren't part of this trunk
		// i.e. arm isn't part of pelvis/spline trunk

		FOR_EACH_VEC( params.trunkBones, t )
		{
			const char *pszTrunk = params.trunkBones[ t ].Get();

			m_jointBones.Set( LookupBone( pszTrunk ) );


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
					tempNormalBones.AddToTail( LookupBone( pszBone ) );
				}
			}
		}

		FOR_EACH_VEC( tempNormalBones, i )
		{
			if ( !m_trunkBones.IsBitSet( tempNormalBones[ i ] ) )
				m_normalBones.Set( tempNormalBones[ i ] );
		}

		if ( m_iBranchRootBone >= 0 )
		{
			for ( int i = 0; i < pHdr->numbones(); i++ )
			{
				if ( BoneParentDepth( GetModelPtr(), i, m_iBranchRootBone ) < 0 )
				{
					m_normalBones.Clear( i );
				}
			}
		}
	}

	Q_memcpy( m_iBoneFlagStorage, m_normalBones.Base(), sizeof( int ) * 4 );
	Q_memcpy( m_iBoneFlagStorage + 4, m_trunkBones.Base(), sizeof( int ) * 4 );
	Q_memcpy( m_iBoneFlagStorage + 8, m_jointBones.Base(), sizeof( int ) * 4 );

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

			if ( i == m_iBranchRootBone )
			{
				rootBone = GetBone( i );
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
				&& m_iBranchRootBone >= 0 )
			{
				GetBoneForWrite( i ) = rootBone;
				MatrixScaleBy( 0.01f, GetBoneForWrite( i ) );
			}
		}
	}
}
