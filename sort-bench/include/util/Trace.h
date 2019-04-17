#ifndef TRACE_H__INCLUDED
#define TRACE_H__INCLUDED

#include <vector>

namespace sortbench {

void reset_trace(void);
void flush_trace(std::vector<unsigned> const& samples, int myrank);

}  // namespace sortbench
#endif
