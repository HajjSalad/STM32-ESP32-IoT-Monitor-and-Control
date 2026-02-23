idf_component_register(
    SRCS "uart_rxtx_task.c" "uart2_driver.c"
    INCLUDE_DIRS "include" "../../main/include"
    REQUIRES driver
)
