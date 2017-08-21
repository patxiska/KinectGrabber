//------------------------------------------------------------------------------
// <copyright file="MultiFace.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

// MultiFace.cpp : Defines the entry point for the application.

#include "stdafx.h"
#include "MultiFace.h"
#include "EggAvatar.h"
#include <FaceTrackLib.h>
#include "FTHelper2.h"


#ifndef KINECT_MAX_PATH
  #define KINECT_MAX_PATH 512
#endif

class MultiFace
{
public:
    MultiFace();

    int Run(HINSTANCE hInst, PWSTR lpCmdLine, int nCmdShow);

protected:
    BOOL                        InitInstance(HINSTANCE hInst, PWSTR lpCmdLine, int nCmdShow);
    void                        ParseCmdString(PWSTR lpCmdLine);
    void                        UninitInstance();
    ATOM                        RegisterClass(PCWSTR szWindowClass);
    static LRESULT CALLBACK     WndProcStatic(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK            WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK     About(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    BOOL                        PaintWindow(HDC hdc, HWND hWnd);
    BOOL                        ShowVideo(HDC hdc, int width, int height, int originX, int originY);
    BOOL                        ShowEggAvatar(HDC hdc, int width, int height, int originX, int originY, UINT avatarId);
    static void                 FTHelperCallingBack(LPVOID lpParam, UINT userId);
    static void                 FTHelperUserSelection(PVOID pVoid, KinectSensor * pKinectSensor, UINT nbUsers, FTHelperContext* pUserContexts);
    static int const            MaxLoadStringChars = 100;

    HINSTANCE                   m_hInst;
    HWND                        m_hWnd;
    HACCEL                      m_hAccelTable;
    
    EggAvatar*                  m_eggavatar;
    FTHelper2                   m_FTHelper;
    IFTImage**                  m_pImageBuffer;
    IFTImage*                   m_pVideoBuffer;

	// set these variables directly into m_FTHelper
	UINT                        m_nbUsers;
    NUI_IMAGE_TYPE              m_depthType;
    NUI_IMAGE_TYPE              m_colorType;
    NUI_IMAGE_RESOLUTION        m_depthRes;
    NUI_IMAGE_RESOLUTION        m_colorRes;
    BOOL                        m_bNearMode;
    BOOL                        m_bSeatedSkeletonMode;
	
	bool						m_isPlaying;
	bool						m_isRecording;

	char						m_seqPathName[KINECT_MAX_PATH];
	char						m_ftPathName[KINECT_MAX_PATH];
};

MultiFace::MultiFace(): m_hInst(NULL), 
    m_hWnd(NULL), 
    m_nbUsers(2),
    m_hAccelTable(NULL), 
    m_pVideoBuffer(NULL),
    m_pImageBuffer(NULL),
    m_eggavatar(NULL),
    m_depthType(NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX),
    m_colorType(NUI_IMAGE_TYPE_COLOR),
    m_depthRes(NUI_IMAGE_RESOLUTION_320x240),
    m_colorRes(NUI_IMAGE_RESOLUTION_640x480),
    m_bNearMode(FALSE),
    m_bSeatedSkeletonMode(FALSE)
{
	m_isPlaying = false;
	m_isRecording = false;
	m_seqPathName[0] = '\0';
	m_ftPathName[0] = '\0';
}


// Run the MultiFace application.
int MultiFace::Run(HINSTANCE hInst, PWSTR lpCmdLine, int nCmdShow)
{
    MSG msg = {static_cast<HWND>(0), static_cast<UINT>(0), static_cast<WPARAM>(-1)};
    if (InitInstance(hInst, lpCmdLine, nCmdShow))
    {
        // Main message loop:
        while (GetMessage(&msg, NULL, 0, 0))
        {
            if (!TranslateAccelerator(msg.hwnd, m_hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
    UninitInstance();

    return (int)msg.wParam;
}

// In this function, we save the instance handle, then create and display the main program window.
BOOL MultiFace::InitInstance(HINSTANCE hInstance, PWSTR lpCmdLine, int nCmdShow)
{
    ParseCmdString(lpCmdLine);

    m_eggavatar = new EggAvatar[m_nbUsers];
    m_pImageBuffer = new IFTImage*[m_nbUsers];

    m_hInst = hInstance; // Store instance handle in our global variable

    WCHAR szTitle[MaxLoadStringChars];                  // The title bar text
    LoadString(m_hInst, IDS_APP_TITLE, szTitle, ARRAYSIZE(szTitle));

    static const PCWSTR RES_MAP[] = { L"80x60", L"320x240", L"640x480", L"1280x960" };
    static const PCWSTR IMG_MAP[] = { L"PLAYERID", L"RGB", L"YUV", L"YUV_RAW", L"DEPTH" };

    // Add mode params in title
    WCHAR szTitleComplete[MAX_PATH];
    swprintf_s(szTitleComplete, L"%s -- Depth:%s:%s Color:%s:%s NearMode:%s SeatedSkeleton:%s", szTitle,
        IMG_MAP[m_depthType], (m_depthRes < 0)? L"ERROR": RES_MAP[m_depthRes], IMG_MAP[m_colorType], (m_colorRes < 0)? L"ERROR": RES_MAP[m_colorRes], m_bNearMode? L"ON": L"OFF", m_bSeatedSkeletonMode? L"ON": L"OFF");

    WCHAR szWindowClass[MaxLoadStringChars];            // the main window class name
    LoadString(m_hInst, IDC_MULTIFACE, szWindowClass, ARRAYSIZE(szWindowClass));

    RegisterClass(szWindowClass);

    m_hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MULTIFACE));

    for (UINT i=0; i<m_nbUsers; i++)
    {
        m_pImageBuffer[i] = FTCreateImage();
    }
    m_pVideoBuffer = FTCreateImage();

    m_hWnd = CreateWindow(szWindowClass, szTitleComplete, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, m_hInst, this);
    if (!m_hWnd)
    {
        return FALSE;
    }

    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);

    // Start the face tracking.
	//return SUCCEEDED(m_FTHelper.Init(m_hWnd, m_nbUsers, FTHelperCallingBack, this, FTHelperUserSelection, this, m_depthType, m_depthRes, m_bNearMode, m_colorType, m_colorRes, m_bSeatedSkeletonMode, m_isPlaying, m_isRecording, m_seqPathName, m_ftPathName));
	return SUCCEEDED(m_FTHelper.Init(m_hWnd, m_nbUsers, FTHelperCallingBack, this, NULL, NULL, m_depthType, m_depthRes, m_bNearMode, m_colorType, m_colorRes, m_bSeatedSkeletonMode, m_isPlaying, m_isRecording, m_seqPathName, m_ftPathName));
}

void MultiFace::UninitInstance()
{
    // Clean up the memory allocated for Face Tracking and rendering.
    m_FTHelper.Stop();

    if (m_hAccelTable)
    {
        DestroyAcceleratorTable(m_hAccelTable);
        m_hAccelTable = NULL;
    }

    DestroyWindow(m_hWnd);
    m_hWnd = NULL;

    if (m_pImageBuffer)
    {
        for (UINT i=0; i<m_nbUsers; i++)
        {
            if (m_pImageBuffer[i])
            {
                m_pImageBuffer[i]->Release();
                m_pImageBuffer[i] = NULL;
            }
        }
        delete[] m_pImageBuffer;
    }

    if (m_pVideoBuffer)
    {
        m_pVideoBuffer->Release();
        m_pVideoBuffer = NULL;
    }

    if (m_eggavatar)
    {
        delete[] m_eggavatar;
    }
}


// Register the window class.
ATOM MultiFace::RegisterClass(PCWSTR szWindowClass)
{
    WNDCLASSEX wcex = {0};

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = &MultiFace::WndProcStatic;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = m_hInst;
    wcex.hIcon          = LoadIcon(m_hInst, MAKEINTRESOURCE(IDI_MULTIFACE));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_MULTIFACE);
    wcex.lpszClassName  = szWindowClass;

    return RegisterClassEx(&wcex);
}

LRESULT CALLBACK MultiFace::WndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static MultiFace* pThis = NULL; // cheating, but since there is just one window now, it will suffice.
    if (WM_CREATE == message)
    {
        pThis = reinterpret_cast<MultiFace*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
    }
    return pThis ? pThis->WndProc(hWnd, message, wParam, lParam) : DefWindowProc(hWnd, message, wParam, lParam);
}

//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_KEYUP    - Exit in response to ESC key
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
LRESULT CALLBACK MultiFace::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(m_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_KEYUP:
        if (wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        // Draw the avatar window and the video window
        PaintWindow(hdc, hWnd);
        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK MultiFace::About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Drawing the video window
BOOL MultiFace::ShowVideo(HDC hdc, int width, int height, int originX, int originY)
{
    BOOL ret = TRUE;

    // Now, copy a fraction of the camera image into the screen.
    IFTImage* colorImage = m_FTHelper.GetColorImage();
    if (colorImage)
    {
        int iWidth = colorImage->GetWidth();
        int iHeight = colorImage->GetHeight();
        if (iWidth > 0 && iHeight > 0)
        {
            int iTop = 0;
            int iBottom = iHeight;
            int iLeft = 0;
            int iRight = iWidth;

            // Keep a separate buffer.
            if (m_pVideoBuffer && SUCCEEDED(m_pVideoBuffer->Allocate(iWidth, iHeight, FTIMAGEFORMAT_UINT8_B8G8R8A8)))
            {
                // Copy do the video buffer while converting bytes
                colorImage->CopyTo(m_pVideoBuffer, NULL, 0, 0);

                // Compute the best approximate copy ratio.
                float w1 = (float)iHeight * (float)width;
                float w2 = (float)iWidth * (float)height;
                if (w2 > w1 && height > 0)
                {
                    // video image too wide
                    float wx = w1/height;
                    iLeft = (int)max(0, m_FTHelper.GetXCenterFace() - wx / 2);
                    iRight = iLeft + (int)wx;
                    if (iRight > iWidth)
                    {
                        iRight = iWidth;
                        iLeft = iRight - (int)wx;
                    }
                }
                else if (w1 > w2 && width > 0)
                {
                    // video image too narrow
                    float hy = w2/width;
                    iTop = (int)max(0, m_FTHelper.GetYCenterFace() - hy / 2);
                    iBottom = iTop + (int)hy;
                    if (iBottom > iHeight)
                    {
                        iBottom = iHeight;
                        iTop = iBottom - (int)hy;
                    }
                }

                int const bmpPixSize = m_pVideoBuffer->GetBytesPerPixel();
                SetStretchBltMode(hdc, HALFTONE);
                BITMAPINFO bmi = {sizeof(BITMAPINFO), static_cast<LONG>(iWidth), static_cast<LONG>(iHeight), static_cast<WORD>(1), static_cast<WORD>(bmpPixSize * CHAR_BIT), BI_RGB, m_pVideoBuffer->GetStride() * iHeight, 5000, 5000, 0, 0};
                if (0 == StretchDIBits(hdc, originX, originY, width, height,
                    iLeft, iBottom, iRight-iLeft, iTop-iBottom, m_pVideoBuffer->GetBuffer(), &bmi, DIB_RGB_COLORS, SRCCOPY))
                {
                    ret = FALSE;
                }
            }
        }
    }
    return ret;
}

// Drawing code
BOOL MultiFace::ShowEggAvatar(HDC hdc, int width, int height, int originX, int originY, UINT avatarId)
{
    static int errCount = 0;
    BOOL ret = FALSE;

	DWORD s_ColorCode[] = {0x0000FFFF, 0x000000FF,  0x0000FF00, 0x00FFFF00, 0x00FF00FF, 0x00FF0000};

    if (m_pImageBuffer[avatarId] && SUCCEEDED(m_pImageBuffer[avatarId]->Allocate(width, height, FTIMAGEFORMAT_UINT8_B8G8R8A8)))
    {
        memset(m_pImageBuffer[avatarId]->GetBuffer(), 0, m_pImageBuffer[avatarId]->GetStride() * height); // clear to black

        m_eggavatar[avatarId].SetScaleAndTranslationToWindow(height, width);
        m_eggavatar[avatarId].DrawImage(m_pImageBuffer[avatarId]);

        BITMAPINFO bmi = {sizeof(BITMAPINFO), width, height, 1, static_cast<WORD>(m_pImageBuffer[avatarId]->GetBytesPerPixel() * CHAR_BIT), BI_RGB, m_pImageBuffer[avatarId]->GetStride() * height, 5000, 5000, 0, 0};
        errCount += (0 == StretchDIBits(hdc, originX, originY, width, height, 0, 0, width, height, m_pImageBuffer[avatarId]->GetBuffer(), &bmi, DIB_RGB_COLORS, SRCCOPY));

        ret = TRUE;

		// Display values as text ----------------
		FLOAT rotationXYZ[3], translationXYZ[3];
		m_eggavatar[avatarId].GetRotation(rotationXYZ);
		m_eggavatar[avatarId].GetTranslation(translationXYZ);
		
		float pitch = rotationXYZ[0]; float yaw = rotationXYZ[1]; float roll = rotationXYZ[2];
		float depth = translationXYZ[2];

		char text[KINECT_MAX_PATH];
		sprintf(text,"%.1f|%.1f|%.1f||%.2f",pitch,yaw,roll,depth);
		TCHAR ttext[KINECT_MAX_PATH];
		mbstowcs (ttext, text, KINECT_MAX_PATH);

		UINT x = 0; UINT y = originY+height/2;

		DWORD color = s_ColorCode[avatarId%6];
		SetBkColor(hdc,color);
		TextOut(hdc,x,y,ttext,strlen(text));
    }

    return ret;
}

// Draw the egg head and the camera video with the mask superimposed.
BOOL MultiFace::PaintWindow(HDC hdc, HWND hWnd)
{
    static int errCount = 0;
    BOOL ret = FALSE;
    RECT rect;
    GetClientRect(hWnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    int halfWidth = width/2;

    // Show the video on the right of the window
    errCount += !ShowVideo(hdc, width - halfWidth, height, halfWidth, 0);

    // Draw the egg avatar on the left of the window
    int avatarHeight = height/m_nbUsers;
    for (UINT i=0; i<m_nbUsers; i++)
    {
        errCount += !ShowEggAvatar(hdc, halfWidth, avatarHeight, 0, i*avatarHeight, i);
    }
    return ret;
}

/*
* The "FT Helper" class is generic. It will call back this function
* after color image, skeleton and depth have been acquired by Kinect. 
* The code needs to document the skeleton corresponding to each user context,
* or set the "count until failure" paarmeter of the user context to zero
* if the context shall not be tracked.
*/
void MultiFace::FTHelperUserSelection(PVOID pVoid, KinectSensor * pKinectSensor,
                                      UINT nbUsers, FTHelperContext* pUserContexts)
{
    MultiFace* pApp = reinterpret_cast<MultiFace*>(pVoid);
    if (!pApp)
        return;
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

/*
* The "FT Helper" class is generic. It will call back this function
* after a face has been successfully tracked. The code in the call back passes the parameters
* to the Egg Avatar, so it can be animated.
*/
void MultiFace::FTHelperCallingBack(PVOID pVoid, UINT userId)
{
    MultiFace* pApp = reinterpret_cast<MultiFace*>(pVoid);
    if (pApp)
    {
        IFTResult* pResult = pApp->m_FTHelper.GetResult(userId);
        if (pResult && SUCCEEDED(pResult->GetStatus()))
        {
            FLOAT* pAU = NULL;
            UINT numAU;
            pResult->GetAUCoefficients(&pAU, &numAU);
            pApp->m_eggavatar[userId].SetCandideAU(pAU, numAU);
            FLOAT scale;
            FLOAT rotationXYZ[3];
            FLOAT translationXYZ[3];
            pResult->Get3DPose(&scale, rotationXYZ, translationXYZ);
            pApp->m_eggavatar[userId].SetTranslations(translationXYZ[0], translationXYZ[1], translationXYZ[2]);
            pApp->m_eggavatar[userId].SetRotations(rotationXYZ[0], rotationXYZ[1], rotationXYZ[2]);
        }
    }
}

void MultiFace::ParseCmdString(PWSTR lpCmdLine)
{
    const WCHAR KEY_USERS[]                                 = L"-Users";
    const WCHAR KEY_DEPTH[]                                 = L"-Depth";
    const WCHAR KEY_COLOR[]                                 = L"-Color";
    const WCHAR KEY_NEAR_MODE[]                             = L"-NearMode";
    const WCHAR KEY_SEATED_SKELETON_MODE[]                  = L"-SeatedSkeleton";
	const WCHAR KEY_PLAY_MODE[]								= L"-Play";
	const WCHAR KEY_RECORD_MODE[]							= L"-Rec";
	const WCHAR KEY_SAVE_FTIMAGES[]							= L"-SaveFTImages";
	const WCHAR KEY_SHOW_FT[]								= L"-ShowFT";

    const WCHAR STR_NUI_IMAGE_TYPE_DEPTH[]                  = L"DEPTH";
    const WCHAR STR_NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX[] = L"PLAYERID";
    const WCHAR STR_NUI_IMAGE_TYPE_COLOR[]                  = L"RGB";
    const WCHAR STR_NUI_IMAGE_TYPE_COLOR_YUV[]              = L"YUV";

    const WCHAR STR_NUI_IMAGE_RESOLUTION_80x60[]            = L"80x60";
    const WCHAR STR_NUI_IMAGE_RESOLUTION_320x240[]          = L"320x240";
    const WCHAR STR_NUI_IMAGE_RESOLUTION_640x480[]          = L"640x480";
    const WCHAR STR_NUI_IMAGE_RESOLUTION_1280x960[]         = L"1280x960";

    enum TOKEN_ENUM
    {
        TOKEN_ERROR,
        TOKEN_USERS,
        TOKEN_DEPTH,
        TOKEN_COLOR,
        TOKEN_NEARMODE,
        TOKEN_SEATEDSKELETON,
		TOKEN_SEQUENCE_PATH,
		TOKEN_FACE_TRACKING_PATH,
		TOKEN_SAVE_FTIMAGES,
		TOKEN_SHOW_FT
    }; 

    int argc = 0;
    LPWSTR *argv = CommandLineToArgvW(lpCmdLine, &argc);

    for(int i = 0; i < argc; i++)
    {
        NUI_IMAGE_TYPE* pType = NULL;
        NUI_IMAGE_RESOLUTION* pRes = NULL;

        TOKEN_ENUM tokenType = TOKEN_ERROR; 
        PWCHAR context = NULL;
        PWCHAR token = wcstok_s(argv[i], L":", &context);
        if(0 == wcsncmp(token, KEY_DEPTH, ARRAYSIZE(KEY_DEPTH)))
        {
            tokenType = TOKEN_DEPTH;
            pType = &m_depthType;
            pRes = &m_depthRes;
        }
        else if(0 == wcsncmp(token, KEY_USERS, ARRAYSIZE(KEY_USERS)))
        {
            tokenType = TOKEN_USERS;
        }
        else if(0 == wcsncmp(token, KEY_COLOR, ARRAYSIZE(KEY_COLOR)))
        {
            tokenType = TOKEN_COLOR;
            pType = &m_colorType;
            pRes = &m_colorRes;
        }
        else if(0 == wcsncmp(token, KEY_NEAR_MODE, ARRAYSIZE(KEY_NEAR_MODE)))
        {
            tokenType = TOKEN_NEARMODE;
            m_bNearMode = TRUE;
        }
        else if(0 == wcsncmp(token, KEY_SEATED_SKELETON_MODE, ARRAYSIZE(KEY_SEATED_SKELETON_MODE)))
        {
            tokenType = TOKEN_SEATEDSKELETON;
            m_bSeatedSkeletonMode = TRUE;
        }
		else if(0 == wcsncmp(token, KEY_PLAY_MODE, ARRAYSIZE(KEY_PLAY_MODE)))
		{
			m_isPlaying = true;
			tokenType = TOKEN_SEQUENCE_PATH;
		}
		else if(0 == wcsncmp(token, KEY_RECORD_MODE, ARRAYSIZE(KEY_RECORD_MODE)))
		{
			m_isRecording = true;
			tokenType = TOKEN_FACE_TRACKING_PATH;
		}
        else if(0 == wcsncmp(token, KEY_SAVE_FTIMAGES, ARRAYSIZE(KEY_SAVE_FTIMAGES)))
        {
            tokenType = TOKEN_SAVE_FTIMAGES;
			m_FTHelper.SetSaveFTImages(TRUE);
        }
        else if(0 == wcsncmp(token, KEY_SHOW_FT, ARRAYSIZE(KEY_SHOW_FT)))
        {
            tokenType = TOKEN_SHOW_FT;
			m_FTHelper.SetShowFT(TRUE);
        }


        if(tokenType == TOKEN_USERS)
        {
            while((token = wcstok_s(NULL, L":", &context)) != NULL)
            {
                UINT users = _wtoi(token);
                //if(users > m_nbUsers)
                {
                    m_nbUsers = users;
                    break;
                }
            }
        }
        else if(tokenType == TOKEN_DEPTH || tokenType == TOKEN_COLOR)
        {
            _ASSERT(pType != NULL && pRes != NULL);

            while((token = wcstok_s(NULL, L":", &context)) != NULL)
            {
                if(0 == wcsncmp(token, STR_NUI_IMAGE_TYPE_DEPTH, ARRAYSIZE(STR_NUI_IMAGE_TYPE_DEPTH)))
                {
                    *pType = NUI_IMAGE_TYPE_DEPTH;
                }
                else if(0 == wcsncmp(token, STR_NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, ARRAYSIZE(STR_NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX)))
                {
                    *pType = NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX;
                }
                else if(0 == wcsncmp(token, STR_NUI_IMAGE_TYPE_COLOR, ARRAYSIZE(STR_NUI_IMAGE_TYPE_COLOR)))
                {
                    *pType = NUI_IMAGE_TYPE_COLOR;
                }
                else if(0 == wcsncmp(token, STR_NUI_IMAGE_TYPE_COLOR_YUV, ARRAYSIZE(STR_NUI_IMAGE_TYPE_COLOR_YUV)))
                {
                    *pType = NUI_IMAGE_TYPE_COLOR_YUV;
                }
                else if(0 == wcsncmp(token, STR_NUI_IMAGE_RESOLUTION_80x60, ARRAYSIZE(STR_NUI_IMAGE_RESOLUTION_80x60)))
                {
                    *pRes = NUI_IMAGE_RESOLUTION_80x60;
                }
                else if(0 == wcsncmp(token, STR_NUI_IMAGE_RESOLUTION_320x240, ARRAYSIZE(STR_NUI_IMAGE_RESOLUTION_320x240)))
                {
                    *pRes = NUI_IMAGE_RESOLUTION_320x240;
                }
                else if(0 == wcsncmp(token, STR_NUI_IMAGE_RESOLUTION_640x480, ARRAYSIZE(STR_NUI_IMAGE_RESOLUTION_640x480)))
                {
                    *pRes = NUI_IMAGE_RESOLUTION_640x480;
                }
                else if(0 == wcsncmp(token, STR_NUI_IMAGE_RESOLUTION_1280x960, ARRAYSIZE(STR_NUI_IMAGE_RESOLUTION_1280x960)))
                {
                    *pRes = NUI_IMAGE_RESOLUTION_1280x960;
                }
            }
        }
		else if(tokenType == TOKEN_SEQUENCE_PATH)
        {
            //if((token = wcstok_s(NULL, L":", &context)) != NULL)
            {
				wcstombs( m_seqPathName, context, KINECT_MAX_PATH );	
            }
        }
		else if(tokenType == TOKEN_FACE_TRACKING_PATH)
		{
			//if((token = wcstok_s(NULL, L":", &context)) != NULL)
			{
				wcstombs( m_ftPathName, context, KINECT_MAX_PATH );	
			}
		}
    }

    if(argv) LocalFree(argv);
}


// Program's main entry point
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    MultiFace app;

    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

    return app.Run(hInstance, lpCmdLine, nCmdShow);
}
