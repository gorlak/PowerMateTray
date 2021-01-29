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

#ifdef PROFILE
#include "Superluminal/PerformanceAPI.h"
#define SCOPE_EVENT(STR) PERFORMANCEAPI_INSTRUMENT(STR)
#else
#define SCOPE_EVENT(STR)
#endif // PROFILE

int OutputDebugFormat(const char* format, ...)
{
	char str[1024];

	va_list argptr;
	va_start(argptr, format);
	int ret = vsnprintf(str, sizeof(str), format, argptr);
	va_end(argptr);

	OutputDebugStringA(str);

	return ret;
}

#ifdef _DEBUG

struct DeviceProperty
{
	DWORD property;
	const wchar_t* property_name;
} g_DeviceProperties[] = {
	{ SPDRP_DEVICEDESC, L"SPDRP_DEVICEDESC" },
	{ SPDRP_CLASS, L"SPDRP_CLASS" },
	{ SPDRP_CLASSGUID, L"SPDRP_CLASSGUID" },
	{ SPDRP_FRIENDLYNAME, L"SPDRP_FRIENDLYNAME" },
	{ SPDRP_ENUMERATOR_NAME, L"SPDRP_ENUMERATOR_NAME" },
	{ SPDRP_DRIVER, L"SPDRP_DRIVER" },
	{ SPDRP_HARDWAREID, L"SPDRP_HARDWAREID" },
	{ SPDRP_MFG, L"SPDRP_MFG" },
	{ SPDRP_LOCATION_INFORMATION, L"SPDRP_LOCATION_INFORMATION" },
	{ SPDRP_SERVICE, L"SPDRP_SERVICE" },
	{ 0, 0 }
};

static bool GetDeviceInfoString(HDEVINFO hDevInfo, PSP_DEVINFO_DATA pDevInfoData, DWORD devProperty, std::wstring& value)
{
	value.clear();

	DWORD dwBytesNeeded = 0;
	if (!SetupDiGetDeviceRegistryProperty(hDevInfo, pDevInfoData, devProperty, NULL, NULL, 0, &dwBytesNeeded))
	{
		if (ERROR_INSUFFICIENT_BUFFER != GetLastError() || dwBytesNeeded < 1)
		{
			return false;
		}
	}

	bool result = true;
	DWORD dwBufSize = dwBytesNeeded + 0x10;;
	PUCHAR pBuf = NULL;

	if (NULL == (pBuf = (PUCHAR)_malloca(dwBufSize)))
	{
		result = false;
	}
	else
	{
		if (SetupDiGetDeviceRegistryProperty(hDevInfo, pDevInfoData, devProperty, NULL, pBuf, dwBufSize, &dwBufSize))
		{
			value = (wchar_t*)pBuf;
		}
		else
		{
			result = false;
		}
	}

	if (pBuf)
	{
		_freea(pBuf);
	}

	return result;
}

static bool ProbeDevicesForFriendlyName(const wchar_t* friendlyName, GUID* classGUID)
{
	bool result = false;

	HDEVINFO hDevInfo = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	SP_DEVINFO_DATA deviceInfoData;
	ZeroMemory(&deviceInfoData, sizeof(SP_DEVINFO_DATA));
	deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
	for (DWORD devInfoIndex = 0; SetupDiEnumDeviceInfo(hDevInfo, devInfoIndex, &deviceInfoData); devInfoIndex++)
	{
#ifdef DUMP_DEVICES
		for (DeviceProperty* property = g_DeviceProperties; property->property_name; property++)
		{
			std::wstring str;
			if (GetDeviceInfoString(hDevInfo, &deviceInfoData, property->property, str))
			{
				wprintf(L"%s: %s\n", property->property_name, str.c_str());
			}
		}
		OutputDebugFormat("\n");
#else
		std::wstring deviceFriendlyName;
		if (GetDeviceInfoString(hDevInfo, &deviceInfoData, SPDRP_FRIENDLYNAME, deviceFriendlyName) || !wcscmp(friendlyName, deviceFriendlyName.c_str()))
		{
			memcpy(classGUID, &deviceInfoData.ClassGuid, sizeof(GUID));
			result = true;
			break;
		}
#endif
	}

	SetupDiDestroyDeviceInfoList(hDevInfo);
	return result;
}

#endif // _DEBUG

