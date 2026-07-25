cd GRAFCET-Studio_Reader\src
del main.exe
g++ -o main main.cpp ..\lib\pugiXML\pugixml.cpp ..\lib\aux_structs\aux_structs.cpp ..\lib\aux_funcs\aux_funcs.cpp
cd ..\..\ST_Translator\src
del main.exe
g++ -o main main.cpp ..\lib\pugiXML\pugixml.cpp ..\lib\aux_structs\aux_structs.cpp ..\lib\aux_funcs\aux_funcs.cpp ../lib/aux_trans_funcs/aux_trans_funcs.cpp
cd ..\..