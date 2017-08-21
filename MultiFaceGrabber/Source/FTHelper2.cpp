//------------------------------------------------------------------------------
// <copyright file="FTHelper2.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include "StdAfx.h"
#include "FTHelper2.h"
#include "Visualize.h"

#ifdef SAMPLE_OPTIONS
#include "Options.h"
#else
PVOID _opt = NULL;
#endif

FTHelper2::FTHelper2()
{
    m_UserContext = 0;
    m_hWnd = NULL;
    m_colorImage = NULL;
    m_depthImage = NULL;
    m_ApplicationIsRunning = false;
    m_CallBack = NULL;
    m_CallBackParam = NULL;
    m_UserSelectCallBack = NULL;
    m_UserSelectCallBackParam = NULL;
    m_XCenterFace = 0;
    m_YCenterFace = 0;
    m_hFaceTrackingThread = NULL;
    m_depthType = NUI_IMAGE_TYPE_DEPTH;
    m_depthRes = NUI_IMAGE_RESOLUTION_INVALID;
    m_bNearMode = FALSE;
    m_colorType = NUI_IMAGE_TYPE_COLOR;
    m_colorRes = NUI_IMAGE_RESOLUTION_INVALID;
    m_bSeatedSkeleton = FALSE;
	m_isRecording = m_isPlaying = false;
	m_KinectSensor = NULL;
	m_KinectPlayer = NULL;
	m_pathName[0] = '\0';
	m_FTpathName[0] = '\0';
	m_roiFaces = NULL;
	m_bSaveFTImages = FALSE;
	m_bShowFT = FALSE;
	m_DrawMask = m_bSaveFTImages || m_bShowFT;
}

FTHelper2::~FTHelper2()
{
    Stop();
}

HRESULT FTHelper2::Init( HWND hWnd, UINT nbUsers, FTHelper2CallBack callBack, PVOID callBackParam, FTHelper2UserSelectCallBack userSelectCallBack, PVOID userSelectCallBackParam, NUI_IMAGE_TYPE depthType, NUI_IMAGE_RESOLUTION depthRes, BOOL bNearMode, NUI_IMAGE_TYPE colorType, NUI_IMAGE_RESOLUTION colorRes, BOOL bSeatedSkeletonMode, bool bPlay, bool bRecord, char* sequencePath, char* ftSequencePath )
{
    if (!hWnd || !callBack)
    {
        return E_INVALIDARG;
    }
    m_hWnd = hWnd;
    m_CallBack = callBack;
    m_CallBackParam = callBackParam;
    m_UserSelectCallBack = userSelectCallBack;
    m_UserSelectCallBackParam = userSelectCallBackParam;
    m_nbUsers = nbUsers;
    m_ApplicationIsRunning = true;
    m_depthType = depthType;
    m_depthRes = depthRes;
    m_bNearMode = bNearMode;
    m_bSeatedSkeleton = bSeatedSkeletonMode;
    m_colorType = colorType;
    m_colorRes = colorRes;
	m_isPlaying = bPlay;
	m_isRecording = bRecord;

	strcpy(m_pathName,sequencePath);
	strcpy(m_FTpathName,ftSequencePath);

    m_hFaceTrackingThread = CreateThread(NULL, 0, FaceTrackingStaticThread, (PVOID)this, 0, 0);

    return S_OK;
}

