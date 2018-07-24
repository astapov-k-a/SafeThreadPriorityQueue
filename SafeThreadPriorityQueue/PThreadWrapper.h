#define _TIMESPEC_DEFINED
#include <pthread.h>
#include <atomic>
#include <system_error>

class PThreadWrapper;

class PFunction {
 public:
  template <typename ClassTn, typename FunTn, typename Arg> PFunction(FunTn f, ClassTn * c, Arg * arg);
  template <typename ClassTn, typename FunTn> PFunction(FunTn , ClassTn *);
  PFunction( const PFunction & ) = delete;
  PFunction( PFunction && ) = default;

  PFunction * DebugGetImplementation() { return implementation_; }

  PFunction & operator=( const PFunction &  ) = delete ;
  PFunction & operator=(       PFunction && to_move ) {
    implementation_ = to_move.implementation_;
    to_move.implementation_ = nullptr;
    return *this;
  }

  virtual ~PFunction() { delete implementation_; }
  virtual void Do( ) {
    return implementation_->Do( );
  }

 protected:
   PFunction() : implementation_( nullptr ) {}

 private:
  friend class PThreadWrapper;

  PFunction * implementation_;
};

template <typename ClassTn, typename FunTn, typename Arg>
class PFunctionImplementation : public PFunction {
 public:
  typedef ClassTn Class;
  typedef FunTn Method;

  PFunctionImplementation( FunTn fun, ClassTn * class_ptr, Arg * arg )
      :  PFunction(),
         type_(class_ptr),
         method_(fun),
         arg_( arg )  {
  }

  PFunctionImplementation( const PFunctionImplementation & ) = delete;
  PFunctionImplementation( PFunctionImplementation && ) = default;
  virtual ~PFunctionImplementation() { }

  PFunctionImplementation & operator=( const PFunctionImplementation & ) = delete;

  virtual void Do() override {
    (type_->*method_)( arg_ );
  }

 protected:
   Class * type_;
   Method method_;
   Arg arg_;
};

template <typename ClassTn, typename FunTn >
class PFunctionImplementationVoid : public PFunction {
public:
  typedef ClassTn Class;
  typedef FunTn Method;

  PFunctionImplementationVoid(FunTn fun, ClassTn * class_ptr)
      : PFunction(),
        type_(class_ptr),
        method_(fun)  {
  }

  PFunctionImplementationVoid(const PFunctionImplementationVoid &) = delete;
  PFunctionImplementationVoid(PFunctionImplementationVoid &&) = default;
  virtual ~PFunctionImplementationVoid() { }

  PFunctionImplementationVoid & operator=(const PFunctionImplementationVoid &) = delete;

  virtual void  Do() override {
    (type_->*method_)();
  }

protected:
  Class * type_;
  Method method_;
};


template <typename ClassTn, typename FunTn, typename Arg> 
PFunction::PFunction( FunTn f, ClassTn * c, Arg * arg ) {
  implementation_ = new (std::nothrow) PFunctionImplementation< ClassTn, FunTn, Arg >( f, c, arg );
}

template <typename ClassTn, typename FunTn>
PFunction::PFunction(FunTn f, ClassTn * c) {
  implementation_ = new (std::nothrow) PFunctionImplementationVoid<ClassTn, FunTn>( f, c );
}

class PThreadWrapper {
public:
  typedef std::atomic<bool> AtomicFlag;
  typedef void * (*Function) (void *);
  static constexpr int kNull = 0;

  PThreadWrapper();
  PThreadWrapper( const PThreadWrapper & ) = delete;
  PThreadWrapper(       PThreadWrapper & ) = delete;
  template < typename ClassTn, typename FunTn > PThreadWrapper( FunTn  fun, ClassTn * arg );
  /// @bug move-семантика не работает
  PThreadWrapper( PThreadWrapper && ) noexcept;
  /// @remarks Поведение деструктора отличается от std::thread - не прибивает поток, как должен.
  ///          Но нас устраивает.
  ~PThreadWrapper();

  PThreadWrapper & operator=( const PThreadWrapper & ) = delete;
  PThreadWrapper & operator=(       PThreadWrapper & ) = delete;
  /// @bug move-семантика не работает
  PThreadWrapper & operator=( PThreadWrapper && ) noexcept;

  bool joinable() const noexcept;
  void join();
  void detach();

 protected:
  static void Move( PThreadWrapper && source, PThreadWrapper & destination );

  pthread_t const &     thread() const { return thread_; }
  pthread_t       & get_thread()       { return thread_; }
  bool joined_detached() const { return joined_detached_;  }
  void joined_detached( bool value ) { joined_detached_ = value; }
  static void * Run( void * this_ptr ) {
    PThreadWrapper * ptr = (PThreadWrapper *) this_ptr;
    ptr->fun_.Do();
    return 0;
  }

 private:
   pthread_t thread_;
   AtomicFlag joined_detached_; // pthread_t зависит от реализации, и не всегда int
   PFunction fun_;
};


template < typename ClassTn, typename FunTn > PThreadWrapper::PThreadWrapper( FunTn  fun, ClassTn * class_ptr )
    :  joined_detached_( 0 ),
       fun_( fun, class_ptr ) {
  int error = pthread_create( &get_thread(), 0 , &PThreadWrapper::Run, this );
  if (error) {
    throw std::system_error( std::error_code(error, std::system_category()), "constructor");
  }
}
