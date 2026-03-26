idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "include"
REQUIRES nvs_flash
         esp_netif
         esp_event
         cloud_mqtt
         wifi
         uart
)