#ifndef HOLO_PANEL_H
#define HOLO_PANEL_H

#include "vgui_controls/Panel.h"

#define HOLO_COLOR255_FRIENDLY 0.3f * 255.0f, 1.0f * 255.0f, 0.5f * 255.0f
#define HOLO_COLOR_FRIENDLY 0.3f, 1.0f, 0.5f

#define HOLO_COLOR_DEFAULT 0.929f, 0.8f, 0.502f
#define HOLO_COLOR255_DEFAULT 0.929f * 255.0f, 0.8f * 255.0f, 0.502f * 255.0f

#define HOLO_COLOR_HIGHLIGHT 0.129f, 0.333f, 0.914f
#define HOLO_COLOR255_HIGHLIGHT 0.129f * 255.0f, 0.333f * 255.0f, 0.914f * 255.0f

#define HOLO_COLOR_WARNING 1.0f, 0.294f, 0.176f
#define HOLO_COLOR255_WARNING 1.0f * 255.0f, 0.294f * 255.0f, 0.176f * 255.0f

#define DECLARE_HOLO_MESSAGE( message, className ) \
	static CUtlVector< className* > __g_MsgVector_ ## message; \
	static void __MsgFunc_ ## message( bf_read &msg ) \
	{ \
		FOR_EACH_VEC( __g_MsgVector_ ## message, i ) \
		{ \
			__g_MsgVector_ ## message[ i ]->MsgFunc_ ## message( msg ); \
			msg.Reset(); \
		} \
	} \
	USER_MESSAGE_REGISTER( message )

#define ADD_HOLO_MESSAGE( message ) \
	__g_MsgVector_ ## message.AddToTail( this )

#define REMOVE_HOLO_MESSAGE( message ) \
	__g_MsgVector_ ## message.FindAndRemove( this )

#define LOAD_SHARED_MATERIAL_REFERENCE( reference, material ) \
	if ( reference.IsValid() ) \
	{ \
		reference->IncrementReferenceCount(); \
	} \
	else \
	{ \
		reference.Init( material ); \
	}

class CHoloPanel : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE_NOBASE( CHoloPanel )
public:
	CHoloPanel( vgui::Panel *pParent, const char *pszName );
	virtual ~CHoloPanel();
	virtual void Setup();

	void SetAngles( const QAngle &angles );
	void SetOrigin( const Vector &origin );

	void PreRenderHierarchy( IMatRenderContext *pRenderContext, Rect_t &position, int maxWidth, int maxHeight );
	void DrawHierarchy( IMatRenderContext *pRenderContext );
	void ThinkHierarchy( float frametime );
	void PerformLayout3DHierarchy( int width, int height, bool useVR );

	void UpdateTransformation();
	void MakeDirty() { m_bTransformationDirty = true; }

	enum MaterialType
	{
		MATERIALTYPE_NORMAL = 0,
		MATERIALTYPE_NORMAL_SCANLINES,
		MATERIALTYPE_VERTEXCOLOR,
		MATERIALTYPE_SCANLINES_VERTEXCOLOR,
		//MATERIALTYPE_VERTEXCOLOR_LINEAR,
		MATERIALTYPE_GLOW,
		MATERIALTYPE_VGUI,
		MATERIALTYPE_COUNT
	};

	float GetHoloAlpha() const { return m_flAlpha; }
	float GetHoloScale() const { return m_flScale; }

protected:
	virtual void PreRender( IMatRenderContext *pRenderContext, Rect_t &position, int maxWidth, int maxHeight );
	virtual void Draw( IMatRenderContext *pRenderContext );
	virtual void Think( float frametime );
	virtual void PerformLayout3D( int width, int height, bool useVR );

	IMaterialVar *GetColorVar( MaterialType type = MATERIALTYPE_NORMAL );
	void SetHoloAlpha( float flAlpha, MaterialType type = MATERIALTYPE_NORMAL );
	IMaterial *GetMaterial( MaterialType type = MATERIALTYPE_NORMAL );

	const QAngle &GetAngles();
	const Vector &GetOrigin();
	Vector GetHoloOffset() const { return Vector( m_flOffsetX, m_flOffsetY, m_flOffsetZ ); }
	QAngle GetHoloAngle() const { return QAngle( m_flAngleX, m_flAngleY, m_flAngleZ ); }

private:
	QAngle m_Angles;
	Vector m_Origin;
	matrix3x4_t m_Transformation;
	bool m_bTransformationDirty;

	CPanelAnimationVar( float, m_flAlpha, "holoalpha", "1.0" );
	CPanelAnimationVar( float, m_flScale, "holoscale", "1.0" );

	CPanelAnimationVar( float, m_flOffsetX, "offsetx", "0.0" );
	CPanelAnimationVar( float, m_flOffsetY, "offsety", "0.0" );
	CPanelAnimationVar( float, m_flOffsetZ, "offsetz", "0.0" );
	CPanelAnimationVar( float, m_flAngleX, "anglex", "0.0" );
	CPanelAnimationVar( float, m_flAngleY, "angley", "0.0" );
	CPanelAnimationVar( float, m_flAngleZ, "anglez", "0.0" );
};

#endif
