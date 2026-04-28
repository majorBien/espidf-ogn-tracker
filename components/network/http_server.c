/*
 * http_server.c
 *
 *  Created on: Oct 20, 2025
 *      Author: majorBien
 */

#include "esp_http_server.h"
#include "esp_log.h"

#include "http_server.h"
#include "tasks_common.h"
#include "wifi_app.h"
#include "freertos/idf_additions.h"
#include "sys/param.h"

#include <string.h> 
#include <cJSON.h> 
#include "nvs_flash.h"
#include "nvs_utils.h"
#include "esp_timer.h"
#include "esp_ota_ops.h" 
#include "esp_partition.h" 
#include <math.h>
#include "parameters_wrapper.h"
nvs_network_data_t network_data;

// Firmware update status
static int g_fw_update_status = OTA_UPDATE_PENDING;


// Tag used for ESP serial console messages
static const char TAG[] = "http_server";

// HTTP server task handle
static httpd_handle_t http_server_handle = NULL;

// HTTP server monitor task handle
static TaskHandle_t task_http_server_monitor = NULL;

// Queue handle used to manipulate the main queue of events
static QueueHandle_t http_server_monitor_queue_handle;

//net settings handlers
static esp_err_t settings_net_post_handler(httpd_req_t *req); 
static esp_err_t settings_net_get_handler(httpd_req_t *req);
static esp_err_t settings_ip_get_handler(httpd_req_t *req);
static esp_err_t settings_identity_get_handler(httpd_req_t *req);
static esp_err_t settings_identity_post_handler(httpd_req_t *req);
static esp_err_t log_book_get_handler(httpd_req_t *req);
static esp_err_t radar_get_handler(httpd_req_t *req);

// Embedded files: JQuery, index.html, app.css, app.js and favicon.ico files
extern const uint8_t jquery_3_3_1_min_js_start[]	asm("_binary_jquery_3_3_1_min_js_start");
extern const uint8_t jquery_3_3_1_min_js_end[]		asm("_binary_jquery_3_3_1_min_js_end");
extern const uint8_t index_html_start[]				asm("_binary_index_html_start");
extern const uint8_t index_html_end[]				asm("_binary_index_html_end");
extern const uint8_t app_css_start[]				asm("_binary_app_css_start");
extern const uint8_t app_css_end[]					asm("_binary_app_css_end");
extern const uint8_t app_js_start[]					asm("_binary_app_js_start");
extern const uint8_t app_js_end[]					asm("_binary_app_js_end");
extern const uint8_t favicon_ico_start[]			asm("_binary_favicon_ico_start");
extern const uint8_t favicon_ico_end[]				asm("_binary_favicon_ico_end");

/**
 * Disable CORS policy by setting appropriate headers.
 * @param req HTTP request for which the headers need to be set.
 */

void set_cors_headers(httpd_req_t *req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Authorization");
}

/**
 * ESP32 timer configuration passed to esp_timer_create.
 */
const esp_timer_create_args_t fw_update_reset_args = {
		.callback = &http_server_fw_update_reset_callback,
		.arg = NULL,
		.dispatch_method = ESP_TIMER_TASK,
		.name = "fw_update_reset"
};
esp_timer_handle_t fw_update_reset;

void http_server_fw_update_reset_callback(void *arg)
{
	ESP_LOGI(TAG, "http_server_fw_update_reset_callback: Timer timed-out, restarting the device");
	esp_restart();
}


/**
 * Checks the g_fw_update_status and creates the fw_update_reset timer if g_fw_update_status is true.
 */
static void http_server_fw_update_reset_timer(void)
{
	if (g_fw_update_status == OTA_UPDATE_SUCCESSFUL)
	{
		ESP_LOGI(TAG, "http_server_fw_update_reset_timer: FW updated successful starting FW update reset timer");

		// Give the web page a chance to receive an acknowledge back and initialize the timer
		ESP_ERROR_CHECK(esp_timer_create(&fw_update_reset_args, &fw_update_reset));
		ESP_ERROR_CHECK(esp_timer_start_once(fw_update_reset, 8000000));
	}
	else
	{
		ESP_LOGI(TAG, "http_server_fw_update_reset_timer: FW update unsuccessful");
	}
}


/**
 * HTTP server monitor task used to track events of the HTTP server
 * @param pvParameters parameter which can be passed to the task.
 */