HRESULT FTHelper2::Stop()
{
    m_ApplicationIsRunning = false;
    if (m_hFaceTrackingThread)
    {
        WaitForSingleObject(m_hFaceTrackingThread, 1000);
    }
    m_hFaceTrackingThread = 0;

    if (m_UserContext != 0)
    {
        for (UINT i=0; i<m_nbUsers; i++)
        {
            if (m_UserContext[i].m_pFTResult != 0)
            {
                m_UserContext[i].m_pFTResult->Release();
                m_UserContext[i].m_pFTResult = 0;
            }
            if (m_UserContext[i].m_pFaceTracker != 0)
            {
                m_UserContext[i].m_pFaceTracker->Release();
                m_UserContext[i].m_pFaceTracker = 0;
            }
        }
        delete[] m_UserContext;
        m_UserContext = 0;
    }

    if (m_colorImage != NULL)
    {
        m_colorImage->Release();
        m_colorImage = NULL;
    }

    if (m_depthImage != NULL)
    {
        m_depthImage->Release();
        m_depthImage = NULL;
    }
    m_CallBack = NULL;

	if(m_KinectSensor) delete m_KinectSensor;
	m_KinectSensor = NULL;

	if(m_KinectPlayer) delete m_KinectPlayer;
	m_KinectPlayer = NULL;

	m_isPlaying = m_isRecording = false;
	m_pathName[0] = '\0';
	m_FTpathName[0] = '\0';

	if (m_roiFaces)
	{
		for (UINT i=0; i<m_nbUsers; i++)
		{
			delete m_roiFaces[i];
			m_roiFaces[i] = NULL;
		}
		delete [] m_roiFaces;
		m_roiFaces = NULL;		
	}

    return S_OK;
}

DWORD s_ColorCode[] = {0x00FFFF00, 0x00FF0000,  0x0000FF00, 0x0000FFFF, 0x00FF00FF, 0x000000FF};


BOOL FTHelper2::SubmitFraceTrackingResult(IFTResult* pResult, UINT userId)
{
    if (pResult != NULL && SUCCEEDED(pResult->GetStatus()))
    {
        if (m_CallBack)
        {
            (*m_CallBack)(m_CallBackParam, userId);
        }

        if (m_DrawMask)
        {
            FLOAT* pSU = NULL;
            UINT numSU;
            BOOL suConverged;
            m_UserContext[userId].m_pFaceTracker->GetShapeUnits(NULL, &pSU, &numSU, &suConverged);
            POINT viewOffset = {0, 0};
            FT_CAMERA_CONFIG cameraConfig;
            if (m_KinectSensorPresent)
            {
				if (m_isPlaying)
				{
					m_KinectPlayer->GetVideoConfiguration(&cameraConfig);
				}
				else
				{
					m_KinectSensor->GetVideoConfiguration(&cameraConfig);
				}
            }
            else
            {
                cameraConfig.Width = 640;
                cameraConfig.Height = 480;
                cameraConfig.FocalLength = 500.0f;
            }
            IFTModel* ftModel;
            HRESULT hr = m_UserContext[userId].m_pFaceTracker->GetFaceModel(&ftModel);
            if (SUCCEEDED(hr))
            {
                DWORD color = s_ColorCode[userId%6];
                hr = VisualizeFaceModel(m_colorImage, ftModel, &cameraConfig, pSU, 1.0, viewOffset, pResult, color);
                ftModel->Release();
            }
        }
    }
    return TRUE;
}

// We compute here the nominal "center of attention" that is used when zooming the presented image.
void FTHelper2::SetCenterOfImage(IFTResult* pResult)
{
    float centerX = ((float)m_colorImage->GetWidth())/2.0f;
    float centerY = ((float)m_colorImage->GetHeight())/2.0f;
    if (pResult)
    {
        if (SUCCEEDED(pResult->GetStatus()))
        {
            RECT faceRect;
            pResult->GetFaceRect(&faceRect);
            centerX = (faceRect.left+faceRect.right)/2.0f;
            centerY = (faceRect.top+faceRect.bottom)/2.0f;
        }
        m_XCenterFace += 0.02f*(centerX-m_XCenterFace);
        m_YCenterFace += 0.02f*(centerY-m_YCenterFace);
    }
    else
    {
        m_XCenterFace = centerX;
        m_YCenterFace = centerY;
    }
}

