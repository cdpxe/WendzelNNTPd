make clean
del src.pro
qmake -project
qmake LIBS+="-lsqlite3 -lqtmain" CONFIG+="qt"
make -f Makefile.release
del release\WendzelNNTPGUI.exe
cd release
	rename src.exe WendzelNNTPGUI.exe
cd ..
pause
