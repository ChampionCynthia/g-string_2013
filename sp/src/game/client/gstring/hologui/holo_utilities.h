#ifndef HOLO_UTILITIES_H
#define HOLO_UTILITIES_H

inline Vector2D GetVector2DFromAngle( float radians )
{
	return Vector2D( cos( radians ), -sin( radians ) );
}

inline void GetVector2DFromAngle( float radians, Vector2D &out )
{
	out.x = cos( radians );
	out.y = -sin( radians );
}

inline void CreateTexturedRect( IMesh *pMesh, float x, float y, float width, float height )
{
	CMeshBuilder builder;
	builder.Begin( pMesh, MATERIAL_QUADS, 1 );

	float color4[] = { 1, 1, 1 };

	builder.Position3f( x, y, 0.0f );
	builder.TexCoord2f( 0, 0, 0 );
	builder.Color3fv( color4 );
	builder.AdvanceVertex();

	builder.Position3f( x + width, y, 0.0f );
	builder.TexCoord2f( 0, 1, 0 );
	builder.Color3fv( color4 );
	builder.AdvanceVertex();

	builder.Position3f( x + width, y + height, 0.0f );
	builder.TexCoord2f( 0, 1, 1 );
	builder.Color3fv( color4 );
	builder.AdvanceVertex();

	builder.Position3f( x, y + height, 0.0f );
	builder.TexCoord2f( 0, 0, 1 );
	builder.Color3fv( color4 );
	builder.AdvanceVertex();

	builder.End();
}

inline void CreateTexturedRectHolo( IMesh *pMesh, float x, float y, float width, float height )
{
	CMeshBuilder builder;
	builder.Begin( pMesh, MATERIAL_QUADS, 1 );

	float color4[] = { 1, 1, 1 };

	builder.Position3f( 0.0f, x, y );
	builder.TexCoord2f( 0, 0, 1 );
	builder.Color3fv( color4 );
	builder.AdvanceVertex();

	builder.Position3f( 0.0f, x + width, y );
	builder.TexCoord2f( 0, 1, 1 );
	builder.Color3fv( color4 );
	builder.AdvanceVertex();

	builder.Position3f( 0.0f, x + width, y + height );
	builder.TexCoord2f( 0, 1, 0 );
	builder.Color3fv( color4 );
	builder.AdvanceVertex();

	builder.Position3f( 0.0f, x, y + height );
	builder.TexCoord2f( 0, 0, 0 );
	builder.Color3fv( color4 );
	builder.AdvanceVertex();

	builder.End();
}

inline void CreateSlantedRect( IMesh *pMesh, float x, float y, float width, float height, float offset = 0.0f )
{
	CMeshBuilder builder;
	builder.Begin( pMesh, MATERIAL_QUADS, 1 );

	builder.Position3f( 0.0f, x, y );
	builder.AdvanceVertex();

	builder.Position3f( 0.0f, x + width, y + offset );
	builder.AdvanceVertex();

	builder.Position3f( 0.0f, x + width, y + offset + height );
	builder.AdvanceVertex();

	builder.Position3f( 0.0f, x, y + height );
	builder.AdvanceVertex();

	builder.End();
}

inline void CreateSlantedRect( CMeshBuilder &builder, float x, float y, float width, float height, float offset = 0.0f )
{
	builder.Position3f( 0.0f, x, y );
	builder.AdvanceVertex();

	builder.Position3f( 0.0f, x + width, y + offset );
	builder.AdvanceVertex();

	builder.Position3f( 0.0f, x + width, y + offset + height );
	builder.AdvanceVertex();

	builder.Position3f( 0.0f, x, y + height );
	builder.AdvanceVertex();
}