HRESULT FTHelper2::GetNewImages()
{
	HRESULT hrCopy = S_OK;
	if (m_isPlaying)
	{
		if(m_KinectPlayer->hasNext())
		{
			BYTE* videoBuffer = m_KinectPlayer->GetVideoBuffer();
			if (videoBuffer)
			{			
				memcpy(m_colorImage->GetBuffer(), videoBuffer, m_colorImage->GetBufferSize());

				USHORT* depthBuffer = m_KinectPlayer->GetDepthBuffer();
				if (depthBuffer)
				{
					memcpy(m_depthImage->GetBuffer(), depthBuffer, m_depthImage->GetBufferSize());
				}
				m_KinectPlayer->nextFrame();
			}
		}
		else
		{
			//TODO change this for the appropriate way to exit the app.
			this->Stop();			
			ExitProcess(0);
						
			return E_FAIL;
		}
	}
	else
	{
		IFTImage* videoBuffer = m_KinectSensor->GetVideoBuffer(); 
		if (m_KinectSensorPresent && videoBuffer)
		{
			hrCopy = videoBuffer->CopyTo(m_colorImage, NULL, 0, 0); 
			IFTImage* depthBuffer = m_KinectSensor->GetDepthBuffer();
			if (SUCCEEDED(hrCopy) && depthBuffer)
			{
				hrCopy = depthBuffer->CopyTo(m_depthImage, NULL, 0, 0);
			}
		}
	}

	return hrCopy;
}

// Get a video image and process it.
void FTHelper2::CheckCameraInput()
{
	HRESULT hrFT = GetNewImages();
	// Do face tracking
	if (SUCCEEDED(hrFT))
	{
		FT_SENSOR_DATA sensorData;
		sensorData.pVideoFrame = m_colorImage; sensorData.pDepthFrame = m_depthImage;
		sensorData.ZoomFactor = 1.0f; sensorData.ViewOffset.x = 0; sensorData.ViewOffset.y = 0;
				
		//Check if we need this callback (same function as "SelectUserToTrack" in MultiFace.cpp
		//if (m_UserSelectCallBack != NULL)
		//{
		//	(*m_UserSelectCallBack)(m_UserSelectCallBackParam, m_KinectSensor, m_nbUsers, m_UserContext);
		//}

		//else
		//{
		//    SelectUserToTrack(m_KinectSensor, m_nbUsers, m_UserContext);
		//}

			
		for (UINT i=0; i<m_nbUsers; i++)
		{
			//if (m_UserContext[i].m_CountUntilFailure == 0 ||
			//    !m_KinectSensor->IsTracked(m_UserContext[i].m_SkeletonId))
			//{
			//    m_UserContext[i].m_LastTrackSucceeded = false;
			//    continue;
			//}
			//FT_VECTOR3D hint[2];
			//hint[0] =  m_KinectSensor->NeckPoint(m_UserContext[i].m_SkeletonId);
			//hint[1] =  m_KinectSensor->HeadPoint(m_UserContext[i].m_SkeletonId);

			if (m_UserContext[i].m_LastTrackSucceeded)
			{
				hrFT = m_UserContext[i].m_pFaceTracker->ContinueTracking(&sensorData, NULL, m_UserContext[i].m_pFTResult);
			}
			else
			{
				hrFT = m_UserContext[i].m_pFaceTracker->StartTracking(&sensorData, m_roiFaces[i], NULL, m_UserContext[i].m_pFTResult);
			}
			m_UserContext[i].m_LastTrackSucceeded = SUCCEEDED(hrFT) && SUCCEEDED(m_UserContext[i].m_pFTResult->GetStatus());
			
			if (m_UserContext[i].m_LastTrackSucceeded)
			{
				SubmitFraceTrackingResult(m_UserContext[i].m_pFTResult, i);
			}
			else
			{
				m_UserContext[i].m_pFTResult->Reset();
			}
			SetCenterOfImage(m_UserContext[i].m_pFTResult);
		}

		//for (UINT i=0; i<m_nbUsers; i++)
		//{
		//	if (m_UserContext[i].m_LastTrackSucceeded)
		//	{
		//		SubmitFraceTrackingResult(m_UserContext[i].m_pFTResult, i);
		//	}
		//	else
		//	{
		//		m_UserContext[i].m_pFTResult->Reset();
		//	}
		//}

		if (m_DrawMask)
		{
			LONG sub = 2;
			visualizeDepthAndPlayers(m_colorImage, m_depthImage, sub, m_colorImage->GetWidth()/3, m_colorImage->GetHeight()-m_depthImage->GetHeight()/sub);
		}

		if (m_isRecording)
		{			
			m_FTRecorder.addFTResult(m_UserContext, (m_DrawMask) ? m_colorImage : NULL);
		}
	}
}

