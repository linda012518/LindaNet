#ifndef _CELL_TASK_H_
#define _CELL_TASK_H_

#include<thread>
#include<mutex>
#include<list>

#include<functional>

#include"Thread.hpp"

namespace linda {
	namespace io {
		//执行任务的服务类型
		class TaskServer
		{
		public:
			//所属serverid
			int serverId = -1;
		private:
			typedef std::function<void()> Task;
		private:
			//任务数据
			std::list<Task> _tasks;
			//任务数据缓冲区
			std::list<Task> _tasksBuf;
			//改变数据缓冲区时需要加锁
			std::mutex _mutex;
			//
			Thread _thread;
		public:
			//添加任务
			void addTask(Task task)
			{
				std::lock_guard<std::mutex> lock(_mutex);
				_tasksBuf.push_back(task);
			}
			//启动工作线程
			void Start()
			{
				_thread.Start(nullptr, [this](Thread* pThread) {
					OnRun(pThread);
				});
			}

			void Close()
			{
				///CELLLog_Info("TaskServer%d.Close begin", serverId);
				_thread.Close();
				//CELLLog_Info("TaskServer%d.Close end", serverId);
			}
		protected:
			//工作函数
			void OnRun(Thread* pThread)
			{
				while (pThread->isRun())
				{
					//从缓冲区取出数据
					if (!_tasksBuf.empty())
					{
						std::lock_guard<std::mutex> lock(_mutex);
						for (auto pTask : _tasksBuf)
						{
							_tasks.push_back(pTask);
						}
						_tasksBuf.clear();
					}
					//如果没有任务
					if (_tasks.empty())
					{
						Thread::Sleep(1);
						continue;
					}
					//处理任务
					for (auto pTask : _tasks)
					{
						pTask();
					}
					//清空任务
					_tasks.clear();
				}
				//处理缓冲队列中的任务
				for (auto pTask : _tasksBuf)
				{
					pTask();
				}
				//CELLLog_Info("TaskServer%d.OnRun exit", serverId);
			}
		};
	}
}
#endif // !_CELL_TASK_H_
