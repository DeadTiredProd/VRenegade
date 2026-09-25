#include "vrmanager.h"

#ifdef W3D_CLIENT

#include "dx8wrapper.h"
#include "camera.h"
#include "matrix3d.h"
#include "wwmath.h"

#include <d3d9.h>
#include <d3d11.h>
#include <dxgi.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

vr::IVRSystem* VRManager::System = nullptr;
vr::IVRCompositor* VRManager::Compositor = nullptr;
bool VRManager::Initialized = false;

vr::TrackedDevicePose_t VRManager::Poses[vr::k_unMaxTrackedDeviceCount];

bool VRManager::StereoFrameActive = false;
bool VRManager::StereoEyeRendered[2] = { false, false };

bool VRManager::HeadReferenceValid = false;
vr::HmdMatrix34_t VRManager::HeadReferencePose = {};
Matrix3D VRManager::HeadDeltaW3D(1);

Matrix3D VRManager::HeadToEyeW3D[2] =
{
    Matrix3D(1),
    Matrix3D(1)
};

float VRManager::EyeViewMinX[2] = { -1.0f, -1.0f };
float VRManager::EyeViewMaxX[2] = { 1.0f, 1.0f };
float VRManager::EyeViewMinY[2] = { -1.0f, -1.0f };
float VRManager::EyeViewMaxY[2] = { 1.0f, 1.0f };

float VRManager::PositionScale = 1.0f;

static const float VRManagerHeadPitchOffsetDegrees = 2.5f;

ID3D11Device* VRManager::D3D11Device = nullptr;
ID3D11DeviceContext* VRManager::D3D11Context = nullptr;

ID3D11Texture2D* VRManager::D3D11Texture = nullptr;
ID3D11Texture2D* VRManager::EyeD3D11Textures[2] =
{
    nullptr,
    nullptr
};

IDirect3DTexture9* VRManager::EyeRenderTextures[2] =
{
    nullptr,
    nullptr
};

IDirect3DSurface9* VRManager::EyeRenderSurfaces[2] =
{
    nullptr,
    nullptr
};

IDirect3DSurface9* VRManager::EyeReadbackSurfaces[2] =
{
    nullptr,
    nullptr
};

IDirect3DSurface9* VRManager::D3D9CaptureSurface = nullptr;

int VRManager::EyeWidth = 0;
int VRManager::EyeHeight = 0;
unsigned long VRManager::EyeFormat =
(unsigned long)D3DFMT_A8R8G8B8;

int VRManager::CaptureWidth = 0;
int VRManager::CaptureHeight = 0;

static void VRLog(const char* format, ...)
{
    FILE* file = fopen(
        "F:\\OpenW3D\\Run\\vr_debug.log",
        "a"
    );

    if (!file) {
        return;
    }

    va_list args;
    va_start(args, format);

    vfprintf(
        file,
        format,
        args
    );

    va_end(args);

    fprintf(
        file,
        "\n"
    );

    fclose(file);
}

int VRManager::EyeIndex(vr::EVREye eye)
{
    return (eye == vr::Eye_Right) ? 1 : 0;
}

bool VRManager::Initialize()
{
    if (Initialized) {
        return true;
    }

    VRLog("========================================");
    VRLog("VRManager::Initialize");

    IDirect3DDevice9* device =
        DX8Wrapper::_Get_D3D_Device8();

    if (!device) {
        VRLog("No D3D9 device");
        return false;
    }

    vr::EVRInitError error =
        vr::VRInitError_None;

    System =
        vr::VR_Init(
            &error,
            vr::VRApplication_Scene
        );

    if (error != vr::VRInitError_None) {

        VRLog(
            "VR_Init failed: %s",
            vr::VR_GetVRInitErrorAsEnglishDescription(error)
        );

        System = nullptr;

        return false;
    }

    VRLog("OpenVR initialized");

    Compositor =
        vr::VRCompositor();

    if (!Compositor) {

        VRLog(
            "VRCompositor() returned null"
        );

        vr::VR_Shutdown();

        System = nullptr;

        return false;
    }

    VRLog(
        "SteamVR compositor initialized"
    );

    if (!InitializeD3D11()) {

        VRLog(
            "InitializeD3D11 failed"
        );

        vr::VR_Shutdown();

        Compositor = nullptr;
        System = nullptr;

        return false;
    }

    if (!CreateEyeRenderTargets()) {

        VRLog(
            "CreateEyeRenderTargets failed"
        );

        ShutdownD3D11();

        vr::VR_Shutdown();

        Compositor = nullptr;
        System = nullptr;

        return false;
    }

    UpdateEyeTransforms();

    HeadReferenceValid = false;
    HeadDeltaW3D = Matrix3D(1);

    Initialized = true;

    VRLog(
        "VRManager initialized successfully"
    );

    VRLog(
        "Eye render size: %dx%d",
        EyeWidth,
        EyeHeight
    );

    VRLog(
        "Position scale: %f",
        PositionScale
    );

    return true;
}