static void http_server_monitor(void *parameter)
{
	http_server_queue_message_t msg;

	for (;;)
	{
		if (xQueueReceive(http_server_monitor_queue_handle, &msg, portMAX_DELAY))
		{
			switch (msg.msgID)
			{
				case HTTP_MSG_WIFI_CONNECT_INIT:
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_INIT");

					break;

				case HTTP_MSG_WIFI_CONNECT_SUCCESS:
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_SUCCESS");

					break;

				case HTTP_MSG_WIFI_CONNECT_FAIL:
					ESP_LOGI(TAG, "HTTP_MSG_WIFI_CONNECT_FAIL");

					break;

				case HTTP_MSG_FIRMWARE_UPDATE_SUCCESSFUL:
					ESP_LOGI(TAG, "HTTP_MSG_OTA_UPDATE_SUCCESSFUL");
					g_fw_update_status = OTA_UPDATE_SUCCESSFUL;
					http_server_fw_update_reset_timer();

					break;

				case HTTP_MSG_FIRMWARE_UPDATE_FAILED:
					ESP_LOGI(TAG, "HTTP_MSG_OTA_UPDATE_FAILED");
					g_fw_update_status = OTA_UPDATE_FAILED;

					break;

				case HTTP_MSG_FIRMWARE_UPATE_INITIALIZED:
					ESP_LOGI(TAG, "HTTP_MSG_OTA_UPATE_INITIALIZED");


					break;

				default:
					break;
			}
		}
	}
}

/**
 * Jquery get handler is requested when accessing the web page.
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_jquery_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "Jquery requested");

	httpd_resp_set_type(req, "application/javascript");
	httpd_resp_send(req, (const char *)jquery_3_3_1_min_js_start, jquery_3_3_1_min_js_end - jquery_3_3_1_min_js_start);

	return ESP_OK;
}

/**
 * Sends the index.html page.
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_index_html_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "index.html requested");

	httpd_resp_set_type(req, "text/html");
	httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);

	return ESP_OK;
}

/**
 * app.css get handler is requested when accessing the web page.
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_app_css_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "app.css requested");

	httpd_resp_set_type(req, "text/css");
	httpd_resp_send(req, (const char *)app_css_start, app_css_end - app_css_start);

	return ESP_OK;
}

/**
 * app.js get handler is requested when accessing the web page.
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_app_js_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "app.js requested");

	httpd_resp_set_type(req, "application/javascript");
	httpd_resp_send(req, (const char *)app_js_start, app_js_end - app_js_start);

	return ESP_OK;
}

/**
 * Sends the .ico (icon) file when accessing the web page.
 * @param req HTTP request for which the uri needs to be handled.
 * @return ESP_OK
 */
static esp_err_t http_server_favicon_ico_handler(httpd_req_t *req)
{
	ESP_LOGI(TAG, "favicon.ico requested");

	httpd_resp_set_type(req, "image/x-icon");
	httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_end - favicon_ico_start);

	return ESP_OK;
}

