@echo off

set "FOLDER=%~dp0shared"

echo Checking if "shared" folder exists ...
if not exist "%FOLDER%\" (
  mkdir "%FOLDER%"
  echo Created missing "shared" folder.
) else (
  echo "shared" folder already exists.
)

echo Compiling GRAFCET-Studio_Reader ...
cd GRAFCET-Studio_Reader\src
del main.exe
g++ -o main main.cpp ..\lib\pugiXML\pugixml.cpp ..\lib\aux_structs\aux_structs.cpp ..\lib\aux_funcs\aux_funcs.cpp
echo GRAFCET-Studio_Reader compiled.

echo Compiling ST_Translator ...
cd ..\..\ST_Translator\src
del main.exe
g++ -o main main.cpp ..\lib\pugiXML\pugixml.cpp ..\lib\aux_structs\aux_structs.cpp ..\lib\aux_funcs\aux_funcs.cpp ../lib/aux_trans_funcs/aux_trans_funcs.cpp
echo ST_Translator compiled.

cd ..\..
