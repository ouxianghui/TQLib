//
//  sigslot2.hpp
//  rcv
//
//  Created by Jackie Ou on 2023/03/06.
//  Copyright © 2023 RingCentral. All rights reserved.
//

#include "sigslot2.hpp"

namespace sigslot2 {

#ifdef _SIGSLOT2_HAS_POSIX_THREADS

pthread_mutex_t* MultiThreadedGlobal::getMutex() {
  static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
  return &g_mutex;
}

#endif  // _SIGSLOT2_HAS_POSIX_THREADS

}  // namespace rcv
