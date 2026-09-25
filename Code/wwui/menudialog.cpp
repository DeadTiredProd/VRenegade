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
**	MERCHANTABILLITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Combat                                                       *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwui/menudialog.cpp          $*
 *                                                                                             *
 *                       Author:: Patrick Smith                                                *
 *                                                                                             *
 *                     $Modtime:: 1/02/02 11:10a                                              $*
 *                                                                                             *
 *                    $Revision:: 11                                                          $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "menudialog.h"
#include "menubackdrop.h"
#include "render2d.h"
#include "stylemgr.h"
#include "dialogmgr.h"
#include "childdialog.h"
#include "dialogcontrol.h"
#include "ww3d.h"
#include "camera.h"
#include "vrmanager.h"


 ////////////////////////////////////////////////////////////////
 //	Static member initialization
 ////////////////////////////////////////////////////////////////
MenuDialogClass* MenuDialogClass::ActiveMenu = nullptr;
MenuBackDropClass* MenuDialogClass::BackDrop = nullptr;
DynamicVectorClass<MenuDialogClass*> MenuDialogClass::MenuStack;


////////////////////////////////////////////////////////////////
//
//	MenuDialogClass
//
////////////////////////////////////////////////////////////////
MenuDialogClass::MenuDialogClass(const DialogResource* dialog_resource) :
	DialogBaseClass(dialog_resource)
{
	MenuStack.Add(this);
	return;
}


////////////////////////////////////////////////////////////////
//
//	~MenuDialogClass
//
////////////////////////////////////////////////////////////////
MenuDialogClass::~MenuDialogClass(void)
{
	if (ActiveMenu == this) {
		ActiveMenu = nullptr;
	}

	int index = MenuStack.ID(this);
	if (index != -1) {
		MenuStack.Delete(index);
	}

	return;
}


////////////////////////////////////////////////////////////////
//
//	Initialize
//
////////////////////////////////////////////////////////////////
void
MenuDialogClass::Initialize(void)
{
	Ensure_BackDrop();
	return;
}


////////////////////////////////////////////////////////////////
//
//	Shutdown
//
////////////////////////////////////////////////////////////////
void
MenuDialogClass::Shutdown(void)
{
	if (BackDrop != nullptr) {
		delete BackDrop;
		BackDrop = nullptr;
	}

	return;
}


////////////////////////////////////////////////////////////////
//
//	Ensure_BackDrop
//
////////////////////////////////////////////////////////////////
void
MenuDialogClass::Ensure_BackDrop(void)
{
	if (BackDrop == nullptr && WW3D::Is_Initted()) {
		BackDrop = new MenuBackDropClass;
	}
}


