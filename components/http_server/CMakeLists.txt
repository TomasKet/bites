idf_component_register(
    SRCS "http_server.c"
    INCLUDE_DIRS "include"
    EMBED_TXTFILES "certs/servercert.pem" "certs/prvtkey.pem"
    REQUIRES esp_wifi esp_https_server storas)