/*
Simple HTTP Parser for ehScripts/WebSocketServerC
*/

#include <string.h>

typedef struct HTTPRequest
{
    char sec_websocket_key[24];
    char upgrade[12];
} HTTPRequest;

typedef enum header
{
    Upgrade,
    Sec_WebSocket_Key,
    NO_HEADER
} header;

HTTPRequest parseHTTP(char buff[])
{
    HTTPRequest request;
    int lineNum = 0;
    int length = strlen(buff);
    int hasStartedReadingLine = 0;
    int hasStartedReadingValue = 0;
    int headerValueSize;
    char headerValue[256];
    header currentHeader;
    for (int index = 0; index < length; index++)
    {
        char ch = buff[index];
        if (ch == '\n')
        {
            lineNum++;
            hasStartedReadingLine = 0;
            hasStartedReadingValue = 0;
            continue;
        };
        if (lineNum < 1)
        {
            continue;
        };
        if (hasStartedReadingLine)
        {
            if (hasStartedReadingValue)
            {
                switch (ch)
                {
                case '\r':
                    switch (currentHeader)
                    {
                    case Upgrade:
                        printf("%s\n", headerValue);
                        memmove(request.upgrade, headerValue, 12);
                        break;
                    case Sec_WebSocket_Key:
                        memmove(request.sec_websocket_key, headerValue, 24);
                        break;
                    case NO_HEADER:
                        break;
                    };
                    currentHeader = NO_HEADER;
                    break;
                default:
                    headerValue[headerValueSize] = ch;
                    headerValueSize++;
                    break;
                };
            }
            else if (ch == ':')
            {
                hasStartedReadingValue = 1;
                headerValueSize = 0;
                memset(headerValue, 0, sizeof(headerValue));
                index++;
            };
        }
        else
        {
            hasStartedReadingLine = 1;
            switch (ch)
            {
            case 'U':
                currentHeader = Upgrade;
                break;
            case 'S':
                currentHeader = Sec_WebSocket_Key;
                break;
            };
        };
    };
    return request;
};