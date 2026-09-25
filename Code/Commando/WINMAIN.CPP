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
**	along with this program.  If not, see http://www.gnu.org/licenses/.
*/

/***********************************************************************************************
***                            Confidential - Westwood Studios                              ***


																					  *


Project Name : Commando                                                     *
*
$Archive:: /Commando/Code/Commando/WINMAIN.CPP                         $*
*
$Author:: Steve_t                                                     $*
*
$Modtime:: 2/17/02 11:08a                                              $*
*
$Revision:: 85                                                          $*
*

*---------------------------------------------------------------------------------------------*

* Functions:                                                                                  *
* WinMain -- Win32 Program Entry Point!                                                     *
* WIN_resize -- Surrender-required function which resizes the main window                   *
* WIN_set_fullscreen -- Surrender-required function for toggling full-screen mode           *
* Main_Window_Proc -- Windows Proc for the main game window                                 *
* Create_Main_Window -- Creates the main game window                                        *
* On_Focus_Loss -- this function is called when the application loses focus                 *
* On_Focus_Restore -- this function is called when the application gets focus               *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "WINMAIN.H"
#define _WIN32_WINDOWS 0x0401
#include "winmain_compat.h"
#if defined(OPENW3D_WIN32)
#include <windows.h>
#elif defined(OPENW3D_SDL3)
#include <SDL3/SDL.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif
#endif
#include "win.h"
#include "resource.h"
#include "ww3d.h"
#include "wwdialog.h"

#include "miscutil.h"
#include "input.h"
#include "useroptions.h"

#include "WWAudio.h"
#include "ffactory.h"

#include "debug.h"
#include "verchk.h"
#include "devoptions.h"
#include "dialogmgr.h"
#include "renegadedialogmgr.h"
#include "Except.h"

#include "directinput.h"
#include "WebBrowser.h"
#include "wwmemlog.h"

#include "datasafe.h"

#include "combatgmode.h"
#include "registry.h"
#include "init.h"
#include "_globals.h"
#include "buildnum.h"
#include "dx8wrapper.h"
#include "formconv.h"
#include "AutoStart.h"
#include "ConsoleMode.h"
#include "specialbuilds.h"
#include "slavemaster.h"

#include "ServerSettings.h"
#include "openw3d.h"
#include "ini.h"
#include "soutil.h"
#include "pathutil.h"
#include <openvr.h>
#include "vrmanager.h"

#ifdef _DEBUG
#include	<crtdbg.h>
#endif //_DEBUG
#include <iostream>
#include "singletoninstancekeeper.h"
#include "packetmgr.h"
#include "ProcessManager.h"

//----------------------------------------------------------------------------
//	Globals
//----------------------------------------------------------------------------
extern "C"
{
	HWND		hWndMain;
	bool		WIN_fullscreen = true;
}

int Start_Application(void);
bool Set_Working_Directory(void);
static BOOL Create_Main_Window(void);
void On_Focus_Loss(void);
void On_Focus_Restore(void);

//#include "packet.h"

#if 1 //ndef _MSC_VER
int Top_Level_Exception_Filter(EXCEPTION_POINTERS* e_info)
{
	return(Exception_Handler(e_info->ExceptionRecord->ExceptionCode, e_info));
}
#endif //_MSC_VER

/***********************************************************************************************

* WinMain -- Win32 Program Entry Point!                                                       *
  *
* INPUT:                                                                                      *
  *
* Standard WinMain inputs :-)																				  *
  *
* OUTPUT:																												  *
  *
* Standard WinMain output																						  *
  *
* WARNINGS:																											  *
  *
* Standard WinMain output																				  *
  *
* HISTORY:                                                                                    *
* 07/18/1997 GH  : Created.                                                                 *
  *=============================================================================================*/
