include(GNUInstallDirs)

execute_process(COMMAND sh "-c" "cat ../VERSION.txt | tr -d '\n'" OUTPUT_VARIABLE VERSION)

idf_component_register(SRCS "main.cpp" "rest_server.cpp"
			INCLUDE_DIRS "include"
			REQUIRES FastLED-idf esp_http_server spiffs json)

set_source_files_properties(rest_server.cpp
			PROPERTIES COMPILE_FLAGS
			-fpermissive
)

set_source_files_properties(main.cpp
			PROPERTIES COMPILE_FLAGS
			-DVERSION=\\"${VERSION}\\"
)


set(WEB_SRC_DIR "../alx-app")
if(EXISTS ${WEB_SRC_DIR}/dist)
	spiffs_create_partition_image(www ${WEB_SRC_DIR}/dist FLASH_IN_PROJECT) 
else() 
	message(FATAL_ERROR "${WEB_SRC_DIR}/dist doesn't exist. Please run 'npm run build' in ${WEB_SRC_DIR}") 
endif() 
