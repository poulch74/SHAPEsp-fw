echo runnig prebuild
echo %cd%
del BULMA\index.html.gz
BULMA\converter.exe BULMA\index.shtml e BULMA\bulma.min.css
BULMA\gzip.exe -c -9 BULMA\index.html >> BULMA\index.html.gz
BULMA\converter.exe BULMA\index.html.gz
copy  %cd%\BULMA\index_html.h %cd%\CODE\SHAPESP\index_html.h /Y
