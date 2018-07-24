#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <thread>
#include <vector>

#include "Thread.h"
#include "SafeThreadPriorityQueue.h"
#include "Runnable.h"

class Worker : public Thread {
 public:
  typedef SafeThreadPriorityQueue< Runnable > Queue;
  typedef std::vector< Worker > WorkersVector;

  bool Initialize( Queue * ptr );

  virtual void Run();
  /// @warning нет контроля от повторного вызова в случае, если предыдущая задача не завершена
  /// @remark тестовая функция
  virtual void Run( std::unique_ptr<Runnable> && to_run );

 protected:
  std::unique_ptr<Runnable> const &     task() const { return task_; }
  std::unique_ptr<Runnable>       & get_task()       { return task_; }
  Queue       & get_queue()       { return *queue_; }
  Queue const &     queue() const { return *queue_; }
  void set_queue( Queue * ptr ) { queue_ = ptr; }

 private:
  std::unique_ptr<Runnable> task_;
  Queue * queue_;
};

class ThreadPool {
 public:
  typedef SafeThreadPriorityQueue< Runnable > Queue;
  typedef std::vector< std::unique_ptr< Worker > > WorkersVector;
  typedef Queue::PriorityLevel PriorityLevel;
  typedef std::atomic<bool> AtomicFlag;

  ThreadPool( size_t size );

  bool Enqueue( std::unique_ptr<Runnable> && value, PriorityLevel priority_value );
  void Stop();
  void Start();

 protected:
  WorkersVector       & get_threads_vect()       { return threadsvect_; }
  WorkersVector const &     threads_vect() const { return threadsvect_; }
  Queue       & get_queue()       { return queue_; }
  Queue const &     queue() const { return queue_; }
  void SetStopFlag();
  void RecreateWorkers();
  bool InitializeWorkers();

 private:
  WorkersVector threadsvect_;
  Queue queue_;
  AtomicFlag must_stop_;
};

#endif // THREADPOOL_H
