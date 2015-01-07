#include "cbase.h"

static bool SolveQuadratic( float a, float b, float c, Vector2D &out )
{
	bool bResult = false;
	if ( abs( a ) < 1e-6 )
	{
		if ( abs( b ) < 1e-6 )
		{
			if ( abs( c ) < 1e-6 )
			{
				bResult = true;
				out.Init();
			}
		}
		else
		{
			bResult = true;
			out.Init( -c / b, -c / b );
		}
	}
	else
	{
		float disc = ( b * b ) - ( 4.0f * a * c );
		if ( disc >= 0 )
		{
			disc = sqrt( disc );
			a = 2.0f * a;
			out.Init( ( -b -disc ) / a, ( -b + disc ) / a );
			bResult = true;
		}
	}
	return bResult;
}

bool UTIL_PredictProjectileTarget( Vector vecProjectileOrigin, Vector vecTargetOrigin, Vector vecTargetLinearVelocity,
								float flLinearProjectileSpeed, Vector &vecPredictedTarget )
{
	bool bResult = false;
	const Vector vecT = vecTargetOrigin - vecProjectileOrigin;

	const float a = vecTargetLinearVelocity.LengthSqr() - flLinearProjectileSpeed * flLinearProjectileSpeed;
	const float b = 2 * ( vecTargetLinearVelocity.x * vecT.x
		+ vecTargetLinearVelocity.y * vecT.y
		+ vecTargetLinearVelocity.z * vecT.z );
	const float c = vecT.LengthSqr();

	Vector2D ts;
	if ( SolveQuadratic( a, b, c, ts ) )
	{
		float t0 = ts[0];
		float t1 = ts[1];
		float t = MIN( t0, t1 );

		if ( t < 0 )
		{
			t = MAX( t0, t1 );
		}

		if ( t > 0 )
		{
			bResult = true;
			vecPredictedTarget = vecTargetOrigin + vecTargetLinearVelocity * t;
		}
	}
	return bResult;
}
