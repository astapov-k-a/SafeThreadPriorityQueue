#ifndef QUEUE_H
#define QUEUE_H

#include <new>
#include <mutex>


template < typename Tn, typename... Args > Tn * Allocate( Args&&... args ) {
  return new (std::nothrow) Tn( std::forward<Args>(args)... );
}
template < typename Tn > void Deallocate( Tn * ptr ) {
  delete ptr;
}

namespace internal {

template < typename Tn > struct QueueElement {
  typedef Tn Type;
  typedef QueueElement< Tn > This;
  
  This * previous;
  This * next;
  Type * data;

  QueueElement() :
    previous( nullptr ),
    next( nullptr ),
    data( nullptr ) {
  }
};

} // namespace internal

/**
 * @brief потокобезопасная очередь с внешним мьютексом
 */
template < typename Tn >
class Queue
{
 public:
  typedef Tn Type;
  typedef Queue<Tn> This;
  typedef std::mutex Mutex;
  typedef std::lock_guard< std::mutex > Locker;
  typedef internal::QueueElement< Type > QueueElement;

  Queue( Mutex * mutex_value );
  ~Queue( );

  template <typename Callback> void DoForAllUnsafe( Callback&& dofun );

 private:
  Mutex * mutex_;
  QueueElement * first_;
  QueueElement * last_;
  static QueueElement invalid;
};

template < typename Tn > internal::QueueElement<Tn> Queue<Tn>::invalid;

template < typename Tn > Queue<Tn>::Queue( Mutex * mutex_value ) {
  first_ = &invalid;
  last_ = &invalid;
  mutex_ = mutex_value;
}

template < typename Tn > Queue<Tn>::~Queue( ) {
  {
    Locker lock( mutex_ );
    DoForAllUnsafe( [](QueueElement * ptr ) -> void {
      Deallocate( ptr );
    }  );
  }
}


template < typename Tn > template < typename Callback >
void Queue<Tn>::DoForAllUnsafe( Callback&& dofun ) {
  QueueElement * current = first_;
  while ( current != &invalid ) {
    dofun( current );
  }
}

#endif // QUEUE_H
