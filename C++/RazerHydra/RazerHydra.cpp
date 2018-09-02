#include "stdafx.h"
#include <Windows.h>
#include <Math.h>

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

#define TOVR_SUCCESS 1
#define TOVR_FAILURE 0

#define GRIPBTN 0x0001
#define THUMBSTICKBTN 0x0002
#define MENUBTN 0x0004
#define SYSTEMBTN 0x0008

#define SIXENSE_BUTTON_BUMPER   (0x01<<7)
#define SIXENSE_BUTTON_JOYSTICK (0x01<<8)
#define SIXENSE_BUTTON_1        (0x01<<5)
#define SIXENSE_BUTTON_2        (0x01<<6)
#define SIXENSE_BUTTON_3        (0x01<<3)
#define SIXENSE_BUTTON_4        (0x01<<4)
#define SIXENSE_BUTTON_START    (0x01<<0)

#define SIXENSE_SUCCESS 0
#define SIXENSE_FAILURE -1

typedef struct _sixenseControllerData {
	float pos[3];
	float rot_mat[3][3];
	float joystick_x;
	float joystick_y;
	float trigger;
	unsigned int buttons;
	unsigned char sequence_number;
	float rot_quat[4];
	unsigned short firmware_revision;
	unsigned short hardware_revision;
	unsigned short packet_type;
	unsigned short magnetic_frequency;
	int enabled;
	int controller_index;
	unsigned char is_docked;
	unsigned char which_hand;
	unsigned char hemi_tracking_enabled;
} sixenseControllerData;

typedef int(__cdecl *_sixenseInit)(void);
typedef int(__cdecl *_sixenseGetData)(int which, int index_back, sixenseControllerData *);
typedef int(__cdecl *_sixenseExit)(void);

_sixenseInit sixenseInit;
_sixenseGetData sixenseGetData;
_sixenseExit sixenseExit;

HMODULE hDll;
bool HydraInit = false, HydraConnected = false;

float Ctrl1YPR[3], Ctrl2YPR[3];//yaw, pitch, roll
float Ctrl1YPROffset[3], Ctrl2YPROffset[3]; 

//double HydraYawOffset = 0; 
double HydraPosYOffset = 0;

double RadToDeg(double r) {
	return r * (180 / 3.14159265358979323846); //180 / PI
}

double OffsetYPR(float f, float f2)
{
	f -= f2;
	if (f < -180) {
		f += 360;
	}
	else if (f > 180) {
		f -= 360;
	}

	return f;
}

void HydraStart()
{
	#ifdef _WIN64
		hDll = LoadLibrary("C:\\Program Files\\Sixence\\bin\\win64\\sixense_x64.dll");
	#else
		hDll = LoadLibrary("C:\\Program Files\\Sixence\\bin\\win32\\sixense.dll");
	#endif

		if (hDll != NULL) {

			sixenseInit = (_sixenseInit)GetProcAddress(hDll, "sixenseInit");
			sixenseGetData = (_sixenseGetData)GetProcAddress(hDll, "sixenseGetData");
			sixenseExit = (_sixenseExit)GetProcAddress(hDll, "sixenseExit");

			if (sixenseInit != NULL && sixenseGetData != NULL && sixenseExit != NULL)
			{
				sixenseInit();
				HydraConnected = true;
			}
			else
			{
				hDll = NULL;
			}
			
	}
}

DLLEXPORT DWORD __stdcall GetHMDData(__out THMD *myHMD)
{
	
	myHMD->X = 0;
	//myHMD->Y = HydraPosYOffset;
	myHMD->Y = 0;
	myHMD->Z = 0;

	//Load library with rotation for HMD
	myHMD->Yaw = 0;
	myHMD->Pitch = 0;
	//myHMD->Pitch = OffsetYPR(0, HydraYawOffset);
	myHMD->Roll = 0;

	if (HydraConnected) {
		return TOVR_SUCCESS;
	}
	else {
		return TOVR_FAILURE;
	}
}

sixenseControllerData HydraController;
double SinR, CosR, SinP, SinY, CosY;
bool ctrlsInitCentring = false;