void FTHelper2::visualizeDepthAndPlayers(IFTImage* faceImage, IFTImage* depthImage, LONG subsample, LONG xpos, LONG ypos)
{
	//lookups for color tinting based on player index
	const int g_IntensityShiftByPlayerR[] = { 1, 2, 0, 2, 0, 0, 2, 0 };
	const int g_IntensityShiftByPlayerG[] = { 1, 2, 2, 0, 2, 0, 0, 1 };
	const int g_IntensityShiftByPlayerB[] = { 1, 0, 2, 2, 0, 2, 0, 2 };

	bool sub_ok = subsample==1 || subsample==2 || subsample == 4 || subsample == 8;
	bool size_ok = (faceImage->GetWidth() >= depthImage->GetWidth() && faceImage->GetHeight() >= depthImage->GetHeight());

	if (!sub_ok || !size_ok)
	{
		return;
	}

	BYTE* rgbrun = (BYTE*)faceImage->GetBuffer();
	USHORT* pBufferRun = (USHORT*)depthImage->GetBuffer();

	LONG fwidth = faceImage->GetWidth()*4;
	LONG dwidth = depthImage->GetWidth();
	//faceImage->GetStride()
	LONG xoffset = xpos*4;
	LONG yoffset = ypos*fwidth;

	//for(LONG y=yoffset; y < depthImage->GetHeight()/subsample;y++)
	//for(LONG x=0; x < depthImage->GetWidth()/subsample;x++)
	for( LONG y = 0 ; y < depthImage->GetHeight()/subsample; y++)
	for( LONG x = 0 ; x < depthImage->GetWidth()/subsample; x++)
	{
		//USHORT depth     = pBufferRun[y*width +x +xoffset];
		USHORT depth     = pBufferRun[dwidth*(y*subsample) + (x*subsample)];
		USHORT realDepth = NuiDepthPixelToDepth(depth);
		USHORT player    = NuiDepthPixelToPlayerIndex(depth);

		// transform 13-bit depth information into an 8-bit intensity appropriate
		// for display (we disregard information in most significant bit)
		BYTE intensity = static_cast<BYTE>(~(realDepth >> 4));

		// tint the intensity by dividing by per-player values
		//*(rgbrun++) = intensity >> g_IntensityShiftByPlayerB[player];
		//*(rgbrun++) = intensity >> g_IntensityShiftByPlayerG[player];
		//*(rgbrun++) = intensity >> g_IntensityShiftByPlayerR[player];
		//// no alpha information, skip the last byte
		//++rgbrun;

		//LONG cx, cy;
		//NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution( m_colorRes, m_depthRes,NULL, x, y, depth, &cx, &cy);
		//if( cx >= 0 && cx < faceImage->GetWidth() && cy >= 0 && cy < faceImage->GetHeight() )
		//{
		//	cx = cx/(faceImage->GetWidth()/depthImage->GetWidth())/subsample;
		//	cy = cy/(faceImage->GetHeight()/depthImage->GetHeight())/subsample;

		//	rgbrun[yoffset+ cy*fwidth + (cx*4) +xoffset] = intensity >> g_IntensityShiftByPlayerB[player];
		//	rgbrun[yoffset+ cy*fwidth + (cx*4)+1 +xoffset] = intensity >> g_IntensityShiftByPlayerG[player];
		//	rgbrun[yoffset+ cy*fwidth + (cx*4)+2 +xoffset] = intensity >> g_IntensityShiftByPlayerR[player];
		//}
	
		//rgbrun[yoffset+ y*fwidth + (x*4) +xoffset] = intensity >> g_IntensityShiftByPlayerB[player];
		//rgbrun[yoffset+ y*fwidth + (x*4)+1 +xoffset] = intensity >> g_IntensityShiftByPlayerG[player];
		//rgbrun[yoffset+ y*fwidth + (x*4)+2 +xoffset] = intensity >> g_IntensityShiftByPlayerR[player];

		CharRGB color = depth2RGB(realDepth);
		rgbrun[yoffset+ y*fwidth + (x*4) +xoffset] = color.B;
		rgbrun[yoffset+ y*fwidth + (x*4)+1 +xoffset] = color.G;
		rgbrun[yoffset+ y*fwidth + (x*4)+2 +xoffset] = color.R;
	}
}