void VRManager::Shutdown()
{
    if (!Initialized &&
        !System &&
        !D3D11Device) {

        return;
    }

    VRLog(
        "VRManager::Shutdown"
    );

    StereoFrameActive = false;

    DestroyEyeRenderTargets();
    ShutdownD3D11();

    if (System) {
        vr::VR_Shutdown();
    }

    System = nullptr;
    Compositor = nullptr;
    Initialized = false;

    HeadReferenceValid = false;
    HeadDeltaW3D = Matrix3D(1);
}

bool VRManager::IsInitialized()
{
    return Initialized;
}

void VRManager::BeginFrame()
{
    if (!Initialized ||
        !Compositor) {

        StereoFrameActive = false;
        return;
    }

    StereoFrameActive = false;

    StereoEyeRendered[0] = false;
    StereoEyeRendered[1] = false;

    if (D3D11Context) {
        D3D11Context->Flush();
    }

    UpdateHeadPose();

    VRLog(
        "BeginFrame: HMD valid=%d",
        Poses[
            vr::k_unTrackedDeviceIndex_Hmd
        ].bPoseIsValid ? 1 : 0
    );

    StereoFrameActive = true;

    VRLog(
        "BeginFrame: StereoFrameActive=true"
    );
}

void VRManager::EndFrame()
{
    if (!Initialized) {
        return;
    }

    if (StereoEyeRendered[0] &&
        StereoEyeRendered[1]) {

        SubmitStereoFrame();
    }
    else {

        SubmitFlatFrame();
    }

    StereoFrameActive = false;
}

void VRManager::SubmitFrame()
{
    if (!Initialized ||
        !Compositor) {

        VRLog(
            "SubmitFrame: VR not initialized"
        );

        return;
    }

    VRLog(
        "SubmitFrame: L=%d R=%d",
        StereoEyeRendered[0] ? 1 : 0,
        StereoEyeRendered[1] ? 1 : 0
    );

    if (StereoEyeRendered[0] &&
        StereoEyeRendered[1]) {

        VRLog(
            "SubmitFrame: submitting STEREO"
        );

        SubmitStereoFrame();
    }
    else {

        VRLog(
            "SubmitFrame: submitting FLAT"
        );

        SubmitFlatFrame();
    }
}

bool VRManager::IsStereoRendering()
{
    return Initialized &&
        StereoFrameActive;
}

bool VRManager::BeginEye(
    vr::EVREye eye,
    CameraClass& camera,
    const Matrix3D& base_camera_transform
)
{
    if (!Initialized) {

        VRLog(
            "BeginEye: VRManager not initialized"
        );

        return false;
    }

    if (!StereoFrameActive) {

        VRLog(
            "BeginEye: StereoFrameActive is false"
        );

        return false;
    }

    const int index =
        EyeIndex(eye);

    VRLog(
        "BeginEye: eye=%d projection=%d",
        index,
        (int)camera.Get_Projection_Type()
    );

    if (!EyeRenderSurfaces[index]) {

        VRLog(
            "BeginEye: eye %d render surface is null",
            index
        );

        return false;
    }

    if (!EyeD3D11Textures[index]) {

        VRLog(
            "BeginEye: eye %d D3D11 texture is null",
            index
        );

        return false;
    }

    if (camera.Get_Projection_Type() !=
        CameraClass::PERSPECTIVE) {

        VRLog(
            "BeginEye: eye %d camera is not perspective",
            index
        );

        return false;
    }

    camera.Set_Viewport(
        Vector2(
            0.0f,
            0.0f
        ),
        Vector2(
            1.0f,
            1.0f
        )
    );

    camera.Set_View_Plane(
        Vector2(
            EyeViewMinX[index],
            EyeViewMinY[index]
        ),
        Vector2(
            EyeViewMaxX[index],
            EyeViewMaxY[index]
        )
    );

    Matrix3D head_camera;
    Matrix3D pitch_correction;
    Matrix3D corrected_head_camera;
    Matrix3D eye_camera;

    Matrix3D::Multiply(
        base_camera_transform,
        HeadDeltaW3D,
        &head_camera
    );

    const float pitch_radians =
        VRManagerHeadPitchOffsetDegrees *
        (3.14159265358979323846f / 180.0f);

    const float pitch_sin =
        sinf(pitch_radians);

    const float pitch_cos =
        cosf(pitch_radians);

    pitch_correction =
        Matrix3D(1);

    pitch_correction[1][1] =
        pitch_cos;

    pitch_correction[1][2] =
        -pitch_sin;

    pitch_correction[2][1] =
        pitch_sin;

    pitch_correction[2][2] =
        pitch_cos;

    Matrix3D::Multiply(
        head_camera,
        pitch_correction,
        &corrected_head_camera
    );

    Matrix3D::Multiply(
        corrected_head_camera,
        HeadToEyeW3D[index],
        &eye_camera
    );

    camera.Set_Transform(
        eye_camera
    );

    VRLog(
        "BeginEye: eye %d configured successfully",
        index
    );

    VRLog(
        "BeginEye: eye transform position %f %f %f",
        eye_camera.Get_X_Translation(),
        eye_camera.Get_Y_Translation(),
        eye_camera.Get_Z_Translation()
    );

    return true;
}

