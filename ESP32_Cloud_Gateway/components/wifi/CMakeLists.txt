idf_component_register(
    SRCS "wifi_manager_task.c" "wifi_driver.c"
    INCLUDE_DIRS "include" "../../main/include"
    REQUIRES esp_wifi
    REQUIRES driver
)