FTHelper2::CharRGB FTHelper2::hsv2rgb(float H, float S, float V){

	unsigned char R, G, B;

	if ( S == 0 )                       //HSV from 0 to 1
	{
		R = V * 255;
		G = V * 255;
		B = V * 255;
	}
	else
	{
		float var_h = H * 6;
		if ( var_h == 6 ) var_h = 0;      //H must be < 1
		float var_i = int( var_h );             //Or ... var_i = floor( var_h )
		float var_1 = V * ( 1 - S );
		float var_2 = V * ( 1 - S * ( var_h - var_i ) );
		float var_3 = V * ( 1 - S * ( 1 - ( var_h - var_i ) ) );
		float var_r, var_g, var_b;

		if      ( var_i == 0 ) { var_r = V     ; var_g = var_3 ; var_b = var_1; }
		else if ( var_i == 1 ) { var_r = var_2 ; var_g = V     ; var_b = var_1; }
		else if ( var_i == 2 ) { var_r = var_1 ; var_g = V     ; var_b = var_3; }
		else if ( var_i == 3 ) { var_r = var_1 ; var_g = var_2 ; var_b = V;     }
		else if ( var_i == 4 ) { var_r = var_3 ; var_g = var_1 ; var_b = V;     }
		else                   { var_r = V     ; var_g = var_1 ; var_b = var_2; }

		R = var_r * 255;                  //RGB results from 0 to 255
		G = var_g * 255;
		B = var_b * 255;
	}

	return CharRGB(R, G, B);
}



bool FTHelper2::FaceRepeated(UINT i)
{
	RECT rectFace_i, rectFace_j;

	IFTResult* pResult = m_UserContext[i].m_pFTResult;
	HRESULT hri = m_UserContext[i].m_pFTResult->GetFaceRect(&rectFace_i);
	if (pResult == NULL || FAILED(pResult->GetStatus()) || FAILED(hri) || !m_UserContext[i].m_LastTrackSucceeded)
	{
		return false;
	}

	for (UINT j=0; j < i && j < m_nbUsers; j++)
	{
		IFTResult* pResultj = m_UserContext[j].m_pFTResult;
		if (pResultj == NULL || FAILED(pResultj->GetStatus()) || !m_UserContext[j].m_LastTrackSucceeded)
		{
			continue;
		}
		HRESULT hrj = pResultj->GetFaceRect(&rectFace_j);
		if ( SUCCEEDED(hrj) &&
			 diffabs(rectFace_i.left, rectFace_j.left) < 10 &&
			 diffabs(rectFace_i.right, rectFace_j.right) < 10 &&
			 diffabs(rectFace_i.bottom, rectFace_j.bottom) < 10 &&
			 diffabs(rectFace_i.top ,rectFace_j.top) < 10 )
		{
			return true;
		}
	}
	return false;
}

DWORD WINAPI FTHelper2::FaceTrackingStaticThread(PVOID lpParam)
{
    FTHelper2* context = static_cast<FTHelper2*>(lpParam);
    if (context)
    {
        return context->FaceTrackingThread();
    }
    return 0;
}

