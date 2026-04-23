#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <esp_http_server.h>

httpd_handle_t webserver();
void port();
void set_moisture(int value);

#endif
