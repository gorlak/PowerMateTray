#include "stdafx.h"

#include <stdio.h>
#include <malloc.h>
#include <string>

#include <objbase.h>
#include <windows.h>
#include <process.h>

#include <setupapi.h>
#pragma comment(lib, "setupapi")

#include <hidsdi.h>
#pragma comment(lib, "hid")

#include "Volume.h"

static HANDLE OpenUSBDevice(const GUID* interfaceGUID)
{
	HANDLE hResult = INVALID_HANDLE_VALUE;

	HDEVINFO hDevInfo = SetupDiGetClassDevs(interfaceGUID, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		return hResult;
	}

	SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
	ZeroMemory(&deviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
	deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	for (DWORD devInterfaceIndex = 0; SetupDiEnumDeviceInterfaces(hDevInfo, NULL, interfaceGUID, devInterfaceIndex, &deviceInterfaceData); devInterfaceIndex++)
	{
		DWORD size = 0;
		if (!SetupDiGetDeviceInterfaceDetail(hDevInfo, &deviceInterfaceData, NULL, 0, &size, 0))
		{
			int err = GetLastError();
			if (err == ERROR_NO_MORE_ITEMS)
			{
				break;
			}

			PSP_DEVICE_INTERFACE_DETAIL_DATA pInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(size);
			if (pInterfaceDetailData)
			{
				ZeroMemory(pInterfaceDetailData, sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA));
				pInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

				SP_DEVINFO_DATA devInfoData;
				ZeroMemory(&devInfoData, sizeof(SP_DEVINFO_DATA));
				devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

				if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &deviceInterfaceData, pInterfaceDetailData, size, &size, &devInfoData))
				{
					if (NULL != _tcsstr((TCHAR*)pInterfaceDetailData->DevicePath, _T("vid_077d&pid_0410")))
					{
						OutputDebugFormat(_T("Found PowerMate USB: %s\n"), pInterfaceDetailData->DevicePath);
						hResult = CreateFile(
							pInterfaceDetailData->DevicePath,
							GENERIC_READ|GENERIC_WRITE,
							FILE_SHARE_READ|FILE_SHARE_WRITE,
							NULL,
							OPEN_EXISTING,
							0,
							NULL);
					}
				}

				free(pInterfaceDetailData);

				if (hResult != INVALID_HANDLE_VALUE)
				{
					break;
				}
			}
		}
	}

	SetupDiDestroyDeviceInfoList(hDevInfo);
	return hResult;
}

bool bExitThread = false;

static void ServiceHIDInputReports(void* hUSBDevice)
{
	PHIDP_PREPARSED_DATA pPreparsedData;
	HidD_GetPreparsedData(hUSBDevice, &pPreparsedData);

	HIDP_CAPS caps;
	ZeroMemory(&caps, sizeof(caps));
	HidP_GetCaps(pPreparsedData, &caps);

	struct PowerMateInput
	{
		char pad;
		char button;
		char dial;
	};
	PowerMateInput previous;
	ZeroMemory(&previous, sizeof(PowerMateInput));

	char* inputReport = (char*)alloca(caps.InputReportByteLength);
	while (!bExitThread)
	{
		DWORD bytesRead = 0;
		if (ReadFile(hUSBDevice, inputReport, caps.InputReportByteLength, &bytesRead, nullptr))
		{
			PowerMateInput& current (*(PowerMateInput*)inputReport);
			if (current.button != previous.button)
			{
				OutputDebugFormat("Input Report obtained Knob Press\n");
				ToggleMute();
			}
			else
			{
				if (current.dial == (char)0x01)
				{
					OutputDebugFormat("Input Report obtained Knob Right\n");
					IncreaseVolume();
				}
				else if (current.dial == (char)0xFF)
				{
					OutputDebugFormat("Input Report obtained Knob Left\n");
					DecreaseVolume();
				}
			}
		}
	}

	HidD_FreePreparsedData(pPreparsedData);
}

HANDLE hUSBDevice = INVALID_HANDLE_VALUE;
HANDLE hThread = INVALID_HANDLE_VALUE;

bool StartupPowerMateUSB()
{
	// this guid is from Bluetooth LE Explorer, in the windows store
	//  the service GUID is the custom service for the custom characteristics on the device
	//  if this were a heart tracker or something standard it would use a uuid from bluetooth's website
	GUID interfaceGUID;
	HidD_GetHidGuid(&interfaceGUID);

	// this searches for a device in the windows device registry that implements the service guid above, and opens a HANDLE
	hUSBDevice = OpenUSBDevice(&interfaceGUID);
	if (hUSBDevice == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	// start the background thread to service data
	hThread = (HANDLE)_beginthread(&ServiceHIDInputReports, 0, hUSBDevice);
	return hThread != INVALID_HANDLE_VALUE;
}

void ShutdownPowerMateUSB()
{
	// exit the thread loop
	bExitThread = true;

	// terminate any active i/o requests
	CancelIoEx(hUSBDevice, nullptr);

	// join the background thread
	BOOL status = STILL_ACTIVE;
	do
	{
		DWORD code = 0;
		status = GetExitCodeThread(hThread, &code);
		SwitchToThread();
	} while (status == STILL_ACTIVE);

	// close the device
	CloseHandle(hUSBDevice);

	// in case we startup again
	bExitThread = false;
}
