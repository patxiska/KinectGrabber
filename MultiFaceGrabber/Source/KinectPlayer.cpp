#include "stdafx.h"
#include <tchar.h>
#include "KinectPlayer.h"
#include "ppmloader.h" 
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

KinectPlayer::KinectPlayer()
{
	//m_colorframeIndex = 0;
	//m_depthframeIndex = 0;
	//m_ccframeIndex = 0;
	//m_frameBasename = new char[512];
	//m_dumped = false;
	//m_totalFrames = 0;
	//m_bufferSize = 0;
	
	TCHAR path[512];
    GetCurrentDirectory(512, path);
	wcstombs( m_frameBasename, path, 512 );

	//m_outputArrayDepthD16 = NULL;
	//m_outputArrayColorCoordinates = NULL;
	//m_outputArrayRGBX = NULL;
	//m_depthArrayTimeStamp = NULL;
 //   m_colorArrayTimeStamp = NULL;

	//m_depthWidth = m_depthHeight = 0;
	//m_colorWidth = m_colorHeight = 0;
	Zero();
}

void KinectPlayer::Zero()
{
	m_colorframeIndex = 0;
	m_depthframeIndex = 0;
	m_ccframeIndex = 0;
	m_totalFrames = 0;
	m_outputArrayDepthD16 = NULL;
	m_outputArrayColorCoordinates = NULL;
	m_outputArrayRGBX = NULL;
	m_depthArrayTimeStamp = NULL;
    m_colorArrayTimeStamp = NULL;

	m_depthWidth  = m_depthHeight = 0;
	m_colorWidth  = m_colorHeight = 0;
}


KinectPlayer::~KinectPlayer(void)
{
	Release();
}

void KinectPlayer::Release()
{
	//if(m_frameBasename) delete [] m_frameBasename;

	if(m_outputArrayDepthD16)
	{
		for(int i=0; i < m_totalFrames; i++)
		{
			if(m_outputArrayDepthD16[i])
			{
				delete [] m_outputArrayDepthD16[i];
				m_outputArrayDepthD16[i] = NULL;
			}
		}
		delete [] m_outputArrayDepthD16;
	}
	if(m_outputArrayRGBX)
	{
		for(int i=0; i < m_totalFrames; i++)
		{
			if(m_outputArrayRGBX[i])
			{
				delete [] m_outputArrayRGBX[i];
				m_outputArrayRGBX[i] = NULL;
			}
		}
		delete [] m_outputArrayRGBX;
	}
	if(m_outputArrayColorCoordinates)
	{
		for(int i=0; i < m_totalFrames; i++)
		{
			if(m_outputArrayColorCoordinates[i])
			{
				delete [] m_outputArrayColorCoordinates[i];	
				m_outputArrayColorCoordinates[i] = NULL;
			}
		}
		delete [] m_outputArrayColorCoordinates;
	}
	
	if(m_colorArrayTimeStamp) delete [] m_colorArrayTimeStamp;
	if(m_depthArrayTimeStamp) delete [] m_depthArrayTimeStamp;

	m_totalFrames = 0;

	Zero();
}