DWORD WINAPI FTHelper2::FaceTrackingThread()
{
    FT_CAMERA_CONFIG videoConfig;
    FT_CAMERA_CONFIG depthConfig;
    FT_CAMERA_CONFIG* pDepthConfig = NULL;

	HRESULT hr = E_FAIL;

	if(m_isPlaying)
	{
		if(m_KinectPlayer != NULL)
		{
			delete m_KinectPlayer;			
		}
		m_KinectPlayer = new KinectPlayer();
		hr = m_KinectPlayer->Init(m_pathName);
		if (FAILED(hr))
		{
			return E_FAIL;
		}
	}

	if(m_KinectSensor != NULL)
	{
		delete m_KinectSensor;
	}
	m_KinectSensor = new KinectSensor();
	hr = m_KinectSensor->Init(m_depthType, m_depthRes, m_bNearMode, FALSE, m_colorType, m_colorRes, m_bSeatedSkeleton); // Try to get the Kinect camera to work

    if (SUCCEEDED(hr))
    {
		if(m_isPlaying)
		{
			m_KinectPlayer->GetVideoConfiguration(&videoConfig);
			m_KinectPlayer->GetDepthConfiguration(&depthConfig);
		}
		else
		{
			m_KinectSensor->GetVideoConfiguration(&videoConfig);
			m_KinectSensor->GetDepthConfiguration(&depthConfig);
		}
		pDepthConfig = &depthConfig;
		m_KinectSensorPresent = TRUE;
    }
    else
    {
        m_KinectSensorPresent = FALSE;

        MessageBoxW(m_hWnd, L"Could not initialize the Kinect sensor.\n", L"Face Tracker Initialization Error\n", MB_OK);
        return 1;
    }

	if (m_isRecording)
	{
		m_FTRecorder.Init(m_FTpathName,m_nbUsers,m_DrawMask,&videoConfig);
	}

    m_UserContext = new FTHelperContext[m_nbUsers];
    if (m_UserContext != 0)
    {
        memset(m_UserContext, 0, sizeof(FTHelperContext)*m_nbUsers);
    }
    else
    {
        MessageBoxW(m_hWnd, L"Could not allocate user context array.\n", L"Face Tracker Initialization Error\n", MB_OK);
        return 2;
    }

    for (UINT i=0; i<m_nbUsers;i++)
    {
        // Try to start the face tracker.
        m_UserContext[i].m_pFaceTracker = FTCreateFaceTracker(_opt);
        if (!m_UserContext[i].m_pFaceTracker)
        {
            MessageBoxW(m_hWnd, L"Could not create the face tracker.\n", L"Face Tracker Initialization Error\n", MB_OK);
            return 3;
        }

        hr = m_UserContext[i].m_pFaceTracker->Initialize(&videoConfig, pDepthConfig, NULL, NULL); 
        if (FAILED(hr))
        {
            WCHAR path[512], buffer[1024];
            GetCurrentDirectoryW(ARRAYSIZE(path), path);
            wsprintf(buffer, L"Could not initialize face tracker (%s) for user %d.\n", path, i);

            MessageBoxW(m_hWnd, buffer, L"Face Tracker Initialization Error\n", MB_OK);

            return 4;
        }
        m_UserContext[i].m_pFaceTracker->CreateFTResult(&m_UserContext[i].m_pFTResult);
        if (!m_UserContext[i].m_pFTResult)
        {
            MessageBoxW(m_hWnd, L"Could not initialize the face tracker result for user %d.\n", L"Face Tracker Initialization Error\n", MB_OK);
            return 5;
        }
        m_UserContext[i].m_LastTrackSucceeded = false;
    }

	// Initialize the ROI for each user
	m_roiFaces = new RECT*[m_nbUsers];
	for (UINT i=0; i<m_nbUsers;i++)
	{
		m_roiFaces[i] = NULL;
		//m_roiFaces[i] = new RECT;
		//m_roiFaces[i]->top = 0;
		//m_roiFaces[i]->left = 0;
		//m_roiFaces[i]->bottom = videoConfig.Height-1;
		//m_roiFaces[i]->right = videoConfig.Width-1;
	}

	//// Rois for seq 1
	//m_roiFaces[0]->top = 103;
	//m_roiFaces[0]->left = 121;
	//m_roiFaces[0]->bottom = 200;
	//m_roiFaces[0]->right = 212;

	//m_roiFaces[1]->top = 96;
	//m_roiFaces[1]->left = 262;
	//m_roiFaces[1]->bottom = 195;
	//m_roiFaces[1]->right = 353;

	//m_roiFaces[2]->top = 141;
	//m_roiFaces[2]->left = 406;
	//m_roiFaces[2]->bottom = 227;
	//m_roiFaces[2]->right = 498;


    // Initialize the RGB image.
    m_colorImage = FTCreateImage();
    if (!m_colorImage || FAILED(hr = m_colorImage->Allocate(videoConfig.Width, videoConfig.Height, FTIMAGEFORMAT_UINT8_B8G8R8X8)))
    {
        return 6;
    }

    if (pDepthConfig)
    {
        m_depthImage = FTCreateImage();
        if (!m_depthImage || FAILED(hr = m_depthImage->Allocate(depthConfig.Width, depthConfig.Height, FTIMAGEFORMAT_UINT16_D13P3)))
        {
            return 7;
        }
    }

    SetCenterOfImage(NULL);

	// check if we are using the sensor or playing a sequence
	while (m_ApplicationIsRunning)
	{
		CheckCameraInput();
		InvalidateRect(m_hWnd, NULL, FALSE);
		UpdateWindow(m_hWnd);
		Sleep(16);
	}	


    return 0;
}

