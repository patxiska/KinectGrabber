#include "stdafx.h"
#include <tchar.h>
#include "KinectRecorder.h"
#include <ppmloader.h>

#define DEFAULT_MAX_FRAMES 800

KinectRecorder::KinectRecorder()
{
	//m_colorframeIndex = 0;
	//m_depthframeIndex = 0;
	//m_ccframeIndex = 0;
	//m_frameBasename = new char[512];
	//m_dumped = false;
	//m_maxFrames = 0;
	//m_bufferSize = 0;
	m_status = KINECT_RECORDER_STATUS_STOPPED;
	
	TCHAR path[KINECT_MAX_PATH];
    GetCurrentDirectory(KINECT_MAX_PATH, path);
	wcstombs( m_frameBasename, path, KINECT_MAX_PATH );

	strcpy(m_sequenceName,"data");
	//m_outputArrayDepthD16 = NULL;
	//m_outputArrayColorCoordinates = NULL;
	//m_outputArrayRGBX = NULL;
	//m_depthArrayTimeStamp = NULL;
 //   m_colorArrayTimeStamp = NULL;

	//m_depthWidth = m_depthHeight = 0;
	//m_colorWidth = m_colorHeight = 0;
	m_maxFrames = DEFAULT_MAX_FRAMES;

	Zero();
}

void KinectRecorder::Zero()
{
	m_colorframeIndex = 0;
	m_depthframeIndex = 0;
//	m_ccframeIndex = 0;
	m_status = KINECT_RECORDER_STATUS_STOPPED;
	m_nearMode = false;
	m_skeletonMode = false;
	m_maxFrames = 0;
	m_bufferSize = 0;
	m_outputArrayDepthD16 = NULL;
	m_outputArrayColorCoordinates = NULL;
	m_outputArrayRGBX = NULL;
	m_depthArrayTimeStamp = NULL;
    m_colorArrayTimeStamp = NULL;

	m_depthWidth = m_depthHeight = 0;
	m_colorWidth = m_colorHeight = 0;
}


KinectRecorder::~KinectRecorder(void)
{
	RecordToDisk();
	Release();
}

