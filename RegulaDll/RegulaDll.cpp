#include "RegulaDll.h"
#include "ThreadPool.h"

// static
std::unique_ptr<ThreadPool> RegulaDll::gThreadPool;

// static
bool RegulaDll::CreateThreadPool(size_t numOfWorkers)
{
	if (gThreadPool != nullptr)
		return false;

	gThreadPool = std::make_unique<ThreadPool>(numOfWorkers);
	return true;
}

// static
bool RegulaDll::EnqueueImage(const boost::filesystem::path& strPathToFile)
{
	if (gThreadPool == nullptr)
		return false;

	gThreadPool->Enqueue(strPathToFile);
	return true;
}

// static
bool RegulaDll::DestroyThreadPool()
{
	if (gThreadPool == nullptr)
		return false;

	gThreadPool.reset();
	return true;
}

