//#include <QCoreApplication>

#include "ThreadPool.h"


std::atomic<int> gCounter (0);
std::atomic<int> gOperationCounter(0);
int gIntResult[10240];
ThreadPool tpool(3);

typedef std::unique_ptr<Runnable> RunnablePtr;

class TestThread : public Thread {
 public:
  TestThread(int * target) : result( target ) {}

  virtual void Run() override {
    *result = 17;
  }

  virtual void HandleException( std::exception & e ) { printf("\nerror: %s", e.what() ); }
  virtual void HandleException( Exception &  e ) { printf("\nerror!!!"); }

 private:
  int * result;
};

class TestRunnable : public Runnable {
 public:
  TestRunnable( 
      std::atomic<int> * counter = &gCounter,
      std::atomic<int> * current_operation_ptr = &gOperationCounter,
      int * result_ptr = gIntResult )
      :  counter_( (*counter)++ ),
         current_operation_( current_operation_ptr ),
         result_( result_ptr ) {
  }

  virtual void Do() override {
    int pos = atomic_fetch_add( current_operation_, 1 );
    result_[pos] = counter_;
  }

 private:
  int counter_;
  std::atomic<int> * current_operation_;
  int * result_;
};

#define CREATE_UNIQUE(type, name, value) \
  std::unique_ptr<type> name ( new (std::nothrow) type ); \
  *name = value;

#define CREATE_UNIQUE_BASE(type, derived, name, ...) \
  std::unique_ptr<type> name ( new (std::nothrow) derived( __VA_ARGS__ ) );

bool TestThreadClass() {
  try {
    int result = 0;
    std::atomic<int> counter;
    TestThread tt(&result);
    tt.Start();
    std::this_thread::sleep_for( std::chrono::milliseconds(500) );
    tt.Stop();
    return result == 17;
  } catch (...) {
    return 0;
  }
}

bool TestWorker() {
  try {
    int result = 0;
    std::atomic<int> counter( 3 );
    std::atomic<int> operation_num( 0 );
    int results[1024];

    TestRunnable tr;
    tr.Do();
    std::unique_ptr<TestRunnable> trptr(  new (std::nothrow) TestRunnable(&counter, &operation_num, results)  );
    Worker tw;
    tw.Run(std::move(trptr));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    int res0 = results[0];
    return res0 == 3;
  }
  catch (...) {
    return 0;
  }
}

