idf_component_register(
    SRCS "micro-esp.c" "shell/parser.c" "shell/commands.c" "fs/fs.c" "shell/init_msg.c"
    INCLUDE_DIRS "." "shell" "fs"
    REQUIRES esp_driver_uart spiffs
)