void VRManager::EndEye(vr::EVREye eye)
{
    if (!Initialized ||
        !StereoFrameActive) {

        VRLog(
            "EndEye: VR not active"
        );

        return;
    }

    const int index =
        EyeIndex(eye);

    VRLog(
        "EndEye: eye %d copying framebuffer",
        index
    );

    if (!CopyEyeToD3D11(eye)) {

        VRLog(
            "EndEye: eye %d CopyEyeToD3D11 FAILED",
            index
        );

        StereoEyeRendered[index] = false;

        return;
    }

    StereoEyeRendered[index] = true;

    VRLog(
        "EndEye: eye %d complete",
        index
    );
}

const vr::TrackedDevicePose_t&
VRManager::GetHMDPose()
{
    return Poses[
        vr::k_unTrackedDeviceIndex_Hmd
    ];
}

vr::IVRSystem* VRManager::GetSystem()
{
    return System;
}

vr::IVRCompositor*
VRManager::GetCompositor()
{
    return Compositor;
}

bool VRManager::InitializeD3D11()
{
    IDXGIFactory1* factory = nullptr;

    HRESULT hr =
        CreateDXGIFactory1(
            __uuidof(IDXGIFactory1),
            reinterpret_cast<void**>(&factory)
        );

    if (FAILED(hr) ||
        !factory) {

        VRLog(
            "CreateDXGIFactory1 failed: 0x%08lX",
            hr
        );

        return false;
    }

    uint32_t vrAdapterIndex = 0;

    if (System) {
        vrAdapterIndex =
            System->GetD3D9AdapterIndex();
    }

    VRLog(
        "OpenVR D3D9 adapter index: %u",
        vrAdapterIndex
    );

    IDXGIAdapter1* adapter = nullptr;

    hr =
        factory->EnumAdapters1(
            vrAdapterIndex,
            &adapter
        );

    if (FAILED(hr) ||
        !adapter) {

        VRLog(
            "EnumAdapters1 failed: 0x%08lX",
            hr
        );

        factory->Release();

        return false;
    }

    DXGI_ADAPTER_DESC1 adapterDesc = {};

    adapter->GetDesc1(
        &adapterDesc
    );

    VRLog(
        "Using adapter: %ls",
        adapterDesc.Description
    );

    const D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    D3D_FEATURE_LEVEL featureLevel =
        D3D_FEATURE_LEVEL_10_0;

    hr =
        D3D11CreateDevice(
            adapter,
            D3D_DRIVER_TYPE_UNKNOWN,
            nullptr,
            0,
            featureLevels,
            sizeof(featureLevels) /
            sizeof(featureLevels[0]),
            D3D11_SDK_VERSION,
            &D3D11Device,
            &featureLevel,
            &D3D11Context
        );

    adapter->Release();
    factory->Release();

    if (FAILED(hr)) {

        VRLog(
            "D3D11CreateDevice failed: 0x%08lX",
            hr
        );

        D3D11Device = nullptr;
        D3D11Context = nullptr;

        return false;
    }

    VRLog(
        "D3D11 device created, feature level 0x%X",
        (unsigned int)featureLevel
    );

    return true;
}

void VRManager::ShutdownD3D11()
{
    DestroyD3D11Texture(
        &EyeD3D11Textures[0]
    );

    DestroyD3D11Texture(
        &EyeD3D11Textures[1]
    );

    DestroyD3D11Texture(
        &D3D11Texture
    );

    if (D3D11Context) {

        D3D11Context->Release();
        D3D11Context = nullptr;
    }

    if (D3D11Device) {

        D3D11Device->Release();
        D3D11Device = nullptr;
    }
}