DLLEXPORT DWORD __stdcall GetControllersData(__out TController *myController, __out TController *myController2)
{
	if (HydraInit == false)
	{
		HydraInit = true;
		HydraStart();
	}

	//Controller 1
	myController->X = 0;
	myController->Y = 0;
	myController->Z = 0;

	myController->Yaw = 0;
	myController->Pitch = 0;
	myController->Roll = 0;

	myController->Buttons = 0;
	myController->Trigger = 0;
	myController->ThumbX = 0;
	myController->ThumbY = 0;

	//Controller 2
	myController2->X = 0;
	myController2->Y = 0;
	myController2->Z = 0;

	myController2->Yaw = 0;
	myController2->Pitch = 0;
	myController2->Roll = 0;

	myController2->Buttons = 0;
	myController2->Trigger = 0;
	myController2->ThumbX = 0;
	myController2->ThumbY = 0;

	if (HydraConnected) {
		
		
		//Controller1
		sixenseGetData(1, 0, &HydraController);

		if (HydraController.buttons & SIXENSE_BUTTON_3)
			HydraPosYOffset += 0.0033;

		if (HydraController.buttons & SIXENSE_BUTTON_1)
			HydraPosYOffset -= 0.0033;

		if (((HydraController.buttons & SIXENSE_BUTTON_1) || (HydraController.buttons & SIXENSE_BUTTON_3)) && (HydraController.buttons & SIXENSE_BUTTON_BUMPER))
			HydraPosYOffset = 0;

		myController->X = HydraController.pos[0] * 0.001;
		myController->Y = HydraController.pos[1] * 0.001 + HydraPosYOffset;
		myController->Z = HydraController.pos[2] * 0.001 - 0.45;

		//Convert quaternion to yaw, pitch, roll
		//Roll
		SinR = 2.0 * (HydraController.rot_quat[0] * HydraController.rot_quat[1] + HydraController.rot_quat[2] * HydraController.rot_quat[3]);
		CosR = 1.0 - 2.0 * (HydraController.rot_quat[1] * HydraController.rot_quat[1] + HydraController.rot_quat[2] * HydraController.rot_quat[2]);
		myController->Yaw = RadToDeg(atan2(SinR, CosR));
		//Pitch
		SinP = 2.0 * (HydraController.rot_quat[0] * HydraController.rot_quat[2] - HydraController.rot_quat[3] * HydraController.rot_quat[1]);
		if (fabs(SinP) >= 1)
			myController->Pitch = RadToDeg(copysign(3.14159265358979323846 / 2, SinP)); // use 90 degrees if out of range
		else
			myController->Pitch = RadToDeg(asin(SinP));
		//Yaw
		SinY = 2.0 * (HydraController.rot_quat[0] * HydraController.rot_quat[3] + HydraController.rot_quat[1] * HydraController.rot_quat[2]);
		CosY = 1.0 - 2.0 * (HydraController.rot_quat[2] * HydraController.rot_quat[2] + HydraController.rot_quat[3] * HydraController.rot_quat[3]);
		myController->Roll = RadToDeg(atan2(SinY, CosY));
		//end convert

		//For centring
		Ctrl1YPR[0] = myController->Yaw;
		Ctrl1YPR[1] = myController->Pitch;
		Ctrl1YPR[2] = myController->Roll;

		//Offset YPR
		myController->Yaw = OffsetYPR(myController->Yaw, Ctrl1YPROffset[0]);
		myController->Pitch = OffsetYPR(myController->Pitch, Ctrl1YPROffset[1]) * -1; //HydraYawOffset 
		myController->Roll = OffsetYPR(myController->Roll, Ctrl1YPROffset[2]) * -1;

		//Buttons
		if (HydraController.buttons & SIXENSE_BUTTON_START)
			myController->Buttons += SYSTEMBTN;
		if (HydraController.buttons & SIXENSE_BUTTON_JOYSTICK)
			myController->Buttons += THUMBSTICKBTN;
		if (HydraController.buttons & SIXENSE_BUTTON_4)
			myController->Buttons += MENUBTN;
		if (HydraController.buttons & SIXENSE_BUTTON_BUMPER)
			myController->Buttons += GRIPBTN;

		//Trigger
		myController->Trigger = round(HydraController.trigger * 255);

		//Stick
		myController->ThumbX = round(HydraController.joystick_x * 32767);
		myController->ThumbY = round(HydraController.joystick_y * 32767);
		//end controller 1


		//Controller 2
		sixenseGetData(0, 1, &HydraController);
		myController2->X = HydraController.pos[0] * 0.001;
		myController2->Y = HydraController.pos[1] * 0.001 + HydraPosYOffset;
		myController2->Z = HydraController.pos[2] * 0.001 - 0.45;

		//Convert quaternion to yaw, pitch, roll
		//Roll
		SinR = 2.0 * (HydraController.rot_quat[0] * HydraController.rot_quat[1] + HydraController.rot_quat[2] * HydraController.rot_quat[3]);
		CosR = 1.0 - 2.0 * (HydraController.rot_quat[1] * HydraController.rot_quat[1] + HydraController.rot_quat[2] * HydraController.rot_quat[2]);
		myController2->Yaw = RadToDeg(atan2(SinR, CosR));
		//Pitch
		SinP = 2.0 * (HydraController.rot_quat[0] * HydraController.rot_quat[2] - HydraController.rot_quat[3] * HydraController.rot_quat[1]);
		if (fabs(SinP) >= 1)
			myController2->Pitch = RadToDeg(copysign(3.14159265358979323846 / 2, SinP)); // use 90 degrees if out of range
		else
			myController2->Pitch = RadToDeg(asin(SinP));
		//Yaw
		SinY = 2.0 * (HydraController.rot_quat[0] * HydraController.rot_quat[3] + HydraController.rot_quat[1] * HydraController.rot_quat[2]);
		CosY = 1.0 - 2.0 * (HydraController.rot_quat[2] * HydraController.rot_quat[2] + HydraController.rot_quat[3] * HydraController.rot_quat[3]);
		myController2->Roll = RadToDeg(atan2(SinY, CosY));
		//end convert
		
		//For centring
		Ctrl2YPR[0] = myController2->Yaw;
		Ctrl2YPR[1] = myController2->Pitch;
		Ctrl2YPR[2] = myController2->Roll;

		//Offset YPR
		myController2->Yaw = OffsetYPR(myController2->Yaw, Ctrl2YPROffset[0]);
		myController2->Pitch = OffsetYPR(myController2->Pitch, Ctrl2YPROffset[1]) * -1; // HydraYawOffset
		myController2->Roll = OffsetYPR(myController2->Roll, Ctrl2YPROffset[2]) * -1;

		//Buttons
		if (HydraController.buttons & SIXENSE_BUTTON_START)
			myController2->Buttons += SYSTEMBTN;
		if (HydraController.buttons & SIXENSE_BUTTON_JOYSTICK)
			myController2->Buttons += THUMBSTICKBTN;
		if (HydraController.buttons & SIXENSE_BUTTON_4)
			myController2->Buttons += MENUBTN;
		if (HydraController.buttons & SIXENSE_BUTTON_BUMPER)
			myController2->Buttons += GRIPBTN;

		/*if (HydraController.buttons & SIXENSE_BUTTON_1)
			if (HydraYawOffset >= -177)
				HydraYawOffset -= 3;

		if (HydraController.buttons & SIXENSE_BUTTON_2)
			if (HydraYawOffset < 177)
				HydraYawOffset += 3;


		if (((HydraController.buttons & SIXENSE_BUTTON_1) || (HydraController.buttons & SIXENSE_BUTTON_2)) && (HydraController.buttons & SIXENSE_BUTTON_BUMPER))
			HydraYawOffset = 0;*/

		//Trigger
		myController2->Trigger = round(HydraController.trigger * 255);

		//Stick
		myController2->ThumbX = round(HydraController.joystick_x * 32767);
		myController2->ThumbY = round(HydraController.joystick_y * 32767);
		//end controller 2

		//Centring on start
		if (ctrlsInitCentring == false)
			if ((myController->Yaw != 0) || (myController->Pitch != 0) || (myController->Roll != 0)) {
				SetCentering(1);
				SetCentering(2);
				ctrlsInitCentring = true;
		}
		
		return TOVR_SUCCESS;
	}
	else {
		return TOVR_FAILURE;
	}
}

DLLEXPORT DWORD __stdcall SetControllerData(__in int dwIndex, __in WORD	MotorSpeed)
{
	return 0;
}

DLLEXPORT DWORD __stdcall SetCentering(__in int dwIndex)
{
	if (HydraConnected) {
		if (dwIndex == 1) {
			Ctrl1YPROffset[0] = Ctrl1YPR[0];
			Ctrl1YPROffset[1] = Ctrl1YPR[1];
			Ctrl1YPROffset[2] = Ctrl1YPR[2];
		}

		if (dwIndex == 2) {
			Ctrl2YPROffset[0] = Ctrl2YPR[0];
			Ctrl2YPROffset[1] = Ctrl2YPR[1];
			Ctrl2YPROffset[2] = Ctrl2YPR[2];
		}

		return TOVR_SUCCESS;
	} else {
		return TOVR_FAILURE;
	}
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	//case DLL_PROCESS_ATTACH:
	//case DLL_THREAD_ATTACH:
	//case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		if (hDll != NULL) {
			sixenseExit();
			FreeLibrary(hDll);
			hDll = nullptr;
		}
		break;
	}
	return TRUE;
}

