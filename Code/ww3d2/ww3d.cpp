/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTBILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : WW3D                                                         *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/ww3d2/ww3d.cpp                               $*
 *                                                                                             *
 *                       Author:: Greg_h                                                       *
 *                                                                                             *
 *                     $Modtime:: 2/14/02 3:00p                                               $*
 *                                                                                             *
 *                    $Revision:: 97                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   WW3D::Init -- Initialize the WW3D Library                                                 *
 *   WW3D::Shutdown -- shutdown the WW3D library                                               *
 *   WW3D::Set_Render_Device -- set the render device being currently used                     *
 *   WW3D::Set_Next_Render_Device -- just go to the next device in the list                    *
 *   WW3D::Set_Device_Resolution -- set the current resolution and bitdepth                    *
 *   WW3D::Get_Render_Device -- Get the index of the current render device                     *
 *   WW3D::Get_Render_Device_Desc -- returns description of the current render device          *
 *   WW3D::Get_Render_Device_Count -- returns the number of render devices available           *
 *   WW3D::Get_Render_Device_Name -- returns the name of the n-th render device                *
 *	  WW3D::Get_Render_Target_Resolution -- get the resolution and bitdepth of the current target*
 *   WW3D::Get_Device_Resolution -- get the current resolution and bitdepth                    *
 *   WW3D::Begin_Render -- mark the start of rendering for a new frame                         *
 *   WW3D::Render -- Render a 3D Scene using the given camera                                  *
 *   WW3D::Render -- Render a single render object                                             *
 *   WW3D::End_Render -- Mark the completion of a frame                                        *
 *   WW3D::Sync -- Time sychronization                                                         *
 *   WW3D::Set_Ext_Swap_Interval -- Sets the swap interval the device should aim sync for.     *
 *   WW3D::Get_Ext_Swap_Interval -- Queries the swap interval the device is aiming sync for.   *
 *   WW3D::Get_Polygon_Mode -- returns the current rendering mode                              *
 *   WW3D::Set_Collision_Box_Display_Mask -- control rendering of collision boxes              *
 *   WW3D::Get_Collision_Box_Display_Mask -- returns the current display mask for collision bo *
 *   WW3D::Normalize_Coordinates -- Convert pixel coords to normalized screen coords 0..1      *
 *   WW3D::Update_Render_Device_Description -- updates the description of the current render d *
 *   WW3D::Make_Screen_Shot -- saves a screenshot with the given base filename                 *
 *   WW3D::Start_Movie_Capture -- begins dumping frames to a movie                             *
 *   WW3D::Stop_Movie_Capture -- ends dumping frames to a movie                               *
 *   WW3D::Toggle_Movie_Capture -- toggles movie capture...                                    *
 *   WW3D::Start_Single_Frame_Movie_Capture -- starts capturing a single frame movie           *
 *   WW3D::Capture_Next_Movie_Frame -- tells ww3d to grab another frame for the movie          *
 *   WW3D::Pause_Movie -- pauses/unpauses movie capturing                                      *
 *   WW3D::Is_Movie_Paused -- returns whether the next frame will be dumped to a movie         *
 *   WW3D::Is_Recording_Next_Frame -- returns whether the next frame will be dumped to a movie *
 *   WW3D::Is_Movie_Ready -- returns whether the movie capture system is ready                 *
 *   WW3D::Update_Movie_Capture -- dumps the current frame into the movie                      *
 *   WW3D::Get_Movie_Capture_Frame_Rate -- returns the framerate at which the movie is being c *
 *   WW3D::Set_Texture_Reduction -- sets the (hacky) texture reduction factor                  *
 *   WW3D::Get_Texture_Reduction -- gets the (hacky) texture reduction factor                 *
 *   WW3D::Flush_Texture_Cache -- dump all textures from the texture cache                     *
 *   WW3D::Allocate_Debug_Resources -- allocates the debug resources                           *
 *   WW3D::Release_Debug_Resources -- releases the debug resources                             *
 *   WW3D::Get_Last_Frame_Poly_Count -- returns the number of polys submitted in the previous  *
 *   WW3D::Flush -- Process all pending rendering tasks                                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#include "ww3d.h"
#include "rinfo.h"
#include "assetmgr.h"
#include "boxrobj.h"
#include "predlod.h"
#include "camera.h"
#include "scene.h"
#include "texfcach.h"
#include "registry.h"
#include "segline.h"
#include "shader.h"
#include "vertmaterial.h"
#include "wwdebug.h"
#include "wwprofile.h"
#include "wwmemlog.h"
#include "shattersystem.h"
#include "textureloader.h"
#include "statistics.h"
#include "pointgr.h"
#include "ffactory.h"
#include "wwstring.h"
#include "ini.h"
#include "dazzle.h"
#include "meshmdl.h"
#include "dx8renderer.h"
#include "render2d.h"
#include "bound.h"
#include "rddesc.h"
#include "vector3i.h"
#include <cmath>
#include <cstdio>
#ifdef _WIN32
#include <d3dx9tex.h>
#endif
#include <limits>
#include "dx8wrapper.h"
#include "TARGA.H"
#include "sortingrenderer.h"
#include "thread.h"
#include "cpudetect.h"
#include "dx8texman.h"
#include "formconv.h"
#include "animatedsoundmgr.h"


#ifndef _UNIX
#include "framgrab.h"
#endif

namespace
{
	enum class MovieCaptureSource
	{
		None,
		Front,
		Back
	};

	MovieCaptureSource MovieCaptureSourceState = MovieCaptureSource::None;
}


const char* DAZZLE_INI_FILENAME = "DAZZLE.INI";

#define DEFAULT_DEBUG_SHADER_BITS	(		SHADE_CNST(\
												ShaderClass::PASS_LEQUAL,\
												ShaderClass::DEPTH_WRITE_ENABLE,\
												ShaderClass::COLOR_WRITE_ENABLE,\
												ShaderClass::SRCBLEND_ONE,\
												ShaderClass::DSTBLEND_ZERO,\
												ShaderClass::FOG_DISABLE,\
												ShaderClass::GRADIENT_MODULATE,\
												ShaderClass::SECONDARY_GRADIENT_DISABLE,\
												ShaderClass::TEXTURING_DISABLE,\
												ShaderClass::ALPHATEST_DISABLE,\
												ShaderClass::CULL_MODE_ENABLE, \
												ShaderClass::DETAILCOLOR_DISABLE,\
												ShaderClass::DETAILALPHA_DISABLE) )

#define LIGHTMAP_DEBUG_SHADER_BITS	(		SHADE_CNST(\
												ShaderClass::PASS_LEQUAL,\
												ShaderClass::DEPTH_WRITE_ENABLE,\
												ShaderClass::COLOR_WRITE_ENABLE,\
												ShaderClass::SRCBLEND_ONE,\
												ShaderClass::DSTBLEND_ZERO,\
												ShaderClass::FOG_DISABLE,\
												ShaderClass::GRADIENT_DISABLE,\
												ShaderClass::SECONDARY_GRADIENT_DISABLE,\
												ShaderClass::TEXTURING_ENABLE,\
												ShaderClass::ALPHATEST_DISABLE,\
												ShaderClass::CULL_MODE_ENABLE, \
												ShaderClass::DETAILCOLOR_DISABLE,\
												ShaderClass::DETAILALPHA_DISABLE) )