// quads * subDivCount
inline void CreateArc( CMeshBuilder &builder, int subDivCount, float radius, float thickness,
	float startAngle, float endAngle, const float *color4 )
{
	const float angleStep = ( endAngle - startAngle ) / (float)subDivCount;
	const float toInnerRadius = ( radius - thickness ) / radius;
	for ( int i = 0; i < subDivCount; ++i )
	{
		Vector2D dir( GetVector2DFromAngle( startAngle + i * angleStep ) * radius );
		builder.Position3f( dir.x, dir.y, 0.0f );
		builder.Color4fv( color4 );
		builder.AdvanceVertex();

		builder.Position3f( dir.x * toInnerRadius, dir.y * toInnerRadius, 0.0f );
		builder.Color4fv( color4 );
		builder.AdvanceVertex();

		dir = GetVector2DFromAngle( startAngle + ( i + 1 ) * angleStep ) * radius;
		builder.Position3f( dir.x * toInnerRadius, dir.y * toInnerRadius, 0.0f );
		builder.Color4fv( color4 );
		builder.AdvanceVertex();

		builder.Position3f( dir.x, dir.y, 0.0f );
		builder.Color4fv( color4 );
		builder.AdvanceVertex();
	}
}

inline void CreateArc( IMesh *pMesh, int subDivCount, float radius, float thickness,
	float startAngle, float endAngle )
{
	Assert( subDivCount >= 1 );

	CMeshBuilder builder;
	builder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, subDivCount * 2 );

	const float angleStep = ( endAngle - startAngle ) / (float)subDivCount;
	const float toInnerRadius = ( radius - thickness ) / radius;

	Vector2D dir( GetVector2DFromAngle( startAngle ) * radius );
	builder.Position3f( dir.x, dir.y, 0.0f );
	builder.AdvanceVertex();

	builder.Position3f( dir.x * toInnerRadius, dir.y * toInnerRadius, 0.0f );
	builder.AdvanceVertex();

	for ( int i = 0; i < subDivCount; ++i )
	{
		dir = GetVector2DFromAngle( startAngle + ( i + 1 ) * angleStep ) * radius;
		builder.Position3f( dir.x, dir.y, 0.0f );
		builder.AdvanceVertex();

		builder.Position3f( dir.x * toInnerRadius, dir.y * toInnerRadius, 0.0f );
		builder.AdvanceVertex();
	}

	builder.End();
}

inline void CreateArc( CMeshBuilder &builder, int subDivCount, float radius, float outerThickness, float innerThickness,
	float startAngle, float endAngle )
{
	Assert( subDivCount >= 1 );

	const float angleStep = ( endAngle - startAngle ) / (float)subDivCount;
	float toInnerRadius = ( radius - outerThickness ) / radius;

	Vector2D dir( GetVector2DFromAngle( startAngle ) * radius );
	builder.Position3f( dir.x, dir.y, 0.0f );
	builder.AdvanceVertex();

	builder.Position3f( dir.x * toInnerRadius, dir.y * toInnerRadius, 0.0f );
	builder.AdvanceVertex();

	const float flDeltaThickness = ( innerThickness - outerThickness ) * 4.0f;
	for ( int i = 0; i < subDivCount; ++i )
	{
		float fraction = i / float( subDivCount - 1 );
		fraction *= 1.0f - fraction;
		toInnerRadius = ( radius - ( outerThickness + flDeltaThickness * fraction ) ) / radius;
		dir = GetVector2DFromAngle( startAngle + ( i + 1 ) * angleStep ) * radius;
		builder.Position3f( dir.x, dir.y, 0.0f );
		builder.AdvanceVertex();

		builder.Position3f( dir.x * toInnerRadius, dir.y * toInnerRadius, 0.0f );
		builder.AdvanceVertex();
	}
}

inline void CreateArc( IMesh *pMesh, int subDivCount, float radius, float outerThickness, float innerThickness,
	float startAngle, float endAngle )
{
	CMeshBuilder builder;
	builder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, subDivCount * 2 );
	CreateArc( builder, subDivCount, radius, outerThickness, innerThickness, startAngle, endAngle );
	builder.End();
}

