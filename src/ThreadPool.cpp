#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t threadNum,const std::string & threadType) : m_stop(false),m_threadType(threadType)
{
    // 启动threadnum个线程，每个线程将阻塞在条件变量上。
    for (size_t i = 0; i < threadNum; ++i)
    {
        // 用lambda函创建线程。
        m_threads.emplace_back([this]
        {
            printf("create %s thread(%ld).\n",m_threadType.data(),syscall(SYS_gettid));     // 显示线程ID。

			while (m_stop==false)
			{
				std::function<void()> task;       // 用于存放出队的元素。

				{   // 锁作用域的开始。 ///////////////////////////////////
					std::unique_lock<std::mutex> lock(this->m_mutex);

					// 等待生产者的条件变量。
					this->m_condition.wait(lock, [this] 
                    { 
                        return ((this->m_stop==true) || (this->m_taskQueue.empty()==false));
                    });

                    // 在线程池停止之前，如果队列中还有任务，执行完再退出。
					if ((this->m_stop==true)&&(this->m_taskQueue.empty()==true)) return;

                    // 出队一个任务。
					task = std::move(this->m_taskQueue.front());
					this->m_taskQueue.pop();
				}   // 锁作用域的结束。 ///////////////////////////////////

                // printf("%s(%ld) execute task\n",m_threadType.data(),syscall(SYS_gettid));
				task();  // 执行任务。
			} });
    }
}

void ThreadPool::addTask(std::function<void()> task)
{
    { // 锁作用域的开始。 ///////////////////////////////////
        std::lock_guard<std::mutex> lock(m_mutex);
        m_taskQueue.push(task);
    } // 锁作用域的结束。 ///////////////////////////////////

    m_condition.notify_one(); // 唤醒一个线程。
}
//获取线程池大小
size_t ThreadPool::size()
{
    return m_threads.size();
}

// 停止线程
void ThreadPool::stopThread()
{ 
    if(m_stop==true) {
        return;
    }
    m_stop = true;

    m_condition.notify_all(); // 唤醒全部的线程。

    // 等待全部线程执行完任务后退出。
    for (std::thread &th : m_threads){
        th.join();
    }
}


ThreadPool::~ThreadPool()
{
    stopThread();
}
