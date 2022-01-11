#ifndef _TOOS_TIMES_H_
#define _TOOS_TIMES_H_

#include <stdint.h>

#if __cplusplus >= 201103L
#define CONSTEXPR constexpr
#else
#define CONSTEXPR
#endif
namespace Tls {
class Times {
  public:
    static int64_t getSystemTimeMs();
    static int64_t getSystemTimeUs();
    static int64_t getSystemTimeNs();
};

}

#endif /*_TOOS_TIMES_H_*/
