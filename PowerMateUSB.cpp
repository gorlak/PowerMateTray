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

static bool FindUSBDevice(WCHAR* devicePath = nullptr, uint32_t devicePathCount = 0)
{
	bool bResult = false;

	GUID interfaceGUID;
	HidD_GetHidGuid(&interfaceGUID);

	HDEVINFO hDevInfo = SetupDiGetClassDevs(&interfaceGUID, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		return bResult;
	}

	SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
	ZeroMemory(&deviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
	deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	for (DWORD devInterfaceIndex = 0; SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &interfaceGUID, devInterfaceIndex, &deviceInterfaceData); devInterfaceIndex++)
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
						if (devicePath && devicePathCount)
						{
							OutputDebugFormat(_T("Found PowerMate USB: %s\n"), pInterfaceDetailData->DevicePath);
							wcsncpy_s(devicePath, devicePathCount, pInterfaceDetailData->DevicePath, ~0);
						}
						bResult = true;
					}
				}

				free(pInterfaceDetailData);

				if (bResult)
				{
					break;
				}
			}
		}
	}

	SetupDiDestroyDeviceInfoList(hDevInfo);
	return bResult;
}

bool bExitThread = false;
HANDLE hUSBDevice = INVALID_HANDLE_VALUE;

static void OpenAndServiceHIDInputReports(wchar_t* devicePath)
{
	hUSBDevice = CreateFile( devicePath,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING,
		NULL);
	if (hUSBDevice == INVALID_HANDLE_VALUE)
	{
		return;
	}

	PHIDP_PREPARSED_DATA pPreparsedData;
	if (!HidD_GetPreparsedData(hUSBDevice, &pPreparsedData))
	{
		return;
	}

	HIDP_CAPS caps;
	ZeroMemory(&caps, sizeof(caps));
	if (!HidP_GetCaps(pPreparsedData, &caps))
	{
		return;
	}

	struct PowerMateInput
	{
		char pad;
		char button;
		char dial;
	};
	PowerMateInput previous;
	ZeroMemory(&previous, sizeof(PowerMateInput));

	char* inputReport = (char*)alloca(caps.InputReportByteLength);

	bool bDisconnected = false;
	while (!bExitThread && !bDisconnected)
	{
		OVERLAPPED overlapped;
		ZeroMemory(&overlapped, sizeof(overlapped));
		if (ReadFileEx(hUSBDevice, inputReport, caps.InputReportByteLength, &overlapped, NULL))
		{
			bool bReadComplete = false;
			while (!bDisconnected && !bReadComplete)
			{
				if (WAIT_OBJECT_0 == WaitForSingleObject(hUSBDevice, 10))
				{
					DWORD OverlappedNumberOfBytesTransfered = 0;
					if (::GetOverlappedResult(hUSBDevice, &overlapped, &OverlappedNumberOfBytesTransfered, false)
						&& OverlappedNumberOfBytesTransfered == caps.InputReportByteLength)
					{
						// data, reset event for next overlapped i/o
						bReadComplete = true;
					}
					else if (GetLastError() == ERROR_DEVICE_NOT_CONNECTED)
					{
						bDisconnected = true;
					}
				}
			}

			if (bReadComplete)
			{
				PowerMateInput& current(*(PowerMateInput*)inputReport);
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
	}

	HidD_FreePreparsedData(pPreparsedData);
}

static void PollForDevices(void*)
{
	wchar_t devicePath[MAX_PATH];
	while (!bExitThread)
	{
		// this searches for a device in the windows device registry that implements the service guid above, and opens a HANDLE
		if (FindUSBDevice(devicePath, sizeof(devicePath)/sizeof(devicePath[0])))
		{
			OpenAndServiceHIDInputReports(devicePath);
		}

		Sleep(10);
	}
}

HANDLE hThread = INVALID_HANDLE_VALUE;

void StartupPowerMateUSB()
{
	// start the background thread to service data
	hThread = (HANDLE)_beginthread(&PollForDevices, 0, nullptr);
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
