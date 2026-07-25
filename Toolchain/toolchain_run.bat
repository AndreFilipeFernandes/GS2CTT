@echo off

echo Executing GRAFCET-Studio_Reader ...
cd GRAFCET-Studio_Reader\src
.\main.exe
echo GRAFCET-Studio_Reader finished.

echo Executing ST_Translator ...
cd ..\..\ST_Translator\src
.\main.exe
echo ST_Translator finished.

cd ..\..
