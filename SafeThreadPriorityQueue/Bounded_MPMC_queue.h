#ifndef BOUNDED_MPMC_QUEUE_H
#define BOUNDED_MPMC_QUEUE_H

#include <memory>
#include <atomic>
#include <cassert>


/// @class MpmcBoundedQueue
/// @author Дмитрий Вьюков aka Remark
/// @warning (Astapov K.A.)
///          Добавлена особенность для указателей: если для очереди сырых указателей
///          использовать move-семантику для запихивания в очередь unique_ptr, то у объекта,
///          который положен в очередь, теряется владелец. Т.е. ответственность за то, чтобы зачистить
///          такие объекты при ликвидации очереди или при извлечении такого указателя из очереди
///          несёт программист. С учётом особенности очереди это может быть очень полезным
template<typename T, size_t kBufferSize = 2<<16 >
class MpmcBoundedQueue {
  typedef MpmcBoundedQueue< T, kBufferSize > This;

  template <typename Tn> struct EnqueueFun {
  #if 0 // включите, если хотите, чтобы для MpmcBoundedQueue<не-указатель> Enqueue( unique_ptr && ) делал "hard copy". выключите, если хотите в этом случае видеть ошибку компиляции
    static bool Enqueue( std::unique_ptr<Tn> && to_move, This & enque ) {
      if ( !to_move ) {
        ASSERTION(0, "Access violation", Exception );
        return;
      }
      bool re = enque.Enqueue( *to_move );
      to_move.reset();
      return re;
    }
  #endif
  };

  template <typename Tn> struct EnqueueFun<Tn *> {
    static bool Enqueue( std::unique_ptr<Tn> && to_move, This & out ) {
      if ( out.Enqueue( to_move.get() ) ) {
        to_move.release();
        return true;
      }
      return false;
    }
  };

 public:
  MpmcBoundedQueue(size_t buffer_size)
     :  buffer_(new cell_t [buffer_size])
       ,buffer_mask_(buffer_size - 1)
  {
    assert((buffer_size >= 2) &&
           ((buffer_size & (buffer_size - 1)) == 0));
    for (size_t i = 0; i != buffer_size; i += 1) {
      buffer_[i].sequence_.store(i, std::memory_order_relaxed);
    }
    enqueue_pos_.store(0, std::memory_order_relaxed);
    dequeue_pos_.store(0, std::memory_order_relaxed);
  }  
  MpmcBoundedQueue() : MpmcBoundedQueue( kBufferSize ) {
  }
  ~MpmcBoundedQueue() {
    delete [] buffer_;
  }

  /// @brief Добавление в очередь с потерей ответственности за хранимые в указателе данные
  /// @warning Возможна утечка памяти!!! Использовать с пониманием
  bool Enqueue( std::unique_ptr< typename std::remove_pointer<T>::type > && ptr ) {
    return EnqueueFun<T>::Enqueue( std::move(ptr), *this );
  }
  bool Enqueue(T const& data) {
    cell_t* cell;
    size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
    for (;;) {
      cell = &buffer_[pos & buffer_mask_];
      size_t seq =
        cell->sequence_.load(std::memory_order_acquire);
      intptr_t dif = (intptr_t)seq - (intptr_t)pos;
      if (dif == 0) {
        if (enqueue_pos_.compare_exchange_weak
            (pos, pos + 1, std::memory_order_relaxed))
          break;
      } else {
        if (dif < 0) {
          return false;
        } else {
          pos = enqueue_pos_.load(std::memory_order_relaxed);
        }
      }
    };
    cell->data_ = data;
    cell->sequence_.store(pos + 1, std::memory_order_release);
    return true;
  }

  bool Dequeue(T& data) {
    cell_t* cell;
    size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
    for (;;) {
      cell = &buffer_[pos & buffer_mask_];
      size_t seq =
        cell->sequence_.load(std::memory_order_acquire);
      intptr_t dif = (intptr_t)seq - (intptr_t)(pos + 1);
      if (dif == 0) {
        if (dequeue_pos_.compare_exchange_weak
            (pos, pos + 1, std::memory_order_relaxed)) {
          break;
        }
      } else {
        if (dif < 0) {
          return false;
        } else {
          pos = dequeue_pos_.load(std::memory_order_relaxed);
        }
      }
    }
    data = cell->data_;
    cell->sequence_.store
      (pos + buffer_mask_ + 1, std::memory_order_release);
    return true;
  }

 private:
  struct cell_t {
    std::atomic<size_t>   sequence_;
    T                     data_;
  };
  static size_t const     cacheline_size = 64;
  typedef char            cacheline_pad_t [cacheline_size];
  cacheline_pad_t         pad0_;
  cell_t* const           buffer_;
  size_t const            buffer_mask_;
  cacheline_pad_t         pad1_;
  std::atomic<size_t>     enqueue_pos_;
  cacheline_pad_t         pad2_;
  std::atomic<size_t>     dequeue_pos_;
  cacheline_pad_t         pad3_;
  MpmcBoundedQueue(MpmcBoundedQueue const&);
  void operator = (MpmcBoundedQueue const&);
};

#endif // BOUNDED_MPMC_QUEUE_H
