#include "ThreadPool.h"

////////////////////////////////////////////////////////////////////////////////
///
/// Worker
///
////////////////////////////////////////////////////////////////////////////////

/// @brief Worker::Run
void Worker::Run() {
  for (; running() ; ) {
    get_queue().Dequeue( get_task() );
    try {
      if ( get_task() ) {
        get_task()->Do();
      } else {
        break; // пустые задачи пропускаются. добавление пустой задачи используется для того, чтобы вывести поток из ступора при операции stop()
      }
    } catch ( std::exception & e ) {
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
}


void Worker::Run( std::unique_ptr<Runnable> && to_run ) {
  get_task().reset( to_run.release() );
  try {
    if ( get_task() ) {
      get_task()->Do();
    } else {
      return; // пустые задачи пропускаются. добавление пустой задачи используется для того, чтобы вывести поток из ступора при операции stop()
    }
  } catch ( std::exception & e ) {
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

bool Worker::Initialize( Queue * ptr ) {
  set_queue( ptr );
  return 1;
}

////////////////////////////////////////////////////////////////////////////////
///
/// ThreadPool
///
////////////////////////////////////////////////////////////////////////////////

ThreadPool::ThreadPool( size_t size )
    :  threadsvect_( size ),
       queue_(  ),
       must_stop_( 0 ) {
  RecreateWorkers();
  bool success = InitializeWorkers();
  if ( !success ) printf("Error: workers not initialized.");
}


bool ThreadPool::Enqueue(
    std::unique_ptr<Runnable> && value,
    PriorityLevel priority_value ) {
  if ( !must_stop_ ) {
    get_queue().Enqueue( std::move(value), priority_value );
    return 1;
  } else {
    return 0;
  }
}


void ThreadPool::Stop( ) {
  SetStopFlag();
  std::atomic_thread_fence(std::memory_order_seq_cst);
  size_t sz = get_threads_vect().size()*4;
  for (  ; sz ; --sz ) { // забиваем пустые задачи, чтобы не "подвесить" потоки
    get_queue().Enqueue( std::unique_ptr<Runnable>(), Queue::kLow );
  }
  std::atomic_thread_fence(std::memory_order_seq_cst);
  for ( WorkersVector::iterator iteratr = get_threads_vect().begin() ;
        iteratr != get_threads_vect().end();
        ++ iteratr ) {
    (*iteratr)->Stop(); // останавливаем
  }
  std::atomic_thread_fence(std::memory_order_seq_cst);

  get_threads_vect().clear();
  get_threads_vect().resize( sz );
  RecreateWorkers();
}


void ThreadPool::SetStopFlag( ) {
  must_stop_ = 1;
  for ( auto iteratr = get_threads_vect().begin() ;
        iteratr != get_threads_vect().end();
        ++ iteratr ) {
    (*iteratr)->SetStopFlag();
  }
}


void ThreadPool::Start( ) {
  for ( WorkersVector::iterator iteratr = get_threads_vect().begin() ;
        iteratr != get_threads_vect().end();
        ++ iteratr ) {
    (*iteratr)->Start(); // запускаем
  }
}

void ThreadPool::RecreateWorkers( ) {
  for ( auto iteratr = get_threads_vect().begin() ;
        iteratr != get_threads_vect().end();
        ++ iteratr ) {
    iteratr->reset( new (std::nothrow) Worker );
  }
}

bool ThreadPool::InitializeWorkers() {

  for ( auto iteratr = get_threads_vect().begin() ;
        iteratr != get_threads_vect().end();
        ++ iteratr ) {
    if ( ! (*iteratr)->Initialize( &queue_ ) ) return 0;
  }
  return 1;
}
