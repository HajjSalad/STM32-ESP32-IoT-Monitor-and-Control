idf_component_register(
    SRCS "cloud_mqtt_task.c"
         "mqtt_driver.c"
    INCLUDE_DIRS "include" "../../main/include"
    EMBED_TXTFILES "certs/AmazonRootCA1.pem"
                   "certs/certificate.pem.crt"
                   "certs/private.pem.key"
     REQUIRES coreMQTT
     REQUIRES driver
)
