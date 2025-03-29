#pragma once

#include <functional>
#include <mutex>
#include <queue>

#include <Windows.h>

namespace Common::Debug {
	struct LogFile;
}

namespace Common::Foundation::Util {
	class TaskQueue {
	public:
		TaskQueue() = default;
		virtual ~TaskQueue() = default;

	public:
		void AddTask(const std::function<BOOL ()>& task);
		BOOL ExecuteTasks(Common::Debug::LogFile* const pLogFile, UINT numThreads);

	private:
		BOOL ExecuteTask(Common::Debug::LogFile* const pLogFile);

	private:
		std::queue<std::function<BOOL()>> mTaskQueue;
		std::mutex mMutex;
	};
}