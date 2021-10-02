#include "stdafx.h"

#include <stdio.h>
#include <malloc.h>
#include <string>

#include <objbase.h>
#include <windows.h>

#include <setupapi.h>
#pragma comment(lib, "setupapi")

#include <bluetoothleapis.h>
#pragma comment(lib, "bluetoothapis")

#include "Volume.h"

static HANDLE OpenBluetoothDevice(const GUID* interfaceGUID)
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
					OutputDebugFormat(_T("Found PowerMate Bluetooth: %s\n"), pInterfaceDetailData->DevicePath);
					hResult = CreateFile(
						pInterfaceDetailData->DevicePath,
						GENERIC_READ,
						FILE_SHARE_READ,
						NULL,
						OPEN_EXISTING,
						0,
						NULL);
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

static void ValueChangedEventHandler(BTH_LE_GATT_EVENT_TYPE EventType, PVOID EventOutParameter, PVOID Context)
{
	SCOPE_EVENT("ValueChangedEventHandler");

	PBLUETOOTH_GATT_VALUE_CHANGED_EVENT ValueChangedEventParameters = (PBLUETOOTH_GATT_VALUE_CHANGED_EVENT)EventOutParameter;

	if (ValueChangedEventParameters->CharacteristicValue->DataSize == 1)
	{
		char data = ValueChangedEventParameters->CharacteristicValue->Data[0];
		switch (data)
		{
		case 101:
			OutputDebugFormat("Notification obtained Knob Press\n");
			ToggleMute();
			break;

		case 103:
			OutputDebugFormat("Notification obtained Knob Left\n");
			DecreaseVolume();
			break;

		case 104:
			OutputDebugFormat("Notification obtained Knob Right\n");
			IncreaseVolume();
			break;

		default:
			OutputDebugFormat("Notification obtained unknown event\n");
			break;
		}
	}
	else
	{
		OutputDebugFormat("Notification obtained unknown data size %d\n", ValueChangedEventParameters->CharacteristicValue->DataSize);
	}
}

HANDLE hBluetoothDevice = INVALID_HANDLE_VALUE;

void StartupPowerMateBluetooth()
{
	// this guid is from Bluetooth LE Explorer, in the windows store
	//  the service GUID is the custom service for the custom characteristics on the device
	//  if this were a heart tracker or something standard it would use a uuid from bluetooth's website
	GUID interfaceGUID;
	HRESULT result = CLSIDFromString(TEXT("{25598CF7-4240-40A6-9910-080F19F91EBC}"), &interfaceGUID);
	assert(result == ERROR_SUCCESS);

	// this searches for a device in the windows device registry that implements the service guid above, and opens a HANDLE
	hBluetoothDevice = OpenBluetoothDevice(&interfaceGUID);
	if (hBluetoothDevice == INVALID_HANDLE_VALUE)
	{
		return;
	}

	// fetch the services array for the device
	USHORT serviceCount = 0;
	AutoFreePointer<BTH_LE_GATT_SERVICE> pServiceBuffer = NULL;
	HRESULT hr = BluetoothGATTGetServices(hBluetoothDevice, 0, NULL, &serviceCount, BLUETOOTH_GATT_FLAG_NONE);
	if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr)
	{
		OutputDebugFormat("BluetoothGATTGetServices returned unexpected HRESULT: %d\n", hr);
	}
	if (serviceCount)
	{
		pServiceBuffer = (PBTH_LE_GATT_SERVICE)malloc(sizeof(BTH_LE_GATT_SERVICE) * serviceCount);
		ZeroMemory(pServiceBuffer, sizeof(BTH_LE_GATT_SERVICE) * serviceCount);
		hr = BluetoothGATTGetServices(hBluetoothDevice, serviceCount, pServiceBuffer, &serviceCount, BLUETOOTH_GATT_FLAG_NONE);
		if (S_OK != hr)
		{
			OutputDebugFormat("BluetoothGATTGetServices returned unexpected HRESULT: %d\n", hr);
		}
	}

	// fetch the characteristics
	USHORT characteristicCount = 0;
	AutoFreePointer<BTH_LE_GATT_CHARACTERISTIC> pCharacteristicBuffer = NULL;
	if (serviceCount)
	{
		hr = BluetoothGATTGetCharacteristics(hBluetoothDevice, pServiceBuffer, 0, NULL, &characteristicCount, BLUETOOTH_GATT_FLAG_NONE);
		if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr)
		{
			OutputDebugFormat("BluetoothGATTGetCharacteristics returned unexpected HRESULT: %d\n", hr);
		}
		if (characteristicCount)
		{
			pCharacteristicBuffer = (PBTH_LE_GATT_CHARACTERISTIC)malloc(sizeof(BTH_LE_GATT_CHARACTERISTIC) * characteristicCount);
			ZeroMemory(pCharacteristicBuffer, sizeof(BTH_LE_GATT_CHARACTERISTIC) * characteristicCount);

			hr = BluetoothGATTGetCharacteristics(hBluetoothDevice, pServiceBuffer, characteristicCount, pCharacteristicBuffer, &characteristicCount, BLUETOOTH_GATT_FLAG_NONE);
			if (S_OK != hr)
			{
				OutputDebugFormat("BluetoothGATTGetCharacteristics returned unexpected HRESULT: %d\n", hr);
			}
		}
	}

	// iterate the characteristics and attach event handler
	if (characteristicCount >= 2)
	{
		// 2 is the magic characteristic, BTLE is obnoxious about characteristic identity
		PBTH_LE_GATT_CHARACTERISTIC currentCharacteristic = &pCharacteristicBuffer[2];
		if (currentCharacteristic->IsNotifiable)
		{
			BLUETOOTH_GATT_VALUE_CHANGED_EVENT_REGISTRATION eventRegistration;
			ZeroMemory(&eventRegistration, sizeof(BLUETOOTH_GATT_VALUE_CHANGED_EVENT_REGISTRATION));
			eventRegistration.Characteristics[0] = *currentCharacteristic;
			eventRegistration.NumCharacteristics = 1;
			BLUETOOTH_GATT_EVENT_HANDLE hValueChangedEvent = INVALID_HANDLE_VALUE;
			hr = BluetoothGATTRegisterEvent(hBluetoothDevice, CharacteristicValueChangedEvent, &eventRegistration, ValueChangedEventHandler, NULL, &hValueChangedEvent, BLUETOOTH_GATT_FLAG_NONE);
			if (S_OK != hr)
			{
				OutputDebugFormat("BluetoothGATTRegisterEvent returned unexpected HRESULT: %d\n", hr);
			}
			else
			{
				OutputDebugFormat("Registered for Event Notification\n");
			}
		}
		else
		{
			OutputDebugFormat("Expected characteristic isn't notifiable!\n");
		}
	}
}

void ShutdownPowerMateBluetooth()
{
	CloseHandle(hBluetoothDevice);
}
