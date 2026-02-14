set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/components/esp-aws-iot)
include($ENV{IDF_PATH}/components/esp-aws-iot/libraries/coreMQTT/CMakeLists.txt)

idf_component_register(
    SRCS "cloud.c" "wifi.c" "uart.c" "main.c" "control_task.c"
    INCLUDE_DIRS "."
    EMBED_TXTFILES "certs/AmazonRootCA1.pem"
                "certs/certificate.pem.crt"
                "certs/private.pem.key"
    REQUIRES demo objects
    REQUIRES driver 
    REQUIRES esp_wifi esp_http_client esp_http_server esp_event nvs_flash
)