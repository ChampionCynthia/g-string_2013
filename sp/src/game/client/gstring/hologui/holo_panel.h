#ifndef HOLO_PANEL_H
#define HOLO_PANEL_H

#define HOLO_COLOR_DEFAULT 0.929f, 0.8f, 0.502f
#define HOLO_COLOR255_DEFAULT 0.929f * 255.0f, 0.8f * 255.0f, 0.502f * 255.0f

#define HOLO_COLOR_HIGHLIGHT 0.129f, 0.333f, 0.914f
#define HOLO_COLOR255_HIGHLIGHT 0.129f * 255.0f, 0.333f * 255.0f, 0.914f * 255.0f

#define HOLO_COLOR_WARNING 1.0f, 0.294f, 0.176f
#define HOLO_COLOR255_WARNING 1.0f * 255.0f, 0.294f * 255.0f, 0.176f * 255.0f

class CHoloPanel
{
public:
	CHoloPanel();
	virtual ~CHoloPanel();
	virtual void Setup() {}

	void SetAngles( const QAngle &angles );
	void SetOrigin( const Vector &origin );

	void PreRenderHierarchy( IMatRenderContext *pRenderContext, Rect_t &position, int maxWidth, int maxHeight );
	void DrawHierarchy( IMatRenderContext *pRenderContext );
	void ThinkHierarchy( float frametime );

protected:
	virtual void PreRender( IMatRenderContext *pRenderContext, Rect_t &position, int maxWidth, int maxHeight );
	virtual void Draw( IMatRenderContext *pRenderContext );
	virtual void Think( float frametime );

	IMaterialVar *GetColorVar();
	IMaterialVar *GetAlphaVar();

	const QAngle &GetAngles();
	const Vector &GetOrigin();

	CMaterialReference m_MaterialWhite;

private:
	void UpdateTransformation();

	QAngle m_Angles;
	Vector m_Origin;
	matrix3x4_t m_Transformation;
	bool m_bTransformationDirty;
};

#endif
