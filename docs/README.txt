* Requirements

Install the following:
- ESP-IDF toolchain
- fastLED-idf library

In the alx-app directory run
$ npm install


* Building
$ cp src/include/local_settings.h-skeleton src/include/local_settings.h

==> edit and modify src/include/local_settings.h

$ export FASTLED_DIR=/path/to/components_dir_with_fastLED-idf_directory/

# build web-app
$ cd alx-app
$ npm run build
$ cd dist && ../../tools/html-js-shorten-filenames.pl . ; cd ../..

# build ESP code & Flash everything
$ idf.py build
$ idf.py -p /dev/ttyUSB0 flash