static HANDLE OpenBluetoothDevice(const GUID* serviceGUID)
{
	HANDLE hResult = INVALID_HANDLE_VALUE;

	HDEVINFO hDevInfo = SetupDiGetClassDevs(serviceGUID, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		return hResult;
	}

	SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
	ZeroMemory(&deviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
	deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	for (DWORD devInterfaceIndex = 0; SetupDiEnumDeviceInterfaces(hDevInfo, NULL, serviceGUID, devInterfaceIndex, &deviceInterfaceData); devInterfaceIndex++)
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

	switch (ValueChangedEventParameters->CharacteristicValue->DataSize)
	{
		case 0:
		{
			OutputDebugFormat("Notification obtained ValueChangedEventParameters->CharacteristicValue->DataSize=0\n");
			break;
		}
		case 1:
		{
			char data = ValueChangedEventParameters->CharacteristicValue->Data[0];
			switch (data)
			{
			case 104:
				OutputDebugFormat("Notification obtained Knob Right\n");
				IncreaseVolume();
				break;

			case 103:
				OutputDebugFormat("Notification obtained Knob Left\n");
				DecreaseVolume();
				break;

			case 101:
				OutputDebugFormat("Notification obtained Knob Press\n");
				ToggleMute();
				break;

			default:
				OutputDebugFormat("Notification obtained Unknown atom %d\n", data);
				break;
			}
			break;
		}
		default:
		{
#if 0
			char hex[256];
			char buf = hex;
			for (ULONG i = 0; i < ValueChangedEventParameters->CharacteristicValue->DataSize; i++)
			{
				size_t count = hex + sizeof(hex) - buf - 1;
				int result = snprintf(buf, count, "%0X", ValueChangedEventParameters->CharacteristicValue->Data[i]);
				buf += result;
			}
			OutputDebugFormat("Notification obtained %s\n", buf);
			break;
#endif
		}
	}
}

HANDLE hBluetoothDevice = INVALID_HANDLE_VALUE;

template< class T >
class AutoFreePointer
{
public:
	AutoFreePointer()
		: m_Pointer(nullptr)
	{

	}

	AutoFreePointer(T* pointer)
		: m_Pointer(pointer)
	{

	}

	~AutoFreePointer()
	{
		if (m_Pointer)
		{
			free(m_Pointer);
		}
	}

	T* operator=(T* pointer)
	{
		if (m_Pointer)
		{
			free(m_Pointer);
		}
		m_Pointer = pointer;
		return m_Pointer;
	}

	T* operator->()
	{
		return m_Pointer;
	}

	operator T* ()
	{
		return m_Pointer;
	}

private:
	T* m_Pointer;
};

bool StartupPowerMate()
{
	// this guid is from Bluetooth LE Explorer, in the windows store
	//  the service GUID is the custom service for the custom characteristics on the device
	//  if this were a heart tracker or something standard it would use a uuid from bluetooth's website
	GUID serviceGUID;
	if (ERROR_SUCCESS != CLSIDFromString(TEXT("{25598CF7-4240-40A6-9910-080F19F91EBC}"), &serviceGUID))
	{
		return false;
	}

	// this searches for a device in the windows device registry that implements the service guid above, and opens a HANDLE
	hBluetoothDevice = OpenBluetoothDevice(&serviceGUID);
	if (hBluetoothDevice == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	// fetch the services array for the device
	USHORT serviceCount = 0;
	AutoFreePointer<BTH_LE_GATT_SERVICE> pServiceBuffer = NULL;
	HRESULT hr = BluetoothGATTGetServices(hBluetoothDevice, 0, NULL, &serviceCount, BLUETOOTH_GATT_FLAG_NONE);
	if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr)
	{
		OutputDebugFormat("BluetoothGATTGetServices returned unexpected HRESULT: %d\n", hr);
	}
	else
	{
		OutputDebugFormat("Got %d services from the device\n", serviceCount);
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
		else
		{
			OutputDebugFormat("Got %d characteristics from the device\n", characteristicCount);
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
	PBTH_LE_GATT_CHARACTERISTIC currentCharacteristic = NULL;
	for (int characteristicIndex = 0; characteristicIndex < characteristicCount; characteristicIndex++)
	{
		currentCharacteristic = &pCharacteristicBuffer[characteristicIndex];

		USHORT descriptorCount;
		hr = BluetoothGATTGetDescriptors(hBluetoothDevice, currentCharacteristic, 0, NULL, &descriptorCount, BLUETOOTH_GATT_FLAG_NONE);
		if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr)
		{
			OutputDebugFormat("BluetoothGATTGetDescriptors returned unexpected HRESULT: %d\n", hr);
		}
		else
		{
			OutputDebugFormat("Characteristic %d has %d descriptors\n", characteristicIndex, descriptorCount);
		}

		AutoFreePointer<BTH_LE_GATT_DESCRIPTOR> pDescriptorBuffer = NULL;
		if (descriptorCount)
		{
			pDescriptorBuffer = (PBTH_LE_GATT_DESCRIPTOR)malloc(sizeof(BTH_LE_GATT_DESCRIPTOR) * descriptorCount);
			ZeroMemory(pDescriptorBuffer, sizeof(BTH_LE_GATT_DESCRIPTOR) * descriptorCount);

			hr = BluetoothGATTGetDescriptors(hBluetoothDevice, currentCharacteristic, descriptorCount, pDescriptorBuffer, &descriptorCount, BLUETOOTH_GATT_FLAG_NONE);
			if (S_OK != hr)
			{
				OutputDebugFormat("BluetoothGATTGetDescriptors returned unexpected HRESULT: %d\n", hr);
			}

			for (int descriptorIndex = 0; descriptorIndex < descriptorCount; descriptorIndex++)
			{
				PBTH_LE_GATT_DESCRIPTOR  currentDescriptor = &pDescriptorBuffer[descriptorIndex];

				USHORT descValueDataSize;
				hr = BluetoothGATTGetDescriptorValue(hBluetoothDevice, currentDescriptor, 0, NULL, &descValueDataSize, BLUETOOTH_GATT_FLAG_NONE);
				if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr)
				{
					OutputDebugFormat("BluetoothGATTGetDescriptorValue returned unexpected HRESULT: %d\n", hr);
				}
				else
				{
					OutputDebugFormat("Characteristic %d, descriptor %d has value data size %d\n", characteristicIndex, descriptorIndex, descValueDataSize);
				}
				AutoFreePointer<BTH_LE_GATT_DESCRIPTOR_VALUE> pDescValueBuffer = (PBTH_LE_GATT_DESCRIPTOR_VALUE)malloc(descValueDataSize);
				ZeroMemory(pDescValueBuffer, descValueDataSize);
				hr = BluetoothGATTGetDescriptorValue(hBluetoothDevice, currentDescriptor, (ULONG)descValueDataSize, pDescValueBuffer, NULL, BLUETOOTH_GATT_FLAG_NONE);
				if (S_OK != hr)
				{
					OutputDebugFormat("BluetoothGATTGetDescriptorValue returned unexpected HRESULT: %d\n", hr);
				}

				//you may also get a descriptor that is read (and not notify) and i am guessing the attribute handle is out of limits
				// we set all descriptors that are notifiable to notify us via IsSubstcibeToNotification
				if (currentDescriptor->DescriptorType != CharacteristicUserDescription)
				{
					BTH_LE_GATT_DESCRIPTOR_VALUE newValue;
					ZeroMemory(&newValue, sizeof(BTH_LE_GATT_DESCRIPTOR_VALUE));
					newValue.DescriptorType = ClientCharacteristicConfiguration;
					newValue.ClientCharacteristicConfiguration.IsSubscribeToNotification = TRUE;

					hr = BluetoothGATTSetDescriptorValue(hBluetoothDevice, currentDescriptor, &newValue, BLUETOOTH_GATT_FLAG_NONE);
					if (S_OK != hr)
					{
						if (E_ACCESSDENIED != hr)
						{
							OutputDebugFormat("BluetoothGATTGetDescriptorValue returned unexpected HRESULT: %d\n", hr);
						}
					}
					else
					{
						OutputDebugFormat("Set notification for service handle %d\n", currentDescriptor->ServiceHandle);
					}
				}
			}
		}

		// set the appropriate callback function when the descriptor change value
		BLUETOOTH_GATT_EVENT_HANDLE hValueChangedEvent = INVALID_HANDLE_VALUE;
		if (currentCharacteristic->IsNotifiable)
		{
			OutputDebugFormat("Setting Notification for ServiceHandle %d\n", currentCharacteristic->ServiceHandle);

			BLUETOOTH_GATT_VALUE_CHANGED_EVENT_REGISTRATION eventRegistration;
			ZeroMemory(&eventRegistration, sizeof(BLUETOOTH_GATT_VALUE_CHANGED_EVENT_REGISTRATION));
			eventRegistration.Characteristics[0] = *currentCharacteristic;
			eventRegistration.NumCharacteristics = 1;
			hr = BluetoothGATTRegisterEvent(hBluetoothDevice, CharacteristicValueChangedEvent, &eventRegistration, ValueChangedEventHandler, NULL, &hValueChangedEvent, BLUETOOTH_GATT_FLAG_NONE);
			if (S_OK != hr)
			{
				OutputDebugFormat("BluetoothGATTRegisterEvent returned unexpected HRESULT: %d\n", hr);
			}
		}

		if (currentCharacteristic->IsReadable)
		{
			USHORT valueDataSize;
			hr = BluetoothGATTGetCharacteristicValue(hBluetoothDevice, currentCharacteristic, 0, NULL, &valueDataSize, BLUETOOTH_GATT_FLAG_NONE);
			if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) != hr)
			{
				OutputDebugFormat("BluetoothGATTGetCharacteristicValue returned unexpected HRESULT: %d\n", hr);
			}
			AutoFreePointer<BTH_LE_GATT_CHARACTERISTIC_VALUE> pValueBuffer = (PBTH_LE_GATT_CHARACTERISTIC_VALUE)malloc(valueDataSize);
			ZeroMemory(pValueBuffer, valueDataSize);
			hr = BluetoothGATTGetCharacteristicValue(hBluetoothDevice, currentCharacteristic, (ULONG)valueDataSize, pValueBuffer, NULL, BLUETOOTH_GATT_FLAG_NONE);
			if (S_OK != hr)
			{
				OutputDebugFormat("BluetoothGATTGetCharacteristicValue returned unexpected HRESULT: %d\n", hr);
			}

			OutputDebugFormat("Read characterstic value: ");
			for (ULONG dataIndex = 0; dataIndex < pValueBuffer->DataSize; dataIndex++)
			{
				OutputDebugFormat("%0X", pValueBuffer->Data[dataIndex]);
			}
			OutputDebugFormat("\n");
		}
	}

	return true;
}

void ShutdownPowerMate()
{
	CloseHandle(hBluetoothDevice);
}