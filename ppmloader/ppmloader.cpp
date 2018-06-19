#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include "ppmloader.h"


/*
 * src = Image pointer
 * pt  = Image type
 *			PPM_LOADER_PIXEL_TYPE_RGB_8B:  Image is organized by rows with colors interleaved
 *			PPM_LOADER_PIXEL_TYPE_GRAY_XB: Image is organized by rows with no colors and X bits per pixel
*/
bool SavePPMFile(const char *filename, const void *src, int width, int height, PPM_LOADER_PIXEL_TYPE pt, const char* comments)
{
	bool ret = true;

	if (filename == NULL || src == NULL)
	{
		return false;
	}

	FILE* fid = fopen(filename,"wb");
	if(!fid){
		printf("ERROR opening file %d\n",filename);
		return false;
	}
	int pix_size = 0;
	switch( pt )
	{
		case PPM_LOADER_PIXEL_TYPE_RGB_8B:
			fprintf(fid, "P6\n");
			if (comments)
			{
				fprintf(fid,"#%s\n",comments);
			}
			fprintf(fid, "%i %i\n%i\n", width, height, 255);
			pix_size = 3; // 3 byte size channels
			break;
		case PPM_LOADER_PIXEL_TYPE_GRAY_8B:
			fprintf(fid, "P5\n");
			if (comments)
			{
				fprintf(fid,"#%s\n",comments);
			}
			fprintf(fid, "%i %i\n%i\n", width, height, 255);
			pix_size = 1; // 1 bytes per channel
			break;
		case PPM_LOADER_PIXEL_TYPE_GRAY_16B:
			fprintf(fid, "P5\n");
			if (comments)
			{
				fprintf(fid,"#%s\n",comments);
			}
			fprintf(fid, "%i %i\n%i\n", width, height, 65535);
			pix_size = 2; // 2 bytes per channel
			break;
		case PPM_LOADER_PIXEL_TYPE_GRAY_32B:
			fprintf(fid, "P5\n");
			if (comments)
			{
				fprintf(fid,"#%s\n",comments);
			}
			fprintf(fid, "%i %i\n%i\n", width, height, 4294967295);
			pix_size = 4; // 4 bytes per channel
			break;
		case PPM_LOADER_PIXEL_TYPE_GRAY_64B:
			fprintf(fid, "P5\n");
			if (comments)
			{
				fprintf(fid,"#%s\n",comments);
			}
			fprintf(fid, "%i %i\n%i\n", width, height, 18446744073709551615);
			pix_size = 8; // 8 bytes per channel
			break;
		case PPM_LOADER_PIXEL_TYPE_INVALID:
			printf("ERROR invalid PPM_LOADER_PIXEL_TYPE\n");
			return false;
			break;
		default:
			printf("ERROR unrecognized PPM_LOADER_PIXEL_TYPE\n");
			return false;
			break;
	}
	int bytes_written = fwrite(src,1,width*height*pix_size,fid);
	ret = bytes_written == width*height*pix_size;

	fclose(fid);

	return ret;
}