/**********************************************************************************
**
**  WW3D Static Globals
**
***********************************************************************************/

unsigned int											WW3D::SyncTime = 0;
unsigned int											WW3D::PreviousSyncTime = 0;
bool														WW3D::IsSortingEnabled = true;

float														WW3D::PixelCenterX = 0.0f;
float														WW3D::PixelCenterY = 0.0f;


bool														WW3D::IsInitted = false;
bool														WW3D::IsRendering = false;
bool														WW3D::IsCapturing = false;
bool														WW3D::IsScreenUVBiased = false;

bool														WW3D::AreDecalsEnabled = true;
float														WW3D::DecalRejectionDistance = 1000000.0f;

bool														WW3D::AreStaticSortListsEnabled = false;
bool														WW3D::MungeSortOnLoad = false;

FrameGrabClass* WW3D::Movie = nullptr;
bool														WW3D::PauseRecord;
bool														WW3D::RecordNextFrame;

int														WW3D::FrameCount = 0;
int														WW3D::UserStat0 = 0;
int														WW3D::UserStat1 = 0;
int														WW3D::UserStat2 = 0;

float														WW3D::DefaultNativeScreenSize = 1.0f;

RefRenderObjListClass* WW3D::DefaultStaticSortLists = nullptr;
RefRenderObjListClass* WW3D::CurrentStaticSortLists = nullptr;
unsigned int											WW3D::MinStaticSortLevel = 1;
unsigned int											WW3D::MaxStaticSortLevel = MAX_SORT_LEVEL;


VertexMaterialClass* WW3D::DefaultDebugMaterial = nullptr;
ShaderClass												WW3D::DefaultDebugShader(DEFAULT_DEBUG_SHADER_BITS);
ShaderClass												WW3D::LightmapDebugShader(LIGHTMAP_DEBUG_SHADER_BITS);

WW3D::PrelitModeEnum									WW3D::PrelitMode = PRELIT_MODE_LIGHTMAP_MULTI_PASS;
bool														WW3D::ExposePrelit = false;

bool														WW3D::SnapshotActivated = false;
bool														WW3D::ThumbnailEnabled = true;

WW3D::MeshDrawModeEnum								WW3D::MeshDrawMode = MESH_DRAW_MODE_OLD;
WW3D::NPatchesGapFillingModeEnum					WW3D::NPatchesGapFillingMode = NPATCHES_GAP_FILLING_ENABLED;
unsigned													WW3D::NPatchesLevel = 1;
bool														WW3D::IsTexturingEnabled = true;

static HWND												_Hwnd = nullptr;
static int												_TextureReduction = 0;
int														WW3D::LastFrameMemoryAllocations;
int														WW3D::LastFrameMemoryFrees;

int														WW3D::TextureFilter;

bool														WW3D::Lite = false;

/**********************************************************************************
**
**  WW3D Static Functions
**
***********************************************************************************/

void WW3D::Set_NPatches_Gap_Filling_Mode(NPatchesGapFillingModeEnum mode)
{
	if (NPatchesGapFillingMode != mode) {
		NPatchesGapFillingMode = mode;
		TheDX8MeshRenderer.Invalidate();
	}
}

void WW3D::Set_NPatches_Level(unsigned level)
{
	if (level > 8) level = 8;
	if (level < 1) level = 1;
	if (NPatchesLevel == 1 && level > 1) TheDX8MeshRenderer.Invalidate();
	if (NPatchesLevel > 1 && level == 1) TheDX8MeshRenderer.Invalidate();
	NPatchesLevel = level;
}

/***********************************************************************************************
 * WW3D::Init -- Initialize the WW3D Library                                                   *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   3/24/98    GTH : Created.                                                                 *
 *=============================================================================================*/
WW3DErrorType WW3D::Init(void* hwnd, char* /*defaultpal*/, bool lite)
{
	assert(IsInitted == false);
	WWDEBUG_SAY(("WW3D::Init hwnd = %p\n", hwnd));
	_Hwnd = (HWND)hwnd;
	Lite = lite;

	Init_D3D_To_WW3_Conversion();

	WWDEBUG_SAY(("Init DX8Wrapper\n"));

	if (!DX8Wrapper::Init(_Hwnd, lite)) {
		return(WW3D_ERROR_DIRECTX8_INITIALIZATION_FAILED);
	}

	WWDEBUG_SAY(("Allocate Debug Resources\n"));
	Allocate_Debug_Resources();

	[[maybe_unused]] MMRESULT r = timeBeginPeriod(1);
	WWASSERT(r == TIMERR_NOERROR);

	if (!lite) {
		WWDEBUG_SAY(("Init Dazzles\n"));

		FileClass* dazzle_ini_file =
			_TheFileFactory->Get_File(DAZZLE_INI_FILENAME);

		if (dazzle_ini_file) {
			INIClass dazzle_ini(*dazzle_ini_file);
			DazzleRenderObjClass::Init_From_INI(&dazzle_ini);
			_TheFileFactory->Return_File(dazzle_ini_file);
		}
	}

	DefaultStaticSortLists =
		new RefRenderObjListClass[MAX_SORT_LEVEL + 1];

	Reset_Current_Static_Sort_Lists_To_Default();

	if (!lite) {
		AnimatedSoundMgrClass::Initialize();
		IsInitted = true;
	}

	WWDEBUG_SAY(("WW3D Init completed\n"));

	return WW3D_ERROR_OK;
}


/***********************************************************************************************
 * WW3D::Shutdown -- shutdown the WW3D Library                                                 *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   3/24/98    GTH : Created.                                                                 *
 *=============================================================================================*/
WW3DErrorType WW3D::Shutdown(void)
{
	assert(Lite || IsInitted == true);

#ifdef WW3D_DX8
	if (IsCapturing) {
		Stop_Movie_Capture();
	}
#endif

	PredictiveLODOptimizerClass::Free();

	if (!Lite) {
		DazzleRenderObjClass::Deinit();
	}

	Release_Debug_Resources();

	if (WW3DAssetManager::Get_Instance()) {
		WW3DAssetManager::Get_Instance()->Free_Assets();
	}

	DX8TextureManagerClass::Shutdown();

	if (!Lite) {
		DX8Wrapper::Shutdown();
	}

	delete[] DefaultStaticSortLists;

	AnimatedSoundMgrClass::Shutdown();

	IsInitted = false;

	return WW3D_ERROR_OK;
}


/***********************************************************************************************
 * WW3D::Set_Render_Device -- set the render device being currently used                       *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   3/24/98    GTH : Created.                                                                 *
 *=============================================================================================*/
WW3DErrorType WW3D::Set_Render_Device(
	const char* dev_name,
	int width,
	int height,
	int bits,
	int windowed,
	bool resize_window
)
{
	bool success =
		DX8Wrapper::Set_Render_Device(
			dev_name,
			width,
			height,
			bits,
			windowed,
			resize_window
		);

	if (success) {
		return WW3D_ERROR_OK;
	}

	return WW3D_ERROR_INITIALIZATION_FAILED;
}


