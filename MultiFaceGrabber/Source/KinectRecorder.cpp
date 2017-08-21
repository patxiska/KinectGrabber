#include "stdafx.h"
#include "KinectRecorder.h"

KinectRecorder::KinectRecorder()
{
	m_frameBasename = new char[256];
	Zero();
}

void KinectRecorder::Zero()
{
	m_dumped = false;
	m_totalFrames = 0;
	strcpy(m_frameBasename, "video");
	m_outputArrayDepthD16 = NULL;
	//m_outputArrayColorCoordinates = NULL;
	m_outputArrayRGBX = NULL;
	m_depthArrayTimeStamp = NULL;
    m_colorArrayTimeStamp = NULL;
}


KinectRecorder::~KinectRecorder(void)
{
	Release();
}

void KinectRecorder::Release()
{
	delete [] m_frameBasename;

	// Heavy memory deallocations -> now in RecorToDisk()
	for(int i=0; i < m_totalFrames; i++)
		delete [] m_outputArrayDepthD16[i];
	//for(int i=0; i < m_totalFrames; i++)
	//	delete [] m_outputArrayColorCoordinates[i];
	for(int i=0; i < m_totalFrames; i++)
		delete [] m_outputArrayRGBX[i];

	delete [] m_outputArrayDepthD16;
	//delete [] m_outputArrayColorCoordinates;
	delete [] m_outputArrayRGBX;
	delete [] m_colorArrayTimeStamp;
	delete [] m_depthArrayTimeStamp;
}

HRESULT KinectRecorder::Init(NUI_IMAGE_TYPE depthType, NUI_IMAGE_RESOLUTION depthRes, BOOL bNearMode, NUI_IMAGE_TYPE colorType, NUI_IMAGE_RESOLUTION colorRes, int total_frames)
{
	m_colorframeIndex = 0;
	m_depthframeIndex = 0;
	m_totalFrames = total_frames;
	m_dumped = false;

	m_outputArrayDepthD16 = new USHORT*[m_totalFrames];
	//m_outputArrayColorCoordinates = new LONG*[m_totalFrames];
	m_outputArrayRGBX = new BYTE*[m_totalFrames];

	m_depthArrayTimeStamp = new LARGE_INTEGER[m_totalFrames];
    m_colorArrayTimeStamp = new LARGE_INTEGER[m_totalFrames];

	DWORD width = 0; DWORD height = 0;

    NuiImageResolutionToSize(depthRes, width, height);
    m_depthWidth  = static_cast<LONG>(width);
    m_depthHeight = static_cast<LONG>(height);

    NuiImageResolutionToSize(colorRes, width, height);
    m_colorWidth  = static_cast<LONG>(width);
    m_colorHeight = static_cast<LONG>(height);

	// Heavy memory allocations
	for(int i=0; i < m_totalFrames; i++)
		m_outputArrayDepthD16[i] = new USHORT[m_depthWidth*m_depthHeight];
	for(int i=0; i < m_totalFrames; i++)
		m_outputArrayRGBX[i] = new BYTE[m_colorWidth*m_colorHeight*4];
	//for(int i=0; i < m_totalFrames; i++)
	//	m_outputArrayColorCoordinates[i] = new LONG[m_depthWidth*m_depthHeight*2];

	return S_OK;
}


void KinectRecorder::addRGBFrame(BYTE* colorRGBX, LARGE_INTEGER colorTimeStamp)
{
	if( colorRGBX && m_colorframeIndex < m_totalFrames)
	{
		memcpy(m_outputArrayRGBX[m_colorframeIndex], colorRGBX, m_colorWidth*m_colorHeight*4*sizeof(BYTE));
		m_colorArrayTimeStamp[m_colorframeIndex] = colorTimeStamp;
		m_colorframeIndex++;
	}
}

void KinectRecorder::addDepthFrame(USHORT* depthImage, LARGE_INTEGER depthTimeStamp)
{
	if ( depthImage && m_depthframeIndex < m_totalFrames )
    {
		memcpy(m_outputArrayDepthD16[m_depthframeIndex], depthImage, m_depthWidth*m_depthHeight*sizeof(USHORT));
		m_depthArrayTimeStamp[m_depthframeIndex] = depthTimeStamp;
		m_depthframeIndex++;
    }
}

