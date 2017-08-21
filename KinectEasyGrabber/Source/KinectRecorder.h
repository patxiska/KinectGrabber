#pragma once

#include <FaceTrackLib.h>
#include <NuiApi.h>

#ifndef KINECT_MAX_PATH
  #define KINECT_MAX_PATH 512
#endif

typedef 
enum _KINECT_RECORDER_STATUS
    {	KINECT_RECORDER_STATUS_INVALID	= -1,
	KINECT_RECORDER_STATUS_STOPPED		= 0,
	KINECT_RECORDER_STATUS_RECORDING	= ( KINECT_RECORDER_STATUS_STOPPED + 1 ) ,
	KINECT_RECORDER_STATUS_SAVING		= ( KINECT_RECORDER_STATUS_RECORDING + 1 ) ,
	KINECT_RECORDER_STATUS_DUMPED		= ( KINECT_RECORDER_STATUS_SAVING + 1 ) 
    } 	KINECT_RECORDER_STATUS;

class KinectRecorder
{
public:
	KinectRecorder();
	~KinectRecorder();

	HRESULT Init(NUI_IMAGE_TYPE depthType, NUI_IMAGE_RESOLUTION depthRes, BOOL bNearMode, NUI_IMAGE_TYPE colorType, NUI_IMAGE_RESOLUTION colorRes, bool bskeletonMode, const char* path, const char* seq_name);

	void	addRGBXFrame(BYTE* colorImage, LARGE_INTEGER colorTimeStamp);
	void	addDepthFrame(USHORT* depthImage, LARGE_INTEGER depthTimeStamp, LONG* ccImage = NULL);
	//void	addColorCoordinatesFrame(LONG* ccImage);

	bool	RecordToDisk();

	void	SetMaxFrames(UINT frames) {m_maxFrames = frames;}
	UINT	GetMaxFrames() {return m_maxFrames;}

	UINT	GetFramesCount() {return max(m_colorframeIndex,m_depthframeIndex);}

	KINECT_RECORDER_STATUS	GetStatus() {return m_status;}

private:
	char					m_frameBasename[KINECT_MAX_PATH];
	char					m_sequenceName[KINECT_MAX_PATH];
	int						m_colorframeIndex;
	int						m_depthframeIndex;
	//int						m_ccframeIndex;
	int						m_maxFrames;
	int						m_bufferSize;
	KINECT_RECORDER_STATUS	m_status;

	USHORT**                m_outputArrayDepthD16;
	BYTE**                  m_outputArrayRGBX;
	LONG**                  m_outputArrayColorCoordinates;

	LARGE_INTEGER*          m_depthArrayTimeStamp;
    LARGE_INTEGER*          m_colorArrayTimeStamp;

	bool					m_isSaving;
	bool					m_nearMode;
	bool					m_skeletonMode;

    LONG					m_depthWidth;
    LONG					m_depthHeight;
    LONG					m_colorWidth;
    LONG					m_colorHeight;

	void	Zero();
	void	Release();
	void	dumpToDisk(int frameIndex, char* frameBasename, USHORT* depthD16, BYTE* colorRGBX, LONG* colorCoordinates, LARGE_INTEGER depthTimeStamp, LARGE_INTEGER colorTimeStamp);
	HRESULT saveTimeStampSequence();
	HRESULT saveDepthSequence();
	HRESULT saveColorSequence();
	HRESULT saveCCSequence();
};