/***********************************************************************************************
 * WW3D::Set_Any_Render_Device -- set any render device you can find                           *
 *=============================================================================================*/
WW3DErrorType WW3D::Set_Any_Render_Device(void)
{
	bool success = DX8Wrapper::Set_Any_Render_Device();

	if (success) {
		return WW3D_ERROR_OK;
	}

	return WW3D_ERROR_INITIALIZATION_FAILED;
}


/***********************************************************************************************
 * WW3D::Set_Render_Device -- set the render device being currently used                       *
 *=============================================================================================*/
WW3DErrorType WW3D::Set_Render_Device(
	int dev,
	int width,
	int height,
	int bits,
	int windowed,
	bool resize_window
)
{
	bool success =
		DX8Wrapper::Set_Render_Device(
			dev,
			width,
			height,
			bits,
			windowed,
			resize_window
		);

	if (success) {
		return WW3D_ERROR_OK;
	}

	return WW3D_ERROR_INITIALIZATION_FAILED;
}


/***********************************************************************************************
 * WW3D::Set_Next_Render_Device -- just go to the next device in the list                      *
 *=============================================================================================*/
WW3DErrorType WW3D::Set_Next_Render_Device(void)
{
	bool success = DX8Wrapper::Set_Next_Render_Device();

	if (success) {
		return WW3D_ERROR_OK;
	}

	return WW3D_ERROR_INITIALIZATION_FAILED;
}


/***********************************************************************************************
 * WW3D::Get_Window -- returns the handle of the render window.                               *
 *=============================================================================================*/
void* WW3D::Get_Window(void)
{
	return _Hwnd;
}


/***********************************************************************************************
 * WW3D::Is_Windowed -- returns wether we are currently in a windowed mode                     *
 *=============================================================================================*/
bool WW3D::Is_Windowed(void)
{
	return DX8Wrapper::Is_Windowed();
}


/***********************************************************************************************
 * WW3D::Toggle_Windowed -- Toggle the current render device between fullscreen and windowed   *
 *=============================================================================================*/
WW3DErrorType WW3D::Toggle_Windowed(void)
{
	bool success = DX8Wrapper::Toggle_Windowed();

	if (success) {
		return WW3D_ERROR_OK;
	}

	return WW3D_ERROR_INITIALIZATION_FAILED;
}


/***********************************************************************************************
 * WW3D::Get_Render_Device -- Get the index of the current render device                       *
 *=============================================================================================*/
int WW3D::Get_Render_Device(void)
{
	return DX8Wrapper::Get_Render_Device();
}


/***********************************************************************************************
 * WW3D::Get_Render_Device_Desc -- returns description of the current render device            *
 *=============================================================================================*/
const RenderDeviceDescClass& WW3D::Get_Render_Device_Desc(int deviceidx)
{
	return DX8Wrapper::Get_Render_Device_Desc(deviceidx);
}


/***********************************************************************************************
 * WW3D::Get_Render_Device_Count -- returns the number of render devices available             *
 *=============================================================================================*/
const int WW3D::Get_Render_Device_Count(void)
{
	return DX8Wrapper::Get_Render_Device_Count();
}


/***********************************************************************************************
 * WW3D::Get_Render_Device_Name -- returns the name of the n-th render device                  *
 *=============================================================================================*/
const char* WW3D::Get_Render_Device_Name(int device_index)
{
	return DX8Wrapper::Get_Render_Device_Name(device_index);
}


/***********************************************************************************************
 * WW3D::Set_Device_Resolution -- set the current resolution and bitdepth                      *
 *=============================================================================================*/
WW3DErrorType WW3D::Set_Device_Resolution(
	int width,
	int height,
	int bits,
	int windowed,
	bool resize_window
)
{
	bool success =
		DX8Wrapper::Set_Device_Resolution(
			width,
			height,
			bits,
			windowed,
			resize_window
		);

	if (success) {
		return WW3D_ERROR_OK;
	}

	return WW3D_ERROR_INITIALIZATION_FAILED;
}


/***********************************************************************************************
 * WW3D::Get_Render_Target_Resolution -- get the resolution and bitdepth of the current target*
 *=============================================================================================*/
void WW3D::Get_Render_Target_Resolution(
	int& set_w,
	int& set_h,
	int& set_bits,
	bool& set_windowed
)
{
	DX8Wrapper::Get_Render_Target_Resolution(
		set_w,
		set_h,
		set_bits,
		set_windowed
	);
}


/***********************************************************************************************
 * WW3D::Get_Device_Resolution -- get the current resolution and bitdepth                      *
 *=============================================================================================*/
void WW3D::Get_Device_Resolution(
	int& set_w,
	int& set_h,
	int& set_bits,
	bool& set_windowed
)
{
	DX8Wrapper::Get_Device_Resolution(
		set_w,
		set_h,
		set_bits,
		set_windowed
	);
}


/***********************************************************************************************
 * WW3D::Registry_Save_Render_Device -- Saves settings to Registry
 *=============================================================================================*/
WW3DErrorType WW3D::Registry_Save_Render_Device(
	const char* sub_key
)
{
	bool success =
		DX8Wrapper::Registry_Save_Render_Device(
			sub_key
		);

	if (success) {
		return WW3D_ERROR_OK;
	}

	return WW3D_ERROR_INITIALIZATION_FAILED;
}


/***********************************************************************************************
 * WW3D::Registry_Save_Render_Device -- Saves settings to Registry
 *=============================================================================================*/
WW3DErrorType WW3D::Registry_Save_Render_Device(
	const char* sub_key,
	int device,
	int width,
	int height,
	int depth,
	bool windowed,
	int texture_depth
)
{
	bool success =
		DX8Wrapper::Registry_Save_Render_Device(
			sub_key,
			device,
			width,
			height,
			depth,
			windowed,
			texture_depth
		);

	if (success) {
		return WW3D_ERROR_OK;
	}

	return WW3D_ERROR_INITIALIZATION_FAILED;
}


/***********************************************************************************************
 * WW3D::Registry_Load_Render_Device -- Loads settings from Registry
 *=============================================================================================*/
WW3DErrorType WW3D::Registry_Load_Render_Device(
	const char* sub_key,
	bool resize_window
)
{
	bool success =
		DX8Wrapper::Registry_Load_Render_Device(
			sub_key,
			resize_window
		);

	if (success) {
		return WW3D_ERROR_OK;
	}

	return WW3D_ERROR_INITIALIZATION_FAILED;
}

bool WW3D::Registry_Load_Render_Device(
	const char* sub_key,
	char* device,
	int device_len,
	int& width,
	int& height,
	int& depth,
	int& windowed,
	int& texture_depth
)
{
	return DX8Wrapper::Registry_Load_Render_Device(
		sub_key,
		device,
		device_len,
		width,
		height,
		depth,
		windowed,
		texture_depth
	);
}

void WW3D::_Invalidate_Mesh_Cache()
{
	TheDX8MeshRenderer.Invalidate();
}

void WW3D::_Invalidate_Textures()
{
	if (!WW3DAssetManager::Get_Instance()) {
		return;
	}

	TextureLoader::Flush_Pending_Load_Tasks();

	HashTemplateIterator<StringClass, TextureClass*> ite(
		WW3DAssetManager::Get_Instance()->Texture_Hash()
	);

	for (
		ite.First();
		!ite.Is_Done();
		ite.Next()
		) {
		TextureClass* tex = ite.Peek_Value();
		tex->Invalidate();
	}
}