inline void CreateArcFaded( IMesh *pMesh, int subDivCount, float radius, float thickness,
	float startAngle, float endAngle,
	float fadeStart, float fadeEnd )
{
	Assert( subDivCount >= 1 );

	CMeshBuilder builder;
	builder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, subDivCount * 2 );

	float color4[] = { 1, 1, 1, RemapValClamped( 1.0f, fadeStart, fadeEnd, 1.0f, 0.0f ) };
	const float angleStep = ( endAngle - startAngle ) / (float)subDivCount;
	const float toInnerRadius = ( radius - thickness ) / radius;

	Vector2D dir( GetVector2DFromAngle( startAngle ) * radius );
	builder.Position3f( radius, 0.0f, 0.0f );
	builder.Color4fv( color4 );
	builder.AdvanceVertex();

	builder.Position3f( radius - thickness, 0.0f, 0.0f );
	builder.Color4fv( color4 );
	builder.AdvanceVertex();

	const int centerPosition = subDivCount / 2;

	for ( int i = 0; i < subDivCount; ++i )
	{
		const float fadePosition = (i > centerPosition ? i - centerPosition : centerPosition - i - 1) / (float)centerPosition;
		color4[3] = RemapValClamped( fadePosition, fadeStart, fadeEnd, 1.0f, 0.0f );

		dir = GetVector2DFromAngle( startAngle + ( i + 1 ) * angleStep ) * radius;
		builder.Position3f( dir.x, dir.y, 0.0f );
		builder.Color4fv( color4 );
		builder.AdvanceVertex();

		builder.Position3f( dir.x * toInnerRadius, dir.y * toInnerRadius, 0.0f );
		builder.Color4fv( color4 );
		builder.AdvanceVertex();
	}

	builder.End();
}

inline void CreateMirroredArcFaded( IMesh *pMesh, float radius, float thickness,
	float fadeStart, float fadeEnd )
{
	const int subDivCount = 33;
	CMeshBuilder builder;
	builder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, subDivCount * 4 + 2 );

	float color4[] = { 1, 1, 1, RemapValClamped( 1.0f, fadeStart, fadeEnd, 1.0f, 0.0f ) };
	const float angleStep = ( M_PI_F ) / (float)subDivCount;
	const float toInnerRadius = ( radius - thickness ) / radius;

	Vector2D dirs[ subDivCount + 1 ];
	float alphas[ subDivCount + 1 ];

	Vector2D initialDir( GetVector2DFromAngle( 0.0f ) * radius );
	dirs[ 0 ] = initialDir;
	alphas[ 0 ] = color4[ 3 ];

	builder.Position3f( radius, 0.0f, 0.0f );
	builder.Color4fv( color4 );
	builder.AdvanceVertex();

	builder.Position3f( radius - thickness, 0.0f, 0.0f );
	builder.Color4fv( color4 );
	builder.AdvanceVertex();

	const int centerPosition = subDivCount / 2;
	for ( int i = 0; i < subDivCount; ++i )
	{
		const float fadePosition = (i > centerPosition ? i - centerPosition : centerPosition - i - 1) / (float)centerPosition;
		alphas[ i + 1 ] = RemapValClamped( fadePosition, fadeStart, fadeEnd, 1.0f, 0.0f );
		color4[3] = alphas[ i + 1 ];

		Vector2D &dir = dirs[ i + 1 ];
		GetVector2DFromAngle( ( i + 1 ) * angleStep, dir );
		dir *= radius;

		builder.Position3f( dir.x, dir.y, 0.0f );
		builder.Color4fv( color4 );
		builder.AdvanceVertex();

		builder.Position3f( dir.x * toInnerRadius, dir.y * toInnerRadius, 0.0f );
		builder.Color4fv( color4 );
		builder.AdvanceVertex();
	}

	for ( int i = subDivCount; i > 0; --i )
	{
		Vector2D &dir = dirs[ i ];
		color4[3] = alphas[ i ];

		builder.Position3f( dir.x, -dir.y, 0.0f );
		builder.Color4fv( color4 );
		builder.AdvanceVertex();

		builder.Position3f( dir.x * toInnerRadius, -dir.y * toInnerRadius, 0.0f );
		builder.Color4fv( color4 );
		builder.AdvanceVertex();
	}

	color4[ 3 ] = alphas[ 0 ];
	builder.Position3f( radius, 0.0f, 0.0f );
	builder.Color4fv( color4 );
	builder.AdvanceVertex();

	builder.Position3f( radius - thickness, 0.0f, 0.0f );
	builder.Color4fv( color4 );
	builder.AdvanceVertex();

	builder.End();
}

inline void RotateVectorYZ( Vector &vec, float radians )
{
	const float cs = cos( radians );
	const float sn = sin( radians );
	const float y = vec.y;
	const float z = vec.z;
	vec.z = z * cs - y * sn;
	vec.y = z * sn + y * cs;
}

