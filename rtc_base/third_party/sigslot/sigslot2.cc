// sigslot.h: Signal/Slot classes
//
// Written by Sarah Thompson (sarah@telergy.com) 2002.
//
// License: Public domain. You are free to use this code however you like, with
// the proviso that the author takes on no responsibility or liability for any
// use.

#include "rtc_base/third_party/sigslot/sigslot2.h"

namespace sigslot2 {

#ifdef _SIGSLOT2_HAS_POSIX_THREADS

pthread_mutex_t* MultiThreadedGlobal::getMutex() {
  static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
  return &g_mutex;
}

#endif  // _SIGSLOT2_HAS_POSIX_THREADS

}  // namespace sigslot
