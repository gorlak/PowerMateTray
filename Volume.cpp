#include "stdafx.h"

#include <windows.h>
#include <commctrl.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>

#define SAFE_RELEASE(punk) \
              if ((punk) != NULL) \
                { (punk)->Release(); (punk) = NULL; }

static IAudioEndpointVolume* pAudioEndpointVolume = NULL;

void StartupVolume()
{
	HRESULT hr = S_OK;

	// Get enumerator for audio endpoint devices.
	IMMDeviceEnumerator* pEnumerator = NULL;
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);

	// Get default audio-rendering device.
	IMMDevice* pDevice = NULL;
	hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);

	// Activate to get the object
	hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pAudioEndpointVolume);

	SAFE_RELEASE(pEnumerator);
	SAFE_RELEASE(pDevice);
}

void ShutdownVolume()
{
	SAFE_RELEASE(pAudioEndpointVolume);
}

void IncreaseVolume()
{
	float level;
	pAudioEndpointVolume->GetMasterVolumeLevel(&level);
	pAudioEndpointVolume->SetMasterVolumeLevel(level + 0.5f, NULL);
}

void DecreaseVolume()
{
	float level;
	pAudioEndpointVolume->GetMasterVolumeLevel(&level);
	pAudioEndpointVolume->SetMasterVolumeLevel(level - 0.5f, NULL);
}

void ToggleMute()
{
	BOOL mute;
	pAudioEndpointVolume->GetMute(&mute);
	pAudioEndpointVolume->SetMute(!mute, NULL);
}