inline void CreateRecticule( IMesh *pMesh, float scale, float thickness, float lineSize, float angle )
{
	CMeshBuilder builder;
	builder.Begin( pMesh, MATERIAL_QUADS, 8 );
	
	const Vector vecDirections[] = {
		Vector( 0, -0.707f, -0.707f ),
		Vector( 0, 0.707f, -0.707f ),
		Vector( 0, -0.707f, 0.707f ),
		Vector( 0, 0.707f, 0.707f ),

		Vector( 0, -0.707f, -0.707f ),
		Vector( 0, -0.707f, 0.707f ),
		Vector( 0, 0.707f, -0.707f ),
		Vector( 0, 0.707f, 0.707f ),
	};
	const Vector vecOffsets[] = {
		Vector( 0, 1, 0 ),
		Vector( 0, -1, 0 ),
		Vector( 0, 1, 0 ),
		Vector( 0, -1, 0 ),

		Vector( 0, 0, 1 ),
		Vector( 0, 0, -1 ),
		Vector( 0, 0, 1 ),
		Vector( 0, 0, -1 ),
	};
	const Vector vecStart[] = {
		Vector( 0, 0, 1 ),
		Vector( 0, 0, 1 ),
		Vector( 0, 0, -1 ),
		Vector( 0, 0, -1 ),

		Vector( 0, 1, 0 ),
		Vector( 0, 1, 0 ),
		Vector( 0, -1, 0 ),
		Vector( 0, -1, 0 ),
	};

	for ( int i = 0; i < 8; ++i )
	{
		Vector start = vecStart[ i ];
		Vector dir = vecDirections[ i ];
		Vector offset = vecOffsets[ i ];

		RotateVectorYZ( start, angle );
		RotateVectorYZ( dir, angle );
		RotateVectorYZ( offset, angle );

		Vector v = start * ( scale - thickness );
		builder.Position3fv( v.Base() );
		builder.AdvanceVertex();

		v += start * thickness;
		builder.Position3fv( v.Base() );
		builder.AdvanceVertex();

		v += dir * lineSize;
		builder.Position3fv( v.Base() );
		builder.AdvanceVertex();

		v += offset * thickness + dir * thickness * 0.707f;
		builder.Position3fv( v.Base() );
		builder.AdvanceVertex();
	}

	builder.End();
}

inline void CreateRecticuleOffScreen( IMesh *pMesh, float scale, float thickness, float lineSize, float angle )
{
	CMeshBuilder builder;
	builder.Begin( pMesh, MATERIAL_QUADS, 2 );
	
	const Vector vecDirections[] = {
		Vector( 0, -0.707f, -0.707f ),
		Vector( 0, 0.707f, -0.707f ),
	};

	const Vector vecOffsets[] = {
		Vector( 0, 1, 0 ),
		Vector( 0, -1, 0 ),
	};

	for ( int i = 0; i < 2; ++i )
	{
		Vector dir = vecDirections[ i ];
		Vector offset = vecOffsets[ i ];

		RotateVectorYZ( dir, angle );
		RotateVectorYZ( offset, angle );

		Vector v = vec3_origin;
		builder.Position3fv( v.Base() );
		builder.AdvanceVertex();

		v += Vector( 0, 0, 1 ) * thickness;
		builder.Position3fv( v.Base() );
		builder.AdvanceVertex();

		v += dir * lineSize;
		builder.Position3fv( v.Base() );
		builder.AdvanceVertex();

		v += offset * thickness + dir * thickness * 0.707f;
		builder.Position3fv( v.Base() );
		builder.AdvanceVertex();
	}

	builder.End();
}