esp_err_t http_server_OTA_update_handler(httpd_req_t *req)
{
	//deactivate CORS
	set_cors_headers(req);
	esp_ota_handle_t ota_handle;

	char ota_buff[1024];
	int content_length = req->content_len;
	int content_received = 0;
	int recv_len;
	bool is_req_body_started = false;
	bool flash_successful = false;

	const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

	do
	{
		// Read the data for the request
		if ((recv_len = httpd_req_recv(req, ota_buff, MIN(content_length, sizeof(ota_buff)))) < 0)
		{
			// Check if timeout occurred
			if (recv_len == HTTPD_SOCK_ERR_TIMEOUT)
			{
				ESP_LOGI(TAG, "http_server_OTA_update_handler: Socket Timeout");
				continue; ///> Retry receiving if timeout occurred
			}
			ESP_LOGI(TAG, "http_server_OTA_update_handler: OTA other Error %d", recv_len);
			return ESP_FAIL;
		}
		printf("http_server_OTA_update_handler: OTA RX: %d of %d\r", content_received, content_length);

		// Is this the first data we are receiving
		// If so, it will have the information in the header that we need.
		if (!is_req_body_started)
		{
			is_req_body_started = true;

			// Get the location of the .bin file content (remove the web form data)
			char *body_start_p = strstr(ota_buff, "\r\n\r\n") + 4;
			int body_part_len = recv_len - (body_start_p - ota_buff);

			printf("http_server_OTA_update_handler: OTA file size: %d\r\n", content_length);

			esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
			if (err != ESP_OK)
			{
				printf("http_server_OTA_update_handler: Error with OTA begin, cancelling OTA\r\n");
				return ESP_FAIL;
			}
			else
			{
				printf("http_server_OTA_update_handler: Writing to partition subtype %d at offset 0x%d\r\n", update_partition->subtype, (int)update_partition->address);
			}

			// Write this first part of the data
			esp_ota_write(ota_handle, body_start_p, body_part_len);
			content_received += body_part_len;
		}
		else
		{
			// Write OTA data
			esp_ota_write(ota_handle, ota_buff, recv_len);
			content_received += recv_len;
		}

	} while (recv_len > 0 && content_received < content_length);

	if (esp_ota_end(ota_handle) == ESP_OK)
	{
		// Lets update the partition
		if (esp_ota_set_boot_partition(update_partition) == ESP_OK)
		{
			const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
			ESP_LOGI(TAG, "http_server_OTA_update_handler: Next boot partition subtype %d at offset 0x%d", boot_partition->subtype, (int)boot_partition->address);
			flash_successful = true;
		}
		else
		{
			ESP_LOGI(TAG, "http_server_OTA_update_handler: FLASHED ERROR!!!");
		}
	}
	else
	{
		ESP_LOGI(TAG, "http_server_OTA_update_handler: esp_ota_end ERROR!!!");
	}

	// We won't update the global variables throughout the file, so send the message about the status
	if (flash_successful) { http_server_monitor_send_message(HTTP_MSG_FIRMWARE_UPDATE_SUCCESSFUL); } else { http_server_monitor_send_message(HTTP_MSG_FIRMWARE_UPDATE_FAILED); }

	return ESP_OK;
}

esp_err_t http_server_OTA_status_handler(httpd_req_t *req)
{
	set_cors_headers(req);
	char otaJSON[100];

	ESP_LOGI(TAG, "OTAstatus requested");

	sprintf(otaJSON, "{\"ota_update_status\":%d,\"compile_time\":\"%s\",\"compile_date\":\"%s\"}", g_fw_update_status, __TIME__, __DATE__);

	httpd_resp_set_type(req, "application/json");
	httpd_resp_send(req, otaJSON, strlen(otaJSON));

	return ESP_OK;
}


/**
 * Sets up the default httpd server configuration.
 * @return http server instance handle if successful, NULL otherwise.
 */