void KinectRecorder::RecordToDisk()
{	
	//if(m_colorframeIndex < m_totalFrames) return;
	//if(m_dumped) return;
	int min_frames = min(m_colorframeIndex, m_depthframeIndex);
	for(int i=0; i < min_frames; i++){
		dumpToDisk(i, m_frameBasename, m_outputArrayDepthD16[i], m_outputArrayRGBX[i], NULL/*m_outputArrayColorCoordinates[i]*/, m_depthArrayTimeStamp[i], m_colorArrayTimeStamp[i]);
		delete [] m_outputArrayDepthD16[i];
		//delete [] m_outputArrayColorCoordinates[i];
		delete [] m_outputArrayRGBX[i];
		m_outputArrayDepthD16[i] = NULL;
		//m_outputArrayColorCoordinates[i] = NULL;
		m_outputArrayRGBX[i] = NULL;
	}

	//-------------------------------------------------------
	// TIMES STAMP --------------------------------
	//-------------------------------------------------------
	FILE* fid = 0;
	sprintf(m_frameBasename,"data/video_ts.txt");			
	fid = fopen(m_frameBasename,"w");
	fprintf(fid,"Frames=%d, Time=%fseg, Depth size=%dx%d, RGB size=%dx%d\n",
		m_totalFrames, (m_depthArrayTimeStamp[m_totalFrames-1].QuadPart - m_depthArrayTimeStamp[0].QuadPart)/1000.0f,
		m_depthWidth,m_depthHeight,m_colorWidth,m_colorHeight);
	fprintf(fid, "Frame\tDepth\tColor\tD_i-D_i-1\tC_i-C_i-1\tC_i-D_i\n");
	if(!fid){
		printf("ERROR abriendo el archivo %d\n",m_frameBasename);
		exit(-1);
	}

	LONGLONG dprev=0, dact=0, cprev=0, cact=0;
	for(int i=0; i < m_totalFrames; i++){
		dprev = dact; 
		cprev = cact;
		dact = m_depthArrayTimeStamp[i].QuadPart;
		cact = m_colorArrayTimeStamp[i].QuadPart;
		fprintf(fid, "%d\t%llu\t%llu\t%llu\t%llu\t%llu\n", i, dact, cact, (dact-dprev), (cact-cprev), abs(cact-dact));
	}
	fclose(fid);
	
	m_dumped = true;

	delete [] m_outputArrayDepthD16;
	//delete [] m_outputArrayColorCoordinates;
	delete [] m_outputArrayRGBX;
	delete [] m_colorArrayTimeStamp;
	delete [] m_depthArrayTimeStamp;
}

void KinectRecorder::dumpToDisk(int frameIndex, char* frameBasename, USHORT* depthD16, BYTE* colorRGBX, LONG* colorCoordinates, LARGE_INTEGER depthTimeStamp, LARGE_INTEGER colorTimeStamp)
{
		//if(m_dumped) return;
		FILE* fid = 0;

		//-------------------------------------------------------
		// DEPTH ---------------------------------
		//-------------------------------------------------------
		if(depthD16)
		{
			sprintf (frameBasename,"data/depth/video_depth_%d.pgm",frameIndex);
			fid = fopen(frameBasename,"wb");
			if(!fid){
				printf("ERROR abriendo el archivo %d\n",frameBasename);
				exit(-1);
			}
			fprintf(fid, "P5\n#TS=%llu\n", depthTimeStamp.QuadPart);
			fprintf(fid, "%i %i\n%i\n", m_depthWidth, m_depthHeight, 65535);

			fwrite(depthD16,1,m_depthWidth*m_depthHeight*sizeof(USHORT),fid);
			fclose(fid);
		}
		//-------------------------------------------------------
		// COLOR FRAME ------------------------------------------
		//-------------------------------------------------------
		if(colorRGBX)
		{
			sprintf (frameBasename,"data/rgb/video_rgb_%d.ppm",frameIndex);
			fid = fopen(frameBasename,"wb");
			if(!fid){
				printf("ERROR abriendo el archivo %d\n",frameBasename);
				exit(-1);
			}
			fprintf(fid, "P6\n#TS=%llu\n", colorTimeStamp.QuadPart);
			fprintf(fid, "%i %i\n%i\n", m_colorWidth, m_colorHeight, 255);
			//optimize this---
			BYTE firsts[6] = {colorRGBX[2],colorRGBX[1],colorRGBX[0],
							  colorRGBX[6],colorRGBX[5],colorRGBX[4]};
			for(int i=0; i < 2; i++)
				colorRGBX[i] = firsts[i];
			//------
			for(int i=2; i < m_colorWidth*m_colorHeight; i++){
				colorRGBX[i*3+2] = colorRGBX[i*4];
				colorRGBX[i*3+1] = colorRGBX[i*4+1];
				colorRGBX[i*3] = colorRGBX[i*4+2];
			}
			fwrite(colorRGBX,1,m_colorWidth*m_colorHeight*3,fid);
			fclose(fid);
		}
		////-------------------------------------------------------
		//// COLOR COORDINATES --------------------------------
		////-------------------------------------------------------
		if(colorCoordinates)
		{
			sprintf (frameBasename,"data/map/video_map_%d.coord",frameIndex);			
			fid = fopen(frameBasename,"wb");
			if(!fid){
				printf("ERROR abriendo el archivo %d\n",frameBasename);
				exit(-1);
			}
			fprintf(fid, "%d %d\n", m_depthWidth, m_depthHeight);
			fwrite(colorCoordinates,1,m_depthWidth*m_depthHeight*2*sizeof(LONG),fid);
			fclose(fid);
		}
}