inline void CreateAimPanel( IMesh *pMesh, int subDivCountX, int subDivCountY,
	const QAngle &mins, const QAngle &maxs, float radius, float alpha )
{
	QAngle delta(
			AngleDistance(maxs.x, mins.x),
			AngleDistance(maxs.y, mins.y),
			AngleDistance(maxs.z, mins.z)
		);

	Vector *vectors = (Vector *)stackalloc( sizeof( Vector ) * ( subDivCountX + 1 ) * ( subDivCountY + 1 ) );

	CMeshBuilder builder;
	builder.Begin( pMesh, MATERIAL_QUADS, subDivCountX * subDivCountY );

	for ( int x = 0; x <= subDivCountX; ++x )
	{
		const float fractionX = x / float( subDivCountX );
		for ( int y = 0; y <= subDivCountY; ++y )
		{
			const float fractionY = y / float( subDivCountY );
			QAngle angle( mins.x + fractionX * delta.x,
				mins.y + fractionY * delta.y,
				mins.z );
			AngleVectors( angle, &vectors[ x + y * ( subDivCountX + 1 ) ] );
			vectors[ x + y * ( subDivCountX + 1 ) ] *= radius;
		}
	}

	for ( int x = 0; x < subDivCountX; ++x )
	{
		for ( int y = 0; y < subDivCountY; ++y )
		{
			float color[4][4] = {
				1, 1, 1, 0,
				1, 1, 1, 0,
				1, 1, 1, 0,
				1, 1, 1, 0
			};

			if ( x == 0 )
			{
				color[0][3] = alpha;
				color[3][3] = alpha;
			}
			else if ( x == subDivCountX - 1 )
			{
				color[1][3] = alpha;
				color[2][3] = alpha;
			}

			if ( y == 0 )
			{
				color[0][3] = alpha;
				color[1][3] = alpha;
			}
			else if ( y == subDivCountY - 1 )
			{
				color[2][3] = alpha;
				color[3][3] = alpha;
			}

			builder.Position3fv( vectors[ x + y * ( subDivCountX + 1 ) ].Base() );
			builder.Color4fv( color[ 0 ] );
			builder.AdvanceVertex();
			builder.Position3fv( vectors[ x + 1 + y * ( subDivCountX + 1 ) ].Base() );
			builder.Color4fv( color[ 1 ] );
			builder.AdvanceVertex();
			builder.Position3fv( vectors[ x + 1 + ( y + 1 ) * ( subDivCountX + 1 ) ].Base() );
			builder.Color4fv( color[ 2 ] );
			builder.AdvanceVertex();
			builder.Position3fv( vectors[ x + ( y + 1 ) * ( subDivCountX + 1 ) ].Base() );
			builder.Color4fv( color[ 3 ] );
			builder.AdvanceVertex();
		}
	}

	builder.End();
}

inline void CreateTargetArrows( IMesh *pMesh, float rollDegrees, float angleDegrees,
	float length, float distance, float indentation )
{
	CMeshBuilder builder;
	builder.Begin( pMesh, MATERIAL_TRIANGLES, 8 );

	for ( int i = 0; i < 4; ++i )
	{
		Vector up;
		QAngle angle( 0, 0, 90.0f * i + rollDegrees + angleDegrees );
		AngleVectors( angle, NULL, NULL, &up );
		up *= distance + length + indentation;

		builder.Position3f( 0.0f, up.y, up.z );
		builder.AdvanceVertex();

		angle.z -= angleDegrees;
		AngleVectors( angle, NULL, NULL, &up );
		Vector temp = up * distance;
		up *= ( distance + length );

		builder.Position3f( 0.0f, temp.y, temp.z );
		builder.AdvanceVertex();

		builder.Position3f( 0.0f, up.y, up.z );
		builder.AdvanceVertex();

		builder.Position3f( 0.0f, up.y, up.z );
		builder.AdvanceVertex();

		builder.Position3f( 0.0f, temp.y, temp.z );
		builder.AdvanceVertex();

		angle.z -= angleDegrees;
		AngleVectors( angle, NULL, NULL, &up );
		up *= distance + length + indentation;

		builder.Position3f( 0.0f, up.y, up.z );
		builder.AdvanceVertex();
	}

	builder.End();
}

