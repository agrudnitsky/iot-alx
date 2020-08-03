include(GNUInstallDirs)

execute_process(COMMAND sh "-c" "cat ../VERSION.txt | tr -d '\n'" OUTPUT_VARIABLE VERSION)

idf_component_register(SRCS "main.cpp" "rest_server.cpp" "mode_power_down.cpp" "mode_power_up.cpp" "mode_constant.cpp" "mode_xmas.cpp" "mode_base.cpp" "mode_tdc.cpp"
			INCLUDE_DIRS "include"
			REQUIRES FastLED-idf nvs_flash esp_http_server esp_http_client spiffs json)

set_source_files_properties(mode_tdc.cpp
			PROPERTIES COMPILE_FLAGS
			-fpermissive
)

set_source_files_properties(mode_base.cpp
			PROPERTIES COMPILE_FLAGS
			-fpermissive
)

set_source_files_properties(mode_xmas.cpp
			PROPERTIES COMPILE_FLAGS
			-fpermissive
)

set_source_files_properties(mode_constant.cpp
			PROPERTIES COMPILE_FLAGS
			-fpermissive
)

set_source_files_properties(mode_power_up.cpp
			PROPERTIES COMPILE_FLAGS
			-fpermissive
)

set_source_files_properties(mode_power_down.cpp
			PROPERTIES COMPILE_FLAGS
			-fpermissive
)

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
