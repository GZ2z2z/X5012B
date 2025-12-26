#ifndef __COM_DEBUG_H__
#define __COM_DEBUG_H__

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// === 修改：包含 App_Network.h 以获取 SOCK_DEBUG 定义 ===
#include "socket.h"
#include "app_data.h" 

void debug_printf(const char *format, ...);

#endif /* __COM_DEBUG_H__ */
