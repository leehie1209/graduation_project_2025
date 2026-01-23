idf_component_register(SRCS "app_main.c"
                            "mqtt/mqtt.c"
                            "wifi/wifi.c"
                            "system_state/system_state.c"
                    INCLUDE_DIRS
                                "."
                                "wifi"
                                "mqtt"
                                "system_state"    )