HRESULT KinectPlayer::LoadSequenceInfo()
{
	// C:\Users\pachi\Dropbox\motion_segmentation\kinect\data\pachi_termos
	std::string base_path(m_frameBasename);

	// 1. Extract name from directory
	std::string seq_name;
	unsigned pos = base_path.rfind("\\");
	if(pos!=std::string::npos)
		seq_name = base_path.substr(pos+1,std::string::npos);
	else
		return E_FAIL;

	// 2. Read from name.txt camera parameters and sequence info (#frame+ts?) 
	//std::ifstream seq_info_file(base_path+"\\"+seq_name+"_info.txt");
	std::ifstream seq_info_file(base_path+"\\info.txt");

	// read header skipping comments
	std::string buffer;
	while(seq_info_file.good())
	{
		buffer.clear();
		std::getline(seq_info_file, buffer);
		if (!buffer.empty() && buffer.front()!='#')
			break;
	}
	
	std::stringstream sbuffer(buffer);
	sbuffer >> m_totalFrames;
	seq_info_file >> m_depthWidth >> m_depthHeight >> m_nearMode >> m_skeletonMode;
	seq_info_file >> m_colorWidth >> m_colorHeight;

	seq_info_file.close();

	if (seq_info_file.bad())
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT KinectPlayer::LoadRGBSequence()
{
	//throw std::logic_error("The method or operation is not implemented.");
	std::string base_path(m_frameBasename);

	std::ifstream rgb_file_list(base_path+"\\rgb.txt");

	uchar* data = new uchar[m_colorWidth*m_colorHeight*3];
	PPM_LOADER_PIXEL_TYPE pt = PPM_LOADER_PIXEL_TYPE_INVALID;
	LONG width = 0, height = 0;

	int framesRead = 0;
	while (rgb_file_list.good() && framesRead < m_totalFrames)
	{
		std::string line;
		std::getline(rgb_file_list, line);

		if(line.front()=='#') continue;

		std::stringstream sline(line);
		std::string filename;
		LONGLONG ts;

		sline >> ts >> filename;
		filename = base_path+"\\"+filename;

		bool ret = LoadPPMFile((void*)data, &width, &height, &pt, filename.c_str());
		if (!ret || width!=m_colorWidth || height!=m_colorHeight|| pt!=PPM_LOADER_PIXEL_TYPE_RGB_8B)
		{
			return E_FAIL;
		}
		convertRGB2BGRX(data, m_outputArrayRGBX[framesRead], width, height);
		framesRead++;
	}
	rgb_file_list.close();

	delete [] data;

	return S_OK;
}


HRESULT KinectPlayer::LoadDepthSequence()
{
	std::string base_path(m_frameBasename);

	std::ifstream file_list(base_path+"\\depth.txt");

	PPM_LOADER_PIXEL_TYPE pt = PPM_LOADER_PIXEL_TYPE_INVALID;
	LONG width = 0, height = 0;

	int framesRead = 0;
	while (file_list.good() && framesRead < m_totalFrames)
	{
		std::string line;
		std::getline(file_list, line);

		if(line.front()=='#') continue;

		std::stringstream sline(line);
		std::string filename;
		LONGLONG ts;

		sline >> ts >> filename;
		filename = base_path+"\\"+filename;

		bool ret = LoadPPMFile((void*)m_outputArrayDepthD16[framesRead], &width, &height, &pt, filename.c_str());
		if (!ret || width!=m_depthWidth || height!=m_depthHeight || pt!=PPM_LOADER_PIXEL_TYPE_GRAY_16B)
		{
			return E_FAIL;
		}
		framesRead++;
	}
	file_list.close();

	HRESULT hr = S_OK;
	if (file_list.bad())
	{
		hr = E_FAIL;
	}
	return hr;
}

HRESULT KinectPlayer::LoadCCoordinatesSequence()
{
	std::string base_path(m_frameBasename);

	std::ifstream file_list(base_path+"\\map.txt");
	LONG width = 0, height = 0;

	int framesRead = 0;
	while (file_list.good() && framesRead < m_totalFrames)
	{
		std::string line;
		std::getline(file_list, line);

		if(line.front()=='#') continue;

		std::stringstream sline(line);
		std::string filename;
		LONGLONG ts;

		sline >> ts >> filename;
		filename = base_path+"\\"+filename;

		FILE* fid = fopen(filename.c_str(),"rb");
		if (!fid)
		{
			return E_FAIL;
		}
		fscanf(fid, "%d %d\n", &width, &height);
		if (width!=m_depthWidth || height!=m_depthHeight)
		{
			return E_FAIL;
		}
		fread(m_outputArrayColorCoordinates[framesRead],1,m_depthWidth*m_depthHeight*2*sizeof(LONG),fid);
		fclose(fid);	

		framesRead++;
	}
	file_list.close();

	HRESULT hr = S_OK;
	if (file_list.bad())
	{
		hr = E_FAIL;
	}
	return hr;
}



HRESULT KinectPlayer::Init(const char* path)
{
	m_colorframeIndex = 0;
	m_depthframeIndex = 0;
	m_ccframeIndex = 0;

	if (strlen(path))
	{
		strncpy(m_frameBasename, path, KINECT_MAX_PATH);
	}

	HRESULT hr;

	// 1. Read from camera parameters and sequence info (#frame+ts?) 
	hr = LoadSequenceInfo();
	if (FAILED(hr))
	{
		return hr;
	}
	// Memory allocations
	m_outputArrayDepthD16 = new USHORT*[m_totalFrames];
	m_outputArrayColorCoordinates = new LONG*[m_totalFrames];
	m_outputArrayRGBX = new BYTE*[m_totalFrames];

	m_depthArrayTimeStamp = new LARGE_INTEGER[m_totalFrames];
	m_colorArrayTimeStamp = new LARGE_INTEGER[m_totalFrames];

	for(int i=0; i < m_totalFrames; i++)
		m_outputArrayDepthD16[i] = new USHORT[m_depthWidth*m_depthHeight];
	for(int i=0; i < m_totalFrames; i++)
		m_outputArrayRGBX[i] = new BYTE[m_colorWidth*m_colorHeight*4];
	for(int i=0; i < m_totalFrames; i++)
		m_outputArrayColorCoordinates[i] = new LONG[m_depthWidth*m_depthHeight*2];

	// 3. Load rgb sequence from images in rgb/
	hr = LoadRGBSequence();
	if (FAILED(hr))
	{
		return hr;
	}
	// 4. Load depth sequence from images in depth
	hr = LoadDepthSequence();
	if (FAILED(hr))
	{
		return hr;
	}
	// 5. Load color coordinates sequence from images in map/
	hr = LoadCCoordinatesSequence();
	if (FAILED(hr))
	{
		return hr;
	}

	return hr;
}

BYTE* KinectPlayer::GetVideoBuffer()
{
	return m_outputArrayRGBX[m_colorframeIndex];
}

USHORT* KinectPlayer::GetDepthBuffer()
{
	return m_outputArrayDepthD16[m_depthframeIndex];
}


HRESULT KinectPlayer::GetVideoConfiguration(FT_CAMERA_CONFIG* videoConfig)
{
    if (!videoConfig)
    {
        return E_POINTER;
    }

	UINT width = m_colorWidth;
	UINT height =  m_colorHeight;
    FLOAT focalLength = 0.f;

    if(width == 640 && height == 480)
    {
        focalLength = NUI_CAMERA_COLOR_NOMINAL_FOCAL_LENGTH_IN_PIXELS;
    }
    else if(width == 1280 && height == 960)
    {
        focalLength = NUI_CAMERA_COLOR_NOMINAL_FOCAL_LENGTH_IN_PIXELS * 2.f;
    }

    if(focalLength == 0.f)
    {
        return E_UNEXPECTED;
    }


    videoConfig->FocalLength = focalLength;
    videoConfig->Width = width;
    videoConfig->Height = height;
    return(S_OK);
}

HRESULT KinectPlayer::GetDepthConfiguration(FT_CAMERA_CONFIG* depthConfig)
{
    if (!depthConfig)
    {
        return E_POINTER;
    }

	UINT width = m_depthWidth;
	UINT height =  m_depthHeight;
    FLOAT focalLength = 0.f;

    if(width == 80 && height == 60)
    {
        focalLength = NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS / 4.f;
    }
    else if(width == 320 && height == 240)
    {
        focalLength = NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS;
    }
    else if(width == 640 && height == 480)
    {
        focalLength = NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS * 2.f;
    }

    if(focalLength == 0.f)
    {
        return E_UNEXPECTED;
    }

    depthConfig->FocalLength = focalLength;
    depthConfig->Width = width;
    depthConfig->Height = height;

    return S_OK;
}

bool KinectPlayer::hasNext()
{
	return m_colorframeIndex < m_totalFrames && m_depthframeIndex < m_totalFrames && m_ccframeIndex < m_totalFrames;
}

void KinectPlayer::nextFrame()
{
	m_colorframeIndex++;
	m_depthframeIndex++;
	m_ccframeIndex++;
}