bool VRManager::CreateD3D11Texture(
    int width,
    int height,
    ID3D11Texture2D** texture
)
{
    if (!D3D11Device ||
        !texture ||
        width <= 0 ||
        height <= 0) {

        return false;
    }

    *texture = nullptr;

    D3D11_TEXTURE2D_DESC desc = {};

    desc.Width = (UINT)width;
    desc.Height = (UINT)height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format =
        DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags =
        D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags =
        D3D11_RESOURCE_MISC_SHARED;

    HRESULT hr =
        D3D11Device->CreateTexture2D(
            &desc,
            nullptr,
            texture
        );

    if (FAILED(hr)) {

        VRLog(
            "CreateTexture2D failed: %dx%d hr=0x%08lX",
            width,
            height,
            hr
        );

        *texture = nullptr;

        return false;
    }

    return true;
}

void VRManager::DestroyD3D11Texture(
    ID3D11Texture2D** texture
)
{
    if (!texture ||
        !*texture) {

        return;
    }

    (*texture)->Release();

    *texture = nullptr;
}

bool VRManager::CreateEyeRenderTargets()
{
    IDirect3DDevice9* device =
        DX8Wrapper::_Get_D3D_Device8();

    if (!device) {

        VRLog(
            "CreateEyeRenderTargets: D3D9 device missing"
        );

        return false;
    }

    DestroyEyeRenderTargets();

    IDirect3DSurface9* backBuffer = nullptr;

    HRESULT hr =
        device->GetRenderTarget(
            0,
            &backBuffer
        );

    if (FAILED(hr) ||
        !backBuffer) {

        VRLog(
            "GetRenderTarget failed: 0x%08lX",
            hr
        );

        return false;
    }

    D3DSURFACE_DESC desc = {};

    hr =
        backBuffer->GetDesc(
            &desc
        );

    if (FAILED(hr)) {

        VRLog(
            "Backbuffer GetDesc failed: 0x%08lX",
            hr
        );

        backBuffer->Release();

        return false;
    }

    EyeWidth =
        (int)desc.Width;

    EyeHeight =
        (int)desc.Height;

    EyeFormat =
        (unsigned long)desc.Format;

    VRLog(
        "Using existing backbuffer for both eyes: %dx%d format=0x%08lX",
        EyeWidth,
        EyeHeight,
        EyeFormat
    );

    for (int i = 0; i < 2; ++i) {

        EyeRenderSurfaces[i] =
            backBuffer;

        EyeRenderSurfaces[i]->AddRef();

        if (!CreateD3D11Texture(
            EyeWidth,
            EyeHeight,
            &EyeD3D11Textures[i]
        )) {

            VRLog(
                "Create D3D11 eye texture %d failed",
                i
            );

            backBuffer->Release();

            DestroyEyeRenderTargets();

            return false;
        }

        VRLog(
            "Created D3D11 eye texture %d",
            i
        );
    }

    if (!CreateD3D11Texture(
        EyeWidth,
        EyeHeight,
        &D3D11Texture
    )) {

        VRLog(
            "Create flat D3D11 texture failed"
        );

        backBuffer->Release();

        DestroyEyeRenderTargets();

        return false;
    }

    VRLog(
        "Created flat D3D11 texture: %dx%d",
        EyeWidth,
        EyeHeight
    );

    backBuffer->Release();

    UpdateEyeTransforms();

    VRLog(
        "Eye resources created successfully; D3D9 readback will be created on first eye copy"
    );

    return true;
}

void VRManager::DestroyEyeRenderTargets()
{
    for (int i = 0; i < 2; ++i) {

        if (EyeRenderSurfaces[i]) {

            EyeRenderSurfaces[i]->Release();

            EyeRenderSurfaces[i] = nullptr;
        }

        if (EyeReadbackSurfaces[i]) {

            EyeReadbackSurfaces[i]->Release();

            EyeReadbackSurfaces[i] = nullptr;
        }

        if (EyeRenderTextures[i]) {

            EyeRenderTextures[i]->Release();

            EyeRenderTextures[i] = nullptr;
        }

        DestroyD3D11Texture(
            &EyeD3D11Textures[i]
        );

        StereoEyeRendered[i] = false;
    }

    if (D3D9CaptureSurface) {

        D3D9CaptureSurface->Release();

        D3D9CaptureSurface = nullptr;
    }

    EyeWidth = 0;
    EyeHeight = 0;
}

