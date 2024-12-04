#include "tasks.h"

#include <Windows.h>

namespace
{
	int run(void* args)
	{
		ThreadPool* threadPool = static_cast<ThreadPool*>(args);
		while (true)
		{
			auto task = threadPool->GetTasksChannel().Pop();
			int res = task();
			if (res)
			{
				break;
			}
		}
		return 0;
	}
}

ThreadPool::ThreadPool(int numThreads) :
	m_numThreads(numThreads)
{
	for (int i = 0; i < m_numThreads; ++i)
	{
		thrd_t t;
		thrd_create(&t, run, this);
	}
}
ThreadPool::~ThreadPool()
{
	for (int i = 0; i < m_numThreads; ++i)
	{
		m_tasksChannel.Push([=]() {
			return 1;
		});
	}
}

void ThreadPool::RunTask(const std::function<void(void)>& task)
{
	m_tasksChannel.Push([=]() {
		task();
		return 0;
	});
}

MPSCChannel<std::function<int(void)>>& ThreadPool::GetTasksChannel()
{
	return m_tasksChannel;
}

std::string GetLastErrorAsString()
{
    //Get the error message ID, if any.
    DWORD errorMessageID = ::GetLastError();
    if(errorMessageID == 0) {
        return std::string(); //No error message has been recorded
    }
    
    LPSTR messageBuffer = nullptr;

    //Ask Win32 to give us the string version of that message ID.
    //The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
    
    //Copy the error message into a std::string.
    std::string message(messageBuffer, size);
    
    //Free the Win32's string's buffer.
    LocalFree(messageBuffer);
            
    return message;
}
