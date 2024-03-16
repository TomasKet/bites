idf_component_register(
    SRCS "bites_i2s_stream.c" "board.c" "board_pins_config.c" "new_codec.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi driver audio_pipeline audio_hal audio_board audio_stream esp_peripherals)

idf_component_get_property(audio_board_lib audio_board COMPONENT_LIB)
set_property(TARGET ${audio_board_lib} APPEND PROPERTY INTERFACE_LINK_LIBRARIES ${COMPONENT_LIB})
