#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include<mutex>
#include<queue>
#include<functional>
#include<future>
#include<thread>
#include<utility>
#include<vector>

using namespace std;
template<typename T>
class SafeQueue
{
private:
	queue<T> m_queue;
	mutex m_mutex;
public:
	SafeQueue() {}
	SafeQueue(SafeQueue&& other) {}
	SafeQueue(const SafeQueue<T>& other) = delete;
	SafeQueue& operator = (const SafeQueue<T> & other) = delete;
	~SafeQueue() {}

	bool empty()
	{
		unique_lock<mutex> lock(m_mutex);
		return m_queue.empty();
	}
	int size()
	{
		unique_lock<mutex> lock(m_mutex);
		return m_queue.size();
	}
	void enqueue(T& t)
	{
		unique_lock<mutex> lock(m_mutex);
		m_queue.emplace(t);
	}
	bool dequeue(T& t)
	{
		unique_lock<mutex> lock(m_mutex);
		if (m_queue.empty())
			return false;

		t = move(m_queue.front());
		
		m_queue.pop();
		return true;
	}
};
class ThreadPool
{
private:
	
	class ThreadWorker
	{
	private:
		int m_id;
		ThreadPool* m_pool;

	public:
		ThreadWorker(ThreadPool* pool, const int id) :m_pool(pool), m_id(id)
		{
		}
		void operator()()
		{
			function<void()> func;
			bool dequeued;

			while (!m_pool->m_shutdown)
			{
				{
					unique_lock<mutex> lock(m_pool->m_condition_mutex);
					if (m_pool->m_queue.empty())
					{
						m_pool->m_condition_lock.wait(lock);
					}
					dequeued = m_pool->m_queue.dequeue(func);
				
				}

				if (dequeued)
				{
					func();
				}
			}
			
		}

	};
	bool m_shutdown;
	SafeQueue<function<void()>> m_queue;
	vector<thread>m_threads;
	mutex m_condition_mutex;
	condition_variable m_condition_lock;
	thread manager;
public:
	ThreadPool(const int min, int max,const int n_threads = 4):m_threads(vector<thread>(n_threads)),m_shutdown(false)
	{
		
	}

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool(ThreadPool&&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	ThreadPool& operator=(ThreadPool&&) = delete;

	void Init()
	{
		for (int i = 0; i < m_threads.size(); ++i)
		{
			m_threads.at(i) = thread(ThreadWorker(this, i));
		}
	}
	void ShutDown()
	{
		m_shutdown = true;
		m_condition_lock.notify_all();

		for (int i = 0; i < m_threads.size(); ++i)
		{
			if (m_threads.at(i).joinable())
			{
				m_threads.at(i).join();
			}
		}
		if (manager.joinable())
		{
			manager.join();
		}
	}
	template<typename F,typename...Args>
	auto submit(F&& f, Args&&...args)->future<decltype(f(args...))>
	{
		function<decltype(f(args...))()> func = bind(forward<F>(f), forward<Args>(args)...);
		auto task_ptr = make_shared<packaged_task<decltype(f(args...))()>>(func);
	
		function<void()> wrapper_func = [task_ptr = task_ptr]()
		{
			(*task_ptr)();
		};

		
		m_queue.enqueue(wrapper_func);

		m_condition_lock.notify_one();

		return task_ptr->get_future();
	}

};
#endif
