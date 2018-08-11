#include "stdafx.h"
#include <windows.h>
#include <thread>
#include "PSMoveService/PSMoveClient_CAPI.h"

#define DLLEXPORT extern "C" __declspec(dllexport)

typedef struct _HMDData
{
	double	X;
	double	Y;
	double	Z;
	double	Yaw;
	double	Pitch;
	double	Roll;
} THMD, *PHMD;

typedef struct _Controller
{
	double	X;
	double	Y;
	double	Z;
	double	Yaw;
	double	Pitch;
	double	Roll;
	WORD	Buttons;
	BYTE	Trigger;
	SHORT	ThumbX;
	SHORT	ThumbY;
} TController, *PController;

DLLEXPORT DWORD __stdcall GetHMDData(__out THMD *myHMD);
DLLEXPORT DWORD __stdcall GetControllersData(__out TController *MyController, __out TController *MyController2);
DLLEXPORT DWORD __stdcall SetControllerData(__in int dwIndex, __in WORD	MotorSpeed);
DLLEXPORT DWORD __stdcall SetCentering(__in int dwIndex);

PSMControllerList controllerList;
PSMHmdList hmdList;
PSMVector3f hmdPos;
PSMVector3f ctrl1Pos, ctrl2Pos;
float s = 0;

static const float k_fScalePSMoveAPIToMeters = 0.01f; // psmove driver in cm

bool PSMConnected = false, PSMoveServiceInit = false;
std::thread *pPSMUpdatethread = NULL;

void PSMoveServiceUpdate()
{
	s = 2;
	while (PSMConnected) {
		PSM_Update();

		if (hmdList.count > 0)
			PSM_GetHmdPosition(hmdList.hmd_id[0], &hmdPos);

		if (controllerList.count > 0)
			PSM_GetControllerPosition(controllerList.controller_id[0], &ctrl1Pos);
			
		if (controllerList.count > 1)
			PSM_GetControllerPosition(controllerList.controller_id[1], &ctrl2Pos);
	}
}

void ConnectToPSMoveService()
{
	if (PSM_Initialize(PSMOVESERVICE_DEFAULT_ADDRESS, PSMOVESERVICE_DEFAULT_PORT, PSM_DEFAULT_TIMEOUT) == PSMResult_Success)
	{
		memset(&controllerList, 0, sizeof(PSMControllerList));
		PSM_GetControllerList(&controllerList, PSM_DEFAULT_TIMEOUT);

		memset(&hmdList, 0, sizeof(PSMHmdList));
		PSM_GetHmdList(&hmdList, PSM_DEFAULT_TIMEOUT);

		s = 1;
	}

	unsigned int data_stream_flags =
		PSMControllerDataStreamFlags::PSMStreamFlags_includePositionData |
		PSMControllerDataStreamFlags::PSMStreamFlags_includePhysicsData |
		PSMControllerDataStreamFlags::PSMStreamFlags_includeCalibratedSensorData |
		PSMControllerDataStreamFlags::PSMStreamFlags_includeRawTrackerData;

	//HMD
	if (hmdList.count > 0) 
		if (PSM_AllocateHmdListener(hmdList.hmd_id[0]) == PSMResult_Success && PSM_StartHmdDataStream(hmdList.hmd_id[0], data_stream_flags, PSM_DEFAULT_TIMEOUT) == PSMResult_Success)
			PSMConnected = true;
	
	//Controller1
	if (controllerList.count > 0) 
		if (PSM_AllocateControllerListener(controllerList.controller_id[0]) == PSMResult_Success && PSM_StartControllerDataStream(controllerList.controller_id[0], data_stream_flags, PSM_DEFAULT_TIMEOUT) == PSMResult_Success)
			PSMConnected = true;
	
	//Controller2
	if (controllerList.count > 1)
		if (PSM_AllocateControllerListener(controllerList.controller_id[1]) == PSMResult_Success && PSM_StartControllerDataStream(controllerList.controller_id[1], data_stream_flags, PSM_DEFAULT_TIMEOUT) == PSMResult_Success)
			PSMConnected = true;

	if (PSMConnected)
		pPSMUpdatethread = new std::thread(PSMoveServiceUpdate);
}

