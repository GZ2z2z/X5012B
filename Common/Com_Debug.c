// 文件：Com_Debug.c

#include "Com_Debug.h"
#include "Int_eth.h"

void debug_printf(const char *format, ...)
{
    char buffer[256];
    va_list args;

    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // <--- 2. 将 DEBUG_SOCKET 修改为 SOCK_DEBUG (或者你在 App_Network.h 里定义的那个名字)
    if (len > 0 && getSn_SR(DEBUG_SOCKET) == SOCK_ESTABLISHED)
    {
        send(DEBUG_SOCKET, (uint8_t *)buffer, len);
    }
}
