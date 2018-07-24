#ifndef SAFETHREADPRIORITYQUEUE_H
#define SAFETHREADPRIORITYQUEUE_H

#include <time.h>
#include <cstddef>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <array>

#include "Bounded_MPMC_queue.h"


template <typename Tn>
class SafeThreadPriorityQueue
{
 public:
  static constexpr size_t kMaxPriorityCounter = 3;
  typedef MpmcBoundedQueue<Tn *> Queue;
  template <typename OtherTn> using unique_ptr = std::unique_ptr< OtherTn >;
  template <typename OtherTn> using shared_ptr = std::shared_ptr< OtherTn >;
  typedef std::mutex Mutex;
  typedef std::unique_lock<std::mutex> Locker;
  typedef std::condition_variable ConditionalVar;
  enum PriorityLevel : size_t {
    kLow  = 0,
    kMid  = 1,
    kHigh = 2
  };

  SafeThreadPriorityQueue()
    :    low_(),
         mid_(),
         hi_(),
         indexator_{ &low_, &mid_, &hi_ },
         priority_counter_( kMaxPriorityCounter ) {
  }
  void Enqueue( unique_ptr<Tn> && value, PriorityLevel priority_value );
  void Dequeue( unique_ptr<Tn> & out );
  void Dequeue( shared_ptr<Tn> & out );

 protected:
  void Enqueue( Tn * value, PriorityLevel priority_value );
  void Dequeue( Tn * & out );
  bool TryEnqueue( Tn * value, PriorityLevel priority_value );
  bool TryDequeue( Tn * & out );

 private:
  Queue low_;
  Queue mid_;
  Queue hi_;
  std::array< Queue *, 3 > indexator_;
  std::atomic<size_t> priority_counter_;
  Mutex mutex_priority_;
  Mutex mutex_deq_;
  Mutex mutex_enq_;
  ConditionalVar conditional_deq_;
  ConditionalVar conditional_enq_;
};

template <typename Tn>
bool SafeThreadPriorityQueue<Tn>::TryEnqueue( Tn * value, PriorityLevel priority_value ) {
  return indexator_[priority_value]->Enqueue( value );
}

template <typename Tn>
void SafeThreadPriorityQueue<Tn>::Enqueue( Tn * value, PriorityLevel priority_value ) {
  bool success = TryEnqueue( value, priority_value );
  if ( !success ) {
    Locker locker( mutex_enq_ );
    do {
      conditional_enq_.wait( locker );
      success = TryEnqueue( value, priority_value );
    } while (  !success  );
  } else {
  }
  conditional_deq_.notify_one();
}

template <typename Tn>
void SafeThreadPriorityQueue<Tn>::Enqueue( unique_ptr<Tn> && value, PriorityLevel priority_value ) {
  Enqueue( value.release(), priority_value );
}


template <typename Tn>
bool SafeThreadPriorityQueue<Tn>::TryDequeue( Tn * & out ) {
  bool hi_priority = 1;
  Locker locker( mutex_priority_ ); {
    if ( priority_counter_ ) {
      // выполняем задачу с высоким (high) приоритетом
      --priority_counter_;
    } else {
      // выполняем задачу со средним (middle) приоритетом
      priority_counter_ = kMaxPriorityCounter;
      hi_priority = 0;
    }
  } // конец блокировки
  if ( hi_priority ) {
    // выполняем задачу с высоким (high) приоритетом
    if ( !hi_.Dequeue( out ) ) {
      /// @warning ниже грязное решение - т.к. priority counter не защищён мьютексом, и может случится упс,
      ///          например, priority_counter_ может стать больше максимума. Но это для нас не принципиально.
      ++priority_counter_;
      if ( !mid_.Dequeue( out ) ) {
        return low_.Dequeue( out );
      }
    }
  } else {
    // выполняем задачу со средним (middle) приоритетом
    if ( !mid_.Dequeue( out ) ) {
      if ( !hi_.Dequeue( out ) ) {
        return low_.Dequeue( out );
      }
    }
  }
  return 1;
}



template <typename Tn>
void SafeThreadPriorityQueue<Tn>::Dequeue( Tn * & out ) {
  bool success = TryDequeue( out );
  if ( !success ) {
    do {
      {
        Locker locker( mutex_deq_ );
        conditional_deq_.wait( locker );
      }
      success = TryDequeue( out );
    } while ( !success );
  }
  conditional_enq_.notify_one();
}

template <typename Tn>
void SafeThreadPriorityQueue<Tn>::Dequeue( unique_ptr<Tn> & out ) {
  Tn * ptr;
  Dequeue( ptr );
  out.reset( ptr );
}

template <typename Tn>
void SafeThreadPriorityQueue<Tn>::Dequeue( shared_ptr<Tn> & out ) {
  Tn * ptr;
  Dequeue( ptr );
  out.reset( ptr );
}

#endif // SAFETHREADPRIORITYQUEUE_H
