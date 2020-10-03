#pragma once


namespace PowerMateTray
{
	class Application;

	namespace EventIDs
	{
		enum EventID
		{
			First = wxID_HIGHEST + 1,
			Exit = First,
			Last = Exit,
		};
	}

	class TrayIcon : public wxTaskBarIcon
	{
	public:
		TrayIcon( Application* application );
		virtual ~TrayIcon();

		void Initialize();    
		void Cleanup();

		bool IsMenuShowing() const
		{
			return m_IsMenuShowing;
		}

		void BeginBusy();
		void EndBusy();
		void Refresh();

	protected:
		void OnTrayIconClick( wxTaskBarIconEvent& evt );
		void OnMenuExit( wxCommandEvent& evt );

	private:
		Application* m_Application;
		wxMenu* m_Menu;
		wxMenuItem* m_UpdateMenuItem;
		int m_BusyCount;
		bool m_IsMenuShowing;
	};
}
