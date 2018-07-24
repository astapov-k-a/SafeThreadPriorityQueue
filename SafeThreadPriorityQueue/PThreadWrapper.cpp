#include <utility>
#include <assert.h>
#include "PThreadWrapper.h"


PThreadWrapper::PThreadWrapper()
    :  joined_detached_( 0 ),
       fun_() {

}

PThreadWrapper::PThreadWrapper( PThreadWrapper && to_move ) noexcept {
  Move(std::move(to_move), *this);
}

PThreadWrapper::~PThreadWrapper() {
}

PThreadWrapper & PThreadWrapper::operator=( PThreadWrapper && to_move ) noexcept {
  Move ( std::move( to_move ), *this );
  return *this;
}

bool PThreadWrapper::joinable() const noexcept {
  return joined_detached();
}

void PThreadWrapper::join() {
  if ( !joined_detached() ) {
    int error = pthread_join( thread() , nullptr );
    if ( !error ) {
      joined_detached(1);
    } else {
      throw std::system_error(  std::error_code( error, std::system_category() ), "join"  );
    }
  } else {
  }
}

void PThreadWrapper::detach() {
  bool need_throw = 1;
  int error = EINVAL;
  if ( !joined_detached() ) {
    error = pthread_detach( thread() );
    if ( !error ) {
      joined_detached( 1 );
      need_throw = 0;
    }
  }
  if ( need_throw ) {
    throw std::system_error(  std::error_code( error, std::system_category() ), "detach"  );
  }
}

void PThreadWrapper::Move( PThreadWrapper && source, PThreadWrapper & destination ) {
  std::swap( source.get_thread(), destination.get_thread() );
  destination.joined_detached( source.joined_detached() );
  destination.fun_ = std::move( source.fun_ );
}
