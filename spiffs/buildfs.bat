gzip.exe -c ../HTML1/index.html > FS/index.html.gz
converter.exe FS/index.html.gz
mkspiffs --create FS -d 5 -b 0x1000 -p 0x100 -s 0x20000 spiffs.bin

copy  index_html.h ..\CODE\SHAPESP\index_html.h /Y
