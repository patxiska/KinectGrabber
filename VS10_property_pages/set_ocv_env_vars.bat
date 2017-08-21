setx -m OPENCV_PATH "C:\Program Files\opencv"
setx -m OPENCV_INCLUDE "%OPENCV_PATH%\build\include"
setx -m OPENCV_BIN "%OPENCV_PATH%\build\x86\vc10\bin"
setx -m OPENCV_LIB "%OPENCV_PATH%\build\x86\vc10\lib"
setx -m PATH "%PATH%;%OPENCV_BIN%"
pause