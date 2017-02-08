#include "ThreadPool.h"
#include "OpencvWrapper.h"

ThreadPool::ThreadPool(size_t numOfWorkers)
{
	// It's obvious that ThreadPool's client (the one who calls ThreadPool::Enqueue method) 
	// enumerates images to process at much faster speed than they are actually processed. 
	// So, to avoid the situation, when m_DetectionTasks will grow to enourmous size, I 
	// introduced a conceptof MaxQueueDepth + EnumeratorWakeupThreshold:
	// 1) ThreadPool client will enqueue new tasks till m_DetectionTasks is less than 
	//    MaxQueueDepth = 4 * numOfWorkers. If queue reaches this depth, enumerator goes to
	//    wait state.
	// 2) MaxQueueDepth = 4 * numOfWorkers. Thus we can be sure that workers will
	//    always be loaded and won't waste time while waiting for Enumerator to wake up.
	// 3) Enumerator will be woken up when m_DetectionTasks <= EnumeratorWakeupThreshold.
	//    EnumeratorWakeupThreshold = MaxQueueDepth / 2.
	// 4) Repeat starting from #1.
	m_MaxQueueDepth = 4 * numOfWorkers;
	m_EnumeratorWakeupThreshold = m_MaxQueueDepth / 2;

    for (size_t i = 0; i < numOfWorkers; i++)
    {
        // Create set of "detection" workers
        m_DetectionWorkers.emplace_back( [this]
        {
            while (true)
            {
				boost::filesystem::path strPathToFile;

                {
                    std::unique_lock<std::mutex> lck(this->m_mtxDetectionQ);

                    this->m_cvDetection.wait(lck, [this]
                    {
                        return this->m_bStopDetection || !this->m_DetectionTasks.empty();
                    });
                    if (this->m_bStopDetection && this->m_DetectionTasks.empty())
                    {
                        return;
                    }

                    strPathToFile = std::move(this->m_DetectionTasks.front());
                    this->m_DetectionTasks.pop();
                }
				// Notify enumerator about removing of one task
				this->m_cvEnumerator.notify_one();

                // Run detection payload
                ImageProcessingResults DetectionResults;

                bool bResult = OpencvWrapper::DetectFacialFeatures(strPathToFile, DetectionResults.vFound);

                // Notify completion workers about new task in case of successful detection.
                if (bResult && DetectionResults.vFound.size() > 0)
                {
					DetectionResults.strPathToFile = strPathToFile;

                    {
                        std::unique_lock<std::mutex> lck(this->m_mtxCompletionQ);
						this->m_CompletionTasks.push(DetectionResults);
                    }
					m_cvCompletion.notify_one();
                }
            }
        });

        // Create set of "completion" workers
        // (I assumed that number of "completion" workers should be the same as
        //  the number of "detection" wrokers. However, some additional research is
        //  required).
        m_CompletionWorkers.emplace_back( [this]
        {
            while (true)
            {
                ImageProcessingResults DetectionResults;

                {
                    std::unique_lock<std::mutex> lck(this->m_mtxCompletionQ);

                    this->m_cvCompletion.wait(lck, [this]
                    {
                        return this->m_bStopCompletion || !this->m_CompletionTasks.empty();
                    });
                    if (this->m_bStopCompletion && this->m_CompletionTasks.empty())
                    {
                        return;
                    }

                    DetectionResults = std::move(this->m_CompletionTasks.front());
                    this->m_CompletionTasks.pop();
                }

                // Run completion payload
                bool bResult = OpencvWrapper::SaveResults(DetectionResults.strPathToFile, DetectionResults.vFound);
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
	// First stop all "detection" threads
    {
        std::unique_lock<std::mutex> lckDetectionQ(m_mtxDetectionQ);
        m_bStopDetection = true;
    }

    m_cvDetection.notify_all();
	for (auto& detectionWorker : m_DetectionWorkers)
		detectionWorker.join();

	// Now stop all "completion" threads
	{
		std::unique_lock<std::mutex> lckCompletionQ(m_mtxCompletionQ);
		m_bStopCompletion = true;
	}

    m_cvCompletion.notify_all();
    for (auto& completionWorker : m_CompletionWorkers)
        completionWorker.join();
}

void ThreadPool::Enqueue(const boost::filesystem::path& strPathToFile)
{
    if (!m_bStopDetection)
    {
		size_t queueDepth;
        {
            std::unique_lock<std::mutex> lck(m_mtxDetectionQ);
            m_DetectionTasks.push(strPathToFile);
			queueDepth = m_DetectionTasks.size();
        }
        m_cvDetection.notify_one();

		// Go to wait state if there are too many tasks in the queue
		if (queueDepth >= m_MaxQueueDepth)
		{
			std::unique_lock<std::mutex> lck(this->m_mtxDetectionQ);

			m_cvEnumerator.wait(lck, [this]
			{
				return (this->m_DetectionTasks.size() <= this->m_EnumeratorWakeupThreshold);
			});
		}
    }
}
