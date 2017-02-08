#include "LibraryInterfaceWin.h"

int LibraryInterfaceWin::LoadRegulaLibrary(const std::wstring& strLibName)
{
	m_hInstLibrary = LoadLibrary(strLibName.c_str());
	if (!m_hInstLibrary)
	{
		return GetLastError();
	}

	_CreateThreadPool = (CreateThreadPoolFunc)GetProcAddress(m_hInstLibrary, "CreateThreadPool");
	if (!_CreateThreadPool)
	{
		return GetLastError();
	}
	_EnqueueImage = (EnqueueImageFunc)GetProcAddress(m_hInstLibrary, "EnqueueImage");
	if (!_EnqueueImage)
	{
		return GetLastError();
	}
	_DestroyThreadPool = (DestroyThreadPoolFunc)GetProcAddress(m_hInstLibrary, "DestroyThreadPool");
	if (!_DestroyThreadPool)
	{
		return GetLastError();
	}

	return 0;
}

void LibraryInterfaceWin::UnloadRegulaLibrary()
{
	if (m_hInstLibrary)
		FreeLibrary(m_hInstLibrary);
}

LibraryInterfaceWin::~LibraryInterfaceWin()
{
	UnloadRegulaLibrary();
}