void VRManager::UpdateEyeTransforms()
{
    if (!System) {
        return;
    }

    for (int i = 0; i < 2; ++i) {

        const vr::EVREye eye =
            (i == 0)
            ? vr::Eye_Left
            : vr::Eye_Right;

        const vr::HmdMatrix34_t& eyeToHead =
            System->GetEyeToHeadTransform(
                eye
            );

        Matrix3D eyeToHeadVR =
            ConvertOpenVRMatrix(
                eyeToHead
            );

        Matrix3D eyeToHeadW3D =
            ConvertOpenVRToW3D(
                eyeToHeadVR
            );

        eyeToHeadW3D.Set_X_Translation(
            eyeToHeadW3D.Get_X_Translation() *
            PositionScale
        );

        eyeToHeadW3D.Set_Y_Translation(
            eyeToHeadW3D.Get_Y_Translation() *
            PositionScale
        );

        eyeToHeadW3D.Set_Z_Translation(
            eyeToHeadW3D.Get_Z_Translation() *
            PositionScale
        );

        HeadToEyeW3D[i] =
            eyeToHeadW3D;

        float left = 0.0f;
        float right = 0.0f;
        float top = 0.0f;
        float bottom = 0.0f;

        System->GetProjectionRaw(
            eye,
            &left,
            &right,
            &top,
            &bottom
        );

        EyeViewMinX[i] = left;
        EyeViewMaxX[i] = right;

        EyeViewMinY[i] = top;
        EyeViewMaxY[i] = bottom;

        VRLog(
            "Eye %d projection raw: L=%f R=%f TOP=%f BOTTOM=%f",
            i,
            left,
            right,
            top,
            bottom
        );

        VRLog(
            "Eye %d W3D view plane: Min=(%f,%f) Max=(%f,%f)",
            i,
            EyeViewMinX[i],
            EyeViewMinY[i],
            EyeViewMaxX[i],
            EyeViewMaxY[i]
        );

        VRLog(
            "Eye %d eye-to-head translation: %f %f %f",
            i,
            HeadToEyeW3D[i].Get_X_Translation(),
            HeadToEyeW3D[i].Get_Y_Translation(),
            HeadToEyeW3D[i].Get_Z_Translation()
        );
    }
}

void VRManager::UpdateHeadPose()
{
    if (!Compositor) {

        HeadDeltaW3D =
            Matrix3D(1);

        return;
    }

    vr::VRCompositorError error =
        Compositor->WaitGetPoses(
            Poses,
            vr::k_unMaxTrackedDeviceCount,
            nullptr,
            0
        );

    if (error !=
        vr::VRCompositorError_None) {

        VRLog(
            "WaitGetPoses failed: %d",
            (int)error
        );

        HeadDeltaW3D =
            Matrix3D(1);

        return;
    }

    const vr::TrackedDevicePose_t& hmdPose =
        Poses[
            vr::k_unTrackedDeviceIndex_Hmd
        ];

    if (!hmdPose.bPoseIsValid) {

        HeadDeltaW3D =
            Matrix3D(1);

        return;
    }

    if (!HeadReferenceValid) {

        memcpy(
            &HeadReferencePose,
            &hmdPose.mDeviceToAbsoluteTracking,
            sizeof(HeadReferencePose)
        );

        HeadReferenceValid = true;

        HeadDeltaW3D =
            Matrix3D(1);

        VRLog(
            "Stored initial HMD reference pose"
        );

        return;
    }

    Matrix3D referenceVR =
        ConvertOpenVRMatrix(
            HeadReferencePose
        );

    Matrix3D currentVR =
        ConvertOpenVRMatrix(
            hmdPose.mDeviceToAbsoluteTracking
        );

    Matrix3D referenceInverseVR(1);

    referenceVR.Get_Orthogonal_Inverse(
        referenceInverseVR
    );

    Matrix3D relativeVR;

    Matrix3D::Multiply(
        referenceInverseVR,
        currentVR,
        &relativeVR
    );

    HeadDeltaW3D =
        ConvertOpenVRToW3D(
            relativeVR
        );

    HeadDeltaW3D.Set_X_Translation(
        HeadDeltaW3D.Get_X_Translation() *
        PositionScale
    );

    HeadDeltaW3D.Set_Y_Translation(
        HeadDeltaW3D.Get_Y_Translation() *
        PositionScale
    );

    HeadDeltaW3D.Set_Z_Translation(
        HeadDeltaW3D.Get_Z_Translation() *
        PositionScale
    );

    VRLog(
        "Head delta W3D position: %f %f %f",
        HeadDeltaW3D.Get_X_Translation(),
        HeadDeltaW3D.Get_Y_Translation(),
        HeadDeltaW3D.Get_Z_Translation()
    );

    VRLog(
        "Head delta W3D rotation:"
    );

    VRLog(
        "%f %f %f",
        HeadDeltaW3D[0][0],
        HeadDeltaW3D[0][1],
        HeadDeltaW3D[0][2]
    );

    VRLog(
        "%f %f %f",
        HeadDeltaW3D[1][0],
        HeadDeltaW3D[1][1],
        HeadDeltaW3D[1][2]
    );

    VRLog(
        "%f %f %f",
        HeadDeltaW3D[2][0],
        HeadDeltaW3D[2][1],
        HeadDeltaW3D[2][2]
    );
}