inline void CreateDamageIndicator( IMesh *pMesh, int subDivCount, float radius, float thickness,
	float startAngle, float endAngle,
	float fadeStart, float fadeEnd )
{
	Assert( subDivCount >= 1 );

	CMeshBuilder builder;
	builder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, subDivCount * 2 );

	float color4[] = { 1, 1, 1, RemapValClamped( 1.0f, fadeStart, fadeEnd, 1.0f, 0.0f ) };
	const float angleStep = ( endAngle - startAngle ) / (float)subDivCount;
	const float toInnerRadius = ( radius - thickness ) / radius;

	Vector2D dir( GetVector2DFromAngle( startAngle ) * radius );
	builder.Position3f( 0.0f, dir.x, dir.y );
	builder.Color4fv( color4 );
	builder.AdvanceVertex();

	color4[3] = 0.0f;
	builder.Position3f( 0.0f, dir.x * toInnerRadius, dir.y * toInnerRadius );
	builder.Color4fv( color4 );
	builder.AdvanceVertex();

	const int centerPosition = subDivCount / 2;

	for ( int i = 0; i < subDivCount; ++i )
	{
		const float fadePosition = (i > centerPosition ? i - centerPosition : centerPosition - i - 1) / (float)centerPosition;
		color4[3] = RemapValClamped( fadePosition, fadeStart, fadeEnd, 1.0f, 0.0f );
		color4[3] = Bias( color4[3], 0.05f );

		dir = GetVector2DFromAngle( startAngle + ( i + 1 ) * angleStep ) * radius;
		builder.Position3f( 0.0f, dir.x, dir.y );
		builder.Color4fv( color4 );
		builder.AdvanceVertex();

		color4[3] = 0.0f;
		builder.Position3f( 0.0f, dir.x * toInnerRadius, dir.y * toInnerRadius );
		builder.Color4fv( color4 );
		builder.AdvanceVertex();
	}

	builder.End();
}

inline void CreateRoundDamageIndicator( IMesh *pMesh, int subDivCount, float radius, float thickness )
{
	Assert( subDivCount >= 1 );

	CMeshBuilder builder;
	builder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, subDivCount * 2 );

	float color4[] = { 1, 1, 1, 1 };
	const float angleStep = M_PI_F * 2.0f / subDivCount;
	const float toInnerRadius = ( radius - thickness ) / radius;

	Vector2D dir( GetVector2DFromAngle( 0.0f ) * radius );
	builder.Position3f( 0.0f, dir.x, dir.y );
	builder.Color4fv( color4 );
	builder.AdvanceVertex();

	color4[3] = 0;
	builder.Position3f( 0.0f, dir.x * toInnerRadius, dir.y * toInnerRadius );
	builder.Color4fv( color4 );
	builder.AdvanceVertex();

	for ( int i = 0; i < subDivCount; ++i )
	{
		color4[3] = 1;
		dir = GetVector2DFromAngle( ( i + 1 ) * angleStep ) * radius;
		builder.Position3f( 0.0f, dir.x, dir.y );
		builder.Color4fv( color4 );
		builder.AdvanceVertex();

		color4[3] = 0;
		builder.Position3f( 0.0f, dir.x * toInnerRadius, dir.y * toInnerRadius );
		builder.Color4fv( color4 );
		builder.AdvanceVertex();
	}

	builder.End();
}

