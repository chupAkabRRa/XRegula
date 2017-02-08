#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>

#include <boost\filesystem.hpp>

#include "OpencvWrapper.h"

//
// ThreadPool includes 2 sets of workers:
// 1) Workers which will run the "payload" (i.e. detection workers)
// 2) Workers which will do saving according to the task (i.e. completion workers)
//
class ThreadPool
{
public:
    ThreadPool(size_t numOfWorkers);
    ~ThreadPool();

    // Enqueue() is used to enqueue new element for workers to process.
    // The element itself is a path to JPEG file to process.
    void Enqueue(const boost::filesystem::path& strPathToFile);

private:
    // As one image can contain couple of faces, we need to process them in completion
    // workers by "packages" called ImageProcessingResult here
    using ImageProcessingResults = struct {
		boost::filesystem::path strPathToFile;
        std::vector<OpencvWrapper::DetectionResult> vFound;
    };

    // First type of workers which are used to run "payload".
    // When thread completes its task, then it enqueues completion element into the
    // queue of completion tasks and notifies CompletionWorker about that.
    std::vector<std::thread> m_DetectionWorkers;
    std::queue<boost::filesystem::path> m_DetectionTasks;

    // Second type of workers which are used to run completion tasks.
    std::vector<std::thread> m_CompletionWorkers;
    std::queue<ImageProcessingResults> m_CompletionTasks;

    // Synchronization for first type of workers.
    std::mutex m_mtxDetectionQ;
    std::condition_variable m_cvDetection;

    // Synchronization of second type of workers.
    std::mutex m_mtxCompletionQ;
    std::condition_variable m_cvCompletion;

    // Flags which are used to stop the overall ThreadPool.
    bool m_bStopDetection = false;
	bool m_bStopCompletion = false;

	// See description in .cpp file
	size_t m_MaxQueueDepth;
	size_t m_EnumeratorWakeupThreshold;
	std::condition_variable m_cvEnumerator;
};

#endif // _THREAD_POOL_H_