Matrix3D VRManager::ConvertOpenVRMatrix(
    const vr::HmdMatrix34_t& matrix
)
{
    Matrix3D result(1);

    for (int row = 0; row < 3; ++row) {

        for (int col = 0; col < 4; ++col) {

            result[row][col] =
                matrix.m[row][col];
        }
    }

    return result;
}

Matrix3D VRManager::ConvertOpenVRToW3D(
    const Matrix3D& matrix
)
{
    Matrix3D result(1);

    result[0][0] =
        matrix[0][0];

    result[0][1] =
        matrix[0][1];

    result[0][2] =
        matrix[0][2];

    result[1][0] =
        matrix[1][0];

    result[1][1] =
        matrix[1][1];

    result[1][2] =
        matrix[1][2];

    result[2][0] =
        matrix[2][0];

    result[2][1] =
        matrix[2][1];

    result[2][2] =
        matrix[2][2];

    result[0][3] =
        matrix[0][3];

    result[1][3] =
        matrix[1][3];

    result[2][3] =
        matrix[2][3];

    return result;
}

bool VRManager::CopyEyeToD3D11(
    vr::EVREye eye
)
{
    if (!D3D11Context ||
        !D3D11Device) {

        return false;
    }

    const int index =
        EyeIndex(eye);

    IDirect3DDevice9* device =
        DX8Wrapper::_Get_D3D_Device8();

    if (!device) {
        return false;
    }

    if (!EyeRenderSurfaces[index] ||
        !EyeD3D11Textures[index]) {

        VRLog(
            "CopyEyeToD3D11 eye %d: missing render surface or D3D11 texture",
            index
        );

        return false;
    }

    if (!D3D9CaptureSurface) {

        HRESULT createHr =
            device->CreateOffscreenPlainSurface(
                (UINT)EyeWidth,
                (UINT)EyeHeight,
                (D3DFORMAT)EyeFormat,
                D3DPOOL_SYSTEMMEM,
                &D3D9CaptureSurface,
                nullptr
            );

        if (FAILED(createHr) ||
            !D3D9CaptureSurface) {

            VRLog(
                "Lazy CreateOffscreenPlainSurface failed: 0x%08lX",
                createHr
            );

            return false;
        }

        VRLog(
            "Lazy-created D3D9 readback surface: %dx%d format=0x%08lX",
            EyeWidth,
            EyeHeight,
            EyeFormat
        );
    }

    HRESULT hr =
        device->GetRenderTargetData(
            EyeRenderSurfaces[index],
            D3D9CaptureSurface
        );

    if (FAILED(hr)) {

        VRLog(
            "GetRenderTargetData eye %d failed: 0x%08lX",
            index,
            hr
        );

        return false;
    }

    D3DLOCKED_RECT locked = {};

    hr =
        D3D9CaptureSurface->LockRect(
            &locked,
            nullptr,
            D3DLOCK_READONLY
        );

    if (FAILED(hr)) {

        VRLog(
            "LockRect eye %d failed: 0x%08lX",
            index,
            hr
        );

        return false;
    }

    const int bytesPerPixel = 4;

    const int rowBytes =
        EyeWidth * bytesPerPixel;

    uint8_t* uploadData =
        new uint8_t[
            (size_t)rowBytes *
                (size_t)EyeHeight
        ];

    if (!uploadData) {

        D3D9CaptureSurface->UnlockRect();

        return false;
    }

    for (int y = 0; y < EyeHeight; ++y) {

        const uint8_t* source =
            reinterpret_cast<const uint8_t*>(
                locked.pBits
                ) +
            (size_t)y *
            (size_t)locked.Pitch;

        uint8_t* destination =
            uploadData +
            (size_t)y *
            (size_t)rowBytes;

        memcpy(
            destination,
            source,
            (size_t)rowBytes
        );
    }

    D3D9CaptureSurface->UnlockRect();

    D3D11Context->UpdateSubresource(
        EyeD3D11Textures[index],
        0,
        nullptr,
        uploadData,
        (UINT)rowBytes,
        0
    );

    delete[] uploadData;

    D3D11Context->Flush();

    VRLog(
        "Copied eye %d to D3D11",
        index
    );

    return true;
}

