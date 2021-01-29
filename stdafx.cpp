#include "stdafx.h"

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

int OutputDebugFormat(const wchar_t* format, ...)
{
	wchar_t str[1024];

	va_list argptr;
	va_start(argptr, format);
	int ret = _vsnwprintf(str, sizeof(str)/sizeof(str[0]), format, argptr);
	va_end(argptr);

	OutputDebugStringW(str);

	return ret;
}
