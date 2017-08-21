//------------------------------------------------------------------------------
// <copyright file="FTHelper2.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#pragma once
#include <FaceTrackLib.h>
#include "KinectSensor.h"
#include "KinectRecorder.h" 

struct FTHelperContext
{
    IFTFaceTracker*     m_pFaceTracker;
    IFTResult*          m_pFTResult;
    //FT_VECTOR3D         m_hint3D[2];
    bool                m_LastTrackSucceeded;
    //int                 m_CountUntilFailure;
    //UINT                m_SkeletonId;
};

typedef void (*FTHelper2CallBack)(PVOID lpParam, UINT userId);
typedef void (*FTHelper2UserSelectCallBack)(PVOID lpParam, KinectSensor * pKinectSensor, UINT nbUsers, FTHelperContext* pUserContexts);


class FTHelper2
{
public:
    FTHelper2();
    ~FTHelper2();

    HRESULT Init(HWND hWnd, UINT nbUsers, FTHelper2CallBack callBack, PVOID callBackParam, FTHelper2UserSelectCallBack userSelectCallBack, PVOID userSelectCallBackParam,
        NUI_IMAGE_TYPE depthType, NUI_IMAGE_RESOLUTION depthRes, BOOL bNearMode, NUI_IMAGE_TYPE colorType, NUI_IMAGE_RESOLUTION colorRes, BOOL bSeatedSkeletonMode);
    //HRESULT Init(HWND hWnd, UINT nbUsers, FTHelper2CallBack callBack, PVOID callBackParam, FTHelper2UserSelectCallBack userSelectCallBack, PVOID userSelectCallBackParam,
    //    NUI_IMAGE_TYPE depthType, NUI_IMAGE_RESOLUTION depthRes, BOOL bNearMode, NUI_IMAGE_TYPE colorType, NUI_IMAGE_RESOLUTION colorRes);

    HRESULT Stop();
    //IFTResult* GetResult(UINT userId)       { return(m_UserContext[userId].m_pFTResult);}
    BOOL IsKinectPresent()                  { return(m_KinectSensorPresent);}
    //IFTImage* GetColorImage()               { return(m_colorImage);}
	IFTImage* GetColorImage()               { return((m_KinectSensorPresent) ?  m_KinectSensor.GetVideoBuffer() : NULL);}
	
    //float GetXCenterFace()                  { return(m_XCenterFace);}
    //float GetYCenterFace()                  { return(m_YCenterFace);}
    //void SetDrawMask(BOOL drawMask)         { m_DrawMask = drawMask;}
    //BOOL GetDrawMask()                      { return(m_DrawMask);}
    //IFTFaceTracker* GetTracker(UINT userId) { return(m_UserContext[userId].m_pFaceTracker);}
    HRESULT GetCameraConfig(FT_CAMERA_CONFIG* cameraConfig);

	void	SetMaxFrames(UINT frames) {m_KinectRecorder.SetMaxFrames(frames);}
	UINT	GetMaxFrames() {return m_KinectRecorder.GetMaxFrames();}
	BOOL	IsRecording() {return m_isRecording;}
	KINECT_RECORDER_STATUS	GetRecordingStatus() {return m_KinectRecorder.GetStatus();}

	UINT	GetFramesCount() {return m_KinectRecorder.GetFramesCount();}

private:
    KinectSensor                m_KinectSensor;
    BOOL                        m_KinectSensorPresent;
    UINT                        m_nbUsers;
    //FTHelperContext*            m_UserContext;
    HWND                        m_hWnd;
    //IFTImage*                   m_colorImage;
    //IFTImage*                   m_depthImage;
    bool                        m_ApplicationIsRunning;
    //FTHelper2CallBack           m_CallBack;
    LPVOID                      m_CallBackParam;
    FTHelper2UserSelectCallBack m_UserSelectCallBack;
    LPVOID                      m_UserSelectCallBackParam;
    //float                       m_XCenterFace;
    //float                       m_YCenterFace;
    HANDLE                      m_hFaceTrackingThread;
    //BOOL                        m_DrawMask;
    NUI_IMAGE_TYPE              m_depthType;
    NUI_IMAGE_RESOLUTION        m_depthRes;
    BOOL                        m_bNearMode;
    NUI_IMAGE_TYPE              m_colorType;
    NUI_IMAGE_RESOLUTION        m_colorRes;
    BOOL						m_bSeatedSkeleton;
	bool						m_isRecording;
	KinectRecorder				m_KinectRecorder;

    //BOOL SubmitFraceTrackingResult(IFTResult* pResult, UINT userId);
    //void SetCenterOfImage(IFTResult* pResult);
    void CheckCameraInput();
    DWORD WINAPI FaceTrackingThread();
    static DWORD WINAPI FaceTrackingStaticThread(PVOID lpParam);
    //static void SelectUserToTrack(KinectSensor * pKinectSensor, UINT nbUsers, FTHelperContext* pUserContexts);
};