bool VRManager::CaptureD3D9Frame()
{
    if (!Initialized) {

        VRLog(
            "CaptureD3D9Frame: VRManager not initialized"
        );

        return false;
    }

    if (!D3D11Device ||
        !D3D11Context ||
        !D3D11Texture) {

        VRLog(
            "CaptureD3D9Frame: missing D3D11 resources device=%p context=%p texture=%p",
            D3D11Device,
            D3D11Context,
            D3D11Texture
        );

        return false;
    }

    IDirect3DDevice9* device =
        DX8Wrapper::_Get_D3D_Device8();

    if (device == nullptr) {

        VRLog(
            "CaptureD3D9Frame: D3D9 device is null"
        );

        return false;
    }

    IDirect3DSurface9* backBuffer =
        nullptr;

    HRESULT hr =
        device->GetRenderTarget(
            0,
            &backBuffer
        );

    if (FAILED(hr) ||
        backBuffer == nullptr) {

        VRLog(
            "CaptureD3D9Frame: GetRenderTarget FAILED hr=0x%08lX surface=%p",
            (unsigned long)hr,
            backBuffer
        );

        if (backBuffer != nullptr) {
            backBuffer->Release();
        }

        return false;
    }

    D3DSURFACE_DESC desc = {};

    hr =
        backBuffer->GetDesc(
            &desc
        );

    if (FAILED(hr)) {

        VRLog(
            "CaptureD3D9Frame: GetDesc FAILED hr=0x%08lX",
            (unsigned long)hr
        );

        backBuffer->Release();

        return false;
    }

    VRLog(
        "CaptureD3D9Frame: backbuffer %ux%u format=0x%08X msaa=%d quality=%lu",
        (unsigned)desc.Width,
        (unsigned)desc.Height,
        (unsigned)desc.Format,
        (int)desc.MultiSampleType,
        (unsigned long)desc.MultiSampleQuality
    );

    if (desc.MultiSampleType !=
        D3DMULTISAMPLE_NONE) {

        VRLog(
            "CaptureD3D9Frame: BACKBUFFER IS MULTISAMPLED"
        );

        backBuffer->Release();

        return false;
    }

    if (desc.Width !=
        (UINT)EyeWidth ||
        desc.Height !=
        (UINT)EyeHeight) {

        VRLog(
            "CaptureD3D9Frame: dimension mismatch expected=%dx%d actual=%ux%u",
            EyeWidth,
            EyeHeight,
            (unsigned)desc.Width,
            (unsigned)desc.Height
        );

        backBuffer->Release();

        return false;
    }

    if (D3D9CaptureSurface != nullptr) {

        D3DSURFACE_DESC captureDesc = {};

        hr =
            D3D9CaptureSurface->GetDesc(
                &captureDesc
            );

        if (FAILED(hr) ||
            captureDesc.Width != desc.Width ||
            captureDesc.Height != desc.Height ||
            captureDesc.Format != desc.Format) {

            VRLog(
                "CaptureD3D9Frame: recreating capture surface"
            );

            D3D9CaptureSurface->Release();
            D3D9CaptureSurface = nullptr;
        }
    }

    if (D3D9CaptureSurface == nullptr) {

        hr =
            device->CreateOffscreenPlainSurface(
                desc.Width,
                desc.Height,
                desc.Format,
                D3DPOOL_SYSTEMMEM,
                &D3D9CaptureSurface,
                nullptr
            );

        if (FAILED(hr) ||
            D3D9CaptureSurface == nullptr) {

            VRLog(
                "CaptureD3D9Frame: CreateOffscreenPlainSurface FAILED hr=0x%08lX format=0x%08X",
                (unsigned long)hr,
                (unsigned)desc.Format
            );

            backBuffer->Release();

            return false;
        }

        VRLog(
            "CaptureD3D9Frame: created system-memory surface"
        );
    }

    hr =
        device->GetRenderTargetData(
            backBuffer,
            D3D9CaptureSurface
        );

    backBuffer->Release();

    if (FAILED(hr)) {

        VRLog(
            "CaptureD3D9Frame: GetRenderTargetData FAILED hr=0x%08lX",
            (unsigned long)hr
        );

        return false;
    }

    D3DLOCKED_RECT locked = {};

    hr =
        D3D9CaptureSurface->LockRect(
            &locked,
            nullptr,
            D3DLOCK_READONLY
        );

    if (FAILED(hr)) {

        VRLog(
            "CaptureD3D9Frame: LockRect FAILED hr=0x%08lX",
            (unsigned long)hr
        );

        return false;
    }

    const int width =
        (int)desc.Width;

    const int height =
        (int)desc.Height;

    const int bytesPerPixel = 4;

    const int rowBytes =
        width * bytesPerPixel;

    static unsigned char* pixelBuffer =
        nullptr;

    static int pixelBufferSize = 0;

    const int requiredSize =
        rowBytes * height;

    if (pixelBufferSize !=
        requiredSize) {

        if (pixelBuffer != nullptr) {

            delete[] pixelBuffer;
            pixelBuffer = nullptr;
        }

        pixelBuffer =
            new unsigned char[
                requiredSize
            ];

        pixelBufferSize =
            requiredSize;

        VRLog(
            "CaptureD3D9Frame: allocated CPU buffer %d bytes",
            requiredSize
        );
    }

    const unsigned char* src =
        static_cast<
        const unsigned char*
        >(locked.pBits);

    unsigned char* dst =
        pixelBuffer;

    for (int y = 0; y < height; ++y) {

        memcpy(
            dst + y * rowBytes,
            src + y * locked.Pitch,
            rowBytes
        );
    }

    D3D9CaptureSurface->UnlockRect();

    D3D11Context->UpdateSubresource(
        D3D11Texture,
        0,
        nullptr,
        pixelBuffer,
        rowBytes,
        0
    );

    D3D11Context->Flush();

    VRLog(
        "CaptureD3D9Frame: SUCCESS"
    );

    return true;
}

