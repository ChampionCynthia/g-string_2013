#ifndef HOLO_SHIP_COMM_H
#define HOLO_SHIP_COMM_H

#include "holo_panel_vgui.h"
#include "vgui_controls/Controls.h"

class ISpacecraftData;

#define WAVEFORM_LENGTH 100

class CHoloShipComm : public CHoloPanelVGUI
{
	DECLARE_CLASS_GAMEROOT( CHoloShipComm, CHoloPanelVGUI );

public:
	CHoloShipComm( vgui::Panel *pParent, ISpacecraftData *pSpacecraftData );
	~CHoloShipComm();

	virtual void PerformLayout();

	void MsgFunc_HoloMessage( bf_read &msg );

protected:
	virtual void Draw( IMatRenderContext *pRenderContext );
	virtual void Think( float frametime );
	virtual void PerformLayout3D( int width, int height, bool useVR );

private:
	struct WaveHeader
	{
		char chunk_id[4];
		int chunk_size;
		char format[4];
		char subchunk1_id[4];
		int subchunk1_size;
		short int audio_format;
		short int num_channels;
		int sample_rate;			// sample_rate denotes the sampling rate.
		int byte_rate;
		short int block_align;
		short int bits_per_sample;
		char subchunk2_id[4];
		int subchunk2_size;			// subchunk2_size denotes the number of samples.
	};

	void DrawWaveForm( IMatRenderContext *pRenderContext );
	void ResetWaveForm( const char *pszWaveFile );
	void UpdateWaveForm( float frametime );

	ISpacecraftData *m_pSpacecraftData;

	vgui::Label *m_pLabelHeader;
	float m_flResetTimer;
	float m_flFlashTimer;
	float m_flWaveVisibilityAmount;

	bool m_bWaveFormActive;
	float m_flWaveFormData[ WAVEFORM_LENGTH ];
	int m_iWaveFormReadStart;
	float m_flWaveFormSampleTime;
	int m_iWaveFormSamples;

	WaveHeader m_RawWaveHeader;
	int m_iRawSamplesRead;
	CUtlBuffer m_WaveBuffer;
};

#endif