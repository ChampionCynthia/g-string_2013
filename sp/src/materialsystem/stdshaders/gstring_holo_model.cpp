
#include "BaseVSShader.h"
#include "mathlib/vmatrix.h"
#include "convar.h"

#include "gstring_holo_model_ps20b.inc"
#include "gstring_holo_model_vs20.inc"

BEGIN_VS_SHADER( gstring_holo_model, "" )

	BEGIN_SHADER_PARAMS
		SHADER_PARAM( EYEPOS, SHADER_PARAM_TYPE_VEC3, "[0 0 0]", "" )
		SHADER_PARAM( FRESNELRANGES, SHADER_PARAM_TYPE_VEC3, "[1 1 1]", "" )
		SHADER_PARAM( FRESNELRANGESWIRE, SHADER_PARAM_TYPE_VEC3, "[1 1 1]", "" )
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS2( MATERIAL_VAR2_SUPPORTS_HW_SKINNING );
	}

	SHADER_FALLBACK
	{
		return 0;
	}

	SHADER_INIT
	{
	}

	void DrawPass( IMaterialVar **params, IShaderShadow *pShaderShadow,
		IShaderDynamicAPI* pShaderAPI, VertexCompressionType_t vertexCompression,
		bool bDepthOnly, ShaderPolyMode_t polyMode = SHADER_POLYMODE_FILL, int iFresnelVar = FRESNELRANGES )
	{
		SHADOW_STATE
		{
			// Reset shadow state manually since we're drawing from two materials
			SetInitialShadowState();

			pShaderShadow->EnableCulling( true );
			if ( bDepthOnly )
			{
				pShaderShadow->EnableColorWrites( false );
			}
			else
			{
				pShaderShadow->DepthFunc( SHADER_DEPTHFUNC_NEAREROREQUAL );
				EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE );
			}

			//pShaderShadow->PolyMode( SHADER_POLYMODEFACE_FRONT, SHADER_POLYMODE_LINE );
			pShaderShadow->PolyMode( SHADER_POLYMODEFACE_FRONT, polyMode );

			unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_COMPRESSED;
			int nTexCoordCount = 1;
			int userDataSize = 0;
			pShaderShadow->VertexShaderVertexFormat( flags, nTexCoordCount, NULL, userDataSize );

			// Vertex Shader
			DECLARE_STATIC_VERTEX_SHADER( gstring_holo_model_vs20 );
			SET_STATIC_VERTEX_SHADER( gstring_holo_model_vs20 );

			DECLARE_STATIC_PIXEL_SHADER( gstring_holo_model_ps20b );
			SET_STATIC_PIXEL_SHADER_COMBO( DEPTH_ONLY, bDepthOnly );
			SET_STATIC_PIXEL_SHADER( gstring_holo_model_ps20b );
		}
		DYNAMIC_STATE
		{
			// Reset render state manually since we're drawing from two materials
			pShaderAPI->SetDefaultState();

			// Set Vertex Shader Combos
			const int numBones = pShaderAPI->GetCurrentNumBones();

			DECLARE_DYNAMIC_VERTEX_SHADER( gstring_holo_model_vs20 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( SKINNING, numBones > 0 );
			SET_DYNAMIC_VERTEX_SHADER_COMBO( COMPRESSED_VERTS, (int)vertexCompression );
			SET_DYNAMIC_VERTEX_SHADER( gstring_holo_model_vs20 );

			DECLARE_DYNAMIC_PIXEL_SHADER( gstring_holo_model_ps20b );
			SET_DYNAMIC_PIXEL_SHADER( gstring_holo_model_ps20b );

			SetModulationVertexShaderDynamicState();
			if ( !bDepthOnly )
			{
				float eyePos[4] = { 0.0f };
				params[ EYEPOS ]->GetVecValue( eyePos, 3 );
				//pShaderAPI->GetWorldSpaceCameraPosition( eyePos );
				pShaderAPI->SetPixelShaderConstant( 0, eyePos );

				float fresnel[4] = { 0.0f };
				params[ iFresnelVar ]->GetVecValue( fresnel, 3 );
				fresnel[0] = ( fresnel[1] - fresnel[0] ) * 2.0f;
				fresnel[2] = ( fresnel[2] - fresnel[1] ) * 2.0f;
				pShaderAPI->SetPixelShaderConstant( 1, fresnel );
			}
		}

		Draw();
	}

	SHADER_DRAW
	{
		DrawPass( params, pShaderShadow, pShaderAPI, vertexCompression, true );
		DrawPass( params, pShaderShadow, pShaderAPI, vertexCompression, false, SHADER_POLYMODE_FILL );
		DrawPass( params, pShaderShadow, pShaderAPI, vertexCompression, false, SHADER_POLYMODE_LINE, FRESNELRANGESWIRE );
	}
END_SHADER

