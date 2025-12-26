// 文件：Com_Debug.c

#include "Com_Debug.h"
#include "App_Network.h"  // <--- 1. 必须包含这个头文件，才能获取 Socket ID 定义

void debug_printf(const char *format, ...)
{
    char buffer[256];
    va_list args;

    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // <--- 2. 将 DEBUG_SOCKET 修改为 SOCK_DEBUG (或者你在 App_Network.h 里定义的那个名字)
    if (len > 0 && getSn_SR(SOCK_DEBUG) == SOCK_ESTABLISHED)
    {
        send(SOCK_DEBUG, (uint8_t *)buffer, len);
    }
}