void WW3D::Set_Texture_Filter(int texture_filter)
{
	if (texture_filter < 0) {
		texture_filter = 0;
	}

	if (
		texture_filter >
		TextureClass::TEXTURE_FILTER_ANISOTROPIC
		) {
		texture_filter =
			TextureClass::TEXTURE_FILTER_ANISOTROPIC;
	}

	TextureFilter = texture_filter;

	TextureClass::_Init_Filters(
		(TextureClass::TextureFilterMode)TextureFilter
	);
}


/***********************************************************************************************
 * WW3D::Begin_Render -- mark the start of rendering for a new frame                           *
 *=============================================================================================*/
WW3DErrorType WW3D::Begin_Render(
	bool clear,
	bool clearz,
	const Vector3& color,
	void(*network_callback)(void)
)
{
	if (!IsInitted) {
		return(WW3D_ERROR_OK);
	}

	WWPROFILE("WW3D::Begin_Render");
	WWASSERT(IsInitted);

	LastFrameMemoryAllocations =
		WWMemoryLogClass::Get_Allocate_Count();

	LastFrameMemoryFrees =
		WWMemoryLogClass::Get_Free_Count();

	WWMemoryLogClass::Reset_Counters();

	TextureLoader::Update(network_callback);

	DynamicVBAccessClass::_Reset(true);
	DynamicIBAccessClass::_Reset(true);

#ifdef WW3D_DX8
	TextureFileClass::Update_Texture_Flash();
#endif

	Debug_Statistics::Begin_Statistics();

	if (
		IsCapturing &&
		MovieCaptureSourceState == MovieCaptureSource::Front &&
		(!PauseRecord || RecordNextFrame)
		) {
		Update_Movie_Capture();
		RecordNextFrame = false;
	}

	WWASSERT(!IsRendering);

	IsRendering = true;

	if (clear || clearz) {
		D3DVIEWPORT9 vp;

		int width;
		int height;
		int bits;
		bool windowed;

		WW3D::Get_Render_Target_Resolution(
			width,
			height,
			bits,
			windowed
		);

		vp.X = 0;
		vp.Y = 0;
		vp.Width = width;
		vp.Height = height;
		vp.MinZ = 0.0f;
		vp.MaxZ = 1.0f;

		DX8Wrapper::Set_Viewport(&vp);

		DX8Wrapper::Clear(
			clear,
			clearz,
			color
		);
	}

	DX8Wrapper::Begin_Scene();

	return WW3D_ERROR_OK;
}


/***********************************************************************************************
 * WW3D::Render -- Render a list of layers, starting at the back.                             *
 *=============================================================================================*/
WW3DErrorType WW3D::Render(const LayerListClass& LayerList)
{
	if (!IsInitted) {
		return(WW3D_ERROR_OK);
	}

	WWASSERT(IsRendering);

	LayerClass* layer = LayerList.Last();

	while (layer->Is_Valid()) {
		WW3DErrorType result = Render(*layer);

		if (result != WW3D_ERROR_OK) {
			return result;
		}

		layer = layer->Prev();
	}

	return WW3D_ERROR_OK;
}


/***********************************************************************************************
 * WW3D::Render -- Render a Layer                                                              *
 *=============================================================================================*/
WW3DErrorType WW3D::Render(const LayerClass& Layer)
{
	if (!IsInitted) {
		return(WW3D_ERROR_OK);
	}

	WWASSERT(IsRendering);

	return Render(
		Layer.Scene,
		Layer.Camera,
		Layer.Clear,
		Layer.ClearZ,
		Layer.ClearColor
	);
}


/***********************************************************************************************
 * WW3D::Render -- Render a 3D Scene using the given camera                                    *
 *=============================================================================================*/
WW3DErrorType WW3D::Render(
	SceneClass* scene,
	CameraClass* cam,
	bool clear,
	bool clearz,
	const Vector3& color
)
{
#ifdef W3D_CLIENT
	static int vr_render_test_counter = 0;

	if (vr_render_test_counter < 10) {
		FILE* file = fopen("F:\\OpenW3D\\Run\\vr_debug.log", "a");

		if (file) {
			fprintf(file, "WW3D::Render SCENE CALLED\n");
			fclose(file);
		}

		++vr_render_test_counter;
	}
#endif
	if (!IsInitted) {
		return(WW3D_ERROR_OK);
	}


	WWPROFILE("WW3D::Render");
	WWMEMLOG(MEM_GAMEDATA);

	WWASSERT(IsInitted);
	WWASSERT(IsRendering);
	WWASSERT(scene);
	WWASSERT(cam);

	cam->On_Frame_Update();

	switch (scene->Get_Polygon_Mode()) {
	case SceneClass::POINT:
		DX8Wrapper::Set_DX8_Render_State(
			D3DRS_FILLMODE,
			D3DFILL_POINT
		);
		break;

	case SceneClass::LINE:
		DX8Wrapper::Set_DX8_Render_State(
			D3DRS_FILLMODE,
			D3DFILL_WIREFRAME
		);
		break;

	case SceneClass::FILL:
		DX8Wrapper::Set_DX8_Render_State(
			D3DRS_FILLMODE,
			D3DFILL_SOLID
		);
		break;
	}

	Vector3 ambient =
		scene->Get_Ambient_Light();

	DX8Wrapper::Set_DX8_Render_State(
		D3DRS_AMBIENT,
		DX8Wrapper::Convert_Color(
			ambient,
			0.0f
		)
	);

	RenderInfoClass rinfo(*cam);

	cam->Apply();

	if (clear || clearz) {
		DX8Wrapper::Clear(
			clear,
			clearz,
			color
		);
	}

	TheDX8MeshRenderer.Set_Camera(
		&rinfo.Camera
	);

	scene->Render(
		rinfo
	);

	Flush(
		rinfo
	);

	return WW3D_ERROR_OK;
}


/***********************************************************************************************
 * WW3D::Render -- Render a single render object                                               *
 *=============================================================================================*/
WW3DErrorType WW3D::Render(
	RenderObjClass& obj,
	RenderInfoClass& rinfo
)
{
	if (!IsInitted) {
		return(WW3D_ERROR_OK);
	}

	WWPROFILENAMED(
		"WW3D::Render",
		top
	);

	WWASSERT(IsInitted);
	WWASSERT(IsRendering);

	{
		WWPROFILE("On_Frame_Update");

		rinfo.Camera.On_Frame_Update();
	}

	rinfo.Camera.Apply();

	DX8Wrapper::Set_DX8_Render_State(
		D3DRS_FILLMODE,
		D3DFILL_SOLID
	);

	if (rinfo.light_environment != nullptr) {
		DX8Wrapper::Set_Light_Environment(
			rinfo.light_environment
		);
	}

	TheDX8MeshRenderer.Set_Camera(
		&rinfo.Camera
	);

	obj.Render(
		rinfo
	);

	Flush(
		rinfo
	);

	return WW3D_ERROR_OK;
}


