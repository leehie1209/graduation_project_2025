idf_component_register(
    SRCS 
        "app_main.c"
        "i2c/bme280.c"
        "i2c/i2c_master_init.c"
        "i2c/ina226.c"
    INCLUDE_DIRS 
        "."
        "i2c"
    REQUIRES driver
)
