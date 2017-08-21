#pragma once

#include <FaceTrackLib.h>
#include <NuiApi.h>

#ifndef KINECT_MAX_PATH
  #define KINECT_MAX_PATH 512
#endif

class KinectPlayer 
{
public:
    KinectPlayer();
    ~KinectPlayer();

    HRESULT Init(const char* path);
    void	Release(); //private?

    HRESULT     GetVideoConfiguration(FT_CAMERA_CONFIG* videoConfig);
    HRESULT     GetDepthConfiguration(FT_CAMERA_CONFIG* depthConfig);

    BYTE*		GetVideoBuffer();
    USHORT*		GetDepthBuffer();

	void		nextFrame();
	bool		hasNext();

	int			GetFrameCount() { return m_totalFrames; }

private:
	char					m_frameBasename[KINECT_MAX_PATH];
	int						m_colorframeIndex;
	int						m_depthframeIndex;
	int						m_ccframeIndex;
	int						m_totalFrames;
	//int						m_bufferSize;

	USHORT**                m_outputArrayDepthD16;
	BYTE**                  m_outputArrayRGBX;
	LONG**                  m_outputArrayColorCoordinates;

	LARGE_INTEGER*          m_depthArrayTimeStamp;
    LARGE_INTEGER*          m_colorArrayTimeStamp;

	//bool					m_dumped;

    LONG					m_depthWidth;
    LONG					m_depthHeight;
    LONG					m_colorWidth;
    LONG					m_colorHeight;
	bool					m_nearMode;
	bool					m_skeletonMode;
	void			Zero();

	HRESULT LoadSequenceInfo();
	HRESULT LoadRGBSequence();
	HRESULT LoadDepthSequence();
	HRESULT LoadCCoordinatesSequence();
	
};
