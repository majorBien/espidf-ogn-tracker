idf_component_register(
    SRCS "http_server.c" "wifi_app.c"
    INCLUDE_DIRS "." 
    EMBED_FILES
        "webpage/favicon.ico"
        "webpage/index.html"
        "webpage/app.js"
        "webpage/app.css"
        "webpage/jquery-3.3.1.min.js"
    REQUIRES
    	esp_wifi
    	esp_http_server
    	nvs_flash
    	flash_storage
    	lwip
    	esp_netif 
    	json
    	esp_timer
    	app_update
    	esp_https_ota
)
