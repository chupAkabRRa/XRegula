#ifndef _REGULA_DLL_H_
#define _REGULA_DLL_H_

#include <memory>
#include <boost\filesystem.hpp>

class ThreadPool;

class RegulaDll
{
public:
	static bool CreateThreadPool(size_t numOfWorkers);
	static bool EnqueueImage(const boost::filesystem::path& strPathToFile);
	static bool DestroyThreadPool();
private:
	static std::unique_ptr<ThreadPool> gThreadPool;
};

#endif //_REGUL_DLL_H_