int main(int argc, char* argv[])
{
	WWMemoryLogClass::Init();

#ifdef _DEBUG
	if (cDevOptions::CrtDbgEnabled.Is_True()) {

		
			_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
		_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	}
	

#endif //_DEBUG

#ifdef WWDEBUG
		if (cDevOptions::PacketOptimizationsEnabled.Is_False()) {
			PacketManager.Disable_Optimizations();
		}
#endif //WWDEBUG

#ifdef FREEDEDICATEDSERVER
	if (!SlaveMaster.Am_I_Slave() && !ServerSettingsClass::Is_Server_Settings_File_Set()) {
		cUserOptions::Set_Server_INI_File("server.ini");
	}
	ConsoleBox.Set_Exclusive(true);
#endif //FREEDEDICATEDSERVER

	
		switch (cUserOptions::Parse_Command_Line(argc, argv)) {
		case cUserOptions::ParseResult::SUCCESS:
			break;
		case cUserOptions::ParseResult::FAILURE:
			fprintf(stderr, "Error during parsing arguments\n");
			cUserOptions::Print_Command_Line_Help(true);
			return 1;
		case cUserOptions::ParseResult::PRINT_HELP:
			cUserOptions::Print_Command_Line_Help(false);
			return 0;
		}
	

#if WEBBROWSER_ENABLED
#ifdef _DEBUG
		WebBrowser::InstallPrerequisites();
#endif // WWDEBUG
#endif // WEBBROWSER_ENABLED

	
		SingletonInstanceKeeperClass instance_keeper;
	if (instance_keeper.Verify_Safe_To_Execute() == false) {
		return 0;
	}

	int retval = Start_Application();
	return retval;
	

}

static bool Graphics_Settings_Trouble_Shooting()
{
	RegistryClass registry(APPLICATION_SUB_KEY_NAME_DEBUG);
	if (!registry.Is_Valid()) return true;

	
		int progress = registry.Get_Int(VALUE_NAME_GAME_INITIALIZATION_IN_PROGRESS, 0);
	if (progress) {
		const char* args[] = {
			"wwconfig.exe",
			nullptr,
			nullptr,
			nullptr,
		};
		if (OpenW3D::Has_Config_File_Path_Override()) {
			args[1] = "--ini";
			args[2] = OpenW3D::Get_Config_File_Path();
		}
		Process* process = ProcessManager::Create_Process(args);
		if (!process) {
			return 1;
		}
		process->Wait();
		bool success = *process->Return_Code() == 0;
		delete process;
		return success;
	}
	return true;
	

}

typedef IDirect3D9* (WINAPI* Direct3DCreate9Type) (UINT SDKVersion);
static Direct3DCreate9Type Direct3DCreate9Ptr = nullptr;
extern const char* Get_D3D9_Object_Name();
static SharedObject* D3D9Lib = nullptr;

static bool Video_Card_Driver_Check()
{
	INIClass ini(W3D_CONF_FILE);

	
		int disabled = ini.Get_Int(W3D_SECTION_RENDER, "DriverVersionCheckDisabled", 0);
	if (disabled >= 87) return true;

	IDirect3D9* d3d = nullptr;
	D3DCAPS9 tmp_caps;
	const D3DCAPS9* d3dcaps = nullptr;
	D3DADAPTER_IDENTIFIER9 adapter_id;

	Init_D3D_To_WW3_Conversion();

	D3D9Lib = SharedObject::LoadObject(Get_D3D9_Object_Name());

	if (D3D9Lib != nullptr) {
		Direct3DCreate9Ptr = reinterpret_cast<Direct3DCreate9Type>(D3D9Lib->LoadFunction("Direct3DCreate9"));
		if (Direct3DCreate9Ptr) {
			d3d = Direct3DCreate9Ptr(D3D_SDK_VERSION);
			if (!d3d) {
				delete D3D9Lib;
				D3D9Lib = nullptr;
				return true;
			}
		}
		else {
			delete D3D9Lib;
			D3D9Lib = nullptr;
			return true;
		}
	}
	else {
		return true;
	}

	int current_adapter_index = D3DADAPTER_DEFAULT;

	char device_name[256] = { 0 };
	ini.Get_String(W3D_SECTION_RENDER, VALUE_INI_RENDER_DEVICE_NAME, "", device_name, sizeof(device_name));

	int adapter_count = d3d->GetAdapterCount();
	for (int adapter_index = 0; adapter_index < adapter_count; adapter_index++) {
		D3DADAPTER_IDENTIFIER9 id;
		memset(&id, 0, sizeof(D3DADAPTER_IDENTIFIER9));
		HRESULT res = d3d->GetAdapterIdentifier(adapter_index, 0, &id);
		if (res == D3D_OK) {
			StringClass name(id.Description, true);
			if (name == device_name) {
				current_adapter_index = adapter_index;
				break;
			}
		}
	}

	if (FAILED(d3d->GetDeviceCaps(
		current_adapter_index,
		D3DDEVTYPE_HAL,
		&tmp_caps))) {
		d3d->Release();
		delete D3D9Lib;
		return true;
	}

	memset(&adapter_id, 0, sizeof(D3DADAPTER_IDENTIFIER9));
	if (FAILED(d3d->GetAdapterIdentifier(
		current_adapter_index,
		0,
		&adapter_id))) {
		d3d->Release();
		delete D3D9Lib;
		D3D9Lib = nullptr;
		return true;
	}

	d3dcaps = &tmp_caps;
	DX8Caps caps(d3d, *d3dcaps, WW3D_FORMAT_UNKNOWN, adapter_id);

	DX8Caps::DriverVersionStatusType status = caps.Get_Driver_Version_Status();

	d3d->Release();
	delete D3D9Lib;
	D3D9Lib = nullptr;

	switch (status) {
	default:
	case DX8Caps::DRIVER_STATUS_GOOD:
	case DX8Caps::DRIVER_STATUS_OK:
	case DX8Caps::DRIVER_STATUS_UNKNOWN:
		ini.Put_Int(W3D_SECTION_RENDER, "DriverVersionCheckDisabled", 87);
		OpenW3D::Save_Config(ini);
		return true;
	case DX8Caps::DRIVER_STATUS_BAD:
		break;
	}

	const char* options[] = {
		"wwconfig.exe",
		"-driverversion",
		nullptr,
		nullptr,
		nullptr,
	};
	if (OpenW3D::Has_Config_File_Path_Override()) {
		options[2] = "--ini";
		options[3] = OpenW3D::Get_Config_File_Path();
	}
	Process* process = ProcessManager::Create_Process(options);
	if (!process) {
		return true;
	}
	process->Wait();
	return process->Return_Code() == 0;
	

}

