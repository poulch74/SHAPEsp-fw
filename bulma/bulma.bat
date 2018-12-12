del index.html.gz
gzip.exe -c -9 index.html >> index.html.gz
converter.exe index.html.gz
copy  index_html.h ..\CODE\SHAPESP\index_html.h /Y
