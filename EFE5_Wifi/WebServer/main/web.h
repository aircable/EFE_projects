#ifndef web_h
#define web_h

#include <esp_http_server.h>


httpd_handle_t setup_server(void);
esp_err_t send_web_page(httpd_req_t *req);
esp_err_t get_req_handler(httpd_req_t *req);
esp_err_t led_on_handler(httpd_req_t *req);
esp_err_t led_off_handler(httpd_req_t *req);

#endif