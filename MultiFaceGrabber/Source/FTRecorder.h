#pragma once

#include <string>
#include <fstream>
#include <FaceTrackLib.h>

struct FTHelperContext; // Forward declaration

class FTRecorder
{
public:
	FTRecorder(void);
	~FTRecorder(void);

	HRESULT Init(const std::string& filePath, UINT nUsers, bool saveFImages, FT_CAMERA_CONFIG* videoConfig);
	void addFTResult(FTHelperContext* m_UserContext, IFTImage* colorImage);
	
private:
	std::string		m_filename;
	std::string		m_basePath;
	std::ofstream	m_ofile;
	UINT			m_nbUsers;
	bool			m_saveFImages;
	IFTImage*       m_faceImage;

	static const std::string FT_CSV_HEADER;
	static const std::string FT_CSV_SEP;
	
	UINT m_frameIndex;

	void saveFTMask(IFTImage* faceImage, const std::string& ftfilename);
};

