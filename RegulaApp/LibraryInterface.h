#ifndef _LIBRARY_INTERFACE_H_
#define _LIBRARY_INTERFACE_H_

#include <string>
#include <boost\filesystem.hpp>

class LibraryInterface
{
protected:
	using CreateThreadPoolFunc = bool(*)(size_t);
	using EnqueueImageFunc = bool(*)(const boost::filesystem::path&);
	using DestroyThreadPoolFunc = bool(*)();
public:
	virtual ~LibraryInterface() {}
	virtual int LoadRegulaLibrary(const std::wstring& strLibName) = 0;
	virtual void UnloadRegulaLibrary() = 0;

	// DLL functions
	CreateThreadPoolFunc _CreateThreadPool;
	EnqueueImageFunc _EnqueueImage;
	DestroyThreadPoolFunc _DestroyThreadPool;
};

#endif // _LIBRARY_INTERFACE_H_