void KinectRecorder::Release()
{
	//if(m_frameBasename) delete [] m_frameBasename;

	if(m_outputArrayDepthD16)
	{
		for(int i=0; i < m_bufferSize; i++)
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
		for(int i=0; i < m_bufferSize; i++)
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
		for(int i=0; i < m_maxFrames; i++)
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

	m_bufferSize = 0;

	Zero();
}



HRESULT KinectRecorder::Init( NUI_IMAGE_TYPE depthType, NUI_IMAGE_RESOLUTION depthRes, BOOL bNearMode, NUI_IMAGE_TYPE colorType, NUI_IMAGE_RESOLUTION colorRes, bool bskeletonMode,const char* path, const char* seq_name )
{
	m_colorframeIndex = 0;
	m_depthframeIndex = 0;
	//m_ccframeIndex = 0;
	//m_maxFrames = total_frames;
	m_status = KINECT_RECORDER_STATUS_RECORDING;
	m_nearMode = bNearMode;
	m_skeletonMode = bskeletonMode;

	DWORD dwidth = 0, dheight = 0, cwidth = 0, cheight = 0;
    NuiImageResolutionToSize(depthRes, dwidth, dheight);
    NuiImageResolutionToSize(colorRes, cwidth, cheight);

	bool sizesOK = dwidth <= m_depthWidth && dheight <= m_depthHeight && cwidth <= m_colorWidth && cheight <= m_colorHeight;

	m_depthWidth  = static_cast<LONG>(dwidth);
    m_depthHeight = static_cast<LONG>(dheight);
	m_colorWidth  = static_cast<LONG>(cwidth);
    m_colorHeight = static_cast<LONG>(cheight);

	if (strlen(path))
	{
		strcpy(m_frameBasename, path);
	}
	if (strlen(seq_name))
	{
		strcpy(m_sequenceName, seq_name);
	}

	TCHAR tbuffer[KINECT_MAX_PATH]; char cbuffer[KINECT_MAX_PATH];

	// base directory
	strcpy(cbuffer, m_frameBasename);
	strcat(cbuffer, "\\");
	strcat(cbuffer, m_sequenceName);
	mbstowcs (tbuffer, cbuffer, KINECT_MAX_PATH);
	CreateDirectory(tbuffer, NULL);

	// depth subdirectory
	strcat(cbuffer, "\\");
	strcat(cbuffer, "depth");
	mbstowcs (tbuffer, cbuffer, KINECT_MAX_PATH);
	CreateDirectory(tbuffer, NULL);
	cbuffer[strlen(cbuffer)-strlen("depth")] = '\0'; // reset strcat

	// rgb subdirectory	
	strcat(cbuffer, "rgb");
	mbstowcs (tbuffer, cbuffer, KINECT_MAX_PATH);
	CreateDirectory(tbuffer, NULL);
	cbuffer[strlen(cbuffer)-strlen("rgb")] = '\0'; // reset strcat

	// map subdirectory
	strcat(cbuffer, "map");
	mbstowcs (tbuffer, cbuffer, KINECT_MAX_PATH);
	CreateDirectory(tbuffer, NULL);

	/*mbstowcs (tbuffer, m_frameBasename, 512);
	_tcscat(tbuffer, L"/depth");
	CreateDirectory(tbuffer, NULL);

	mbstowcs (tbuffer, m_frameBasename, 512);
	_tcscat(tbuffer, L"/rgb");
	CreateDirectory(tbuffer, NULL);

	mbstowcs (tbuffer, m_frameBasename, 512);
	_tcscat(tbuffer, L"/map");
	CreateDirectory(tbuffer, NULL);*/

	if (m_maxFrames <= m_bufferSize && sizesOK)
	{
		return S_OK;
	}

	m_bufferSize = m_maxFrames;

	m_outputArrayDepthD16 = new USHORT*[m_maxFrames];
	m_outputArrayColorCoordinates = new LONG*[m_maxFrames];
	m_outputArrayRGBX = new BYTE*[m_maxFrames];

	m_depthArrayTimeStamp = new LARGE_INTEGER[m_maxFrames];
    m_colorArrayTimeStamp = new LARGE_INTEGER[m_maxFrames];

	// Heavy memory allocations
	for(int i=0; i < m_maxFrames; i++)
		m_outputArrayDepthD16[i] = new USHORT[m_depthWidth*m_depthHeight];
	for(int i=0; i < m_maxFrames; i++)
		m_outputArrayRGBX[i] = new BYTE[m_colorWidth*m_colorHeight*4];
	for(int i=0; i < m_maxFrames; i++)
		m_outputArrayColorCoordinates[i] = new LONG[m_depthWidth*m_depthHeight*2];

	return S_OK;
}


void KinectRecorder::addRGBXFrame(BYTE* colorRGBX, LARGE_INTEGER colorTimeStamp)
{
	bool newFrame = m_colorframeIndex == 0 || m_colorArrayTimeStamp[m_colorframeIndex-1].QuadPart != colorTimeStamp.QuadPart;

	if( newFrame && colorRGBX != NULL && m_colorframeIndex < m_maxFrames && m_colorframeIndex < m_bufferSize)
	{
		memcpy(m_outputArrayRGBX[m_colorframeIndex], colorRGBX, m_colorWidth*m_colorHeight*4*sizeof(BYTE));
		m_colorArrayTimeStamp[m_colorframeIndex] = colorTimeStamp;
		m_colorframeIndex++;
	}
}

void KinectRecorder::addDepthFrame(USHORT* depthImage, LARGE_INTEGER depthTimeStamp, LONG* ccImage)
{
	bool newFrame = m_depthframeIndex == 0 || m_depthArrayTimeStamp[m_depthframeIndex-1].QuadPart != depthTimeStamp.QuadPart;

	if ( newFrame && depthImage != NULL && m_depthframeIndex < m_maxFrames && m_depthframeIndex < m_bufferSize )
    {
		memcpy(m_outputArrayDepthD16[m_depthframeIndex], depthImage, m_depthWidth*m_depthHeight*sizeof(USHORT));
		m_depthArrayTimeStamp[m_depthframeIndex] = depthTimeStamp;
		if (ccImage)
		{
			memcpy(m_outputArrayColorCoordinates[m_depthframeIndex], ccImage, m_depthWidth*m_depthHeight*2*sizeof(LONG));
		}
		m_depthframeIndex++;
    }
}

//void KinectRecorder::addColorCoordinatesFrame( LONG* ccImage )
//{
//	if ( ccImage && m_ccframeIndex < m_maxFrames && m_ccframeIndex < m_bufferSize )
//    {
//		memcpy(m_outputArrayColorCoordinates[m_ccframeIndex], ccImage, m_depthWidth*m_depthHeight*2*sizeof(LONG));
//		m_ccframeIndex++;
//    }
//}


bool KinectRecorder::RecordToDisk()
{	
	if(m_status == KINECT_RECORDER_STATUS_SAVING || m_status == KINECT_RECORDER_STATUS_DUMPED)
	{
		return true;
	}

	m_status = KINECT_RECORDER_STATUS_SAVING;

	HRESULT hr = S_OK;

	hr = saveTimeStampSequence();
	if(!SUCCEEDED(hr))
	{
		OutputDebugString( L"Error saving time stamp sequence\r\n" );
	}
	hr = saveDepthSequence();
	if(!SUCCEEDED(hr))
	{
		OutputDebugString( L"Error saving depth sequence\r\n" );
	}
	hr = saveColorSequence();
	if(!SUCCEEDED(hr))
	{
		OutputDebugString( L"Error saving color sequence\r\n" );
	}
	hr = saveCCSequence();
	if(!SUCCEEDED(hr))
	{
		OutputDebugString( L"Error saving color coordinates sequence\r\n" );
	}

	m_status = KINECT_RECORDER_STATUS_DUMPED;

	return true;
}

HRESULT KinectRecorder::saveColorSequence()
{
	char base_path[KINECT_MAX_PATH], filename[KINECT_MAX_PATH], name[KINECT_MAX_PATH];
	strcpy(base_path,m_frameBasename);
	strcat(base_path,"\\");
	strcat(base_path,m_sequenceName);

	strcpy(filename,base_path);
	strcat(filename,"\\rgb.txt");
	FILE* file_list = fopen(filename,"w");
	if(!file_list){
		printf("ERROR opening file %d\n",filename);
		return E_FAIL;
	}
	fprintf(file_list,"# timestamp filename\n");

	bool ret = true;
	for(int i=0; i < m_colorframeIndex; i++)
	{
		BYTE* colorRGBX = m_outputArrayRGBX[i];
		LARGE_INTEGER colorTimeStamp = m_colorArrayTimeStamp[i];

		sprintf(name,"rgb\\%llu_%d.ppm",colorTimeStamp.QuadPart,i);
		fprintf(file_list,"%llu %s\n", colorTimeStamp.QuadPart, name);

		strcpy(filename,base_path);
		strcat(filename,"\\");
		strcat(filename,name);

		//fid = fopen(m_frameBasename,"wb");
		//if(!fid){
		//	printf("ERROR abriendo el archivo %d\n",m_frameBasename);
		//	return E_FAIL;
		//}
		//fprintf(fid, "P6\n#TS=%llu\n", colorTimeStamp.QuadPart);
		char comments[100];
		sprintf(comments, "TS=%llu", colorTimeStamp.QuadPart);
		//fprintf(fid, "%i %i\n%i\n", m_colorWidth, m_colorHeight, 255);
		
		//Ignoring X components
		//BYTE firsts[6] = {colorRGBX[2],colorRGBX[1],colorRGBX[0],
		//					colorRGBX[6],colorRGBX[5],colorRGBX[4]};
		//for(int i=0; i < 2; i++) // optimize this
		//	colorRGBX[i] = firsts[i];
		////------
		//for(int i=2; i < m_colorWidth*m_colorHeight; i++){
		//	colorRGBX[i*3+2] = colorRGBX[i*4];
		//	colorRGBX[i*3+1] = colorRGBX[i*4+1];
		//	colorRGBX[i*3] = colorRGBX[i*4+2];
		//}
		ConvertBGRX2RGB(colorRGBX, m_colorWidth, m_colorHeight);

		ret &= SavePPMFile(filename, colorRGBX, m_colorWidth, m_colorHeight, PPM_LOADER_PIXEL_TYPE_RGB_8B, comments);
		//fwrite(colorRGBX,1,m_colorWidth*m_colorHeight*3,fid);
		//fclose(fid);
	}
	fclose(file_list);

	HRESULT hr = (ret) ? S_OK : E_FAIL;
	return hr;
}

HRESULT KinectRecorder::saveDepthSequence()
{
	char base_path[KINECT_MAX_PATH], filename[KINECT_MAX_PATH], name[KINECT_MAX_PATH];
	strcpy(base_path,m_frameBasename);
	strcat(base_path,"\\");
	strcat(base_path,m_sequenceName);

	strcpy(filename,base_path);
	strcat(filename,"\\depth.txt");
	FILE* file_list = fopen(filename,"w");
	if(!file_list){
		printf("ERROR opening file %d\n",filename);
		return E_FAIL;
	}
	fprintf(file_list,"# timestamp filename\n");

	bool ret = true;
	for(int i=0; i < m_depthframeIndex; i++)
	{
		USHORT* depthD16 = m_outputArrayDepthD16[i];
		LARGE_INTEGER depthTimeStamp = m_depthArrayTimeStamp[i];

		//char filename[100];
		//strcpy(filename,m_frameBasename);
		//sprintf(filename+strlen(m_frameBasename),"/depth/video_depth_%d.pgm",i);
		
		sprintf(name,"depth\\%llu_%d.pgm",depthTimeStamp.QuadPart,i);
		fprintf(file_list,"%llu %s\n", depthTimeStamp.QuadPart, name);

		strcpy(filename,base_path);
		strcat(filename,"\\");
		strcat(filename,name);

		char comments[100];
		sprintf(comments, "TS=%llu", depthTimeStamp.QuadPart);
		ret &= SavePPMFile(filename, depthD16, m_depthWidth, m_depthHeight, PPM_LOADER_PIXEL_TYPE_GRAY_16B, comments);
	}
	fclose(file_list);

	//if(depthD16)
	//	{
	//		sprintf (frameBasename,"data/depth/video_depth_%d.pgm",frameIndex);
	//		fid = fopen(frameBasename,"wb");
	//		if(!fid){
	//			printf("ERROR abriendo el archivo %d\n",frameBasename);
	//			exit(-1);
	//		}
	//		fprintf(fid, "P5\n#TS=%llu\n", depthTimeStamp.QuadPart);
	//		fprintf(fid, "%i %i\n%i\n", m_depthWidth, m_depthHeight, 65535);

	//		fwrite(depthD16,1,m_depthWidth*m_depthHeight*sizeof(USHORT),fid);
	//		fclose(fid);
	//	}
	HRESULT hr = (ret) ? S_OK : E_FAIL;
	return hr;
}

HRESULT KinectRecorder::saveCCSequence()
{
	char base_path[KINECT_MAX_PATH], filename[KINECT_MAX_PATH], name[KINECT_MAX_PATH];
	strcpy(base_path,m_frameBasename);
	strcat(base_path,"\\");
	strcat(base_path,m_sequenceName);

	strcpy(filename,base_path);
	strcat(filename,"\\map.txt");
	FILE* file_list = fopen(filename,"w");
	if(!file_list){
		printf("ERROR opening file %d\n",filename);
		return E_FAIL;
	}

	fprintf(file_list,"# timestamp filename\n");

	for(int i=0; i < m_depthframeIndex; i++)
	{
		LONG* colorCoordinates = m_outputArrayColorCoordinates[i];
		LARGE_INTEGER depthTimeStamp = m_depthArrayTimeStamp[i];

		sprintf(name,"map\\%llu_%d.coord",depthTimeStamp.QuadPart,i);
		fprintf(file_list,"%llu %s\n", depthTimeStamp.QuadPart, name);

		strcpy(filename,base_path);
		strcat(filename,"\\");
		strcat(filename,name);

		//char filename[100];
		//strcpy(filename,m_frameBasename);
		//sprintf(filename+strlen(m_frameBasename),"/map/video_map_%d.coord",i);

		FILE* fid = fopen(filename,"wb");
		if(!fid){
			printf("ERROR opening file %d\n",filename);
			return E_FAIL;
		}
		fprintf(fid, "%d %d\n", m_depthWidth, m_depthHeight);
		fwrite(colorCoordinates,1,m_depthWidth*m_depthHeight*2*sizeof(LONG),fid);
		fclose(fid);
	}
	fclose(file_list);

	return S_OK;
}


HRESULT KinectRecorder::saveTimeStampSequence()
{
	char base_path[KINECT_MAX_PATH], filename[KINECT_MAX_PATH]; //name[KINECT_MAX_PATH];
	strcpy(base_path,m_frameBasename);
	strcat(base_path,"\\");
	strcat(base_path,m_sequenceName);

	strcpy(filename,base_path);
	strcat(filename,"\\");
	//strcat(filename,m_sequenceName);
	strcat(filename,"info.txt");

	FILE* fid = fopen(filename,"w");
	if(!fid){
		printf("ERROR opening file %d\n",filename);
		return E_FAIL;
	}

	int recordedFrames = min(m_colorframeIndex, m_depthframeIndex);

	fprintf(fid,"#Frames=%d, Time=%fseg, Depth size=%dx%d nearMode skeletonMode, RGB size=%dx%d\n",
		recordedFrames, (m_depthArrayTimeStamp[recordedFrames-1].QuadPart - m_depthArrayTimeStamp[0].QuadPart)/1000.0f,
		m_depthWidth,m_depthHeight,m_colorWidth,m_colorHeight);
	fprintf(fid, "#Frame\tDepth\tColor\tD_i-D_i-1\tC_i-C_i-1\tC_i-D_i\n");

	fprintf(fid,"%d\n%d %d %d %d\n%d %d\n",
				recordedFrames, m_depthWidth,m_depthHeight,m_nearMode, m_skeletonMode, 
				m_colorWidth,m_colorHeight);

	LONGLONG dprev=0, dact=0, cprev=0, cact=0;
	for(int i=0; i < recordedFrames; i++){
		dprev = dact; 
		cprev = cact;
		dact = m_depthArrayTimeStamp[i].QuadPart;
		cact = m_colorArrayTimeStamp[i].QuadPart;
		fprintf(fid, "%d\t%llu\t%llu\t%llu\t%llu\t%llu\n", i, dact, cact, (dact-dprev), (cact-cprev), abs(cact-dact));
	}
	fclose(fid);

	return S_OK;
}
