#include "stdafx.h"
#include "Application.h"

#include "TrayIcon.h"
#include "resource.h"

#include "PowerMate.h"
#include "Volume.h"

#include <regex>

using namespace PowerMateTray;

static int g_UpdateIntervalMS = 1000 * 60;

Application::Application()
	: m_MutexHandle( NULL )
	, m_MutexName (wxT("PowerMateTray"))
	, m_Title( wxT("PowerMateTray v") VERSION_STRING )
	, m_TrayIcon( NULL )
	, m_RefreshTimer( this )
{
	Connect( wxEVT_TIMER, wxTimerEventHandler( Application::OnRefreshTimer ), NULL, this );
}

Application::~Application()
{
	Disconnect( wxEVT_TIMER, wxTimerEventHandler( Application::OnRefreshTimer), NULL, this );
}

void Application::OnInitCmdLine( wxCmdLineParser& parser )
{
	SetVendorName( wxT("@gorlak") );
	parser.SetLogo( wxT("PowerMateTray (c) 20xx - @gorlak\n") );

	return __super::OnInitCmdLine( parser );
}

bool Application::OnCmdLineParsed( wxCmdLineParser& parser )
{
	return __super::OnCmdLineParsed( parser );
}

bool Application::OnInit()
{    
	if ( !__super::OnInit() )
	{
		return false;
	}

	bool tryAgain = true;
	bool haveMutex = false;
	do 
	{
		//---------------------------------------
		// try to get the Mutex
		m_MutexHandle = ::CreateMutex( NULL, true, m_MutexName.c_str() );

		if ( m_MutexHandle == NULL || ::GetLastError() == ERROR_ALREADY_EXISTS )
		{
			// if we couldn't get it, ask if we want to try again
			int promptResult = wxMessageBox(
				wxT( "PowerMateTray is currently running.\n\nPlease close all instances of it now, then click OK to continue, or Cancel to exit." ),
				wxT( "PowerMateTray" ),
				wxOK |wxCANCEL | wxCENTER | wxICON_QUESTION );

			if ( promptResult == wxCANCEL )
			{
				return false;
			}

			CloseHandle(m_MutexHandle);
			m_MutexHandle = NULL;
			tryAgain = true;
		}
		else
		{
			// we got it
			haveMutex = true;
			tryAgain = false;
		}
	}
	while ( tryAgain );

	wxInitAllImageHandlers();

	wxImageHandler* curHandler = wxImage::FindHandler( wxBITMAP_TYPE_CUR );
	if ( curHandler )
	{
		// Force the cursor handler to the end of the list so that it doesn't try to open TGA files.
		wxImage::RemoveHandler( curHandler->GetName() );
		curHandler = NULL;
		wxImage::AddHandler( new wxCURHandler );
	}

	m_TrayIcon = new TrayIcon( this );

	m_RefreshTimer.Start( g_UpdateIntervalMS, wxTIMER_ONE_SHOT );

	StartupVolume();
	StartupPowerMate();

	return true;
}

int Application::OnRun()
{
	m_TrayIcon->Initialize();

	return __super::OnRun();
}

int Application::OnExit()
{
	// clean up the TrayIcon
	m_TrayIcon->Cleanup();
	delete m_TrayIcon;
	m_TrayIcon = NULL;

	wxImage::CleanUpHandlers();

	CloseHandle(m_MutexHandle);
	m_MutexHandle = NULL;

	ShutdownPowerMate();
	ShutdownVolume();

	return __super::OnExit();
}

void Application::OnRefreshTimer( wxTimerEvent& evt )
{
	if( evt.GetId() == m_RefreshTimer.GetId() )
	{
		m_TrayIcon->Refresh();
		m_RefreshTimer.Start( g_UpdateIntervalMS, wxTIMER_ONE_SHOT );
	}
}

#ifdef _DEBUG
long& g_BreakOnAlloc (_crtBreakAlloc);
#endif

#ifdef _DEBUG
IMPLEMENT_APP_CONSOLE( Application );
#else
IMPLEMENT_APP( Application );
#endif