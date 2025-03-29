#include "Common/Foundation/Util/TaskQueue.hpp"
#include "Common/Debug/Logger.hpp"

#include <future>
#include <vector>

using namespace Common::Foundation::Util;

void TaskQueue::AddTask(const std::function<BOOL()>& task) {
	mTaskQueue.push(task);
}

BOOL TaskQueue::ExecuteTasks(Common::Debug::LogFile* const pLogFile, UINT numThreads) {
	std::vector<std::future<BOOL>> threads;

	for (UINT i = 0; i < numThreads; ++i)
		threads.emplace_back(std::async(std::launch::async, &TaskQueue::ExecuteTask, this, pLogFile));

	BOOL status = TRUE;
	for (UINT i = 0; i < numThreads; ++i)
		status = status && threads[i].get();

	return status;
}

BOOL TaskQueue::ExecuteTask(Common::Debug::LogFile* const pLogFile) {
	while (TRUE) {
		std::function<BOOL()> task;
		{
			std::lock_guard<std::mutex> lock(mMutex);

			if (mTaskQueue.empty()) break;
			task = std::move(mTaskQueue.front());

			mTaskQueue.pop();
		}
		CheckReturn(pLogFile, task());
	}

	return TRUE;
}