bool TestQueue() {
  SafeThreadPriorityQueue<int> test_queue;

  CREATE_UNIQUE(int, uu1, 22);
  test_queue.Enqueue(std::move(uu1), SafeThreadPriorityQueue<int>::PriorityLevel::kLow);
  std::unique_ptr<int> uur1;
  test_queue.Dequeue(uur1);
  CREATE_UNIQUE(int, uu2, 23);
  test_queue.Enqueue(std::move(uu2), SafeThreadPriorityQueue<int>::PriorityLevel::kMid);
  std::unique_ptr<int> uur2;
  test_queue.Dequeue(uur2);
  CREATE_UNIQUE(int, uu3, 24);
  test_queue.Enqueue(std::move(uu3), SafeThreadPriorityQueue<int>::PriorityLevel::kHigh);
  std::unique_ptr<int> uur3;
  test_queue.Dequeue(uur3);


  SafeThreadPriorityQueue<int> test_queue2;
  CREATE_UNIQUE(int, u1, 3);
  CREATE_UNIQUE(int, u2, 4);
  CREATE_UNIQUE(int, u3, 5);
  CREATE_UNIQUE(int, u4, 6);
  CREATE_UNIQUE(int, u5, 5);
  CREATE_UNIQUE(int, u6, 6);
  CREATE_UNIQUE(int, u7, 7);
  CREATE_UNIQUE(int, u8, 8);
  CREATE_UNIQUE(int, u9, 9);
  CREATE_UNIQUE(int, uA, 0);
  CREATE_UNIQUE(int, uB, 11);
  CREATE_UNIQUE(int, uC, 12);
  CREATE_UNIQUE(int, uD, 13);

  test_queue2.Enqueue(std::move(u1), SafeThreadPriorityQueue<int>::PriorityLevel::kLow);
  test_queue2.Enqueue(std::move(u2), SafeThreadPriorityQueue<int>::PriorityLevel::kMid);
  test_queue2.Enqueue(std::move(u3), SafeThreadPriorityQueue<int>::PriorityLevel::kHigh);
  test_queue2.Enqueue(std::move(u4), SafeThreadPriorityQueue<int>::PriorityLevel::kLow);

  std::unique_ptr<int> ur1, ur2, ur3, ur4, ur5, ur6, ur7, ur8, ur9, urA, urB, urC, urD;
  test_queue2.Dequeue(ur1);
  test_queue2.Dequeue(ur2);
  test_queue2.Dequeue(ur3);
  test_queue2.Dequeue(ur4);

  test_queue2.Enqueue(std::move(u5), SafeThreadPriorityQueue<int>::PriorityLevel::kHigh);
  test_queue2.Enqueue(std::move(u6), SafeThreadPriorityQueue<int>::PriorityLevel::kMid);
  test_queue2.Enqueue(std::move(u7), SafeThreadPriorityQueue<int>::PriorityLevel::kHigh);
  test_queue2.Enqueue(std::move(u8), SafeThreadPriorityQueue<int>::PriorityLevel::kLow);
  test_queue2.Enqueue(std::move(uC), SafeThreadPriorityQueue<int>::PriorityLevel::kMid);
  test_queue2.Enqueue(std::move(u9), SafeThreadPriorityQueue<int>::PriorityLevel::kHigh);
  test_queue2.Enqueue(std::move(uA), SafeThreadPriorityQueue<int>::PriorityLevel::kHigh);
  test_queue2.Enqueue(std::move(uD), SafeThreadPriorityQueue<int>::PriorityLevel::kMid);
  test_queue2.Enqueue(std::move(uB), SafeThreadPriorityQueue<int>::PriorityLevel::kHigh);
  test_queue2.Dequeue(ur5);
  test_queue2.Dequeue(ur6);
  test_queue2.Dequeue(ur7);
  test_queue2.Dequeue(ur8);
  test_queue2.Dequeue(ur9);
  test_queue2.Dequeue(urA);
  test_queue2.Dequeue(urB);
  test_queue2.Dequeue(urC);
  test_queue2.Dequeue(urD);
  // 22 23 24; 5, 4, 3, 6
         //  5, 7, 6, 9, 0, 11, 12, 13, 8
  return (*uur1 == 22) &&
         (*uur2 == 23) &&
         (*uur3 == 24) &&
         (*ur1 == 5) &&
         (*ur2 == 4) &&
         (*ur3 == 3) &&
         (*ur4 == 6) &&

         (*ur5 == 5) &&
         (*ur6 == 7) &&
         (*ur7 == 6) &&
         (*ur8 == 9) &&
         (*ur9 == 0) &&
         (*urA == 11) &&
         (*urB == 12) &&
         (*urC == 13) &&
         (*urD == 8)  ;
}

bool TestSimpleThreadPool() {
  std::atomic<int> counter ( 0 );
  std::atomic<int> current_op ( 0 );
  int results[1024];
  ThreadPool tpl(1);

  tpl.Enqueue(RunnablePtr(new (std::nothrow) TestRunnable(&counter, &current_op, results) ), ThreadPool::PriorityLevel::kLow);
  tpl.Enqueue(RunnablePtr(new (std::nothrow) TestRunnable(&counter, &current_op, results) ), ThreadPool::PriorityLevel::kHigh);
  tpl.Enqueue(RunnablePtr(new (std::nothrow) TestRunnable(&counter, &current_op, results) ), ThreadPool::PriorityLevel::kMid);
  tpl.Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  tpl.Stop();
  return (results[0] == 1) && (results[1] == 2) && (results[2] == 0);
}

bool TestSimpleThreadPoolStop() {
  std::atomic<int> counter ( 0 );
  std::atomic<int> current_op ( 0 );
  int results[1024];
  ThreadPool tpl(1);

  tpl.Enqueue(RunnablePtr(new (std::nothrow) TestRunnable(&counter, &current_op, results) ), ThreadPool::PriorityLevel::kLow);
  tpl.Enqueue(RunnablePtr(new (std::nothrow) TestRunnable(&counter, &current_op, results) ), ThreadPool::PriorityLevel::kHigh);
  tpl.Enqueue(RunnablePtr(new (std::nothrow) TestRunnable(&counter, &current_op, results) ), ThreadPool::PriorityLevel::kMid);
  tpl.Start();
  tpl.Stop();
  return (results[0] == 1) && (results[1] == 2) && (results[2] == 0);
}

bool CompareArray( int * results, std::vector<int> & patterns ) {
  bool re = 1;
  typedef std::vector<int>::const_iterator Iterator;
  for ( Iterator i = patterns.begin(); i != patterns.end(); ++i ) {
    int * base = results - 1;
    bool local = 0;
    for ( size_t index = patterns.size() ; index; --index ) {
      local |= ( base[index] == *i );
    }
    re &= local;
  }
  return re;
}