/***********************************************************************************************
 * WW3D::Flush -- Process all pending rendering tasks                                          *
 *=============================================================================================*/
void WW3D::Flush(RenderInfoClass& rinfo)
{
	TheDX8MeshRenderer.Flush();

	WW3D::Render_And_Clear_Static_Sort_Lists(
		rinfo
	);

	SortingRendererClass::Flush();

	TheDX8MeshRenderer.Clear_Pending_Delete_Lists();
}


/***********************************************************************************************
 * WW3D::End_Render -- Mark the completion of a frame                                          *
 *=============================================================================================*/
WW3DErrorType WW3D::End_Render(bool flip_frame)
{
	if (!IsInitted) {
		return(WW3D_ERROR_OK);
	}

	WWPROFILENAMED(
		"WW3D::End_Render",
		top
	);

	assert(IsRendering);
	assert(IsInitted);

	SortingRendererClass::Flush();

	IsRendering = false;

	{
		WWPROFILE("DX8Wrapper::End_Scene");

		DX8Wrapper::End_Scene(
			flip_frame
		);
	}

	FrameCount++;

	{
		WWPROFILE("End_Statistics");

		Debug_Statistics::End_Statistics();
	}

	Activate_Snapshot(false);

	return WW3D_ERROR_OK;
}


/***********************************************************************************************
 * WW3D::Flip_To_Primary                                                                       *
 *=============================================================================================*/
void WW3D::Flip_To_Primary(void)
{
	DX8Wrapper::Flip_To_Primary();
}


/***********************************************************************************************
 * WW3D::Get_Last_Frame_Poly_Count                                                             *
 *=============================================================================================*/
unsigned int WW3D::Get_Last_Frame_Poly_Count(void)
{
	return Debug_Statistics::Get_DX8_Polygons();
}

unsigned int WW3D::Get_Last_Frame_Vertex_Count(void)
{
	return Debug_Statistics::Get_DX8_Vertices();
}


/***********************************************************************************************
 * WW3D::Sync -- Time sychronization                                                           *
 *=============================================================================================*/
void WW3D::Sync(unsigned int sync_time)
{
	PreviousSyncTime = SyncTime;
	SyncTime = sync_time;
}


/***********************************************************************************************
 * WW3D::Set_Ext_Swap_Interval                                                                 *
 *=============================================================================================*/
void WW3D::Set_Ext_Swap_Interval(int swap)
{
	DX8Wrapper::Set_Swap_Interval(swap);
}


/***********************************************************************************************
 * WW3D::Get_Ext_Swap_Interval                                                                 *
 *=============================================================================================*/
int WW3D::Get_Ext_Swap_Interval(void)
{
	return DX8Wrapper::Get_Swap_Interval();
}


/***********************************************************************************************
 * WW3D::Set_Collision_Box_Display_Mask                                                        *
 *=============================================================================================*/
void WW3D::Set_Collision_Box_Display_Mask(int mask)
{
	BoxRenderObjClass::Set_Box_Display_Mask(mask);
}


/***********************************************************************************************
 * WW3D::Get_Collision_Box_Display_Mask                                                        *
 *=============================================================================================*/
int WW3D::Get_Collision_Box_Display_Mask(void)
{
	return BoxRenderObjClass::Get_Box_Display_Mask();
}


/***********************************************************************************************
 * WW3D::Normalize_Coordinates                                                                 *
 *=============================================================================================*/
void WW3D::Normalize_Coordinates(
	int x,
	int y,
	float& fx,
	float& fy
)
{
	x = Bound(
		x,
		0,
		DX8Wrapper::Get_Device_Resolution_Width()
	);

	y = Bound(
		y,
		0,
		DX8Wrapper::Get_Device_Resolution_Height()
	);

	fx =
		(float)x /
		DX8Wrapper::Get_Device_Resolution_Width();

	fy =
		(float)y /
		DX8Wrapper::Get_Device_Resolution_Height();
}


namespace
{
	int Make_Back_Buffer_Screen_Shot_Filename(
		const char* filename_base,
		StringClass& filename
	)
	{
		if (
			filename_base == nullptr ||
			filename_base[0] == '\0' ||
			_TheFileFactory == nullptr
			) {
			return 0;
		}

		static int frame_number = 1;
		int screenshot_number = 0;
		bool done = false;

		while (!done) {
			screenshot_number = frame_number++;

			filename.Format(
				"%s%.2d.tga",
				filename_base,
				screenshot_number
			);

			FileClass* file =
				_TheFileFactory->Get_File(
					filename.Peek_Buffer()
				);

			if (file != nullptr) {
				file->Open();

				done = !file->Is_Available();

				_TheFileFactory->Return_File(
					file
				);
			}
			else {
				done = true;
			}
		}

		return screenshot_number;
	}
}


/***********************************************************************************************
 * WW3D::Make_Screen_Shot                                                                      *
 *=============================================================================================*/
void WW3D::Make_Screen_Shot(
	const char* filename_base
)
{
	WWASSERT(!IsRendering);

	char filename[80];

	static int frame_number = 1;

	bool done = false;

	while (!done) {
		sprintf(
			filename,
			"%s%.2d.tga",
			filename_base,
			frame_number++
		);

		FileClass* file =
			_TheFileFactory->Get_File(
				filename
			);

		if (file) {
			file->Open();

			done = !file->Is_Available();

			_TheFileFactory->Return_File(
				file
			);
		}
		else {
			done = true;
		}
	}

	WWDEBUG_SAY((
		"Creating Screen Shot %s\n",
		filename
		));

	IDirect3DSurface9* fb;

	fb =
		DX8Wrapper::_Get_DX8_Front_Buffer();

	D3DSURFACE_DESC desc;

	fb->GetDesc(
		&desc
	);

	RECT bounds;

	GetWindowRect(
		_Hwnd,
		&bounds
	);

	D3DLOCKED_RECT lrect;

	DX8_ErrorCode(
		fb->LockRect(
			&lrect,
			&bounds,
			D3DLOCK_READONLY
		)
	);

	unsigned int x;
	unsigned int y;
	unsigned int index;
	unsigned int index2;
	unsigned int width;
	unsigned int height;

	width =
		bounds.right -
		bounds.left;

	height =
		bounds.bottom -
		bounds.top;

	char* image =
		new char[
			3 *
				width *
				height
		];

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			index =
				3 *
				(x + y * width);

			index2 =
				y *
				lrect.Pitch +
				4 *
				x;

			image[index] =
				*((char*)lrect.pBits + index2 + 2);

			image[index + 1] =
				*((char*)lrect.pBits + index2 + 1);

			image[index + 2] =
				*((char*)lrect.pBits + index2 + 0);
		}
	}

	fb->Release();

	Targa targ;

	memset(
		&targ.Header,
		0,
		sizeof(targ.Header)
	);

	targ.Header.Width =
		short(width);

	targ.Header.Height =
		short(height);

	targ.Header.PixelDepth =
		24;

	targ.Header.ImageType =
		TGA_TRUECOLOR;

	targ.SetImage(
		image
	);

	targ.YFlip();

	FileClass* file =
		_TheWritingFileFactory->Get_File(
			filename
		);

	if (file) {
		file->Create();
		file->Close();

		_TheWritingFileFactory->Return_File(
			file
		);
	}

	targ.Save(
		filename,
		TGAF_IMAGE,
		false
	);

	delete[] image;
}


