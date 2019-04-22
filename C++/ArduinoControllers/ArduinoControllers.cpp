#include "stdafx.h"
#include <windows.h>
#include <thread>
#include <atlstr.h> 
#include "IniReader\IniReader.h"
#include <math.h>

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
	unsigned short	Buttons;
	float	Trigger;
	float	AxisX;
	float	AxisY;
} TController, *PController;

#define TOVR_SUCCESS 0
#define TOVR_FAILURE 1

DLLEXPORT DWORD __stdcall GetHMDData(__out THMD *HMD);
DLLEXPORT DWORD __stdcall GetControllersData(__out TController *FirstController, __out TController *SecondController);
DLLEXPORT DWORD __stdcall SetControllerData(__in int dwIndex, __in unsigned char MotorSpeed);

HANDLE hSerial, hSerial2;
bool Controller1Connected = false, Controller2Connected = false, ControllersInit = false;
std::thread *pCtrl1thread = NULL;
std::thread *pCtrl2thread = NULL;
float ArduinoCtrl1[7] = { 0, 0, 0, 0, 0, 0, 0 };
float ArduinoCtrl2[7] = { 0, 0, 0, 0, 0, 0, 0 };
float LastArduinoArduinoCtrl1[7] = { 0, 0, 0, 0, 0, 0, 0 };
float LastArduinoArduinoCtrl2[7] = { 0, 0, 0, 0, 0, 0, 0 };
float Ctrl1OffsetYPR[3] = { 0, 0, 0 }, Ctrl2OffsetYPR[3] = { 0, 0, 0 };
bool Ctrl1InitCentring = false, Ctrl2InitCentring = false;

void SetCentering()
{
	Ctrl1OffsetYPR[0] = ArduinoCtrl1[0];
	Ctrl1OffsetYPR[1] = ArduinoCtrl1[1];
	Ctrl1OffsetYPR[2] = ArduinoCtrl1[2];

	Ctrl2OffsetYPR[0] = ArduinoCtrl2[0];
	Ctrl2OffsetYPR[1] = ArduinoCtrl2[1];
	Ctrl2OffsetYPR[2] = ArduinoCtrl2[2];
}

