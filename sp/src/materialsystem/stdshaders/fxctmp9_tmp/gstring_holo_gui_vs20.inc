#include "shaderlib/cshader.h"
class gstring_holo_gui_vs20_Static_Index
{
private:
	int m_nVERTEXCOLOR;
#ifdef _DEBUG
	bool m_bVERTEXCOLOR;
#endif
public:
	void SetVERTEXCOLOR( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nVERTEXCOLOR = i;
#ifdef _DEBUG
		m_bVERTEXCOLOR = true;
#endif
	}
	void SetVERTEXCOLOR( bool i )
	{
		m_nVERTEXCOLOR = i ? 1 : 0;
#ifdef _DEBUG
		m_bVERTEXCOLOR = true;
#endif
	}
private:
	int m_nTEXTURE;
#ifdef _DEBUG
	bool m_bTEXTURE;
#endif
public:
	void SetTEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nTEXTURE = i;
#ifdef _DEBUG
		m_bTEXTURE = true;
#endif
	}
	void SetTEXTURE( bool i )
	{
		m_nTEXTURE = i ? 1 : 0;
#ifdef _DEBUG
		m_bTEXTURE = true;
#endif
	}
public:
	gstring_holo_gui_vs20_Static_Index( )
	{
#ifdef _DEBUG
		m_bVERTEXCOLOR = false;
#endif // _DEBUG
		m_nVERTEXCOLOR = 0;
#ifdef _DEBUG
		m_bTEXTURE = false;
#endif // _DEBUG
		m_nTEXTURE = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bVERTEXCOLOR && m_bTEXTURE;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nVERTEXCOLOR ) + ( 2 * m_nTEXTURE ) + 0;
	}
};
#define shaderStaticTest_gstring_holo_gui_vs20 vsh_forgot_to_set_static_VERTEXCOLOR + vsh_forgot_to_set_static_TEXTURE + 0
class gstring_holo_gui_vs20_Dynamic_Index
{
public:
	gstring_holo_gui_vs20_Dynamic_Index()
	{
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
#endif // _DEBUG
		return 0;
	}
};
#define shaderDynamicTest_gstring_holo_gui_vs20 0
