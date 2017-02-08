#ifndef _LIBRARY_INTERFACE_WIN_H_
#define _LIBRARY_INTERFACE_WIN_H_

#include "LibraryInterface.h"

#include <Windows.h>

class LibraryInterfaceWin : public LibraryInterface
{
public:
	virtual int LoadRegulaLibrary(const std::wstring& strLibName) override;
	virtual void UnloadRegulaLibrary() override;
	virtual ~LibraryInterfaceWin();
private:
	HINSTANCE m_hInstLibrary;
};

#endif