/***********************************************************************************************

* Start_Application -- Handles main execution.	                                               *
  *=============================================================================================*/
int Start_Application(void)
{
	{
		WWMEMLOG(MEM_GAMEINIT);

		
			if (!Set_Working_Directory()) {
				::Show_Message_Box(MESSAGEBOX_BUTTONS_OK | MESSAGEBOX_SEVERITY_ERROR, "Set working folder and try again...", "Invalid working folder");
				return 0;
			}

		char tmp_buffer[256];
		char* tmp_ptr;
		if (!SearchPathA(
			"data",
			"always.dat",
			nullptr,
			sizeof(tmp_buffer),
			tmp_buffer,
			&tmp_ptr)) {
			::Show_Message_Box(MESSAGEBOX_BUTTONS_OK | MESSAGEBOX_SEVERITY_ERROR, "Invalid working folder", "Set working folder and try again...");
			return 0;
		}

		if (AutoRestart.Get_Restart_Flag() == false && !ConsoleBox.Is_Exclusive()) {
			if (!Video_Card_Driver_Check()) return 0;
			if (!Graphics_Settings_Trouble_Shooting()) return 0;
		}

		if (!Create_Main_Window()) return 0;

		Register_Thread_ID(ThreadClass::Get_Current_Thread_ID(), "Main Thread", true);
		

	}

	int exitCode = EXIT_SUCCESS;

#ifdef WWDEBUG
	if (cDevOptions::EnableExceptionHandler.Is_False()) {
		exitCode = Game_Main_Loop();
	}
	else {
#endif

		
			Register_Application_Exception_Callback(&Application_Exception_Callback);
		Register_Application_Version_Callback(&BuildInfoClass::Composite_Build_Info);
		

#if 0
			__try {
#else
			SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)&Top_Level_Exception_Filter);
#endif

		
			WWDEBUG_SAY(("Game_Main_Loop\n"));
		exitCode = Game_Main_Loop();
		

#if 0
		}
	__except (Exception_Handler(GetExceptionCode(), GetExceptionInformation())) {};
#else
			SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)&Top_Level_Exception_Filter);
#endif

#ifdef WWDEBUG
	}
#endif

#ifdef W3D_CLIENT
	VRManager::Shutdown();
#endif

	
		Unregister_Thread_ID(ThreadClass::Get_Current_Thread_ID(), "Main Thread");

	return exitCode;
	

}

/***********************************************************************************************

* Main_Window_Proc -- This function handles the main game window.
  ***********************************************************************************************/
