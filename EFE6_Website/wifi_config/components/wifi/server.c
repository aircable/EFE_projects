#include <stdio.h>
#include <sys/param.h>

#include "connect.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "mdns.h"
#include "toggleLed.h"
#include "cJSON.h"
#include "pushBtn.h"
#include "esp_spiffs.h"
#include "esp_vfs.h"

#include "sdkconfig.h"
#include "server.h"

const static char* TAG = "SERVER";

/* Max length a file path can have on storage */
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + CONFIG_LITTLEFS_OBJ_NAME_LEN)
// Scratch buffer size 4017, max MTU of the wifi driver on ESP32
// but maximum 802.11 MTU is 2304 bytes
#define SCRATCH_BUFSIZE ( 2304 / 2 )

typedef struct {
    /* Base path of file storage */
    char base_path[ ESP_VFS_PATH_MAX + 1 ];
    /* Scratch buffer for temporary storage during file transfer */
    char scratch[ SCRATCH_BUFSIZE ];
} file_server_data_t;

#ifdef CONFIG_ESP_WIFI_TASK_PINNED_TO_CORE_0
#   define WIFI_TASK_PINNED_TO_CORE              (0)
#elif CONFIG_ESP_WIFI_TASK_PINNED_TO_CORE_1
#   define WIFI_TASK_PINNED_TO_CORE              (1)
#else
#   define WIFI_TASK_PINNED_TO_CORE              (tskNO_AFFINITY)
#endif

httpd_handle_t server = NULL;
// REST context
file_server_data_t *server_data = NULL;

#define IS_FILE_EXT(filename, ext) \
    (strcasecmp(&filename[strlen(filename) - sizeof(ext) + 1], ext) == 0)

/* Set HTTP response content type according to file extension */
esp_err_t 
set_content_type_from_file(httpd_req_t *req, const char *filename) {
    
    if (IS_FILE_EXT(filename, ".pdf")){
        return httpd_resp_set_type(req, "application/pdf");
    } else if (IS_FILE_EXT(filename, ".html") || IS_FILE_EXT(filename, ".html.gz")){
        return httpd_resp_set_type(req, "text/html");
    } else if (IS_FILE_EXT(filename, ".jpeg")){
        return httpd_resp_set_type(req, "image/jpeg");
    } else if (IS_FILE_EXT(filename, ".ico")){
        return httpd_resp_set_type(req, "image/x-icon");
    } else if (IS_FILE_EXT(filename, ".js") || IS_FILE_EXT(filename, ".js.gz")){
        return httpd_resp_set_type(req, "application/javascript");
    } else if (IS_FILE_EXT(filename, ".css") || IS_FILE_EXT(filename, ".css.gz")){
        return httpd_resp_set_type(req, "text/css");
	} else if (IS_FILE_EXT(filename, ".png")){
        return httpd_resp_set_type(req, "image/png");
    } else if (IS_FILE_EXT(filename, ".bmp")){
        return httpd_resp_set_type(req, "image/bmp");
    } else if (IS_FILE_EXT(filename, ".ico")){
        return httpd_resp_set_type(req, "image/x-icon");
    } else if (IS_FILE_EXT(filename, ".svg")){
        return httpd_resp_set_type(req, "text/xml");
    }
    /* This is a limited set only */
    /* For any other type always set as plain text */
    return httpd_resp_set_type(req, "text/plain");
}

// REST URI 
/* Copies the full path into destination buffer and returns
 * pointer to path (skipping the preceding base path) */
const char*
get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize)
{
    const size_t base_pathlen = strlen(base_path);
    size_t pathlen = strlen(uri);

    const char *quest = strchr(uri, '?');
    if (quest){
        pathlen = MIN(pathlen, quest - uri);
    }
    const char *hash = strchr(uri, '#');
    if (hash){
        pathlen = MIN(pathlen, hash - uri);
    }
    if (base_pathlen + pathlen + 1 > destsize){
        /* Full path string won't fit into destination buffer */
        return NULL;
    }
    /* Construct full path (base + path) */
    strcpy(dest, base_path);
    strlcpy(dest + base_pathlen, uri, pathlen + 1);

    /* Return pointer to path, skipping the base */
    return dest + base_pathlen;
}


