#include "stdafx.h"
#include "FTRecorder.h"
#include <iostream>
#include "FTHelper2.h"
#include "ppmloader.h"

FTRecorder::FTRecorder(void) :
	m_filename("ftresult"),
	m_nbUsers(0),
	m_frameIndex(0),
	m_saveFImages(false),
	m_faceImage(NULL)
{
	TCHAR tpath[KINECT_MAX_PATH];
	char cpath[KINECT_MAX_PATH];
	GetCurrentDirectory(KINECT_MAX_PATH, tpath);
	wcstombs( cpath, tpath, 512 );
	m_basePath = cpath;
}


FTRecorder::~FTRecorder(void)
{
	m_ofile.close();
	if (m_faceImage!=NULL)
	{
		m_faceImage->Release();
	}
}

HRESULT FTRecorder::Init(const std::string& filePath, UINT nUsers, bool saveFImages, FT_CAMERA_CONFIG* videoConfig)
{
	m_saveFImages = saveFImages;
	m_nbUsers = nUsers;
	m_basePath = filePath;

	if (!m_basePath.empty())
	{
		TCHAR tbuffer[KINECT_MAX_PATH]; char cbuffer[KINECT_MAX_PATH];
		strcpy(cbuffer, m_basePath.c_str());
		mbstowcs (tbuffer, cbuffer, KINECT_MAX_PATH);
		CreateDirectory(tbuffer, NULL);
	}

	// extract path from filename
	UINT pos = m_basePath.rfind('\\');
	if (pos!=std::string::npos)
	{
		m_filename = m_basePath.substr(pos+1,std::string::npos);
	}
	m_filename += ".csv";

	m_ofile.open(m_basePath+'\\'+m_filename);
	if (!m_ofile.good())
	{
		return E_FAIL;
	}
	m_ofile << FT_CSV_HEADER << std::endl;

	if (m_saveFImages)
	{
		TCHAR tbuffer[KINECT_MAX_PATH]; char cbuffer[KINECT_MAX_PATH];
		strcpy(cbuffer, m_basePath.c_str());
		strcat(cbuffer, "\\");
		strcat(cbuffer, "ftimages");
		mbstowcs (tbuffer, cbuffer, KINECT_MAX_PATH);
		CreateDirectory(tbuffer, NULL);
		
		HRESULT hr = S_OK;
		m_faceImage = FTCreateImage();
		if (!m_faceImage || FAILED(hr = m_faceImage->Allocate(videoConfig->Width, videoConfig->Height, FTIMAGEFORMAT_UINT8_R8G8B8)))
		{
			return hr;
		}
	}

	return S_OK;
}

const std::string FTRecorder::FT_CSV_HEADER = "Frame,UserID,FaceTop,FaceLeft,FaceBottom,FaceRight,Scale,TransX,TransY,TransZ,Pitch,Yaw,Roll,";
const std::string FTRecorder::FT_CSV_SEP = ",";

void FTRecorder::addFTResult( FTHelperContext* m_UserContext, IFTImage* faceImage)
{
	// Add a new line to csv file for each user
	for (UINT i=0; i<m_nbUsers; i++)
	{
		FLOAT scale, rotationXYZ[3], translationXYZ[3];
		RECT rectFace;
		IFTResult* pResult = m_UserContext[i].m_pFTResult;
		if (pResult == NULL || FAILED(pResult->GetStatus()) || !m_UserContext[i].m_LastTrackSucceeded)
		{
			continue;
		}

		HRESULT hr = pResult->Get3DPose(&scale, rotationXYZ, translationXYZ);
		HRESULT hr2 = pResult->GetFaceRect(&rectFace);
		//if (!(SUCCEEDED(hr) && SUCCEEDED(hr2)))
		if(FAILED(hr) || FAILED(hr2))
		{
			continue;
		}

		m_ofile << m_frameIndex << FT_CSV_SEP << i  << FT_CSV_SEP 
				<< rectFace.top << FT_CSV_SEP << rectFace.left << FT_CSV_SEP << rectFace.bottom << FT_CSV_SEP << rectFace.right << FT_CSV_SEP
				<< scale << FT_CSV_SEP 
				<< translationXYZ[0] << FT_CSV_SEP << translationXYZ[1] << FT_CSV_SEP << translationXYZ[2] << FT_CSV_SEP
				<< rotationXYZ[0] << FT_CSV_SEP << rotationXYZ[1] << FT_CSV_SEP << rotationXYZ[2] << FT_CSV_SEP
				<< std::endl;
	}
	if (m_saveFImages && faceImage)
	{
		std::string ftfilename = m_basePath+"\\ftimages\\fi_"+std::to_string(m_frameIndex)+".ppm";
		saveFTMask(faceImage, ftfilename);
	}

	m_frameIndex++;
}

void FTRecorder::saveFTMask(IFTImage* faceImage, const std::string& ftfilename)
{
	{
		// save to ppm in ftimages
		/*std::string ftfilename = m_basePath+"\\ftimages\\fi_"+std::to_string(m_frameIndex)+".ppm";*/
		HRESULT hr = faceImage->CopyTo(m_faceImage,NULL,0,0);
		if (FAILED(hr))
		{
			std::cout << "ERROR: couldn't copy Face Image" << std::endl;
		}
		//ConvertBGRX2RGB(m_faceImage->GetBuffer,m_faceImage->GetWidth(),m_faceImage->GetHeight());
		bool ret = SavePPMFile(ftfilename.c_str(),m_faceImage->GetBuffer(),m_faceImage->GetWidth(),m_faceImage->GetHeight(),PPM_LOADER_PIXEL_TYPE_RGB_8B);
		if (!ret)
		{
			std::cout << "ERROR: couldn't save Face Image to ppm file" << std::endl;
		}
	}
}