HRESULT FTHelper2::GetCameraConfig(FT_CAMERA_CONFIG* cameraConfig)
{
	if (m_isPlaying)
	{
		return m_KinectSensorPresent ? m_KinectPlayer->GetVideoConfiguration(cameraConfig) : E_FAIL;
	}
	else
	{
		return m_KinectSensorPresent ? m_KinectSensor->GetVideoConfiguration(cameraConfig) : E_FAIL;
	}
}


void FTHelper2::SelectUserToTrack(KinectSensor * pKinectSensor,
                                  UINT nbUsers, FTHelperContext* pUserContexts)
{
    // Initialize an array of the available skeletons
    bool SkeletonIsAvailable[NUI_SKELETON_COUNT];
    for (UINT i=0; i<NUI_SKELETON_COUNT; i++)
    {
        SkeletonIsAvailable[i] = pKinectSensor->IsTracked(i);
    }
    // If the user's skeleton is still tracked, mark it unavailable
    // and make sure we will keep associating the user context to that skeleton
    // If the skeleton is not track anaymore, decrease a counter until we 
    // deassociate the user context from that skeleton id.
    for (UINT i=0; i<nbUsers; i++)
    {
        if (pUserContexts[i].m_CountUntilFailure > 0)
        {
            if (SkeletonIsAvailable[pUserContexts[i].m_SkeletonId])
            {
                SkeletonIsAvailable[pUserContexts[i].m_SkeletonId] = false;
                pUserContexts[i].m_CountUntilFailure++;
                if (pUserContexts[i].m_CountUntilFailure > 5)
                {
                    pUserContexts[i].m_CountUntilFailure = 5;
                }
            }
            else
            {
                pUserContexts[i].m_CountUntilFailure--;
            }
        }
    }

    // Try to find an available skeleton for users who do not have one
    for (UINT i=0; i<nbUsers; i++)
    {
        if (pUserContexts[i].m_CountUntilFailure == 0)
        {
            for (UINT j=0; j<NUI_SKELETON_COUNT; j++)
            {
                if (SkeletonIsAvailable[j])
                {
                    pUserContexts[i].m_SkeletonId = j;
                    pUserContexts[i].m_CountUntilFailure = 1;
                    SkeletonIsAvailable[j] = false;
                    break;
                }
            }
        }
    }
}

LONG FTHelper2::diffabs(LONG left, LONG right)
{
	return (LONG)abs((INT)(left) - (INT)(right));
}

FTHelper2::CharRGB FTHelper2::depth2RGB( USHORT depth )
{

	// The Kinect depth sensor range is: minimum 800mm and maximum 4000mm. 
	// The Kinect for Windows Hardware can however be switched to Near Mode which provides a range of 500mm to 3000mm instead of the Default range

	static const float MIN_RANGE = 700.0f;
	static const float MAX_RANGE = 4200.0f;

	if (depth == 0)
	{
		return CharRGB(0,0,0);
	}

	if (depth < MIN_RANGE)
	{
		return CharRGB(255,255,0);
	}

	if (depth > MAX_RANGE)
	{
		return CharRGB(255,255,255);
	}

	float H = (float)depth;
	H = (H-MIN_RANGE)/(MAX_RANGE-MIN_RANGE);

	//float S = (float)((depth >>4) & 0x03);
	//S = S / (2 << 2);
	//S = (S * 0.25f) + 0.5f; // saturation range [0.5 - 0.75]
	float S = 0.45f;

	float V = (float)((depth >> 8) & 0x07);
	V = V / (1 << 3);
	V = 1-V;
	V = (V * 0.5f) + 0.25f; // saturation range [0.25 - 0.75]

	//float V = (float)(depth & 0x0F);
	//float V = 0.9f;

	return hsv2rgb(H,S,V);
}