static esp_err_t 
on_default_url( httpd_req_t *req )
{
    char filepath[ FILE_PATH_MAX ];
    FILE *fd = NULL;
    struct stat file_stat;
    esp_err_t err = ESP_OK;

	//ESP_LOGI(TAG, "URL: %s", req->uri);
	strncpy( filepath, req->uri, sizeof( filepath ) );

	// removes base_path and REST 
	// we must copy the filename because any other call to parse the request invalidates the pointer
    const char *filename = get_path_from_uri( filepath, ((file_server_data_t *)req->user_ctx)->base_path, req->uri, sizeof(filepath));
    if( filename ){
		//ESP_LOGI( TAG, "serving file name %s, basepath \"%s\", filepath %s", filename, 
        //                    ((file_server_data_t *)req->user_ctx)->base_path, 
        //                    filepath );        
    } else {
        ESP_LOGE(TAG, "Filename is too long");
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err( req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long" );
        return ESP_FAIL;
    }  

    // entry point for the webserver: either / or /index.html
    if( filepath[ strlen(filepath) - 1 ] == '/' ) {
		// ends with / (for directories) or is index.html
		strncpy( filepath, "/index.html", sizeof( filepath ));
	}
	if( stat( filepath, &file_stat ) == -1 && strchr(filepath, '.') == NULL ){
		// File doesn't exist and no file extension, assume .html
		strlcat( filepath, ".html", FILE_PATH_MAX );
	}
	if( stat( filepath, &file_stat ) == -1 && 
		( IS_FILE_EXT(filepath, ".html") || IS_FILE_EXT(filepath, ".js") || IS_FILE_EXT(filepath, ".css" ))){
		// File doesn't exist, but is potentially gzip'd
		strlcat( filepath, ".gz", FILE_PATH_MAX);
	}
	if( stat( filepath, &file_stat ) == -1 ) {
		ESP_LOGE(TAG, "Failed to stat file: %s", filepath);
		// Respond with 404 Not Found 
		httpd_resp_send_err( req, HTTPD_404_NOT_FOUND, "File does not exist") ;
		return ESP_FAIL;
	}

	fd = fopen( filepath, "r" );
	if( !fd ){
		ESP_LOGE(TAG, "Failed to read existing file : %s", filepath);
		// Respond with 500 Internal Server Error 
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
		return ESP_FAIL;
	}

    //ESP_LOGI(TAG, "Sending file : %s (%ld bytes)...", filepath, file_stat.st_size);
    set_content_type_from_file( req, filepath );

    if( IS_FILE_EXT( filepath, ".gz" )){
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    }

    // Retrieve the pointer to scratch buffer for temporary storage
    char *chunk = ((file_server_data_t *)req->user_ctx)->scratch;
    // first chunk
    size_t chunksize = fread( chunk, 1, SCRATCH_BUFSIZE, fd );
        
    while( chunksize > 0 ) {
        /* Send the buffer contents as HTTP response chunk */
        if(( err = httpd_resp_send_chunk(req, chunk, chunksize)) != ESP_OK) {

            fclose(fd);
            ESP_LOGE( TAG, "File %s sending failed: %s", filepath, esp_err_to_name( err )); 
            /* Abort sending file */
            httpd_resp_sendstr_chunk( req, NULL );
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
            return ESP_FAIL;
            
        } else {
            /* Read file in chunks into the scratch buffer */
            chunksize = fread( chunk, 1, SCRATCH_BUFSIZE, fd );
            //ESP_LOGI(TAG, "chunk %d", chunksize);
        }
        vTaskDelay( 1 / portTICK_PERIOD_MS ); // must yield while transferring file data
        //ESP_LOGI(TAG, "RAM left %d", esp_get_free_heap_size());
        /* Keep looping till the whole file is sent */
    }

    /* Close file after sending complete */
    fclose(fd);
    //ESP_LOGI(TAG, "File sending complete");

    // Respond with an empty chunk to signal HTTP response completion
    httpd_resp_send_chunk(req, NULL, 0);
	// must yield
    vTaskDelay(10 / portTICK_PERIOD_MS); 
    return ESP_OK;
}


static esp_err_t on_toggle_led_url(httpd_req_t *req){
  char buffer[100];
  memset(&buffer, 0, sizeof(buffer));

  httpd_req_recv(req, buffer, req->content_len);
  cJSON *payload = cJSON_Parse(buffer);
  cJSON *is_on_json = cJSON_GetObjectItem(payload, "is_on");
  bool is_on = cJSON_IsTrue(is_on_json);
  cJSON_Delete(payload);
  toggle_led(is_on);
  httpd_resp_set_status(req, "204 NO CONTENT");
  httpd_resp_send(req, NULL, 0);
  return ESP_OK;
}
/********************Web Socket *******************/

#define WS_MAX_SIZE 1024
static int client_session_id = 0 ;

esp_err_t send_ws_message(char *message)
{
  if( !client_session_id ){
    //ESP_LOGE(TAG, "no client_session_id");
    return -1;
  }
  httpd_ws_frame_t ws_message = {
      .final = true,
      .fragmented = false,
      .len = strlen(message),
      .payload = (uint8_t *)message,
      .type = HTTPD_WS_TYPE_TEXT};
  return httpd_ws_send_frame_async(server, client_session_id, &ws_message);
}

static esp_err_t on_web_socket_url(httpd_req_t *req){
  client_session_id = httpd_req_to_sockfd(req);
  if (req->method == HTTP_GET)
    return ESP_OK;

  httpd_ws_frame_t ws_pkt;
  memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
  ws_pkt.type = HTTPD_WS_TYPE_TEXT;
  ws_pkt.payload = malloc(WS_MAX_SIZE);
  httpd_ws_recv_frame(req, &ws_pkt, WS_MAX_SIZE);
  printf("ws payload: %.*s\n", ws_pkt.len, ws_pkt.payload);
  free(ws_pkt.payload);

  char *response = "connected OK 😊";
  httpd_ws_frame_t ws_responce = {
      .final = true,
      .fragmented = false,
      .type = HTTPD_WS_TYPE_TEXT,
      .payload = (uint8_t *)response,
      .len = strlen(response)};
  return httpd_ws_send_frame(req, &ws_responce);
}

/*******************************************/

void init_server( const char *base_path ){
  esp_err_t err;

    /* Allocate memory for server data */
    server_data = calloc( 1, sizeof( file_server_data_t ));
    strlcpy( server_data->base_path, base_path, sizeof( server_data->base_path ));

	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.core_id = WIFI_TASK_PINNED_TO_CORE; // set on same core as Wifi
	// pages reload
	config.lru_purge_enable = true;
	config.max_open_sockets = 13;  // must be configured in MENUCONFIG: LWIP_MAX_SOCKETS 16 - 3, 3 internal use
	// a device had broadcast a STP packet (STP=Spanning Tree Protocol), which shuts down traffic for up to 30 seconds
	config.recv_wait_timeout = 30;
	config.send_wait_timeout = 5; // default is 5 seconds
	config.stack_size = 8 * 1024; // lwip_arch thread_sem_init: out of memory 
	// allow URI wildcard
	config.uri_match_fn = httpd_uri_match_wildcard;

	ESP_LOGI( TAG, "Starting HTTP Server on core %d", config.core_id );
	if(( err = httpd_start( &server, &config )) != ESP_OK ){
		ESP_LOGE( TAG, "httpd_start failed: %s", esp_err_to_name( err ));
		return;
	}

	// increase log level
	esp_log_level_set( "httpd", ESP_LOG_INFO ); 
	esp_log_level_set( "httpd_txrx", ESP_LOG_ERROR ); // annoying "httpd_sock_err: error in recv : 113" warnings


	httpd_uri_t toggle_led_url = {
		.uri = "/api/toggle-led",
		.method = HTTP_POST,
		.handler = on_toggle_led_url,
        .user_ctx = server_data // Pass server data as context
	};
	httpd_register_uri_handler(server, &toggle_led_url);

	httpd_uri_t web_socket_url = {
		.uri = "/ws",
		.method = HTTP_GET,
		.handler = on_web_socket_url,
		.is_websocket = true,
        .user_ctx = server_data // Pass server data as context
	};
	httpd_register_uri_handler(server, &web_socket_url);

	// anything else
	httpd_uri_t default_url = {
		.uri = "/*",
		.method = HTTP_GET,
		.handler = on_default_url,
        .user_ctx = server_data // Pass server data as context
	};
	httpd_register_uri_handler(server, &default_url);  

}


