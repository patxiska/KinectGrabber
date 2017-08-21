#pragma once

#include <FaceTrackLib.h>
#include <NuiApi.h>

class KinectRecorder
{
public:
	KinectRecorder(void);
	~KinectRecorder(void);

	HRESULT Init(NUI_IMAGE_TYPE depthType, NUI_IMAGE_RESOLUTION depthRes, BOOL bNearMode, NUI_IMAGE_TYPE colorType, 
				 NUI_IMAGE_RESOLUTION colorRes, int total_frames);

	void	addRGBFrame(BYTE* colorImage, LARGE_INTEGER colorTimeStamp);
	void	addDepthFrame(USHORT* depthImage, LARGE_INTEGER depthTimeStamp);

	void	RecordToDisk();

private:
	char*					m_frameBasename;
	int						m_colorframeIndex;
	int						m_depthframeIndex;
	int						m_totalFrames;

	USHORT**                m_outputArrayDepthD16;
	BYTE**                  m_outputArrayRGBX;
	//LONG**                  m_outputArrayColorCoordinates;

	LARGE_INTEGER*          m_depthArrayTimeStamp;
    LARGE_INTEGER*          m_colorArrayTimeStamp;

	bool					m_dumped;

    LONG					m_depthWidth;
    LONG					m_depthHeight;
    LONG					m_colorWidth;
    LONG					m_colorHeight;

	void Zero();
	void Release();
	void dumpToDisk(int frameIndex, char* frameBasename, USHORT* depthD16, BYTE* colorRGBX, LONG* colorCoordinates, LARGE_INTEGER depthTimeStamp, LARGE_INTEGER colorTimeStamp);
};

