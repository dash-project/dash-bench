#ifndef CAFCLOCK_INCLUDED
#define CAFCLOCK_INCLUDED

bool cafchecktime(
    int & nrep,
    const std::chrono::microseconds & time,
    const std::chrono::microseconds & targettime);

#endif
