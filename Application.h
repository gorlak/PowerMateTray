#pragma once

#include <set>
#include <string>
#include <stdint.h>

namespace PowerMateTray
{
	class TrayIcon;

	class Application : public wxApp
	{
	public:
		Application();
		virtual ~Application();

		friend class PowerMateTray::TrayIcon;

	protected:
		virtual void OnInitCmdLine( wxCmdLineParser& parser );
		virtual bool OnCmdLineParsed( wxCmdLineParser& parser );
		virtual bool OnInit();
		virtual int OnRun();
		virtual int OnExit();

		void OnRefreshTimer( wxTimerEvent& evt );

	private:
		HANDLE m_MutexHandle;
		tstring m_MutexName;
		tstring m_Title;
		TrayIcon* m_TrayIcon;
		wxTimer m_RefreshTimer;
	};
}
