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

    if (len > 0 && getSn_SR(DEBUG_SOCKET) == SOCK_ESTABLISHED)
    {
        send(DEBUG_SOCKET, (uint8_t *)buffer, len);
    }
}