bool CorrectAngleValue(float Value)
{
	if (Value > -180 && Value < 180)
	{
		return true;
	}
	else
	{
		return false;
	}
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

void Controller1Read()
{
	DWORD bytesRead;

	while (Controller1Connected) {
		ReadFile(hSerial, &ArduinoCtrl1, sizeof(ArduinoCtrl1), &bytesRead, 0);

		//Filter incorrect values
		if (CorrectAngleValue(ArduinoCtrl1[0]) == false || CorrectAngleValue(ArduinoCtrl1[1]) == false || CorrectAngleValue(ArduinoCtrl1[2]) == false)
		{
			//Last correct values
			ArduinoCtrl1[0] = LastArduinoArduinoCtrl1[0];
			ArduinoCtrl1[1] = LastArduinoArduinoCtrl1[1];
			ArduinoCtrl1[2] = LastArduinoArduinoCtrl1[2];
			ArduinoCtrl1[3] = LastArduinoArduinoCtrl1[3];
			ArduinoCtrl1[4] = LastArduinoArduinoCtrl1[4];
			ArduinoCtrl1[5] = LastArduinoArduinoCtrl1[5];
			ArduinoCtrl1[6] = LastArduinoArduinoCtrl1[6];

			PurgeComm(hSerial, PURGE_TXCLEAR | PURGE_RXCLEAR);
		}

		//Save last correct values
		if (CorrectAngleValue(ArduinoCtrl1[0]) && CorrectAngleValue(ArduinoCtrl1[1]) && CorrectAngleValue(ArduinoCtrl1[2]))
		{
			LastArduinoArduinoCtrl1[0] = ArduinoCtrl1[0];
			LastArduinoArduinoCtrl1[1] = ArduinoCtrl1[1];
			LastArduinoArduinoCtrl1[2] = ArduinoCtrl1[2];
			LastArduinoArduinoCtrl1[3] = ArduinoCtrl1[3];
			LastArduinoArduinoCtrl1[4] = ArduinoCtrl1[4];
			LastArduinoArduinoCtrl1[5] = ArduinoCtrl1[5];
			LastArduinoArduinoCtrl1[6] = ArduinoCtrl1[6];
		}

		if (Ctrl1InitCentring == false)
			if (ArduinoCtrl1[0] != 0 || ArduinoCtrl1[1] != 0 || ArduinoCtrl1[2] != 0) {
				SetCentering();
				Ctrl1InitCentring = true;
			}
	}
}

void Controller2Read()
{
	DWORD bytesRead;

	while (Controller2Connected) {
		ReadFile(hSerial2, &ArduinoCtrl2, sizeof(ArduinoCtrl2), &bytesRead, 0);

		//Filter incorrect values
		if (CorrectAngleValue(ArduinoCtrl2[0]) == false || CorrectAngleValue(ArduinoCtrl2[1]) == false || CorrectAngleValue(ArduinoCtrl2[2]) == false)
		{
			//Last correct values
			ArduinoCtrl2[0] = LastArduinoArduinoCtrl2[0];
			ArduinoCtrl2[1] = LastArduinoArduinoCtrl2[1];
			ArduinoCtrl2[2] = LastArduinoArduinoCtrl2[2];
			ArduinoCtrl2[3] = LastArduinoArduinoCtrl2[3];
			ArduinoCtrl2[4] = LastArduinoArduinoCtrl2[4];
			ArduinoCtrl2[5] = LastArduinoArduinoCtrl2[5];
			ArduinoCtrl2[6] = LastArduinoArduinoCtrl2[6];

			PurgeComm(hSerial2, PURGE_TXCLEAR | PURGE_RXCLEAR);
		}

		//Save last correct values
		if (CorrectAngleValue(ArduinoCtrl2[0]) && CorrectAngleValue(ArduinoCtrl2[1]) && CorrectAngleValue(ArduinoCtrl2[2]))
		{
			LastArduinoArduinoCtrl2[0] = ArduinoCtrl2[0];
			LastArduinoArduinoCtrl2[1] = ArduinoCtrl2[1];
			LastArduinoArduinoCtrl2[2] = ArduinoCtrl2[2];
			LastArduinoArduinoCtrl2[3] = ArduinoCtrl2[3];
			LastArduinoArduinoCtrl2[4] = ArduinoCtrl2[4];
			LastArduinoArduinoCtrl2[5] = ArduinoCtrl2[5];
			LastArduinoArduinoCtrl2[6] = ArduinoCtrl2[6];
		}

		if (Ctrl2InitCentring == false)
			if (ArduinoCtrl2[0] != 0 || ArduinoCtrl2[1] != 0 || ArduinoCtrl2[2] != 0) {
				SetCentering();
				Ctrl2InitCentring = true;
			}
	}
}

void ControllersStart() {
	CRegKey key;
	TCHAR _driversPath[MAX_PATH];
	LONG status = key.Open(HKEY_CURRENT_USER, _T("Software\\TrueOpenVR"));
	if (status == ERROR_SUCCESS)
	{
		ULONG regSize = sizeof(_driversPath);
		status = key.QueryStringValue(_T("Drivers"), _driversPath, &regSize);
	}
	key.Close();

	CString configPath(_driversPath);
	configPath.Format(_T("%sArduinoControllers.ini"), _driversPath);

	if (status == ERROR_SUCCESS && PathFileExists(configPath)) {

		CIniReader IniFile((char *)configPath.GetBuffer());
		//Controller 1
		CString sPortName;
		//sPortName.Format(_T("COM%d"), 3);
		sPortName.Format(_T("COM%d"), IniFile.ReadInteger("Controller1", "ComPort", 2));

		hSerial = ::CreateFile(sPortName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

		if (hSerial != INVALID_HANDLE_VALUE && GetLastError() != ERROR_FILE_NOT_FOUND) {

			DCB dcbSerialParams = { 0 };
			dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

			if (GetCommState(hSerial, &dcbSerialParams))
			{
				dcbSerialParams.BaudRate = CBR_115200;
				dcbSerialParams.ByteSize = 8;
				dcbSerialParams.StopBits = ONESTOPBIT;
				dcbSerialParams.Parity = NOPARITY;

				if (SetCommState(hSerial, &dcbSerialParams))
				{
					Controller1Connected = true;
					PurgeComm(hSerial, PURGE_TXCLEAR | PURGE_RXCLEAR);
					pCtrl1thread = new std::thread(Controller1Read);
				}
			}
		}

		//Controller 2
		sPortName.Format(_T("COM%d"), IniFile.ReadInteger("Controller2", "ComPort", 3));

		hSerial2 = ::CreateFile(sPortName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

		if (hSerial2 != INVALID_HANDLE_VALUE && GetLastError() != ERROR_FILE_NOT_FOUND) {

			DCB dcbSerialParams = { 0 };
			dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

			if (GetCommState(hSerial2, &dcbSerialParams))
			{
				dcbSerialParams.BaudRate = CBR_115200;
				dcbSerialParams.ByteSize = 8;
				dcbSerialParams.StopBits = ONESTOPBIT;
				dcbSerialParams.Parity = NOPARITY;

				if (SetCommState(hSerial2, &dcbSerialParams))
				{
					Controller2Connected = true;
					PurgeComm(hSerial2, PURGE_TXCLEAR | PURGE_RXCLEAR);
					pCtrl2thread = new std::thread(Controller2Read);
				}
			}
		}


	}
}


BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_DETACH:
		if (Controller1Connected) {
			Controller1Connected = false;
			if (pCtrl1thread) {
				pCtrl1thread->join();
				delete pCtrl1thread;
				pCtrl1thread = nullptr;
			}
			CloseHandle(hSerial);
		}
		if (Controller2Connected) {
			Controller2Connected = false;
			if (pCtrl2thread) {
				pCtrl2thread->join();
				delete pCtrl2thread;
				pCtrl2thread = nullptr;
			}
			CloseHandle(hSerial2);
		}
		break;

	}
	return true;
}

DLLEXPORT DWORD __stdcall GetHMDData(__out THMD *HMD)
{
	HMD->X = 0;
	HMD->Y = 0;
	HMD->Z = 0;

	HMD->Yaw = 0;
	HMD->Pitch = 0;
	HMD->Roll = 0;

	return TOVR_FAILURE;
}

DLLEXPORT DWORD __stdcall GetControllersData(__out TController *FirstController, __out TController *SecondController)
{
	if (ControllersInit == false) {
		ControllersInit = true;
		ControllersStart();
	}
	//Controller 1
	FirstController->X = -0.1;
	FirstController->Y = -0.05;
	FirstController->Z = -0.25;

	if (Controller1Connected) {
		FirstController->Yaw = OffsetYPR(ArduinoCtrl1[0], Ctrl1OffsetYPR[0]);
		FirstController->Pitch = OffsetYPR(ArduinoCtrl1[1], Ctrl1OffsetYPR[1]);
		FirstController->Roll = OffsetYPR(ArduinoCtrl1[2], Ctrl1OffsetYPR[2]);

		FirstController->Buttons = round(ArduinoCtrl1[4]);
		FirstController->Trigger = ArduinoCtrl1[3];
		FirstController->AxisX = ArduinoCtrl1[5];
		FirstController->AxisY = ArduinoCtrl1[6];
	}
	else 
	{
		FirstController->Yaw = 0;
		FirstController->Pitch = 0;
		FirstController->Roll = 0;

		FirstController->Buttons = 0;
		FirstController->Trigger = 0;
		FirstController->AxisX = 0;
		FirstController->AxisY = 0;
	}

	//Controller 2
	SecondController->X = 0.1;
	SecondController->Y = -0.05;
	SecondController->Z = -0.25;

	if (Controller2Connected) {
		SecondController->Yaw = OffsetYPR(ArduinoCtrl2[0], Ctrl2OffsetYPR[0]);
		SecondController->Pitch = OffsetYPR(ArduinoCtrl2[1], Ctrl2OffsetYPR[1]);
		SecondController->Roll = OffsetYPR(ArduinoCtrl2[2], Ctrl2OffsetYPR[2]);

		SecondController->Buttons = round(ArduinoCtrl2[4]);
		SecondController->Trigger = ArduinoCtrl2[3];
		SecondController->AxisX = ArduinoCtrl2[5];
		SecondController->AxisY = ArduinoCtrl2[6];
	}
	else {
		SecondController->Yaw = 0;
		SecondController->Pitch = 0;
		SecondController->Roll = 0;

		SecondController->Buttons = 0;
		SecondController->Trigger = 0;
		SecondController->AxisX = 0;
		SecondController->AxisY = 0;
	}

	if (Controller1Connected || Controller2Connected) {
		return TOVR_SUCCESS;
	}
	else {
		return TOVR_FAILURE;
	}
}

DLLEXPORT DWORD __stdcall SetControllerData(__in int dwIndex, __in unsigned char MotorSpeed)
{
	//Soon
	return TOVR_FAILURE;
}

