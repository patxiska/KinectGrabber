#ifndef __PPMPLOADER_H__
#define __PPMLOADER_H__

typedef unsigned char uchar;

typedef 
enum _PPM_LOADER_PIXEL_TYPE {
	PPM_LOADER_PIXEL_TYPE_INVALID = -1,
	PPM_LOADER_PIXEL_TYPE_RGB_8B = 0,
	PPM_LOADER_PIXEL_TYPE_GRAY_8B = ( PPM_LOADER_PIXEL_TYPE_RGB_8B + 1 ),
	PPM_LOADER_PIXEL_TYPE_GRAY_16B = ( PPM_LOADER_PIXEL_TYPE_GRAY_8B + 1 ),
	PPM_LOADER_PIXEL_TYPE_GRAY_32B = ( PPM_LOADER_PIXEL_TYPE_GRAY_16B + 1 ),
	PPM_LOADER_PIXEL_TYPE_GRAY_64B = ( PPM_LOADER_PIXEL_TYPE_GRAY_32B + 1 )
} PPM_LOADER_PIXEL_TYPE;

bool LoadPPMFile(void *data, LONG *width, LONG *height, PPM_LOADER_PIXEL_TYPE* pt, const char *filename);
bool SavePPMFile(const char *filename, const void *src, int width, int height, PPM_LOADER_PIXEL_TYPE pt, const char* comments = NULL);

void ConvertBGRX2RGB(BYTE* colorRGBX, UINT colorWidth, UINT colorHeight);
void convertRGB2BGRX(BYTE* colorRGBX, BYTE* outputArrayRGBX, LONG width, LONG height);

#endif //__PPMLOADER_H__