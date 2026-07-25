import os
import subprocess

# Path declarations
script_dir = os.path.dirname(os.path.abspath(__file__))
grafcet_src = os.path.join(script_dir, "GRAFCET-Studio_Reader", "src")
st_translator_src = os.path.join(script_dir, "ST_Translator", "src")
plcopenxml_path = os.path.join(script_dir, "shared", "CODESYSProject.xml")
xml_path = os.path.join(script_dir, "shared", "SymbolConfiguration.xml")
#####

# Execute relevant toolchain programs
print("[INFO] Executing Grafcet-Studio Reader...")
subprocess.call(os.path.join(grafcet_src, "main.exe"), cwd=grafcet_src)
print("[INFO] Grafcet-Studio Reader finished.")

print("[INFO] Executing ST Translator...")
subprocess.call(os.path.join(st_translator_src, "main.exe"), cwd=st_translator_src)
print("[INFO] ST Translator finished.")
#####

# Find project
proj = projects.primary
if proj is None:
    raise Exception("No active CODESYS project.")
#####

# Import PLCOpenXML project
print("[INFO] Starting main project import...")
proj.import_xml(plcopenxml_path, proj)
print("[INFO] Finished main project import.")
#####

# Find created application
app = proj.active_application
if app is None:
    raise Exception("No active application in the project.")
#####

# Import symbol configuration XML
print("[INFO] Starting symbol configuration import...")
app.import_native(xml_path)
print("[INFO] Finished symbol configuration import.")
#####

# Build project (mostly because of symbol configuration)
print("[INFO] Building...")
app.build()
print("[INFO] Finished building.")
#####

print("[INFO] All tasks completed.")