/***********************************************************************************************
 * WW3D::Make_Back_Buffer_Screen_Shot -- saves the current render-device back buffer            *
 *=============================================================================================*/
int WW3D::Make_Back_Buffer_Screen_Shot(
	const char* filename_base
)
{
	WWASSERT(!IsRendering);

#ifdef _WIN32
	StringClass filename;

	const int screenshot_number =
		Make_Back_Buffer_Screen_Shot_Filename(
			filename_base,
			filename
		);

	if (screenshot_number == 0) {
		return 0;
	}

	WWDEBUG_SAY((
		"Creating Back Buffer Screen Shot %s\n",
		filename.Peek_Buffer()
		));

	IDirect3DDevice9* device =
		DX8Wrapper::_Get_D3D_Device8();

	if (device == nullptr) {
		return 0;
	}

	IDirect3DSurface9* back_buffer = nullptr;

	if (
		FAILED(
			device->GetBackBuffer(
				0,
				0,
				D3DBACKBUFFER_TYPE_MONO,
				&back_buffer
			)
		) ||
		back_buffer == nullptr
		) {
		return 0;
	}

	const HRESULT save_result =
		D3DXSaveSurfaceToFileA(
			filename.Peek_Buffer(),
			D3DXIFF_TGA,
			back_buffer,
			nullptr,
			nullptr
		);

	back_buffer->Release();

	return SUCCEEDED(save_result)
		? screenshot_number
		: 0;
#else
	(void)filename_base;
	return 0;
#endif
}


/***********************************************************************************************
 * WW3D::Start_Movie_Capture                                                                  *
 *=============================================================================================*/
void WW3D::Start_Movie_Capture(
	const char* filename_base,
	float frame_rate
)
{
#ifdef _WIN32
	if (
		IsCapturing ||
		Movie != nullptr ||
		MovieCaptureSourceState != MovieCaptureSource::None
		) {
		Stop_Movie_Capture();
	}

	WWASSERT(!IsCapturing);

	RECT bounds = {};

	if (
		_Hwnd == nullptr ||
		!GetWindowRect(
			_Hwnd,
			&bounds
		)
		) {
		return;
	}

	int height =
		bounds.bottom -
		bounds.top;

	int width =
		bounds.right -
		bounds.left;

	int depth = 24;

	WWASSERT(Movie == nullptr);

	if (frame_rate == 0.0f) {
		frame_rate = 1.0f;
		PauseRecord = true;
	}
	else {
		PauseRecord = false;
	}

	FrameGrabClass* movie =
		new FrameGrabClass(
			filename_base,
			FrameGrabClass::AVI,
			width,
			height,
			depth,
			frame_rate
		);

	if (
		movie == nullptr ||
		!movie->IsReady()
		) {
		delete movie;

		MovieCaptureSourceState =
			MovieCaptureSource::None;

		IsCapturing = false;

		return;
	}

	Movie = movie;

	MovieCaptureSourceState =
		MovieCaptureSource::Front;

	IsCapturing = true;

	WWDEBUG_SAY((
		"Starting Movie %s\n",
		filename_base
		));
#endif
}


/***********************************************************************************************
 * WW3D::Try_Start_Movie_Capture_From_Back_Buffer
 *=============================================================================================*/
bool WW3D::Try_Start_Movie_Capture_From_Back_Buffer(
	const char* filename_base,
	float frame_rate
)
{
#ifdef _WIN32
	if (
		IsCapturing ||
		Movie != nullptr ||
		MovieCaptureSourceState != MovieCaptureSource::None
		) {
		Stop_Movie_Capture();
	}

	if (
		filename_base == nullptr ||
		filename_base[0] == '\0' ||
		!(frame_rate > 0.0f) ||
		!std::isfinite(frame_rate)
		) {
		return false;
	}

	IDirect3DDevice9* device =
		DX8Wrapper::_Get_D3D_Device8();

	if (device == nullptr) {
		return false;
	}

	IDirect3DSurface9* back_buffer = nullptr;

	HRESULT result =
		device->GetBackBuffer(
			0,
			0,
			D3DBACKBUFFER_TYPE_MONO,
			&back_buffer
		);

	if (
		FAILED(result) ||
		back_buffer == nullptr
		) {
		return false;
	}

	D3DSURFACE_DESC desc = {};

	result =
		back_buffer->GetDesc(
			&desc
		);

	back_buffer->Release();

	if (
		FAILED(result) ||
		desc.Width == 0 ||
		desc.Height == 0 ||
		desc.Width >
		static_cast<unsigned int>(
			std::numeric_limits<int>::max()
			) ||
		desc.Height >
		static_cast<unsigned int>(
			std::numeric_limits<int>::max()
			) ||
		desc.MultiSampleType !=
		D3DMULTISAMPLE_NONE ||
		(
			desc.Format != D3DFMT_A8R8G8B8 &&
			desc.Format != D3DFMT_X8R8G8B8
			)
		) {
		return false;
	}

	FrameGrabClass* movie =
		new FrameGrabClass(
			filename_base,
			FrameGrabClass::AVI,
			static_cast<int>(desc.Width),
			static_cast<int>(desc.Height),
			24,
			frame_rate
		);

	if (
		movie == nullptr ||
		!movie->IsReady()
		) {
		delete movie;
		return false;
	}

	Movie = movie;

	PauseRecord = true;
	RecordNextFrame = false;

	MovieCaptureSourceState =
		MovieCaptureSource::Back;

	IsCapturing = true;

	WWDEBUG_SAY((
		"Starting back-buffer movie %s\n",
		filename_base
		));

	return true;
#else
	(void)filename_base;
	(void)frame_rate;
	return false;
#endif
}


/***********************************************************************************************
 * WW3D::Stop_Movie_Capture                                                                    *
 *=============================================================================================*/
void WW3D::Stop_Movie_Capture(
	void
)
{
#ifdef _WIN32
	if (IsCapturing) {
		WWDEBUG_SAY((
			"Stoping Movie\n"
			));
	}

	IsCapturing = false;
	PauseRecord = false;
	RecordNextFrame = false;

	MovieCaptureSourceState =
		MovieCaptureSource::None;

	if (Movie != nullptr) {
		delete Movie;
		Movie = nullptr;
	}
#endif
}


/***********************************************************************************************
 * WW3D::Toggle_Movie_Capture                                                                  *
 *=============================================================================================*/
void WW3D::Toggle_Movie_Capture(
	const char* filename_base,
	float frame_rate
)
{
	if (IsCapturing) {
		Stop_Movie_Capture();
	}
	else {
		Start_Movie_Capture(
			filename_base,
			frame_rate
		);
	}
}


/***********************************************************************************************
 * WW3D::Start_Single_Frame_Movie_Capture                                                     *
 *=============================================================================================*/
void WW3D::Start_Single_Frame_Movie_Capture(
	const char* filename_base
)
{
	Start_Movie_Capture(
		filename_base,
		0.0f
	);
}


/***********************************************************************************************
 * WW3D::Capture_Next_Movie_Frame                                                              *
 *=============================================================================================*/
