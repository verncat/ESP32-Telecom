idf_component_register( SRCS "app_main.c" 
                            "tasks/rgb_state_task.c" 
                            "tasks/ota_task.c"
                            "tasks/mqtt_task.c"
                            "tasks/wifi_task.c"
                            "tasks/gpio_monitor_task.c"
                        INCLUDE_DIRS ".")
