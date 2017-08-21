#pragma once

#include <FaceTrackLib.h>
#include <NuiApi.h>

class RGBDStream
{
public:

    virtual HRESULT Init(NUI_IMAGE_TYPE depthType, NUI_IMAGE_RESOLUTION depthRes, BOOL bNearMode, BOOL bFallbackToDefault, 
						 NUI_IMAGE_TYPE colorType, NUI_IMAGE_RESOLUTION colorRes, BOOL bSeatedSkeletonMode) = 0;
    virtual void Release() = 0;

    virtual HRESULT     GetVideoConfiguration(FT_CAMERA_CONFIG* videoConfig) = 0;
    virtual HRESULT     GetDepthConfiguration(FT_CAMERA_CONFIG* depthConfig) = 0;

    virtual IFTImage*   GetVideoBuffer() = 0;
    virtual IFTImage*   GetDepthBuffer() = 0;
    virtual float       GetZoomFactor() = 0;
    virtual POINT*      GetViewOffSet() = 0;

    virtual HRESULT     GetClosestHint(FT_VECTOR3D* pHint3D) = 0;
    virtual bool        IsTracked(UINT skeletonId) = 0;
    virtual FT_VECTOR3D NeckPoint(UINT skeletonId) = 0;
    virtual FT_VECTOR3D HeadPoint(UINT skeletonId) = 0;
};
