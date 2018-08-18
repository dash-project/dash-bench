#include <util/benchdata.h>

void detail::deallocate(void * p) {
#ifdef ENABLE_MEMKIND
  if (hbw_check_available()) {
    hbw_free(p);
  }
  else {
    std::free(p);
  }
#else
  std::free(p);
#endif
}
