#ifndef __COM_DEBUG_H__
#define __COM_DEBUG_H__

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "socket.h"
#include "app_data.h" 

void debug_printf(const char *format, ...);

#endif /* __COM_DEBUG_H__ */