static httpd_handle_t http_server_configure(void)
{
	// Generate the default configuration
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.max_uri_handlers = 20;
	config.max_open_sockets = 4;
	// Create HTTP server monitor task
	xTaskCreate(&http_server_monitor, "http_server_monitor", HTTP_SERVER_MONITOR_STACK_SIZE, NULL, HTTP_SERVER_MONITOR_PRIORITY, &task_http_server_monitor);

	// Create the message queue
	http_server_monitor_queue_handle = xQueueCreate(3, sizeof(http_server_queue_message_t));

	// The core that the HTTP server will run on
	config.core_id = HTTP_SERVER_TASK_CORE_ID;

	// Adjust the default priority to 1 less than the wifi application task
	config.task_priority = HTTP_SERVER_TASK_PRIORITY;

	// Bump up the stack size (default is 4096)
	config.stack_size = HTTP_SERVER_TASK_STACK_SIZE;

	// Increase uri handlers
	config.max_uri_handlers = 20;

	// Increase the timeout limits
	config.recv_wait_timeout = 10;
	config.send_wait_timeout = 10;

	ESP_LOGI(TAG,
			"http_server_configure: Starting server on port: '%d' with task priority: '%d'",
			config.server_port,
			config.task_priority);

	// Start the httpd server
	if (httpd_start(&http_server_handle, &config) == ESP_OK)
	{
		ESP_LOGI(TAG, "http_server_configure: Registering URI handlers");

		// register query handler
		httpd_uri_t jquery_js = {
				.uri = "/jquery-3.3.1.min.js",
				.method = HTTP_GET,
				.handler = http_server_jquery_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &jquery_js);

		// register index.html handler
		httpd_uri_t index_html = {
				.uri = "/",
				.method = HTTP_GET,
				.handler = http_server_index_html_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &index_html);

		// register app.css handler
		httpd_uri_t app_css = {
				.uri = "/app.css",
				.method = HTTP_GET,
				.handler = http_server_app_css_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &app_css);

		// register app.js handler
		httpd_uri_t app_js = {
				.uri = "/app.js",
				.method = HTTP_GET,
				.handler = http_server_app_js_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &app_js);
		// register favicon.ico handler
		httpd_uri_t favicon_ico = {
				.uri = "/favicon.ico",
				.method = HTTP_GET,
				.handler = http_server_favicon_ico_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &favicon_ico);
		
		httpd_uri_t OTA_update = {
				.uri = "/api/OTA/update",
				.method = HTTP_POST,
				.handler = http_server_OTA_update_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &OTA_update);

		// register OTAstatus handler
		httpd_uri_t OTA_status = {
				.uri = "/api/OTA/status",
				.method = HTTP_POST,
				.handler = http_server_OTA_status_handler,
				.user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &OTA_status);

		
		//network settings handlers	    
	    httpd_uri_t settings_net_post = {
    			 .uri = "/api/config/network",
   				 .method = HTTP_POST,
   				 .handler = settings_net_post_handler,
   				 .user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &settings_net_post);
		
		httpd_uri_t settings_net_get = {
    			 .uri = "/api/config/network",
   				 .method = HTTP_GET,
   				 .handler = settings_net_get_handler,
   				 .user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &settings_net_get);
		
				httpd_uri_t ip_addr_get = {
    			 .uri = "/api/config/ip_addr",
   				 .method = HTTP_GET,
   				 .handler = settings_ip_get_handler,
   				 .user_ctx = NULL
		};
		httpd_register_uri_handler(http_server_handle, &ip_addr_get);
		
			httpd_uri_t identity_get = {
				 .uri = "/api/config/identity",
   				 .method = HTTP_GET,
   				 .handler = settings_identity_get_handler,
   				 .user_ctx = NULL
   		};
   		httpd_register_uri_handler(http_server_handle, &identity_get);
   		
			httpd_uri_t identity_post = {
				 .uri = "/api/config/identity",
   				 .method = HTTP_POST,
   				 .handler = settings_identity_post_handler,
   				 .user_ctx = NULL
   		};
   		//httpd_register_uri_handler(http_server_handle, &identity_post);
   		
   		httpd_uri_t log_book_get = {
				 .uri = "/api/logbook",
   				 .method = HTTP_GET,
   				 .handler = log_book_get_handler,
   				 .user_ctx = NULL
   		};
   		httpd_register_uri_handler(http_server_handle, &log_book_get);
   		
   		httpd_uri_t radar_get = {
				 .uri = "/api/radar",
   				 .method = HTTP_GET,
   				 .handler = radar_get_handler,
   				 .user_ctx = NULL
   		};
		httpd_register_uri_handler(http_server_handle, &radar_get);
		
		return http_server_handle;
	}

	return NULL;
}


void http_server_start(void)
{
	if (http_server_handle == NULL)
	{
		http_server_handle = http_server_configure();
	}
}

void http_server_stop(void)
{
	if (http_server_handle)
	{
		httpd_stop(http_server_handle);
		ESP_LOGI(TAG, "http_server_stop: stopping HTTP server");
		http_server_handle = NULL;
	}
	if (task_http_server_monitor)
	{
		vTaskDelete(task_http_server_monitor);
		ESP_LOGI(TAG, "http_server_stop: stopping HTTP server monitor");
		task_http_server_monitor = NULL;
	}
}

BaseType_t http_server_monitor_send_message(http_server_message_e msgID)
{
	http_server_queue_message_t msg;
	msg.msgID = msgID;
	return xQueueSend(http_server_monitor_queue_handle, &msg, portMAX_DELAY);
}



//***************************SETTINGS HANDLERS*****************************/
static esp_err_t settings_net_post_handler(httpd_req_t *req){
	set_cors_headers(req);
    ESP_LOGI(TAG, "Set serial number requested");
    char buf[256] = { 0 };
    int ret, total_length = 0;
    
    
   // Receive full request body
    do {
        ret = httpd_req_recv(req, buf + total_length, sizeof(buf) - total_length - 1);
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            ESP_LOGI(TAG, "timeout, continue receiving");
            continue;
        }
        if (ret < 0) {
            ESP_LOGE(TAG, "Error receiving data! (status = %d)", ret);
            return ESP_FAIL;
        }
        total_length += ret;
        buf[total_length] = '\0';
    } while (ret >= sizeof(buf) - total_length);

    ESP_LOGI(TAG, "Received JSON: %s", buf);

    // Parse JSON to extract password and ssid
    cJSON *json = cJSON_Parse(buf);
    if (!json) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    const cJSON *password_json = cJSON_GetObjectItemCaseSensitive(json, "password");
    const cJSON *ssid_json = cJSON_GetObjectItemCaseSensitive(json, "ssid");
    if (!cJSON_IsString(password_json) || (password_json->valuestring == NULL)||!cJSON_IsString(ssid_json) || (ssid_json->valuestring == NULL)) {
        ESP_LOGE(TAG, "network data missing or not a string");
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "serial_number is required");
        return ESP_FAIL;
    }
	
	strcpy(network_data.ssid, ssid_json->valuestring);
	strcpy(network_data.password, password_json->valuestring);
	
    // Save to NVS
    esp_err_t err = nvs_save_network_data(&network_data);
    cJSON_Delete(json);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save serial number (err=0x%x)", err);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to save serial number");
        return err;
    }

    httpd_resp_sendstr(req, "Serial set");
    return ESP_OK;
}

