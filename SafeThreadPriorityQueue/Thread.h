#ifndef THREAD_H
#define THREAD_H

#include <atomic>
#include <thread>
#include <system_error>

#include "Definitions.h"

#ifndef USE_STD
#   include "PThreadWrapper.h"
#endif

/**
  * @class Thread
  * @author Astapov K.A.  +7(931)-29-17-0-16
  * @date 01.05.2018
  **/


class Thread
{
public:
  typedef std::atomic<bool> AtomicFlag;
#ifdef USE_STD
  typedef std::thread ThreadType;
#else
  typedef PThreadWrapper ThreadType;
#endif

  Thread();
  virtual ~Thread() {}
  //Thread( Thread && to_move ) = default;
  //Thread( const Thread & to_copy ) = delete;

  //Thread & operator=( const Thread & to_copy ) = default;

  virtual void Run() {printf("PURE VIRTUAL CALL");}
  virtual void HandleException( std::exception & ) {}
  virtual void HandleException( Exception & ) {}
  virtual void HandleException() {}
  virtual void EventAfterHandlingException() {}

  void Start();
  void Stop();
  void SetStopFlag();

 protected:
  INLINE AtomicFlag const &     running() const { return still_running_; }
  INLINE AtomicFlag       & get_running()       { return still_running_; }

  INLINE ThreadType const &     thread() const { return thread_; }
  INLINE ThreadType       & get_thread()       { return thread_; }

 private:
    void LaunchThread();

    AtomicFlag still_running_;
    ThreadType thread_;
};

INLINE Thread::Thread()
    : still_running_(false),
      thread_() {
}


INLINE void Thread::SetStopFlag() {
  get_running().store( false );
}

INLINE void Thread::Stop() {
  SetStopFlag();
  try {
    if ( !get_thread().joinable() ) {
      throw std::system("non-joinable thread");
    }
      get_thread().join();
  } catch ( std::system_error & se ) {
    //задача закончилась между проверкой joinable и join. упс... ничего не делаем. теряем время, примерно 5000-1000 тактов
    printf( "\n Thread::Stop system exception" );
  }
}

INLINE void Thread::Start() {
  get_thread().~ThreadType();
  ::new ( &get_thread() ) ThreadType( &Thread::LaunchThread, this );
}


INLINE void Thread::LaunchThread() {
  get_running().store( true );

  try {
    Run();
  } catch ( std::exception & e ) {
    printf("\nerror %s", e.what() );
    HandleException( e );
    EventAfterHandlingException();
  } catch ( Exception & e) {
    HandleException( e );
    EventAfterHandlingException();
  } catch(...) {
    HandleException( );
    EventAfterHandlingException();
  }

}


#endif // THREAD_H
