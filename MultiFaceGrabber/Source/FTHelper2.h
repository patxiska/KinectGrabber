//------------------------------------------------------------------------------
// <copyright file="FTHelper2.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#pragma once

#include <FaceTrackLib.h>
#include "KinectSensor.h"
#include "KinectRecorder.h" 
#include "KinectPlayer.h" 
#include "FTRecorder.h"

struct FTHelperContext
{
    IFTFaceTracker*     m_pFaceTracker;
    IFTResult*          m_pFTResult;
    FT_VECTOR3D         m_hint3D[2];
    bool                m_LastTrackSucceeded;
    int                 m_CountUntilFailure;
    UINT                m_SkeletonId;
};

typedef void (*FTHelper2CallBack)(PVOID lpParam, UINT userId);
typedef void (*FTHelper2UserSelectCallBack)(PVOID lpParam, KinectSensor * pKinectSensor, UINT nbUsers, FTHelperContext* pUserContexts);

class FTHelper2
{
public:
    FTHelper2();
    ~FTHelper2();

	HRESULT Init(HWND hWnd, UINT nbUsers, FTHelper2CallBack callBack, PVOID callBackParam, FTHelper2UserSelectCallBack userSelectCallBack, PVOID userSelectCallBackParam, NUI_IMAGE_TYPE depthType, NUI_IMAGE_RESOLUTION depthRes, BOOL bNearMode, NUI_IMAGE_TYPE colorType, NUI_IMAGE_RESOLUTION colorRes, BOOL bSeatedSkeletonMode, bool bPlay, bool bRecord, char* sequencePath, char* ftSequencePath);
    //HRESULT Init(HWND hWnd, UINT nbUsers, FTHelper2CallBack callBack, PVOID callBackParam, FTHelper2UserSelectCallBack userSelectCallBack, PVOID userSelectCallBackParam,
    //    NUI_IMAGE_TYPE depthType, NUI_IMAGE_RESOLUTION depthRes, BOOL bNearMode, NUI_IMAGE_TYPE colorType, NUI_IMAGE_RESOLUTION colorRes);

    HRESULT		Stop();
    IFTResult*	GetResult(UINT userId)       { return(m_UserContext[userId].m_pFTResult);}
    BOOL		IsKinectPresent()                  { return(m_KinectSensorPresent);}
    IFTImage*	GetColorImage()               { return(m_colorImage);}
    float		GetXCenterFace()                  { return(m_XCenterFace);}
    float		GetYCenterFace()                  { return(m_YCenterFace);}
    BOOL		GetDrawMask()                      { return(m_DrawMask);}
    IFTFaceTracker* GetTracker(UINT userId) { return(m_UserContext[userId].m_pFaceTracker);}
    HRESULT		GetCameraConfig(FT_CAMERA_CONFIG* cameraConfig);

	void		SetSaveFTImages(BOOL bSaveFTImages)     { m_bSaveFTImages = bSaveFTImages; m_DrawMask = m_bSaveFTImages || m_bShowFT;}
	void		SetShowFT(BOOL bShowFT)					{ m_bShowFT = bShowFT; m_DrawMask = m_bSaveFTImages || m_bShowFT;}

private:
    KinectSensor*               m_KinectSensor;
	KinectPlayer*               m_KinectPlayer;

    BOOL                        m_KinectSensorPresent;
    UINT                        m_nbUsers;
    FTHelperContext*            m_UserContext;
    HWND                        m_hWnd;
    IFTImage*                   m_colorImage;
    IFTImage*                   m_depthImage;
    bool                        m_ApplicationIsRunning;
    FTHelper2CallBack           m_CallBack;
    LPVOID                      m_CallBackParam;
    FTHelper2UserSelectCallBack m_UserSelectCallBack;
    LPVOID                      m_UserSelectCallBackParam;
    float                       m_XCenterFace;
    float                       m_YCenterFace;
    HANDLE                      m_hFaceTrackingThread;
    BOOL                        m_DrawMask;
    NUI_IMAGE_TYPE              m_depthType;
    NUI_IMAGE_RESOLUTION        m_depthRes;
    BOOL                        m_bNearMode;
    NUI_IMAGE_TYPE              m_colorType;
    NUI_IMAGE_RESOLUTION        m_colorRes;
    BOOL						m_bSeatedSkeleton;
	
	bool						m_isRecording;
	bool						m_isPlaying;

	bool						m_bSaveFTImages;
	bool						m_bShowFT;

	FTRecorder					m_FTRecorder;

	char						m_pathName[KINECT_MAX_PATH];
	char						m_FTpathName[KINECT_MAX_PATH];
	
	RECT**						m_roiFaces;

	HRESULT				GetNewImages();
	//void				SetDrawMask(BOOL drawMask)				{ m_DrawMask = drawMask;}
    BOOL				SubmitFraceTrackingResult(IFTResult* pResult, UINT userId);
    void				SetCenterOfImage(IFTResult* pResult);
    void				CheckCameraInput();
    DWORD WINAPI		FaceTrackingThread();
    static DWORD WINAPI FaceTrackingStaticThread(PVOID lpParam);
    static void			SelectUserToTrack(KinectSensor * pKinectSensor, UINT nbUsers, FTHelperContext* pUserContexts);
	bool				FaceRepeated(UINT i);
	static inline LONG	diffabs(LONG left, LONG right);
	void				visualizeDepthAndPlayers(IFTImage* faceImage, IFTImage* depthImage, LONG subsample, LONG xpos, LONG ypos);
	void				displayPYRDBesideEggs(IFTResult* pResult, UINT userId);

	struct CharRGB {
		unsigned char R, G, B;
		CharRGB(unsigned char _R,unsigned char _G,unsigned char _B) : R(_R),G(_G),B(_B) {}
	};

	static CharRGB		hsv2rgb(float H, float S, float V);
	static CharRGB		depth2RGB(USHORT depth);

};