bool LoadPPMFile(void *data, LONG *width, LONG *height, PPM_LOADER_PIXEL_TYPE* pt, const char *filename)
//void LoadPPMFile(uchar **dst, int *width, int *height, const char *name)
{
	if (data == NULL || width== NULL || height == NULL || pt == NULL || filename == NULL)
	{
		return false;
	}
	// Open file
	FILE* fid = fopen(filename, "rb");
	if (!fid) {
		printf("PPM load error: file access denied %s\n", filename);
		return false;
	}
	// Read PPM/PGM header P5/P6
	int channels = 0;
	char phead[10];
	fgets(phead, 10, fid);
	if ( phead[0]=='P' && phead[1]=='5' ) {  
		channels = 1;
	} else if ( phead[0]=='P' && phead[1]=='6' ) {  
		channels = 3;
	} else {
		printf("Wrong image type\n");
		return false;
	}

	// Parse comments
	char line[256], *ptr;
	fgets(line, 256, fid);
	while(line[0]=='#')
		fgets(line, 256, fid);

	// Read dimensions
	*width = strtol(line, &ptr, 10);
	*height = strtol(ptr, &ptr, 10);
	if(ptr == NULL || *ptr == '\n'){
		fgets(line, 256, fid);
		ptr = line;
	}

	// Read pixel depth
	int levels = strtol(ptr, &ptr, 10);
	int pixel_depth = 0;
	if (channels==3 && (levels == 255)) {
		*pt = PPM_LOADER_PIXEL_TYPE_RGB_8B;
		pixel_depth = 1;
	}else if (levels == 255) {
		*pt = PPM_LOADER_PIXEL_TYPE_GRAY_8B;
		pixel_depth = 1;
	}else if (levels == 65535) {
		*pt = PPM_LOADER_PIXEL_TYPE_GRAY_16B;
		pixel_depth = 2;
	} else if (levels == 4294967295) {
		*pt = PPM_LOADER_PIXEL_TYPE_GRAY_32B;
		pixel_depth = 4;
	} else if (levels == 18446744073709551615) {
		*pt = PPM_LOADER_PIXEL_TYPE_GRAY_64B;
		pixel_depth = 8;
	} else {
		printf("ERROR: Wrong number of levels\n");
		return false;
	}

	// Read raw data from file
	int size = (*width) * (*height) * channels * pixel_depth;
	fread(data, 1, size, fid);
	fclose(fid);

	return true;
	//unsigned char* data = new unsigned char[size];	
	//*dst = new uchar[*width * *height * 3];
	//int numel = *width * *height;
	//uchar* dest = *dst;
	//for(int c=0; c < 3; c++)
	//	for(int i=0; i<numel; i++)
	//		*dest++ = data[3*i + c];
	//delete [] data;
}

void ConvertBGRX2RGB(BYTE* colorRGBX, UINT colorWidth, UINT colorHeight)
{
	if (colorRGBX==NULL)
	{
		return;
	}
	//Ignoring X components
	BYTE firsts[6] = {colorRGBX[2],colorRGBX[1],colorRGBX[0],
		colorRGBX[6],colorRGBX[5],colorRGBX[4]};
	for(int i=0; i < 6; i++) // optimize this
		colorRGBX[i] = firsts[i];
	//------
	for(int i=2; i < colorWidth*colorHeight; i++){
		colorRGBX[i*3+2] = colorRGBX[i*4];
		colorRGBX[i*3+1] = colorRGBX[i*4+1];
		colorRGBX[i*3] = colorRGBX[i*4+2];
	}
}

void convertRGB2BGRX(BYTE* colorRGBX, BYTE* outputArrayRGBX, LONG width, LONG height)
{
	//Ignoring X components
	for(int i=0; i < width*height; i++){
		//colorRGBX[i*3+2] = colorRGBX[i*4];
		//colorRGBX[i*3+1] = colorRGBX[i*4+1];
		//colorRGBX[i*3] = colorRGBX[i*4+2];
		outputArrayRGBX[i*4] = colorRGBX[i*3+2];
		outputArrayRGBX[i*4+1] = colorRGBX[i*3+1];
		outputArrayRGBX[i*4+2] = colorRGBX[i*3];
	}
}


//void SavePPMFile(uchar *src, int width, int height, const char *file)
//{
//	bool ret = true;
//	FILE* fid;
//	int size = width * height * 3;
//
//	unsigned char* data = new unsigned char[size];
//
//	int numel = width * height;
//	for(int c=0; c < 3; c++)
//		for(int i=0; i<numel; i++)
//			data[3*i + c] = *src++;
//
//	fid = fopen(file,"wb");
//
//	if (!fid) {
//		printf("Can't open file: %s\n",file);
//		exit(-1);
//	}
//
//	// Write PPM header
//	fprintf(fid, "P6\n%i %i\n%i\n", width, height, 255);
//
//	fwrite(data,1,size,fid);
//
//	fclose(fid);
//
//	delete [] data;
//
//	return ret;	
//}
