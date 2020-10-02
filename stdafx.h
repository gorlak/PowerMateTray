#pragma once

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>

#include <stdint.h>
#include <algorithm>
#include <cstring>
#include <direct.h>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <shellapi.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <time.h>
#include <vector>
#include <regex>

#include <wx/wx.h>
#include <wx/msw/private.h>
#include <wx/artprov.h>
#include <wx/bitmap.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/clipbrd.h>
#include <wx/cmdline.h>
#include <wx/dnd.h>
#include <wx/evtloop.h>
#include <wx/image.h>
#include <wx/msw/dialog.h>
#include <wx/msw/private.h>
#include <wx/panel.h>
#include <wx/ptr_scpd.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/taskbar.h>
#include <wx/toplevel.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/xml/xml.h>

#define TO_STRING(x) #x
#define TODO_STRING(x) TO_STRING(x)
#define TODO(msg) message (__FILE__ "(" TODO_STRING(__LINE__) ") : TODO: " msg)

#ifdef _UNICODE
typedef wchar_t tchar_t;
#else
typedef char tchar_t;
#endif

typedef std::basic_string< tchar_t > tstring;

typedef std::basic_istream< tchar_t, std::char_traits< tchar_t > > tistream;
typedef std::basic_ostream< tchar_t, std::char_traits< tchar_t > > tostream;
typedef std::basic_iostream< tchar_t, std::char_traits< tchar_t > > tiostream;

typedef std::basic_ifstream< tchar_t, std::char_traits< tchar_t > > tifstream;
typedef std::basic_ofstream< tchar_t, std::char_traits< tchar_t > > tofstream;
typedef std::basic_fstream< tchar_t, std::char_traits< tchar_t > > tfstream;

typedef std::basic_istringstream< tchar_t, std::char_traits< tchar_t >, std::allocator< tchar_t > > tistringstream;
typedef std::basic_ostringstream< tchar_t, std::char_traits< tchar_t >, std::allocator< tchar_t > > tostringstream;
typedef std::basic_stringstream< tchar_t, std::char_traits< tchar_t >, std::allocator< tchar_t > > tstringstream;

typedef std::tr1::basic_regex<tchar_t, std::tr1::regex_traits<tchar_t> > tregex;
typedef std::tr1::match_results<const tchar_t*> tcmatch;
typedef std::tr1::match_results<tstring::const_iterator> tsmatch;
typedef std::tr1::regex_token_iterator< const tchar_t*> tcregex_token_iterator;
typedef std::tr1::regex_token_iterator< tstring::const_iterator> tsregex_token_iterator;
typedef std::tr1::regex_iterator< const tchar_t* > tcregex_iterator;
typedef std::tr1::regex_iterator< tstring::const_iterator > tsregex_iterator;