DLLEXPORT DWORD __stdcall GetHMDData(__out THMD *myHMD)
{
	if (PSMoveServiceInit == false) {
		PSMoveServiceInit = true;
		ConnectToPSMoveService();
	}

	if (PSMConnected && hmdList.count > 0) {
		myHMD->X = hmdPos.x * k_fScalePSMoveAPIToMeters;
		myHMD->Y = hmdPos.y * k_fScalePSMoveAPIToMeters;
		myHMD->Z = hmdPos.z * k_fScalePSMoveAPIToMeters;
		myHMD->Yaw = 0; //????? splitter?
		myHMD->Pitch = 0;
		myHMD->Roll = 0;

		return 1;
	}
	else {
		myHMD->X = s;
		myHMD->Y = 0;
		myHMD->Z = 0;
		myHMD->Yaw = 0;
		myHMD->Pitch = 0;
		myHMD->Roll = 0;

		return 1;
	}
}

DLLEXPORT DWORD __stdcall GetControllersData(__out TController *myController, __out TController *myController2)
{
	if (PSMoveServiceInit == false) {
		PSMoveServiceInit = true;
		ConnectToPSMoveService();
	}

	//Controller 1

	if (PSMConnected && controllerList.count > 0) {
		myController->X = ctrl1Pos.x * k_fScalePSMoveAPIToMeters;
		myController->Y = ctrl1Pos.y * k_fScalePSMoveAPIToMeters;
		myController->Z = ctrl1Pos.z * k_fScalePSMoveAPIToMeters;
	}
	else {
		myController->X = 0;
		myController->Y = 0;
		myController->Z = 0;
	}
	

	myController->Yaw = 0;
	myController->Pitch = 0;
	myController->Roll = 0;

	myController->Buttons = 0;
	myController->Trigger = 0;
	myController->ThumbX = 0;
	myController->ThumbY = 0;

	//Controller 2
	if (PSMConnected && controllerList.count > 1) {
		myController2->X = ctrl2Pos.x * k_fScalePSMoveAPIToMeters;
		myController2->Y = ctrl2Pos.y * k_fScalePSMoveAPIToMeters;
		myController2->Z = ctrl2Pos.z * k_fScalePSMoveAPIToMeters;
	}
	else {
		myController2->X = 0;
		myController2->Y = 0;
		myController2->Z = 0;
	}
	

	myController2->Yaw = 0;
	myController2->Pitch = 0;
	myController2->Roll = 0;

	myController2->Buttons = 0;
	myController2->Trigger = 0;
	myController2->ThumbX = 0;
	myController2->ThumbY = 0;

	if (PSMConnected) {
		return 1;
	} 
	else 
	{
		return 0;
	}
}

DLLEXPORT DWORD __stdcall SetControllerData(__in int dwIndex, __in WORD	MotorSpeed)
{
	return 0;
}

DLLEXPORT DWORD __stdcall SetCentering(__in int dwIndex)
{
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_DETACH:
		if (PSMConnected) {
			PSMConnected = false;
			if (pPSMUpdatethread) {
				pPSMUpdatethread->join();
				delete pPSMUpdatethread;
				pPSMUpdatethread = nullptr;
			}

			if (hmdList.count > 0)
			{
				PSM_StopHmdDataStream(hmdList.hmd_id[0], PSM_DEFAULT_TIMEOUT);
				PSM_FreeHmdListener(hmdList.hmd_id[0]);
			}

			if (controllerList.count > 0)
			{
				PSM_StopControllerDataStream(controllerList.controller_id[0], PSM_DEFAULT_TIMEOUT);
				PSM_FreeControllerListener(controllerList.controller_id[0]);
			}

			if (controllerList.count > 1)
			{
				PSM_StopControllerDataStream(controllerList.controller_id[1], PSM_DEFAULT_TIMEOUT);
				PSM_FreeControllerListener(controllerList.controller_id[1]);
			}

			PSM_Shutdown();
		}
		break;
	}
	return TRUE;
}