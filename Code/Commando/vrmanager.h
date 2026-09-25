#ifndef VRMANAGER_H
#define VRMANAGER_H

#include "../../lib/openvr/headers/openvr.h"
#include <stdint.h>
#include "matrix3d.h"
class CameraClass;


struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Texture2D;

struct IDirect3DTexture9;
struct IDirect3DSurface9;

class VRManager
{
public:
    static bool Initialize();
    static void Shutdown();

    static bool IsInitialized();

    static void BeginFrame();
    static void EndFrame();
    static void SubmitFrame();

    static bool BeginEye(vr::EVREye eye, CameraClass& camera, const Matrix3D& base_camera_transform);
    static void EndEye(vr::EVREye eye);

    static bool IsStereoRendering();

    static const vr::TrackedDevicePose_t& GetHMDPose();
    static vr::IVRSystem* GetSystem();
    static vr::IVRCompositor* GetCompositor();

private:
    static bool InitializeD3D11();
    static void ShutdownD3D11();

    static bool CreateEyeRenderTargets();
    static void DestroyEyeRenderTargets();

    static bool CreateD3D11Texture(int width, int height, ID3D11Texture2D** texture);
    static void DestroyD3D11Texture(ID3D11Texture2D** texture);

    static bool CaptureD3D9Frame();

    static bool CopyEyeToD3D11(vr::EVREye eye);
    static void SubmitStereoFrame();
    static void SubmitFlatFrame();

    static void UpdateHeadPose();
    static void UpdateEyeTransforms();

    static Matrix3D ConvertOpenVRMatrix(const vr::HmdMatrix34_t& matrix);
    static Matrix3D ConvertOpenVRToW3D(const Matrix3D& matrix);

    static int EyeIndex(vr::EVREye eye);

private:
    static vr::IVRSystem* System;
    static vr::IVRCompositor* Compositor;
    static bool Initialized;

    static vr::TrackedDevicePose_t Poses[vr::k_unMaxTrackedDeviceCount];

    static bool StereoFrameActive;
    static bool StereoEyeRendered[2];

    static bool HeadReferenceValid;
    static vr::HmdMatrix34_t HeadReferencePose;
    static Matrix3D HeadDeltaW3D;

    static Matrix3D HeadToEyeW3D[2];
    static float EyeViewMinX[2];
    static float EyeViewMaxX[2];
    static float EyeViewMinY[2];
    static float EyeViewMaxY[2];

    static float PositionScale;

    static ID3D11Device* D3D11Device;
    static ID3D11DeviceContext* D3D11Context;

    static ID3D11Texture2D* D3D11Texture;
    static ID3D11Texture2D* EyeD3D11Textures[2];

    static IDirect3DTexture9* EyeRenderTextures[2];
    static IDirect3DSurface9* EyeRenderSurfaces[2];
    static IDirect3DSurface9* EyeReadbackSurfaces[2];

    static IDirect3DSurface9* D3D9CaptureSurface;

    static int EyeWidth;
    static int EyeHeight;
    static unsigned long EyeFormat;

    static int CaptureWidth;
    static int CaptureHeight;
};

#endif