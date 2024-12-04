#include <threads.h>
#include <queue>
#include <functional>
#include <string>

template <typename T>
class MPSCChannel
{
private:
	mtx_t m_mutex;
	cnd_t m_condition;
	std::queue<T> m_queue;

public:
	MPSCChannel()
	{
		mtx_init(&m_mutex, mtx_plain);
		cnd_init(&m_condition);
	}
	~MPSCChannel()
	{
		mtx_destroy(&m_mutex);
		cnd_destroy(&m_condition);
	}

	void Push(const T& item)
	{
		mtx_lock(&m_mutex);
		m_queue.push(item);
		mtx_unlock(&m_mutex);
		cnd_signal(&m_condition);
	}

	T Pop()
	{
		mtx_lock(&m_mutex);
		while (m_queue.empty())
		{
			cnd_wait(&m_condition, &m_mutex);
		}
		T res = m_queue.front();
		m_queue.pop();
		mtx_unlock(&m_mutex);
		cnd_signal(&m_condition);
		return res;
	}
};


class ThreadPool
{
private:
	int m_numThreads;
	MPSCChannel<std::function<int(void)>> m_tasksChannel;

public:
	ThreadPool(int numThreads);
	~ThreadPool();

	void RunTask(const std::function<void(void)>& task);

	MPSCChannel<std::function<int(void)>>& GetTasksChannel();
};

std::string GetLastErrorAsString();