void VRManager::SubmitStereoFrame()
{
    if (!Compositor ||
        !EyeD3D11Textures[0] ||
        !EyeD3D11Textures[1]) {

        VRLog(
            "SubmitStereoFrame: missing compositor or eye texture"
        );

        return;
    }

    vr::VRTextureBounds_t bounds = {};

    bounds.uMin = 0.0f;
    bounds.vMin = 0.0f;
    bounds.uMax = 1.0f;
    bounds.vMax = 1.0f;

    vr::Texture_t leftTexture = {};

    leftTexture.handle =
        EyeD3D11Textures[0];

    leftTexture.eType =
        vr::TextureType_DirectX;

    leftTexture.eColorSpace =
        vr::ColorSpace_Auto;

    vr::Texture_t rightTexture = {};

    rightTexture.handle =
        EyeD3D11Textures[1];

    rightTexture.eType =
        vr::TextureType_DirectX;

    rightTexture.eColorSpace =
        vr::ColorSpace_Auto;

    vr::EVRCompositorError leftError =
        Compositor->Submit(
            vr::Eye_Left,
            &leftTexture,
            &bounds
        );

    if (leftError !=
        vr::VRCompositorError_None) {

        VRLog(
            "Stereo left Submit failed: %d",
            (int)leftError
        );
    }

    vr::EVRCompositorError rightError =
        Compositor->Submit(
            vr::Eye_Right,
            &rightTexture,
            &bounds
        );

    if (rightError !=
        vr::VRCompositorError_None) {

        VRLog(
            "Stereo right Submit failed: %d",
            (int)rightError
        );
    }

    VRLog(
        "Submitted stereo frame: L=%d R=%d",
        (int)leftError,
        (int)rightError
    );
}

void VRManager::SubmitFlatFrame()
{
    VRLog(
        "SubmitFlatFrame: begin"
    );

    if (!CaptureD3D9Frame()) {

        VRLog(
            "SubmitFlatFrame: CaptureD3D9Frame FAILED"
        );

        return;
    }

    VRLog(
        "SubmitFlatFrame: capture succeeded"
    );

    if (!D3D11Texture) {

        VRLog(
            "SubmitFlatFrame: D3D11Texture is null after capture"
        );

        return;
    }

    vr::Texture_t texture = {};

    texture.handle =
        D3D11Texture;

    texture.eType =
        vr::TextureType_DirectX;

    texture.eColorSpace =
        vr::ColorSpace_Auto;

    vr::EVRCompositorError left_error =
        Compositor->Submit(
            vr::Eye_Left,
            &texture
        );

    VRLog(
        "SubmitFlatFrame: left Submit returned %d",
        (int)left_error
    );

    vr::EVRCompositorError right_error =
        Compositor->Submit(
            vr::Eye_Right,
            &texture
        );

    VRLog(
        "SubmitFlatFrame: right Submit returned %d",
        (int)right_error
    );
}

#endif