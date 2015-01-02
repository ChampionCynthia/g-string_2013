#ifndef HOLO_UTILITIES_H
#define HOLO_UTILITIES_H

inline Vector2D GetVector2DFromAngle( float radians )
{
	return Vector2D( cos( radians ), -sin( radians ) );
}

inline void CreateSlantedRect( IMesh *pMesh, float x, float y, float width, float height, float offset = 0.0f )
{
	CMeshBuilder builder;
	builder.Begin( pMesh, MATERIAL_QUADS, 1 );

	const float color3[] = { 1, 1, 1 };
	const float normal[] = { 1, 0, 0 };

	builder.Position3f( 0.0f, x, y );
	builder.Color3fv( color3 );
	builder.Normal3fv( normal );
	builder.AdvanceVertex();

	builder.Position3f( 0.0f, x + width, y + offset );
	builder.Color3fv( color3 );
	builder.Normal3fv( normal );
	builder.AdvanceVertex();

	builder.Position3f( 0.0f, x + width, y + offset + height );
	builder.Color3fv( color3 );
	builder.Normal3fv( normal );
	builder.AdvanceVertex();

	builder.Position3f( 0.0f, x, y + height );
	builder.Color3fv( color3 );
	builder.Normal3fv( normal );
	builder.AdvanceVertex();

	builder.End();
}

inline void CreateArc( IMesh *pMesh, int subDivCount, float radius, float thickness,
	float startAngle, float endAngle )
{
	Assert( subDivCount >= 1 );

	CMeshBuilder builder;
	builder.Begin( pMesh, MATERIAL_TRIANGLE_STRIP, subDivCount * 2 );

	const float color3[] = { 1, 1, 1 };
	const float normal[] = { 0, 0, 1 };
	const float angleStep = ( endAngle - startAngle ) / (float)subDivCount;
	const float toInnerRadius = ( radius - thickness ) / radius;

	Vector2D dir( GetVector2DFromAngle( startAngle ) * radius );
	builder.Position3f( dir.x, dir.y, 0.0f );
	builder.Color3fv( color3 );
	builder.Normal3fv( normal );
	builder.AdvanceVertex();

	builder.Position3f( dir.x * toInnerRadius, dir.y * toInnerRadius, 0.0f );
	builder.Color3fv( color3 );
	builder.Normal3fv( normal );
	builder.AdvanceVertex();

	for ( int i = 0; i < subDivCount; ++i )
	{
		dir = GetVector2DFromAngle( startAngle + ( i + 1 ) * angleStep ) * radius;
		builder.Position3f( dir.x, dir.y, 0.0f );
		builder.Color3fv( color3 );
		builder.Normal3fv( normal );
		builder.AdvanceVertex();

		builder.Position3f( dir.x * toInnerRadius, dir.y * toInnerRadius, 0.0f );
		builder.Color3fv( color3 );
		builder.Normal3fv( normal );
		builder.AdvanceVertex();
	}

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
	const float normal[] = { 0, 0, 1 };
	const float angleStep = ( endAngle - startAngle ) / (float)subDivCount;
	const float toInnerRadius = ( radius - thickness ) / radius;

	Vector2D dir( GetVector2DFromAngle( startAngle ) * radius );
	builder.Position3f( radius, 0.0f, 0.0f );
	builder.Color4fv( color4 );
	builder.Normal3fv( normal );
	builder.AdvanceVertex();

	builder.Position3f( radius - thickness, 0.0f, 0.0f );
	builder.Color4fv( color4 );
	builder.Normal3fv( normal );
	builder.AdvanceVertex();

	const int centerPosition = subDivCount / 2;

	for ( int i = 0; i < subDivCount; ++i )
	{
		const float fadePosition = (i > centerPosition ? i - centerPosition : centerPosition - i - 1) / (float)centerPosition;
		color4[3] = RemapValClamped( fadePosition, fadeStart, fadeEnd, 1.0f, 0.0f );

		dir = GetVector2DFromAngle( startAngle + ( i + 1 ) * angleStep ) * radius;
		builder.Position3f( dir.x, dir.y, 0.0f );
		builder.Color4fv( color4 );
		builder.Normal3fv( normal );
		builder.AdvanceVertex();

		builder.Position3f( dir.x * toInnerRadius, dir.y * toInnerRadius, 0.0f );
		builder.Color4fv( color4 );
		builder.Normal3fv( normal );
		builder.AdvanceVertex();
	}

	builder.End();
}

#endif