void WW3D::Capture_Next_Movie_Frame()
{
	RecordNextFrame = true;
}


/***********************************************************************************************
 * WW3D::Pause_Movie                                                                           *
 *=============================================================================================*/
void WW3D::Pause_Movie(bool mode)
{
	PauseRecord = mode;
}


/***********************************************************************************************
 * WW3D::Is_Movie_Paused                                                                       *
 *=============================================================================================*/
bool WW3D::Is_Movie_Paused()
{
	return PauseRecord;
}


/***********************************************************************************************
 * WW3D::Is_Recording_Next_Frame                                                               *
 *=============================================================================================*/
bool WW3D::Is_Recording_Next_Frame()
{
	return (Movie != 0) &&
		(!PauseRecord || RecordNextFrame);
}


/***********************************************************************************************
 * WW3D::Is_Movie_Ready                                                                         *
 *=============================================================================================*/
bool WW3D::Is_Movie_Ready()
{
	return Movie != 0;
}


/***********************************************************************************************
 * WW3D::Update_Movie_Capture                                                                   *
 *=============================================================================================*/
void WW3D::Update_Movie_Capture(
	void
)
{
#ifdef _WIN32
	WWASSERT(IsCapturing);

	WWPROFILE(
		"WW3D::Update_Movie_Capture"
	);

	WWDEBUG_SAY((
		"Updating\n"
		));

	if (
		!IsCapturing ||
		MovieCaptureSourceState != MovieCaptureSource::Front ||
		Movie == nullptr ||
		!Movie->IsReady()
		) {
		Stop_Movie_Capture();
		return;
	}

	IDirect3DSurface9* fb =
		DX8Wrapper::_Get_DX8_Front_Buffer();

	if (fb == nullptr) {
		Stop_Movie_Capture();
		return;
	}

	RECT bounds = {};

	if (
		_Hwnd == nullptr ||
		!GetWindowRect(
			_Hwnd,
			&bounds
		)
		) {
		fb->Release();
		Stop_Movie_Capture();
		return;
	}

	const int width =
		bounds.right -
		bounds.left;

	const int height =
		bounds.bottom -
		bounds.top;

	if (
		width != Movie->GetWidth() ||
		height != Movie->GetHeight()
		) {
		fb->Release();
		Stop_Movie_Capture();
		return;
	}

	D3DLOCKED_RECT lrect = {};

	const HRESULT lock_result =
		fb->LockRect(
			&lrect,
			&bounds,
			D3DLOCK_READONLY
		);

	if (FAILED(lock_result)) {
		fb->Release();
		Stop_Movie_Capture();
		return;
	}

	void* movie_buffer =
		Movie->GetBuffer();

	const bool converted =
		FrameGrabClass::ConvertBGRA32ToBGR24(
			lrect.pBits,
			lrect.Pitch,
			movie_buffer,
			Movie->GetRowStride(),
			width,
			height
		);

	const HRESULT unlock_result =
		fb->UnlockRect();

	fb->Release();

	if (
		!converted ||
		FAILED(unlock_result) ||
		!Movie->TryGrab(movie_buffer)
		) {
		Stop_Movie_Capture();
	}
#endif
}


/***********************************************************************************************
 * WW3D::Try_Update_Movie_Capture_From_Back_Buffer
 *=============================================================================================*/
bool WW3D::Try_Update_Movie_Capture_From_Back_Buffer(
	void
)
{
#ifdef _WIN32
	if (
		MovieCaptureSourceState !=
		MovieCaptureSource::Back
		) {
		return false;
	}

	if (
		!IsCapturing ||
		Movie == nullptr ||
		!Movie->IsReady()
		) {
		Stop_Movie_Capture();
		return false;
	}

	WWPROFILE(
		"WW3D::Try_Update_Movie_Capture_From_Back_Buffer"
	);

	IDirect3DDevice9* device =
		DX8Wrapper::_Get_D3D_Device8();

	if (device == nullptr) {
		Stop_Movie_Capture();
		return false;
	}

	IDirect3DSurface9* back_buffer = nullptr;

	HRESULT result =
		device->GetBackBuffer(
			0,
			0,
			D3DBACKBUFFER_TYPE_MONO,
			&back_buffer
		);

	if (
		FAILED(result) ||
		back_buffer == nullptr
		) {
		Stop_Movie_Capture();
		return false;
	}

	D3DSURFACE_DESC desc = {};

	result =
		back_buffer->GetDesc(
			&desc
		);

	if (
		FAILED(result) ||
		desc.Width !=
		static_cast<unsigned int>(
			Movie->GetWidth()
			) ||
		desc.Height !=
		static_cast<unsigned int>(
			Movie->GetHeight()
			) ||
		desc.MultiSampleType !=
		D3DMULTISAMPLE_NONE ||
		(
			desc.Format != D3DFMT_A8R8G8B8 &&
			desc.Format != D3DFMT_X8R8G8B8
			)
		) {
		back_buffer->Release();
		Stop_Movie_Capture();
		return false;
	}

	IDirect3DSurface9* staging_buffer =
		nullptr;

	result =
		device->CreateOffscreenPlainSurface(
			desc.Width,
			desc.Height,
			desc.Format,
			D3DPOOL_SYSTEMMEM,
			&staging_buffer,
			nullptr
		);

	if (SUCCEEDED(result)) {
		result =
			device->GetRenderTargetData(
				back_buffer,
				staging_buffer
			);
	}

	back_buffer->Release();

	if (
		FAILED(result) ||
		staging_buffer == nullptr
		) {
		if (staging_buffer != nullptr) {
			staging_buffer->Release();
		}

		Stop_Movie_Capture();
		return false;
	}

	D3DLOCKED_RECT locked = {};

	result =
		staging_buffer->LockRect(
			&locked,
			nullptr,
			D3DLOCK_READONLY
		);

	if (FAILED(result)) {
		staging_buffer->Release();
		Stop_Movie_Capture();
		return false;
	}

	void* movie_buffer =
		Movie->GetBuffer();

	const bool converted =
		FrameGrabClass::ConvertBGRA32ToBGR24(
			locked.pBits,
			locked.Pitch,
			movie_buffer,
			Movie->GetRowStride(),
			Movie->GetWidth(),
			Movie->GetHeight()
		);

	const HRESULT unlock_result =
		staging_buffer->UnlockRect();

	staging_buffer->Release();

	if (
		!converted ||
		FAILED(unlock_result) ||
		!Movie->TryGrab(movie_buffer)
		) {
		Stop_Movie_Capture();
		return false;
	}

	return true;
#else
	return false;
#endif
}


/***********************************************************************************************
 * WW3D::Get_Movie_Capture_Frame_Rate                                                           *
 *=============================================================================================*/
float WW3D::Get_Movie_Capture_Frame_Rate(
	void
)
{
#ifdef _WIN32
	if (IsCapturing) {
		return Movie->GetFrameRate();
	}
#endif

	return 0;
}


/***********************************************************************************************
 * WW3D::Set_Texture_Reduction                                                                 *
 *=============================================================================================*/
void WW3D::Set_Texture_Reduction(
	int value
)
{
	if (_TextureReduction != value) {
		_TextureReduction = value;
		_Invalidate_Textures();
	}
}