inline void CreateAimPanelDecor( IMesh *pMesh, int subDivCountX, int subDivCountY,
	const QAngle &mins, const QAngle &maxs, float radius, float alpha )
{
	QAngle delta(
			AngleDistance(maxs.x, mins.x),
			AngleDistance(maxs.y, mins.y),
			AngleDistance(maxs.z, mins.z)
		);

	Vector *vectors = (Vector *)stackalloc( sizeof( Vector ) * ( subDivCountX + 1 ) * ( subDivCountY + 1 ) );
	for ( int x = 0; x <= subDivCountX; ++x )
	{
		const float fractionX = x / float( subDivCountX );
		for ( int y = 0; y <= subDivCountY; ++y )
		{
			const float fractionY = y / float( subDivCountY );
			QAngle angle( mins.x + fractionX * delta.x,
				mins.y + fractionY * delta.y,
				mins.z );
			AngleVectors( angle, &vectors[ x + y * ( subDivCountX + 1 ) ] );
			vectors[ x + y * ( subDivCountX + 1 ) ] *= radius;
		}
	}

	float color0[] = { 1, 1, 1, 1 };
	float color1[] = { 1, 1, 1, 1 };
	const float thickness = 0.03f;

	CMeshBuilder builder;
	builder.Begin( pMesh, MATERIAL_QUADS, subDivCountX * 2 + 2 + 4 );

	// sides
	for ( int i = 0; i <= subDivCountY; i += subDivCountY )
	{
		for ( int x = 3; x < subDivCountX; ++x )
		{
			color0[ 3 ] = ( x - 3 ) / float( subDivCountX - 3 );
			color1[ 3 ] = ( x - 2 ) / float( subDivCountX - 3 );

			const int direction = i == 0 ? 1 : -1;
			const int topIndex = x + i * ( subDivCountX + 1 );
			const int topIndexShifted = x + ( i + direction ) * ( subDivCountX + 1 );

			builder.Position3fv( vectors[ topIndex ].Base() );
			builder.Color4fv( color0 );
			builder.AdvanceVertex();

			builder.Position3fv( vectors[ topIndex + 1 ].Base() );
			builder.Color4fv( color1 );
			builder.AdvanceVertex();

			Vector vec = vectors[ topIndexShifted + 1 ] - vectors[ topIndex + 1 ];
			vec = vec * thickness + vectors[ topIndex + 1 ];
			builder.Position3fv( vec.Base() );
			builder.Color4fv( color1 );
			builder.AdvanceVertex();

			vec = vectors[ topIndexShifted ] - vectors[ topIndex ];
			vec = vec * thickness + vectors[ topIndex ];
			builder.Position3fv( vec.Base() );
			builder.Color4fv( color0 );
			builder.AdvanceVertex();
		}
	}

	color0[ 3 ] = 0.0f;
	color1[ 3 ] = 1.0f;

	// bottom
	for ( int y = 0; y <= subDivCountY; y += subDivCountY )
	{
		const int direction = y == 0 ? 1 : -1;
		const int index0 = subDivCountX + y * ( subDivCountX + 1 );
		const int index1 = subDivCountX + ( y + direction ) * ( subDivCountX + 1 );

		builder.Position3fv( vectors[ index0 ].Base() );
		builder.Color4fv( color1 );
		builder.AdvanceVertex();
		Vector vecDelta = vectors[ index0 - 1 ] - vectors[ index0 ];
		vecDelta = vecDelta * thickness + vectors[ index0 ];
		builder.Position3fv( vecDelta.Base() );
		builder.Color4fv( color1 );
		builder.AdvanceVertex();

		vecDelta = vectors[ index1 - 1 ] - vectors[ index1 ];
		vecDelta = vecDelta * thickness + vectors[ index1 ];
		builder.Position3fv( vecDelta.Base() );
		builder.Color4fv( color0 );
		builder.AdvanceVertex();
		builder.Position3fv( vectors[ index1 ].Base() );
		builder.Color4fv( color0 );
		builder.AdvanceVertex();
	}

	// top
	const int iTopWidth = 4;
	for ( int i = subDivCountY / 2 - iTopWidth; i < subDivCountY / 2 + iTopWidth; ++i )
	{
		const int index0 = 0 + i * ( subDivCountX + 1 );
		const int index1 = 0 + ( i + 1 ) * ( subDivCountX + 1 );
		color1[ 3 ] = 1.0f - abs( i - subDivCountY / 2 ) / float( iTopWidth );
		color0[ 3 ] = 1.0f - abs( i + 1 - subDivCountY / 2 ) / float( iTopWidth );

		builder.Position3fv( vectors[ index0 ].Base() );
		builder.Color4fv( color1 );
		builder.AdvanceVertex();
		Vector vecDelta = vectors[ index0 + 1 ] - vectors[ index0 ];
		vecDelta = vecDelta * thickness + vectors[ index0 ];
		builder.Position3fv( vecDelta.Base() );
		builder.Color4fv( color1 );
		builder.AdvanceVertex();

		vecDelta = vectors[ index1 + 1 ] - vectors[ index1 ];
		vecDelta = vecDelta * thickness + vectors[ index1 ];
		builder.Position3fv( vecDelta.Base() );
		builder.Color4fv( color0 );
		builder.AdvanceVertex();
		builder.Position3fv( vectors[ index1 ].Base() );
		builder.Color4fv( color0 );
		builder.AdvanceVertex();
	}

	builder.End();
}

#endif
