
#include "cbase.h"
#include "c_clientpartialragdoll.h"
#include "c_gstring_util.h"
#include "c_gibconfig.h"
#include "cgore_generator.h"

#include "jigglebones.h"
#include "viewrender.h"
#include "gamestringpool.h"
#include "decals.h"
#include "itempents.h"

void DispatchGibParticle( C_BaseAnimating *pEntity, const char *pszBone, bool bExplosion, int iBloodType )
{
	if ( iBloodType != BLOOD_COLOR_RED )
		return;

	if ( pszBone == NULL )
		return;

	const char *pszParticle = "blood_advisor_puncture_withdraw";

	if ( bExplosion )
	{
		const char *pszExplosionParticles[] = {
			"blood_advisor_spray_strong",
			"blood_advisor_spray_strong_2",
			"blood_advisor_spray_strong_3",
		};

		pszParticle = pszExplosionParticles[ RandomInt( 0, ARRAYSIZE( pszExplosionParticles ) - 1 ) ];
	}

	DispatchParticleEffect( pszParticle, PATTACH_BONE_FOLLOW, pEntity, pszBone );
}

BEGIN_DATADESC( C_ClientPartialRagdoll )

	DEFINE_FIELD( m_bShrinking, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIsPartial, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_strRecursiveParent, FIELD_STRING ),
	DEFINE_FIELD( m_strRecursiveGoreName, FIELD_STRING ),
	DEFINE_FIELD( m_strRecursiveGoreMaterialName, FIELD_STRING ),
	DEFINE_ARRAY( m_iBoneFlagStorage, FIELD_INTEGER, 12 ),
	DEFINE_FIELD( m_iBranchRootBone, FIELD_INTEGER ),
	DEFINE_FIELD( m_iBloodColor, FIELD_INTEGER ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( client_ragdoll_partial, C_ClientPartialRagdoll );

CMaterialReference C_ClientPartialRagdoll::m_GoreMaterial;
float C_ClientPartialRagdoll::m_flLastParticleTime = 0.0f;
int C_ClientPartialRagdoll::m_iParticleCount = 0;

C_ClientPartialRagdoll::C_ClientPartialRagdoll( bool bRestoring )
	: BaseClass( bRestoring )
	, m_bShrinking( true )
	, m_bIsPartial( false )
	, m_iBranchRootBone( -1 )
	, m_iBloodColor( DONT_BLEED )
	, m_strRecursiveGoreName( NULL )
	, m_strRecursiveGoreMaterialName( NULL )
	, m_flTouchDecalDelay( gpGlobals->curtime + 0.25f )
{
	SetClassname( "client_ragdoll_partial" );

	Q_memset( m_iBoneFlagStorage, 0, sizeof( m_iBoneFlagStorage ) );
}

C_ClientPartialRagdoll::~C_ClientPartialRagdoll()
{
	DestroyGore();
}

void C_ClientPartialRagdoll::SetShrinkingEnabled( bool bEnable )
{
	m_bShrinking = bEnable;
}

void C_ClientPartialRagdoll::SetRecursiveGibData( const char *pszParentName, const char *pszGoreName,
	const char *pszGoreMaterialName )
{
	Assert( pszParentName && *pszParentName );

	m_strRecursiveParent = AllocPooledString( pszParentName );

	if ( pszGoreName && *pszGoreName )
	{
		m_strRecursiveGoreName = AllocPooledString( pszGoreName );
	}

	if ( pszGoreMaterialName && *pszGoreMaterialName )
	{
		m_strRecursiveGoreMaterialName = AllocPooledString( pszGoreMaterialName );
	}
}

void C_ClientPartialRagdoll::OnRestore()
{
	BaseClass::OnRestore();

	Q_memcpy( m_normalBones.Base(), m_iBoneFlagStorage, sizeof( int ) * 4 );
	Q_memcpy( m_trunkBones.Base(), m_iBoneFlagStorage + 4, sizeof( int ) * 4 );
	Q_memcpy( m_jointBones.Base(), m_iBoneFlagStorage + 8, sizeof( int ) * 4 );

	RebuildGore();
}

void C_ClientPartialRagdoll::ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName )
{
	BaseClass::ImpactTrace( pTrace, iDamageType, pCustomImpactName );

	// client entities have the network index -1 so lots of effects don't work easily
	if ( BloodColor() != DONT_BLEED )
	{
		const char *pszDecalName = NULL;

		switch ( BloodColor() )
		{
		case BLOOD_COLOR_RED:
			pszDecalName = "Blood";
			break;
		}

		int index = decalsystem->GetDecalIndexForName( pszDecalName );
		Vector vecDir = pTrace->endpos - pTrace->startpos;
		vecDir.NormalizeInPlace();

		if ( index >= 0 )
		{
			SetShrinkingEnabled( false );
			InvalidateBoneCache();
			SetupBones( NULL, -1, BONE_USED_BY_ANYTHING, gpGlobals->curtime );

			// add decal to model
			AddDecal( pTrace->startpos, pTrace->endpos, pTrace->endpos, pTrace->hitbox,
						index, false, *pTrace );

			SetShrinkingEnabled( true );
			InvalidateBoneCache();
		}

		// add decal to world
		trace_t tr;
		UTIL_TraceLine( pTrace->endpos, pTrace->endpos + vecDir * 35.0f, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

		UTIL_BloodDecalTrace( &tr, BloodColor() );
		if ( ShouldCreateBloodParticles() )
		{
			UTIL_BloodImpact( pTrace->endpos, -vecDir, BloodColor(), 5 );
		}
	}

	if ( m_bReleaseRagdoll || m_bFadingOut )
		return;

	const float flGibbingChance = ( ( iDamageType & DMG_BLAST ) != 0 ) ?
		gstring_gibbing_explosion_chance.GetFloat() : gstring_gibbing_chance.GetFloat();

	if ( RandomFloat() > flGibbingChance / 100.0f )
		return;

	CStudioHdr *pHdr = GetModelPtr();

	if ( pHdr == NULL )
		return;

	int iStudioBone = ConvertPhysBoneToStudioBone( this, pTrace->physicsbone );
	const bool bExplosionImpact = ( iDamageType & DMG_BLAST ) != 0;

	// if the hit bone is a valid studio bone and we know that this bone
	// is drawn as a normal bone (not shrunken)
	if ( iStudioBone >= 0
		&& m_normalBones.IsBitSet( iStudioBone )
		|| bExplosionImpact )
	{
		CUtlVector< ragdollparams_partial_t > gibModels;

		// we've been analysed already so use the known hit group
		// and try recusive splitting
		GibbingParamsRecursive_t params;
		params.pHdr = pHdr;
		params.pszHitBone = ( iStudioBone >= 0 && !bExplosionImpact ) ? pHdr->pBone( iStudioBone )->pszName() : NULL;
		params.pszParentName = STRING( m_strRecursiveParent );
		params.pszRootBone = ( m_iBranchRootBone >= 0 && m_iBranchRootBone < pHdr->numbones() )
			? pHdr->pBone( m_iBranchRootBone )->pszName() : NULL;
		params.pJointBones = &m_jointBones;

		const char *pszParentSplitBone;

		// find a suitable joint to cut
		if ( C_GibConfig::GetInstance()->GetGibsForGroup( params, gibModels, &pszParentSplitBone )
			|| bExplosionImpact && C_GibConfig::GetInstance()->GetRandomGibsForGroup( params, gibModels, &pszParentSplitBone ) )
		{
			int iSplitboneIndex = Studio_BoneIndexByName( pHdr, pszParentSplitBone );

			// don't do cutting if we cut this joint in the past
			// or if this joint is our current branch root
			if ( iSplitboneIndex < 0
				|| m_jointBones.IsBitSet( iSplitboneIndex )
				|| m_iBranchRootBone == iSplitboneIndex )
				return;

			matrix3x4_t boneDelta0[MAXSTUDIOBONES];
			matrix3x4_t boneDelta1[MAXSTUDIOBONES];
			matrix3x4_t currentBones[MAXSTUDIOBONES];
			const float boneDt = 0.1f;

			// setup bones without shrinking to position the new gibs
			SetShrinkingEnabled( false );
			GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
			SetShrinkingEnabled( true );

			InvalidateBoneCache();

			// create the new gibmodels
			FOR_EACH_VEC( gibModels, i )
			{
				ragdollparams_partial_t &partial = gibModels[ i ];

				// add old trunk joints
				for ( int iBit = m_jointBones.FindNextSetBit( 0 );
					iBit >= 0; iBit = m_jointBones.FindNextSetBit( iBit + 1 ) )
				{
					if ( m_iBranchRootBone == iBit )
						continue;

					const char *pszName = pHdr->pBone( iBit )->pszName();

					partial.trunkBones.AddToTail( pszName );
				}

				// if we create a trunk from an existing branch
				// we have to propagate the branch root bone
				if ( partial.rootBone.IsEmpty()
					&& m_iBranchRootBone >= 0 )
				{
					partial.rootBone = pHdr->pBone( m_iBranchRootBone )->pszName();
				}

				C_BaseAnimating *pGib = CreateRagdollCopy( false );
				C_ClientPartialRagdoll *pRecursiveRagdoll = dynamic_cast< C_ClientPartialRagdoll* >( pGib );
				Assert( pRecursiveRagdoll );

				// apply force and propagate cut information
				if ( pRecursiveRagdoll != NULL )
				{
					pRecursiveRagdoll->SetRecursiveGibData( STRING( m_strRecursiveParent ), STRING( m_strRecursiveGoreName ),
						STRING( m_strRecursiveGoreMaterialName ) );
					pRecursiveRagdoll->SetShrinkingEnabled( true );

					pRecursiveRagdoll->m_vecForce = ( pTrace->endpos - pTrace->startpos ).Normalized() * RandomFloat( 15000.0f, 35000.0f );
					pRecursiveRagdoll->m_nForceBone = pTrace->physicsbone;

					pRecursiveRagdoll->SetBloodColor( BloodColor() );
					pRecursiveRagdoll->m_jointBones.Or( m_jointBones, &pRecursiveRagdoll->m_jointBones );

					FOR_EACH_VEC( m_Gore, i )
					{
						const int iBone = m_Gore[ i ].m_iBone;

						if ( iBone >= 0 && iBone == m_iBranchRootBone )
						{

						}
						//pRecursiveRagdoll->m_Gore.AddToTail( m_Gore[ i ] );
						//m_Gore.Remove( i );
						//i--;
					}
				}

				pGib->InitAsClientRagdoll( boneDelta0, boneDelta1, currentBones, boneDt, false, &partial );

				if ( bExplosionImpact
					&& RandomFloat() <= gstring_gibbing_explosion_recursive_chance.GetFloat() / 100.0f )
				{
					pGib->ImpactTrace( pTrace, iDamageType, pCustomImpactName );
				}

				if ( BloodColor() == BLOOD_COLOR_RED
					&& ( !pRecursiveRagdoll || !pRecursiveRagdoll->m_bReleaseRagdoll )
					&& ShouldCreateBloodParticles() )
				{
					Assert( pszParentSplitBone != NULL );

					DispatchGibParticle( pGib, pszParentSplitBone, bExplosionImpact, BloodColor() );
				}
			}

			// the entity we've cut will be destroyed
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
		}
	}

	if ( TraceToStudio( physprops, ray, pStudioHdr, &partialSet, hitboxbones, fContentsMask, GetRenderOrigin(), GetModelScale(), tr ) )
	//if ( TraceToStudio( physprops, ray, pStudioHdr, set, hitboxbones, fContentsMask, GetRenderOrigin(), GetModelScale(), tr ) )
	{
		mstudiobbox_t *pbox = partialSet.pHitbox( tr.hitbox );
		mstudiobone_t *pBone = pStudioHdr->pBone(pbox->bone);

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

	const bool bRet = BaseClass::InitAsClientRagdoll( pDeltaBones0, pDeltaBones1, pCurrentBonePosition, boneDt, bFixedConstraints, pPartialParams );
	VPhysicsGetObject()->SetCallbackFlags( VPhysicsGetObject()->GetCallbackFlags() | CALLBACK_GLOBAL_TOUCH | CALLBACK_GLOBAL_TOUCH_STATIC );
	return bRet;
}

void C_ClientPartialRagdoll::Touch( C_BaseEntity *pOther )
{
	if ( m_flTouchDecalDelay < gpGlobals->curtime )
	{
		m_flTouchDecalDelay = gpGlobals->curtime + 0.1f;

		trace_t touchTrace;
		touchTrace = BaseClass::GetTouchTrace();
		UTIL_BloodDecalTrace( &touchTrace, BloodColor() );
	}

	BaseClass::Touch( pOther );
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
			// this bone has to be moved to the branch root
			if ( !m_normalBones.IsBitSet( i )
				&& !m_trunkBones.IsBitSet( i ) )
			{
				SetIdentityMatrix( GetBoneForWrite( i ) );
				continue;
			}

			// remember branch root xforms for moving
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

		// this bone is a child of a trunk joint, scale it down
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

			// now move all bones that aren't children of the current branch
			// on the position of the branch and shrink them
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

int C_ClientPartialRagdoll::DrawModel( int flags )
{
	const int ret = BaseClass::DrawModel( flags );

	if ( m_Gore.Count() > 0 )
	{
		CMatRenderContextPtr pRenderContext( materials );
		pRenderContext->MatrixMode( MATERIAL_MODEL );
		pRenderContext->PushMatrix();

		pRenderContext->SetNumBoneWeights( 0 );
		pRenderContext->Bind( m_GoreMaterial );

		FOR_EACH_VEC( m_Gore, i )
		{
			const PartialRagdollGore_t &gore = m_Gore[ i ];

			const matrix3x4_t &boneMatrix = GetBone( gore.m_iBone );
			pRenderContext->LoadMatrix( boneMatrix );

			//pRenderContext->LoadIdentity();

			gore.m_pMesh->Draw();

			//for ( int f = 0; f < g_pClientShadowMgr->GetNumShadowDepthtextures(); f++ )
			//{
			//	ShadowHandle_t handle = g_pClientShadowMgr->GetShadowDepthHandle( f );

			//	if ( handle == SHADOW_HANDLE_INVALID )
			//	{
			//		continue;
			//	}

			//	shadowmgr->SetFlashlightRenderState( handle );
			//	gore.m_pMesh->Draw();
			//	pRenderContext->SetFlashlightMode( false );
			//}
		}

		pRenderContext->MatrixMode( MATERIAL_MODEL );
		pRenderContext->PopMatrix();
	}

	return ret;
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

		if ( !params.rootBone.IsEmpty() )
		{
			const char *pszBranch = params.rootBone;
			const int iBone = LookupBone( pszBranch );

			m_iBranchRootBone = iBone;

			m_jointBones.Set( iBone );

			for ( int i = 0; i < pHdr->numbones(); i++ )
			{
				const char *pszBone = pHdr->pBone( i )->pszName();

				// this bone is not a child of our new branch root, cut it
				if ( !BoneHasParent( pHdr, pszBone, pszBranch )
					&& i != iBone )
				{
					if ( !params.cutBones.HasElement( pszBone ) )
					{
						params.cutBones.AddToTail( pszBone );
					}
				}
				else // otherwise it will potentially be visible
				{
					tempNormalBones.AddToTail( i );
				}
			}
		}

		// cut all bones that aren't part of this trunk
		// i.e. arm isn't part of pelvis/spline trunk

		FOR_EACH_VEC( params.trunkBones, t )
		{
			const char *pszTrunk = params.trunkBones[ t ].Get();
			const int iBone = LookupBone( pszTrunk );

			// don't evaluate trunks that are parents of our branch root
			if ( m_iBranchRootBone >= 0
				&& BoneHasParent( pHdr, m_iBranchRootBone, iBone ) )
				continue;

			Assert( params.rootBone.IsEmpty() || params.rootBone != pszTrunk );

			m_jointBones.Set( iBone );

			for ( int i = 0; i < pHdr->numbones(); i++ )
			{
				const char *pszBone = pHdr->pBone( i )->pszName();

				// this bone is a child of the flagged trunk bone, cut it
				if ( BoneParentDepth( pHdr, pszBone, pszTrunk ) >= 0 )
				{
					if ( !params.cutBones.HasElement( pszBone ) )
					{
						params.cutBones.AddToTail( pszBone );

						m_trunkBones.Set( i );
					}
				}
				else if ( m_iBranchRootBone < 0 // otherwise it will potentially be visible
					|| BoneParentDepth( GetModelPtr(), i, m_iBranchRootBone ) >= 0 ) // if it's a child of our branch root
				{
					tempNormalBones.AddToTail( i );
				}
			}
		}

		// now accept all potential normal bones
		// if they haven't been flagged as trunk joint
		FOR_EACH_VEC( tempNormalBones, i )
		{
			if ( !m_trunkBones.IsBitSet( tempNormalBones[ i ] ) )
				m_normalBones.Set( tempNormalBones[ i ] );
		}

		//if ( m_iBranchRootBone >= 0 )
		//{
		//	for ( int i = 0; i < pHdr->numbones(); i++ )
		//	{
		//		// make sure that we didn't add normal bones which aren't
		//		// part of the new root by accident
		//		if ( BoneParentDepth( GetModelPtr(), i, m_iBranchRootBone ) < 0 )
		//		{
		//			Assert( i != m_iBranchRootBone );

		//			m_normalBones.Clear( i );
		//		}
		//	}
		//}

		Assert( m_iBranchRootBone < 0 || m_normalBones.IsBitSet( m_iBranchRootBone ) );
		Assert( m_normalBones.FindNextSetBit( 0 ) >= 0 );
	}

	// copy current flag state to storable arrays
	Q_memcpy( m_iBoneFlagStorage, m_normalBones.Base(), sizeof( int ) * 4 );
	Q_memcpy( m_iBoneFlagStorage + 4, m_trunkBones.Base(), sizeof( int ) * 4 );
	Q_memcpy( m_iBoneFlagStorage + 8, m_jointBones.Base(), sizeof( int ) * 4 );

	RebuildGore();
}

void C_ClientPartialRagdoll::RebuildGore()
{
	if ( m_strRecursiveGoreMaterialName == NULL )
	{
		return;
	}

	if ( !m_GoreMaterial.IsValid() )
	{
		m_GoreMaterial.Init( materials->FindMaterial( m_strRecursiveGoreMaterialName, TEXTURE_GROUP_MODEL ) );

		if ( !m_GoreMaterial.IsValid() )
		{
			Assert( 0 );
			return;
		}
	}

	CStudioHdr *pHdr = GetModelPtr();
	if ( m_iBranchRootBone > 0 )
	{
		IMesh *pMesh = CreateGoreMeshForBone( pHdr, m_iBranchRootBone, true );
		if ( pMesh != NULL )
		{
			PartialRagdollGore_t gore;
			gore.m_iBone = m_iBranchRootBone;
			gore.m_pMesh = pMesh;
			m_Gore.AddToTail( gore );
		}
	}

	for ( int i = 0; i < pHdr->numbones(); i++ )
	{
		const int iParentBone = pHdr->boneParent( i );
		if ( iParentBone >= 0
			&& pHdr->boneParent( iParentBone ) >= 0
			&& m_jointBones.IsBitSet( iParentBone )
			&& !m_normalBones.IsBitSet( i ) )
		{
			IMesh *pMesh = CreateGoreMeshForBone( pHdr, iParentBone, false );
			if ( pMesh != NULL )
			{
				PartialRagdollGore_t gore;
				gore.m_iBone = pHdr->boneParent( iParentBone );
				gore.m_pMesh = pMesh;
				m_Gore.AddToTail( gore );
			}
		}
	}
}

IMesh *C_ClientPartialRagdoll::CreateGoreMeshForBone( const CStudioHdr *pHdr, int iBone, bool bRoot )
{
	Assert( pHdr );
	Assert( iBone >= 0 );
	Assert( m_GoreMaterial.IsValid() );

	FOR_EACH_VEC( m_Gore, i )
	{
		if ( m_Gore[ i ].m_iBone == iBone )
		{
			return NULL;
		}
	}

	GoreParams_t goreParams;
	goreParams.pszGoreClassName = STRING( m_strRecursiveGoreName );
	goreParams.pszBone = pHdr->pBone( iBone )->pszName();
	goreParams.bAsRoot = bRoot;

	GoreConfig_t goreConfig;
	if ( C_GibConfig::GetInstance()->GetGoreConfig( goreParams, goreConfig ) )
	{
		CGoreGenerator goreGenerator;
		return goreGenerator.GenerateMesh( m_GoreMaterial, goreConfig );
	}
	else
	{
		return NULL;
	}
}

bool C_ClientPartialRagdoll::ShouldCreateBloodParticles()
{
	if ( ( m_flLastParticleTime > gpGlobals->curtime && m_flLastParticleTime < gpGlobals->curtime + 10.0f )
		&& m_iParticleCount > 15
		&& RandomInt( 0, 9 ) > 0 )
	{
		return false;
	}

	if ( m_flLastParticleTime < gpGlobals->curtime )
	{
		m_iParticleCount = 0;
		m_flLastParticleTime = gpGlobals->curtime + 2.0f;
	}

	m_iParticleCount++;
	return true;
}

void C_ClientPartialRagdoll::DestroyGore()
{
	CMatRenderContextPtr pRenderContext( materials );
	FOR_EACH_VEC( m_Gore, i )
	{
		PartialRagdollGore_t &gore = m_Gore[ i ];
		pRenderContext->DestroyStaticMesh( gore.m_pMesh );
	}
	m_Gore.Purge();
}