LRESULT CALLBACK Main_Window_Proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool _reset = false;

	if (_TheWWUIInput && Input::Is_Console_Enabled() == false) {
		LRESULT result = 0;

		
			if (_TheWWUIInput->ProcessMessage(hwnd, message, wParam, lParam, result)) {
				return result;
			}
		

	}

	switch (message)
	{
	case WM_ACTIVATEAPP:
		if (wParam && !GameInFocus) {
			WWDEBUG_SAY(("***** FOCUS GAINED *****\n"));
			GameInFocus = true;
			On_Focus_Restore();
		}
		else if (!wParam && GameInFocus) {
			WWDEBUG_SAY(("***** FOCUS LOST *****\n"));
			GameInFocus = false;
			On_Focus_Loss();
		}
		return(0);

	case WM_ERASEBKGND:
		return 1;

	case WM_PAINT:
		ValidateRect(hwnd, nullptr);
		break;

	case WM_SYSKEYDOWN:
		if (wParam == VK_RETURN && ((lParam >> 16) & KF_ALTDOWN) && !((lParam >> 16) & KF_REPEAT))
		{
			if (WIN_fullscreen) {
				WIN_fullscreen = false;
			}
			else {
				WIN_fullscreen = true;
			}
		}

		
			break;
		

	case WM_CHAR:
		Input::Console_Add_Key(int(wParam));
		break;

	case WM_CREATE:
		break;

	case WM_DESTROY:
		ReleaseCapture();
		PostQuitMessage(0);
		break;

	case WM_SYSCOMMAND:
		switch (wParam) {

			
		case SC_CLOSE:
			return (0);

		case SC_KEYMENU:
			return (0);

		case SC_SCREENSAVE:
			return (0);
		}
		break;
		

#if(0)
	case WM_KILLFOCUS:
		{
			if (WW3D::Is_Initted()) {

				
					int width = 0;
				int height = 0;
				int bits = 0;
				bool bwindowed = false;
				WW3D::Get_Device_Resolution(width, height, bits, bwindowed);

				if (bwindowed == false) {
					WW3D::Set_Device_Resolution(-1, -1, -1, true);
					::ShowWindow(hwnd, SW_MINIMIZE);
					_reset = true;
				}
			}
		}
		break;

	case WM_SETFOCUS:

		if (_reset == true) {
			WW3D::Set_Device_Resolution(-1, -1, -1, false);
			::ShowWindow(hwnd, SW_RESTORE);
			_reset = false;
		}
		break;
		

#endif

			
	case WM_COMMAND:
		if (LOWORD(wParam) == IDM_TOGGLE_FULLSCREEN) {
			WW3D::Toggle_Windowed();
		}

		break;

	default:
		break;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
	

}

/***********************************************************************************************

* Create_Main_Window -- Creates the main game window
  ***********************************************************************************************/
static BOOL Create_Main_Window(void)
{
	WNDCLASSA wc;
	BOOL rc;

	ProgramInstance = (HINSTANCE)GetModuleHandleA(nullptr);

	if (!ConsoleBox.Is_Exclusive()) {
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc = Main_Window_Proc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = ProgramInstance;
		wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = SingletonInstanceKeeperClass::Get_GUID();

		
			rc = RegisterClassA(&wc);
		if (!rc) return FALSE;

		MainWindow = CreateWindowExA(
			0,
			SingletonInstanceKeeperClass::Get_GUID(),
			"Renegade",
			WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
			0,
			0,
			0,
			0,
			nullptr,
			nullptr,
			ProgramInstance,
			nullptr);

		if (!MainWindow) {
			return FALSE;
		}

		SetFocus(MainWindow);
		

	}

	return TRUE;
}

void On_Focus_Loss(void)
{
	DirectInput::Unacquire();
}

void On_Focus_Restore(void)
{
#if WEBBROWSER_ENABLED
	if (WebBrowser::IsWebPageDisplayed() == false) {
#else
		{
#endif
			DirectInput::Acquire();
		}

		
			GameModeManager::Hide_Render_Frames(1);
		

	}

void Prog_End(void)
{
}

bool Set_Working_Directory(void)
{
	const char* gamedir = cUserOptions::GameDir.Get();
#if defined(OPENW3D_WIN32)
	char path_buffer[_MAX_PATH];
	if (gamedir == nullptr || gamedir[0] == '\0') {
		char path_to_exe[256];
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		GetModuleFileNameA(nullptr, path_to_exe, sizeof(path_to_exe));
		_splitpath(path_to_exe, drive, dir, nullptr, nullptr);
		_makepath(path_buffer, drive, dir, nullptr, nullptr);
		gamedir = path_buffer;
	}
	return cPathUtil::SetWorkingDirectory(gamedir);
#elif defined(OPENW3D_SDL3)
	if (gamedir == nullptr || gamedir[0] == '\0') {
		gamedir = SDL_GetBasePath();
	}
	return cPathUtil::SetWorkingDirectory(gamedir);
#else
	assert(0);
#endif
}
