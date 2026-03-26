idf_component_register(
    SRCS "crc_16.c" "queue_push_task.c" "uart_router_task.c" "uart_rx_task.c" "uart_tx_task.c" "uart_rxtx_task.c" "uart2_driver.c"
    INCLUDE_DIRS "include" "../../main/include"
    REQUIRES driver
)
