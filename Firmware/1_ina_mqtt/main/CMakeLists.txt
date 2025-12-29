idf_component_register(SRCS "app_main.c"
                            "mqtt/mqtt.c"
                            "wifi/wifi.c"
                            "sds/sds011.c"
                            "ina/ina226.c"
                            "pms/pms7003.c"
                            "sdcard/sdcard.c"
                            "bme/bme280.c"
                    INCLUDE_DIRS "." "board")