void WW3D::Enable_Texturing(bool b)
{
	if (b == IsTexturingEnabled) {
		return;
	}

	IsTexturingEnabled = b;
}


/***********************************************************************************************
 * WW3D::Get_Texture_Reduction                                                                  *
 *=============================================================================================*/
int WW3D::Get_Texture_Reduction(
	void
)
{
	return _TextureReduction;
}


/***********************************************************************************************
 * WW3D::Peek_Default_Debug_Material                                                            *
 *=============================================================================================*/
VertexMaterialClass* WW3D::Peek_Default_Debug_Material(
	void
)
{
#ifdef WWDEBUG
	WWASSERT(DefaultDebugMaterial);

	return DefaultDebugMaterial;
#else
	return nullptr;
#endif
}


/***********************************************************************************************
 * WW3D::Peek_Default_Debug_Shader                                                              *
 *=============================================================================================*/
ShaderClass WW3D::Peek_Default_Debug_Shader(
	void
)
{
	return DefaultDebugShader;
}


/***********************************************************************************************
 * WW3D::Peek_Lightmap_Debug_Shader                                                             *
 *=============================================================================================*/
ShaderClass WW3D::Peek_Lightmap_Debug_Shader(
	void
)
{
	return LightmapDebugShader;
}


/***********************************************************************************************
 * WW3D::Allocate_Debug_Resources                                                               *
 *=============================================================================================*/
void WW3D::Allocate_Debug_Resources(
	void
)
{
#ifdef WWDEBUG
	WWASSERT(DefaultDebugMaterial == nullptr);

	DefaultDebugMaterial =
		new VertexMaterialClass;

	DefaultDebugMaterial->Set_Shininess(0.0f);
	DefaultDebugMaterial->Set_Opacity(1.0f);
	DefaultDebugMaterial->Set_Ambient(0, 0, 0);
	DefaultDebugMaterial->Set_Diffuse(0, 0, 0);
	DefaultDebugMaterial->Set_Specular(0, 0, 0);
	DefaultDebugMaterial->Set_Emissive(0, 0, 0);
#endif
}


/***********************************************************************************************
 * WW3D::Release_Debug_Resources                                                               *
 *=============================================================================================*/
void WW3D::Release_Debug_Resources(
	void
)
{
#ifdef WWDEBUG
	WWASSERT(DefaultDebugMaterial);

	REF_PTR_RELEASE(
		DefaultDebugMaterial
	);
#endif
}


WW3DErrorType WW3D::On_Deactivate_App(void)
{
#ifdef WW3D_DX8
	assert(!IsRendering);

	if (Gerd == nullptr) {
		return WW3D_ERROR_OK;
	}

	if (IsWindowed) {
		return WW3D_ERROR_OK;
	}

	if (!Gerd->isWindowOpen()) {
		return WW3D_ERROR_OK;
	}

	Gerd->closeWindow();
#endif

	return WW3D_ERROR_OK;
}


WW3DErrorType WW3D::On_Activate_App(void)
{
#ifdef WW3D_DX8
	if (Gerd == nullptr) {
		return WW3D_ERROR_OK;
	}

	if (IsWindowed) {
		return WW3D_ERROR_OK;
	}

	assert(!Gerd->isWindowOpen());

	srGERD::DisplayMode disp_mode;

	disp_mode =
		Gerd->getDisplayMode(
			ResolutionWidth,
			ResolutionHeight,
			BitDepth
		);

	if (
		Gerd->openWindow(disp_mode) !=
		srGERD::ERROR_NONE
		) {
		return WW3D_ERROR_WINDOW_NOT_OPEN;
	}
#endif

	return WW3D_ERROR_OK;
}


void WW3D::Get_Pixel_Center(
	float& x,
	float& y
)
{
	x = PixelCenterX;
	y = PixelCenterY;
}


void WW3D::Update_Pixel_Center(
	void
)
{
#ifdef WW3D_DX8
	const char* name =
		_RenderDeviceShortNameTable.getString(
			CurRenderDevice
		);

	if (strstr(name, "OpenGL")) {
		PixelCenterX = 0.0f;
		PixelCenterY = 0.0f;
	}
	else if (strstr(name, "Glide")) {
		PixelCenterX = 0.0f;
		PixelCenterY = 0.0f;
	}
	else if (strstr(name, "DirectX")) {
		PixelCenterX = 0.5f;
		PixelCenterY = 0.5f;
	}
	else if (strstr(name, "Software")) {
		PixelCenterX = 0.0f;
		PixelCenterY = 0.0f;
	}
	else if (strstr(name, "Null")) {
		PixelCenterX = 0.0f;
		PixelCenterY = 0.0f;
	}
	else {
		PixelCenterX = 0.0f;
		PixelCenterY = 0.0f;
	}
#endif
}


void WW3D::Set_Texture_Bitdepth(
	int bitdepth
)
{
	DX8Wrapper::Set_Texture_Bitdepth(
		bitdepth
	);
}


int WW3D::Get_Texture_Bitdepth()
{
	return DX8Wrapper::Get_Texture_Bitdepth();
}


void WW3D::Add_To_Static_Sort_List(
	RenderObjClass* robj,
	unsigned int sort_level
)
{
	if (
		sort_level < 1 ||
		sort_level > MAX_SORT_LEVEL
		) {
		WWASSERT(0);
		return;
	}

	CurrentStaticSortLists[sort_level].Add_Tail(
		robj,
		false
	);
}


void WW3D::Render_And_Clear_Static_Sort_Lists(
	RenderInfoClass& rinfo
)
{
	bool old_enable =
		AreStaticSortListsEnabled;

	AreStaticSortListsEnabled = false;

	for (
		unsigned int sort_level = MaxStaticSortLevel;
		sort_level >= MinStaticSortLevel;
		sort_level--
		) {
		bool render = false;

		for (
			RenderObjClass* robj =
			CurrentStaticSortLists[sort_level].Remove_Head();

			robj;

			robj->Release_Ref(),
			robj =
			CurrentStaticSortLists[sort_level].Remove_Head()
			) {
			robj->Render(rinfo);
			render = true;
		}

		if (render) {
			TheDX8MeshRenderer.Flush();
		}
	}

	AreStaticSortListsEnabled = old_enable;
}


void WW3D::Enable_Sorting(
	bool onoff
)
{
	IsSortingEnabled = onoff;

	TheDX8MeshRenderer.Invalidate();
}


void WW3D::Override_Current_Static_Sort_Lists(
	RefRenderObjListClass* sort_list,
	unsigned int min_sort,
	unsigned int max_sort
)
{
	CurrentStaticSortLists = sort_list;

	if (min_sort <= max_sort) {
		MinStaticSortLevel = min_sort;
		MaxStaticSortLevel = max_sort;
	}
	else {
		WWASSERT(0);

		MinStaticSortLevel = max_sort;
		MaxStaticSortLevel = min_sort;
	}
}


void WW3D::Reset_Current_Static_Sort_Lists_To_Default(
	void
)
{
	CurrentStaticSortLists =
		DefaultStaticSortLists;

	MinStaticSortLevel = 1;
	MaxStaticSortLevel = MAX_SORT_LEVEL;
}