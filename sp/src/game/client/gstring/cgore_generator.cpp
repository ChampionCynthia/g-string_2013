#include "cbase.h"
#include "cgore_generator.h"
#include "c_gibconfig.h"

#include "materialsystem/imesh.h"

CGoreGenerator::CGoreGenerator()
{
}

IMesh *CGoreGenerator::GenerateMesh( IMaterial *pMaterial, const GoreConfig_t &config )
{
	const float flStubHeight = 3.0f;
	const int iStubSubDivision = 6;
	const int iStubSubDivisionPlusOne = iStubSubDivision + 1;
	const int iNumStubTris = iStubSubDivision * iStubSubDivision * 2 - 4;
	const float flStubSize = 5.0f;

	const bool bFringe = config.flFringeSize > 0.0f;
	const int iNumFringeTris = bFringe ? 8 : 0;

	const float flBoneThickMin = 0.3f;
	const float flBoneThickMax = 0.5f;
	const bool bBone = config.flBoneSize > 0.0f;
	const int iNumBoneTris = bBone ? 7 : 0;

	const int iNumTris = iNumStubTris + iNumFringeTris + iNumBoneTris;

	const float flUVPadding = 0.02f;

	Vector vecFwd, vecRight, vecUp;
	AngleVectors( config.angOrientation, &vecFwd, &vecRight, &vecUp );

	Vector vecOffset = vecFwd * config.vecOffset.x +
		vecRight * config.vecOffset.y +
		vecUp * config.vecOffset.z;

	const Vector vecFringeUp = vecUp * config.flFringeSize;

	const Vector vecBoneFwd = vecFwd;
	const Vector vecBoneRight = vecRight;
	const Vector vecBoneUp = vecUp * config.flBoneSize;

	vecFwd *= config.vecScale.x;
	vecRight *= -config.vecScale.y;
	vecUp *= config.vecScale.z;

	VertexFormat_t vertexFormat = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_TEXCOORD_SIZE( 0, 2 ) |
		VERTEX_USERDATA_SIZE( 4 );

	CMatRenderContextPtr pRenderContext( materials );
	IMesh *pMesh = pRenderContext->CreateStaticMesh( vertexFormat, TEXTURE_GROUP_MODEL, pMaterial );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_TRIANGLES, iNumTris );

	const float flUserData[ 4 ] = { 1.0f, 0.0f, 0.0f, 1.0f };

	///////////////////////
	// Generate stub
	///////////////////////
	{
		// Build geometry helpers
		const int iNumPoints = iStubSubDivisionPlusOne * iStubSubDivisionPlusOne;
		Vector vecPositions[ iNumPoints ];
		Vector vecNormals[ iNumPoints ];
		Vector2D vecUVs[ iNumPoints ];
		for ( int x = 0; x <= iStubSubDivision; x++ )
		{
			for ( int y = 0; y <= iStubSubDivision; y++ )
			{
				const int iIndex = x + y * iStubSubDivisionPlusOne;
				const float flFractionX = ( x / float( iStubSubDivision ) );
				const float flFractionY = ( y / float( iStubSubDivision ) );

				Vector &vecPosition = vecPositions[ iIndex ];
				vecPosition = vecFwd * ( flStubSize * -0.5f + flStubSize * flFractionX );
				vecPosition += vecRight * ( flStubSize * -0.5f + flStubSize * flFractionY );
				vecPosition += vecUp *( Bias( 1.0f - abs( ( flFractionX - 0.5f ) * 2.0f ), 0.85f ) *
					Bias( 1.0f - abs( ( flFractionY - 0.5f ) * 2.0f ), 0.85f ) * flStubHeight );

				vecNormals[ iIndex ] = vecPosition.Normalized();

				vecPosition += vecOffset;

				Vector2D &vecUV = vecUVs[ iIndex ];
				vecUV.x = flFractionX * 0.5f;
				vecUV.y = flFractionY;

				if ( x == 0 )
				{
					vecUV.x += flUVPadding;
				}
				else if ( x == iStubSubDivision )
				{
					vecUV.x -= flUVPadding;
				}

				if ( y == 0 )
				{
					vecUV.y += flUVPadding;
				}
				else if ( y == iStubSubDivision )
				{
					vecUV.y -= flUVPadding;
				}
			}
		}

		// Build mesh.
		for ( int x = 0; x < iStubSubDivision; x++ )
		{
			for ( int y = 0; y < iStubSubDivision; y++ )
			{
				const int iBaseIndex = x + y * iStubSubDivisionPlusOne;

				if ( ( x < iStubSubDivision - 1 || y > 0 ) && ( x > 0 || y < iStubSubDivision - 1 ) )
				{
					if ( x > 0 || y > 0 )
					{
						meshBuilder.Position3fv( vecPositions[ iBaseIndex ].Base() );
						meshBuilder.Normal3fv( vecNormals[ iBaseIndex ].Base() );
						meshBuilder.TexCoord2fv( 0, vecUVs[ iBaseIndex ].Base() );
						meshBuilder.UserData( flUserData );
						meshBuilder.AdvanceVertex();

						meshBuilder.Position3fv( vecPositions[ iBaseIndex + iStubSubDivisionPlusOne ].Base() );
						meshBuilder.Normal3fv( vecNormals[ iBaseIndex + iStubSubDivisionPlusOne ].Base() );
						meshBuilder.TexCoord2fv( 0, vecUVs[ iBaseIndex + iStubSubDivisionPlusOne ].Base() );
						meshBuilder.UserData( flUserData );
						meshBuilder.AdvanceVertex();

						meshBuilder.Position3fv( vecPositions[ iBaseIndex + 1 ].Base() );
						meshBuilder.Normal3fv( vecNormals[ iBaseIndex + 1 ].Base() );
						meshBuilder.TexCoord2fv( 0, vecUVs[ iBaseIndex + 1 ].Base() );
						meshBuilder.UserData( flUserData );
						meshBuilder.AdvanceVertex();
					}

					if ( x < iStubSubDivision - 1 || y < iStubSubDivision - 1 )
					{
						meshBuilder.Position3fv( vecPositions[ iBaseIndex + 1 ].Base() );
						meshBuilder.Normal3fv( vecNormals[ iBaseIndex + 1 ].Base() );
						meshBuilder.TexCoord2fv( 0, vecUVs[ iBaseIndex + 1 ].Base() );
						meshBuilder.UserData( flUserData );
						meshBuilder.AdvanceVertex();

						meshBuilder.Position3fv( vecPositions[ iBaseIndex + iStubSubDivisionPlusOne ].Base() );
						meshBuilder.Normal3fv( vecNormals[ iBaseIndex + iStubSubDivisionPlusOne ].Base() );
						meshBuilder.TexCoord2fv( 0, vecUVs[ iBaseIndex + iStubSubDivisionPlusOne ].Base() );
						meshBuilder.UserData( flUserData );
						meshBuilder.AdvanceVertex();

						meshBuilder.Position3fv( vecPositions[ iBaseIndex + iStubSubDivisionPlusOne + 1 ].Base() );
						meshBuilder.Normal3fv( vecNormals[ iBaseIndex + iStubSubDivisionPlusOne + 1 ].Base() );
						meshBuilder.TexCoord2fv( 0, vecUVs[ iBaseIndex + iStubSubDivisionPlusOne + 1 ].Base() );
						meshBuilder.UserData( flUserData );
						meshBuilder.AdvanceVertex();
					}
				}
				else if ( ( x == iStubSubDivision - 1 && y == 0 ) )
				{
					meshBuilder.Position3fv( vecPositions[ iBaseIndex ].Base() );
					meshBuilder.Normal3fv( vecNormals[ iBaseIndex ].Base() );
					meshBuilder.TexCoord2fv( 0, vecUVs[ iBaseIndex ].Base() );
					meshBuilder.UserData( flUserData );
					meshBuilder.AdvanceVertex();

					meshBuilder.Position3fv( vecPositions[ iBaseIndex + iStubSubDivisionPlusOne ].Base() );
					meshBuilder.Normal3fv( vecNormals[ iBaseIndex + iStubSubDivisionPlusOne ].Base() );
					meshBuilder.TexCoord2fv( 0, vecUVs[ iBaseIndex + iStubSubDivisionPlusOne ].Base() );
					meshBuilder.UserData( flUserData );
					meshBuilder.AdvanceVertex();

					meshBuilder.Position3fv( vecPositions[ iBaseIndex + iStubSubDivisionPlusOne + 1 ].Base() );
					meshBuilder.Normal3fv( vecNormals[ iBaseIndex + iStubSubDivisionPlusOne + 1 ].Base() );
					meshBuilder.TexCoord2fv( 0, vecUVs[ iBaseIndex + iStubSubDivisionPlusOne + 1 ].Base() );
					meshBuilder.UserData( flUserData );
					meshBuilder.AdvanceVertex();
				}
				else
				{
					meshBuilder.Position3fv( vecPositions[ iBaseIndex ].Base() );
					meshBuilder.Normal3fv( vecNormals[ iBaseIndex ].Base() );
					meshBuilder.TexCoord2fv( 0, vecUVs[ iBaseIndex ].Base() );
					meshBuilder.UserData( flUserData );
					meshBuilder.AdvanceVertex();

					meshBuilder.Position3fv( vecPositions[ iBaseIndex + iStubSubDivisionPlusOne + 1 ].Base() );
					meshBuilder.Normal3fv( vecNormals[ iBaseIndex + iStubSubDivisionPlusOne + 1 ].Base() );
					meshBuilder.TexCoord2fv( 0, vecUVs[ iBaseIndex + iStubSubDivisionPlusOne + 1 ].Base() );
					meshBuilder.UserData( flUserData );
					meshBuilder.AdvanceVertex();

					meshBuilder.Position3fv( vecPositions[ iBaseIndex + 1 ].Base() );
					meshBuilder.Normal3fv( vecNormals[ iBaseIndex + 1 ].Base() );
					meshBuilder.TexCoord2fv( 0, vecUVs[ iBaseIndex + 1 ].Base() );
					meshBuilder.UserData( flUserData );
					meshBuilder.AdvanceVertex();
				}
			}
		}
	}

	///////////////////////
	// Generate fringe
	///////////////////////
	if ( bFringe )
	{
		const int iFringeVertices = 8;
		Vector vecPositions[ iFringeVertices ];
		Vector vecNormals[ iFringeVertices ];
		Vector2D vecUVs[ iFringeVertices + 2 ];

		vecUVs[ 0 ].y = 0.0f;
		vecUVs[ 2 ].y = 0.25f;
		vecUVs[ 4 ].y = 0.5f;
		vecUVs[ 6 ].y = 0.75f;
		vecUVs[ 8 ].y = 1.0f;

		for ( int i = 0; i < iFringeVertices + 2; i += 2 )
		{
			vecUVs[ i ].x = 0.5f + flUVPadding;
			vecUVs[ i + 1 ].x = 0.75f - flUVPadding;
			vecUVs[ i + 1 ].y = vecUVs[ i ].y;
		}

		vecPositions[ 0 ] = vecFwd * flStubSize * 0.5f;
		vecPositions[ 2 ] = vecRight * flStubSize * 0.5f;
		vecPositions[ 4 ] = vecFwd * flStubSize * -0.5f;
		vecPositions[ 6 ] = vecRight * flStubSize * -0.5f;

		for ( int i = 1; i < iFringeVertices; i += 2 )
		{
			vecNormals[ i ] = vecPositions[ i - 1 ].Normalized();
			vecNormals[ i - 1 ] = vecNormals[ i ];
			vecPositions[ i ] = vecPositions[ i - 1 ] + vecFringeUp;
		}
		
		for ( int i = 0; i < iFringeVertices; i++ )
		{
			vecPositions[ i ] += vecOffset;
		}

		for ( int i = 0; i < iFringeVertices / 2; i++ )
		{
			const int iIndex0 = i * 2;
			const int iIndex1 = ( iIndex0 == iFringeVertices - 2 ) ? 0 : ( iIndex0 + 2 );

			meshBuilder.Position3fv( vecPositions[ iIndex1 ].Base() );
			meshBuilder.Normal3fv( vecNormals[ iIndex1 ].Base() );
			meshBuilder.TexCoord2fv( 0, vecUVs[ iIndex0 + 2 ].Base() );
			meshBuilder.UserData( flUserData );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( vecPositions[ iIndex0 ].Base() );
			meshBuilder.Normal3fv( vecNormals[ iIndex0 ].Base() );
			meshBuilder.TexCoord2fv( 0, vecUVs[ iIndex0 ].Base() );
			meshBuilder.UserData( flUserData );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( vecPositions[ iIndex0 + 1 ].Base() );
			meshBuilder.Normal3fv( vecNormals[ iIndex0 + 1 ].Base() );
			meshBuilder.TexCoord2fv( 0, vecUVs[ iIndex0 + 1 ].Base() );
			meshBuilder.UserData( flUserData );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( vecPositions[ iIndex1 ].Base() );
			meshBuilder.Normal3fv( vecNormals[ iIndex1 ].Base() );
			meshBuilder.TexCoord2fv( 0, vecUVs[ iIndex0 + 2 ].Base() );
			meshBuilder.UserData( flUserData );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( vecPositions[ iIndex0 + 1 ].Base() );
			meshBuilder.Normal3fv( vecNormals[ iIndex0 + 1 ].Base() );
			meshBuilder.TexCoord2fv( 0, vecUVs[ iIndex0 + 1 ].Base() );
			meshBuilder.UserData( flUserData );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( vecPositions[ iIndex1 + 1 ].Base() );
			meshBuilder.Normal3fv( vecNormals[ iIndex1 + 1 ].Base() );
			meshBuilder.TexCoord2fv( 0, vecUVs[ iIndex0 + 3 ].Base() );
			meshBuilder.UserData( flUserData );
			meshBuilder.AdvanceVertex();
		}
	}

	///////////////////////
	// Generate bone
	///////////////////////
	if ( bBone )
	{
		const int iBoneVertices = 6;
		const float flUVXStep = ( 2.0f / iBoneVertices ) * 0.25f;
		Vector vecPositions[ iBoneVertices ];
		Vector vecNormals[ iBoneVertices ];
		Vector2D vecUVs[ iBoneVertices + 2 ];

		vecPositions[ 0 ] = vecBoneFwd * flBoneThickMax;
		vecPositions[ 2 ] = vecBoneFwd * flBoneThickMax * -0.5f + vecBoneRight * flBoneThickMax;
		vecPositions[ 4 ] = vecBoneFwd * flBoneThickMax * -0.5f - vecBoneRight * flBoneThickMax;
		vecPositions[ 1 ] = vecBoneFwd * flBoneThickMin;
		vecPositions[ 3 ] = vecBoneFwd * flBoneThickMin * -0.5f + vecBoneRight * flBoneThickMin;
		vecPositions[ 5 ] = vecBoneFwd * flBoneThickMin * -0.5f - vecBoneRight * flBoneThickMin;

		for ( int i = 0; i < iBoneVertices + 2; i += 2 )
		{
			vecUVs[ i ].y = 0.0f;
			vecUVs[ i + 1 ].y = 1.0f;
			vecUVs[ i ].x = 0.75f + ( i / 2.0f ) * flUVXStep;
			vecUVs[ i + 1 ].x = vecUVs[ i ].x;
		}

		for ( int i = 0; i < iBoneVertices; i += 2 )
		{
			vecPositions[ i + 1 ] += vecBoneUp * ( config.flBoneSize - i * 0.1f );
			vecNormals[ i ] = vecPositions[ i ].Normalized();
			vecNormals[ i + 1 ] = vecNormals[ i ];
		}

		for ( int i = 0; i < iBoneVertices / 2; i++ )
		{
			const int iIndex0 = i * 2;
			const int iIndex1 = ( iIndex0 == iBoneVertices - 2 ) ? 0 : ( iIndex0 + 2 );

			meshBuilder.Position3fv( vecPositions[ iIndex1 ].Base() );
			meshBuilder.Normal3fv( vecNormals[ iIndex1 ].Base() );
			meshBuilder.TexCoord2fv( 0, vecUVs[ iIndex0 + 2 ].Base() );
			meshBuilder.UserData( flUserData );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( vecPositions[ iIndex0 ].Base() );
			meshBuilder.Normal3fv( vecNormals[ iIndex0 ].Base() );
			meshBuilder.TexCoord2fv( 0, vecUVs[ iIndex0 ].Base() );
			meshBuilder.UserData( flUserData );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( vecPositions[ iIndex0 + 1 ].Base() );
			meshBuilder.Normal3fv( vecNormals[ iIndex0 + 1 ].Base() );
			meshBuilder.TexCoord2fv( 0, vecUVs[ iIndex0 + 1 ].Base() );
			meshBuilder.UserData( flUserData );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( vecPositions[ iIndex1 ].Base() );
			meshBuilder.Normal3fv( vecNormals[ iIndex1 ].Base() );
			meshBuilder.TexCoord2fv( 0, vecUVs[ iIndex0 + 2 ].Base() );
			meshBuilder.UserData( flUserData );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( vecPositions[ iIndex0 + 1 ].Base() );
			meshBuilder.Normal3fv( vecNormals[ iIndex0 + 1 ].Base() );
			meshBuilder.TexCoord2fv( 0, vecUVs[ iIndex0 + 1 ].Base() );
			meshBuilder.UserData( flUserData );
			meshBuilder.AdvanceVertex();

			meshBuilder.Position3fv( vecPositions[ iIndex1 + 1 ].Base() );
			meshBuilder.Normal3fv( vecNormals[ iIndex1 + 1 ].Base() );
			meshBuilder.TexCoord2fv( 0, vecUVs[ iIndex0 + 3 ].Base() );
			meshBuilder.UserData( flUserData );
			meshBuilder.AdvanceVertex();
		}

		meshBuilder.Position3fv( vecPositions[ 5 ].Base() );
		meshBuilder.Normal3fv( vecNormals[ 5 ].Base() );
		meshBuilder.TexCoord2fv( 0, vecUVs[ 5 ].Base() );
		meshBuilder.UserData( flUserData );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3fv( vecPositions[ 1 ].Base() );
		meshBuilder.Normal3fv( vecNormals[ 1 ].Base() );
		meshBuilder.TexCoord2fv( 0, vecUVs[ 0 ].Base() );
		meshBuilder.UserData( flUserData );
		meshBuilder.AdvanceVertex();

		meshBuilder.Position3fv( vecPositions[ 3 ].Base() );
		meshBuilder.Normal3fv( vecNormals[ 3 ].Base() );
		meshBuilder.TexCoord2fv( 0, vecUVs[ 3 ].Base() );
		meshBuilder.UserData( flUserData );
		meshBuilder.AdvanceVertex();
	}

	//meshBuilder.Reset();

	//for ( int i = 0; i < iNumTris; i += 3 )
	//{
	//	const float *flPos0 = meshBuilder.Position();
	//	const float *flNormal0 = meshBuilder.Normal();
	//	const float *flUV0 = meshBuilder.TexCoord( 0 );
	//	meshBuilder.AdvanceVertex();
	//	const float *flPos1 = meshBuilder.Position();
	//	const float *flNormal1 = meshBuilder.Normal();
	//	const float *flUV1 = meshBuilder.TexCoord( 0 );
	//	meshBuilder.AdvanceVertex();
	//	const float *flPos2 = meshBuilder.Position();
	//	const float *flNormal2 = meshBuilder.Normal();
	//	const float *flUV2 = meshBuilder.TexCoord( 0 );
	//	meshBuilder.AdvanceVertices( -2 );

	//	meshBuilder.AdvanceVertices( 3 );
	//}

	meshBuilder.End();

	return pMesh;
}