////////////////////////////////////////////////////////////////
//
//	Render
//
////////////////////////////////////////////////////////////////
void
MenuDialogClass::Render(void)
{
	if (ActiveMenu == this || DialogMgrClass::Peek_Transitioning_Dialog() == this) {

		Ensure_BackDrop();

		if (BackDrop != nullptr) {

			CameraClass* menu_camera = BackDrop->Peek_Camera();

			if (VRManager::IsInitialized() &&
				VRManager::IsStereoRendering() &&
				menu_camera != nullptr) {

				Matrix3D original_camera_transform =
					menu_camera->Get_Transform();

				Vector2 original_view_min;
				Vector2 original_view_max;

				menu_camera->Get_View_Plane(
					original_view_min,
					original_view_max
				);

				Vector2 original_viewport_min;
				Vector2 original_viewport_max;

				menu_camera->Get_Viewport(
					original_viewport_min,
					original_viewport_max
				);

				Matrix3D menu_rotation_correction(1);

				menu_rotation_correction[1][1] = 0.0f;
				menu_rotation_correction[1][2] = 1.0f;
				menu_rotation_correction[2][1] = -1.0f;
				menu_rotation_correction[2][2] = 0.0f;

				Matrix3D corrected_menu_transform;

				Matrix3D::Multiply(
					original_camera_transform,
					menu_rotation_correction,
					&corrected_menu_transform
				);

				bool stereo_success = true;

				for (int eye_index = 0;
					eye_index < 2;
					++eye_index) {

					vr::EVREye eye =
						(eye_index == 0)
						? vr::Eye_Left
						: vr::Eye_Right;

					if (!VRManager::BeginEye(
						eye,
						*menu_camera,
						corrected_menu_transform
					)) {
						stereo_success = false;
						break;
					}

					BackDrop->Render();

					DialogBaseClass::Render();

					VRManager::EndEye(eye);

					menu_camera->Set_Transform(
						original_camera_transform
					);

					menu_camera->Set_View_Plane(
						original_view_min,
						original_view_max
					);

					menu_camera->Set_Viewport(
						original_viewport_min,
						original_viewport_max
					);

					menu_camera->Apply();
				}

				if (!stereo_success) {

					menu_camera->Set_Transform(
						original_camera_transform
					);

					menu_camera->Set_View_Plane(
						original_view_min,
						original_view_max
					);

					menu_camera->Set_Viewport(
						original_viewport_min,
						original_viewport_max
					);

					menu_camera->Apply();

					BackDrop->Render();

					DialogBaseClass::Render();
				}

				menu_camera->Set_Transform(
					original_camera_transform
				);

				menu_camera->Set_View_Plane(
					original_view_min,
					original_view_max
				);

				menu_camera->Set_Viewport(
					original_viewport_min,
					original_viewport_max
				);

				menu_camera->Apply();

			}
			else {

				BackDrop->Render();

				DialogBaseClass::Render();
			}
		}
		else {
			DialogBaseClass::Render();
		}
	}

	return;
}


////////////////////////////////////////////////////////////////
//
//	On_Init_Dialog
//
////////////////////////////////////////////////////////////////
/*void
MenuDialogClass::On_Init_Dialog(void)
{
	DialogBaseClass::Set_Default_Focus();
	return;
}*/


////////////////////////////////////////////////////////////////
//
//	Start_Dialog
//
////////////////////////////////////////////////////////////////
void
MenuDialogClass::Start_Dialog(void)
{
	Rect = Render2DClass::Get_Screen_Resolution();

	DialogBaseClass::Start_Dialog();
	return;
}


////////////////////////////////////////////////////////////////
//
//	On_Activate
//
////////////////////////////////////////////////////////////////
void
MenuDialogClass::On_Activate(bool onoff)
{
	if (onoff) {

		if (ActiveMenu != nullptr) {
			ActiveMenu->On_Menu_Activate(false);
		}

		ActiveMenu = this;
		On_Menu_Activate(true);
	}

	DialogBaseClass::On_Activate(onoff);
	return;
}


////////////////////////////////////////////////////////////////
//
//	On_Menu_Activate
//
////////////////////////////////////////////////////////////////
void
MenuDialogClass::On_Menu_Activate(bool /* onoff */)
{
	return;
}


////////////////////////////////////////////////////////////////
//
//	End_Dialog
//
////////////////////////////////////////////////////////////////
void
MenuDialogClass::End_Dialog(void)
{
	if (DialogMgrClass::Is_Flushing_Dialogs() == false) {

		if (MenuStack.Count() == 1) {
			On_Last_Menu_Ending();
		}
		else {

			StyleMgrClass::Play_Sound(
				StyleMgrClass::EVENT_MENU_BACK
			);
		}
	}

	DialogBaseClass::End_Dialog();
	return;
}


////////////////////////////////////////////////////////////////
//
//	Replace_BackDrop
//
////////////////////////////////////////////////////////////////
MenuBackDropClass*
MenuDialogClass::Replace_BackDrop(MenuBackDropClass* backdrop)
{
	MenuBackDropClass* retval = BackDrop;
	BackDrop = backdrop;
	return retval;
}