static esp_err_t settings_net_get_handler(httpd_req_t *req){
	set_cors_headers(req);
    ESP_LOGI(TAG, "JSON data requested");

    esp_err_t err = nvs_load_network_data(&network_data);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Serial number not found");
        httpd_resp_sendstr(req, "{\"serial_number\":null}");
        return ESP_OK;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to load serial number (err=0x%x)", err);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read serial number");
        return err;
    }

    // Construct JSON response
    char json_response[128];
    snprintf(json_response, sizeof(json_response), "{\"ssid\":\"%s\",\"password\":\"%s\"}", network_data.ssid, network_data.password);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_response);

    return ESP_OK;

}


static esp_err_t settings_ip_get_handler(httpd_req_t *req){
    char resp_str[64] = {0};

    if (esp_netif_sta != NULL){
        esp_netif_ip_info_t ip_info;
        if (esp_netif_get_ip_info(esp_netif_sta, &ip_info) == ESP_OK){
            snprintf(resp_str, sizeof(resp_str), "{\"ip\":\"" IPSTR "\"}", IP2STR(&ip_info.ip));
        }
        else snprintf(resp_str, sizeof(resp_str), "{\"ip\":\"0.0.0.0\"}");

    }
    else snprintf(resp_str, sizeof(resp_str), "{\"ip\":\"0.0.0.0\"}");
    

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

static esp_err_t settings_identity_get_handler(httpd_req_t *req)
{
    char resp_str[512];

    snprintf(resp_str, sizeof(resp_str),
    "{"
      "\"pilot\":\"%s\","
      "\"crew\":\"%s\","
      "\"reg\":\"%s\","
      "\"base\":\"%s\","
      "\"manuf\":\"%s\","
      "\"model\":\"%s\","
      "\"type\":\"%s\","
      "\"sn\":\"%s\""
    "}",
    params_get_pilot(),
    params_get_crew(),
    params_get_reg(),
    params_get_base(),
    params_get_manuf(),
    params_get_model(),
    params_get_type(),
    params_get_sn()
    );

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

static esp_err_t settings_identity_post_handler(httpd_req_t *req)
{
    char buffer[512];
    int ret = httpd_req_recv(req, buffer, sizeof(buffer) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    buffer[ret] = '\0';

    // Parse JSON - using cJSON
    cJSON *json = cJSON_Parse(buffer);
    if (json == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    // Extract and set string parameters
    cJSON *item = NULL;
    
    item = cJSON_GetObjectItem(json, "pilot");
    if (item && cJSON_IsString(item)) params_set_pilot(item->valuestring);
    
    item = cJSON_GetObjectItem(json, "crew");
    if (item && cJSON_IsString(item)) params_set_crew(item->valuestring);
    
    item = cJSON_GetObjectItem(json, "reg");
    if (item && cJSON_IsString(item)) params_set_reg(item->valuestring);
    
    item = cJSON_GetObjectItem(json, "base");
    if (item && cJSON_IsString(item)) params_set_base(item->valuestring);
    
    item = cJSON_GetObjectItem(json, "manuf");
    if (item && cJSON_IsString(item)) params_set_manuf(item->valuestring);
    
    item = cJSON_GetObjectItem(json, "model");
    if (item && cJSON_IsString(item)) params_set_model(item->valuestring);
    
    item = cJSON_GetObjectItem(json, "type");
    if (item && cJSON_IsString(item)) params_set_type(item->valuestring);
    
    item = cJSON_GetObjectItem(json, "sn");
    if (item && cJSON_IsString(item)) params_set_sn(item->valuestring);

    // Save to flash (persistent storage)
    if (params_save_to_flash()) {
        cJSON_Delete(json);
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, "{\"status\":\"ok\",\"message\":\"Parameters saved successfully\"}", 
                        HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    } else {
        cJSON_Delete(json);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
}

static esp_err_t log_book_get_handler(httpd_req_t *req)
{
    char *resp_str = malloc(2048);  

    if (!resp_str) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    snprintf(resp_str, 2048,
    "{"
        "\"logbook\":["
            "{"
                "\"date\":\"2024-01-01\","
                "\"duration\":\"2h\","
                "\"aircraft\":\"Glider Junior\","
                "\"remarks\":\"Nice flight!\""
            "},"
            "{"
                "\"date\":\"2024-02-15\","
                "\"duration\":\"1.5h\","
                "\"aircraft\":\"Glider Junior\","
                "\"remarks\":\"Challenging weather conditions.\""
            "},"
            "{"
                "\"date\":\"2024-03-03\","
                "\"duration\":\"2.3h\","
                "\"aircraft\":\"DG-1000\","
                "\"remarks\":\"Strong thermals, XC flight.\""
            "},"
            "{"
                "\"date\":\"2024-03-10\","
                "\"duration\":\"1.2h\","
                "\"aircraft\":\"Cessna 172\","
                "\"remarks\":\"Navigation training.\""
            "},"
            "{"
                "\"date\":\"2024-03-18\","
                "\"duration\":\"1.7h\","
                "\"aircraft\":\"AT3\","
                "\"remarks\":\"Touch & go practice.\""
            "}"
        "]"
    "}");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    free(resp_str);  

    return ESP_OK;
}
static float own_lat = 52.2730;
static float own_lon = 20.9100;
static float own_alt = 1100;
static float own_heading = 90.0f; // z magnetometru (mock)

typedef struct {
    const char *id;
    const char *type;
    float lat;
    float lon;
    float alt;
    float dx;
    float dy;
} aircraft_t;

// ===== MOCK AIRCRAFT STATE =====
static aircraft_t fleet[] = {
    {"GLD1",   "Glider",     52.2690, 20.9070, 1200,  0.00001,  0.00000},
    {"C172-1", "Cessna 172", 52.2805, 20.9201, 1800, -0.00002,  0.00001},
    {"AT3-7",  "AT3",        52.2750, 20.8950,  900,  0.00000, -0.00002}
};

#define FLEET_SIZE (sizeof(fleet)/sizeof(fleet[0]))

// ===== SIMPLE DRIFT SIMULATION =====
static void update_fleet()
{
    for (int i = 0; i < FLEET_SIZE; i++) {
        fleet[i].lat += fleet[i].dy;
        fleet[i].lon += fleet[i].dx;

        // minimal random wobble
        fleet[i].dx += ((float)(rand() % 100) - 50) * 0.00000001f;
        fleet[i].dy += ((float)(rand() % 100) - 50) * 0.00000001f;
    }

    // own ship small drift simulation (optional)
    own_lat += 0.000001f;
    own_lon += 0.000001f;
}

// ===== RADAR HANDLER =====
static esp_err_t radar_get_handler(httpd_req_t *req)
{
    update_fleet();

    char *resp_str = malloc(2048);
    if (!resp_str) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    int offset = 0;

    // ===== OWN AIRCRAFT =====
    offset += snprintf(resp_str + offset, 2048 - offset,
    "{"
      "\"own\":{"
        "\"lat\":%.6f,"
        "\"lon\":%.6f,"
        "\"alt\":%.0f,"
        "\"heading\":%.1f"
      "},"
      "\"aircraft\":[",
      own_lat,
      own_lon,
      own_alt,
      own_heading
    );

    // ===== FLEET =====
    for (int i = 0; i < FLEET_SIZE; i++) {

        // safety: prevent overflow
        if (offset > 1900) break;

        offset += snprintf(resp_str + offset, 2048 - offset,
        "{"
          "\"id\":\"%s\","
          "\"type\":\"%s\","
          "\"lat\":%.6f,"
          "\"lon\":%.6f\","
          "\"alt\":%.0f"
        "}%s",
        fleet[i].id,
        fleet[i].type,
        fleet[i].lat,
        fleet[i].lon,
        fleet[i].alt,
        (i < FLEET_SIZE - 1) ? "," : ""
        );
    }

    // close JSON
    snprintf(resp_str + offset, 2048 - offset, "] }");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);

    free(resp_str);
    return ESP_OK;
}