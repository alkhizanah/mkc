#ifndef FILEWATCH_H_
#define FILEWATCH_H_
#include "config.h"
// number of milliseconds that will be used to debounce the file_watcher 
// so that it doesn't retrigger builds too quick. 
#define DEBOUNCE_MS 100
// returns true on detecting change, false on error.
bool file_watcher(const Config& config);

#endif