bool TestThreadPool() {
  std::atomic<int> counter (1);
  std::atomic<int> current_op (0);
  int results[1024];
  ThreadPool test_queue(4);

  CREATE_UNIQUE_BASE(Runnable, TestRunnable, u1, &counter, &current_op, results);
  CREATE_UNIQUE_BASE(Runnable, TestRunnable, u2, &counter, &current_op, results);
  CREATE_UNIQUE_BASE(Runnable, TestRunnable, u3, &counter, &current_op, results);
  CREATE_UNIQUE_BASE(Runnable, TestRunnable, u4, &counter, &current_op, results);
  CREATE_UNIQUE_BASE(Runnable, TestRunnable, u5, &counter, &current_op, results);
  CREATE_UNIQUE_BASE(Runnable, TestRunnable, u6, &counter, &current_op, results);
  CREATE_UNIQUE_BASE(Runnable, TestRunnable, u7, &counter, &current_op, results);
  CREATE_UNIQUE_BASE(Runnable, TestRunnable, u8, &counter, &current_op, results);
  CREATE_UNIQUE_BASE(Runnable, TestRunnable, u9, &counter, &current_op, results);
  CREATE_UNIQUE_BASE(Runnable, TestRunnable, uA, &counter, &current_op, results);
  CREATE_UNIQUE_BASE(Runnable, TestRunnable, uB, &counter, &current_op, results);
  CREATE_UNIQUE_BASE(Runnable, TestRunnable, uC, &counter, &current_op, results);
  CREATE_UNIQUE_BASE(Runnable, TestRunnable, uD, &counter, &current_op, results);

  test_queue.Enqueue(std::move(u1), ThreadPool::PriorityLevel::kLow);
  test_queue.Enqueue(std::move(u2), ThreadPool::PriorityLevel::kMid);
  test_queue.Enqueue(std::move(u3), ThreadPool::PriorityLevel::kHigh);
  test_queue.Enqueue(std::move(u4), ThreadPool::PriorityLevel::kLow);

  test_queue.Enqueue(std::move(u5), ThreadPool::PriorityLevel::kHigh);
  test_queue.Enqueue(std::move(u6), ThreadPool::PriorityLevel::kMid);
  test_queue.Enqueue(std::move(u7), ThreadPool::PriorityLevel::kHigh);
  test_queue.Enqueue(std::move(u8), ThreadPool::PriorityLevel::kLow);
  test_queue.Enqueue(std::move(u9), ThreadPool::PriorityLevel::kHigh);
  test_queue.Enqueue(std::move(uA), ThreadPool::PriorityLevel::kHigh);
  test_queue.Enqueue(std::move(uB), ThreadPool::PriorityLevel::kMid);
  test_queue.Enqueue(std::move(uC), ThreadPool::PriorityLevel::kMid);
  test_queue.Enqueue(std::move(uD), ThreadPool::PriorityLevel::kHigh);
  // 22 23 24; 5, 4, 3, 6
  //  5, 7, 6, 9, 0, 11, 12, 13, 8

  //  3,  5,  7,  2, 
  //  9, 10, 13,  6,
  // 11, 12,
  //  1,  4, 8, 

  test_queue.Start();
  std::this_thread::sleep_for(std::chrono::milliseconds(1200));
  test_queue.Stop();
  bool re = 1;
  typedef std::vector<int> Vect;
  Vect v1 {  3,  5,  7 };
  Vect v2 {  9, 10, 13 };
  Vect v3 {  6, 11, 12 };
  Vect v4 {  1,  4,  8 };
  re &= CompareArray( results, v1 );
  re &= results[3] == 2;
  re &= CompareArray( results +  4, v2 );
  re &= CompareArray( results +  7, v3 );
  re &= CompareArray( results + 10, v4 );
  return re;
}

#ifndef USE_STD
struct X {
  X() : a(0) {}
  int a;
  void Fun() {
    a = 1;
  }
} xx, yy;

bool TestPFunction() {
  PFunction fn( &X::Fun, &xx );
  fn.Do(  );
  return xx.a == 1;
}

void PFun() {}

bool TestPThreadWrapper() {
  PThreadWrapper wr( &X::Fun, &yy );
  wr.join();
  return yy.a == 1;
}
#endif

#define TEST( TestName, Fun ) \
        printf( "\nTEST: %s %s\n", TestName, Fun() ? "success" : "fail" );

int main( int argc, char *argv[] )
{
  printf("main\n");
#ifndef USE_STD
  TestPThreadWrapper();
  TestPFunction();
#endif
  TEST( "class ThreadPool Stop", TestSimpleThreadPoolStop);
  TEST( "class Thread", TestThreadClass );
  TEST( "class Worker", TestWorker );
  TEST( "class Queue" , TestQueue );
  TEST( "class ThreadPool (simple)", TestSimpleThreadPool );
  TEST( "class ThreadPool"         , TestThreadPool );

  printf("\nfinish main");

  return 0;
}
