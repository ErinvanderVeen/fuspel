#ifndef _H_LOG
#define _H_LOG

#define LOG_DEBUG 1

#define LOG_LEVEL 10

#if(LOG_LEVEL >= LOG_DEBUG)
#define log_debug(x)
#else
void log_debug(char*);
#endif

#endif
