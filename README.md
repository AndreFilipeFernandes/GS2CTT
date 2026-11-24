# GS2C-translation-tool
Toolchain that translates grafcets built in Grafcet-Studio into IEC 61131-3 compliant ST code.

# How to use
1. Download "Toolchain.zip" from the latest release.
2. Unzip "Toolchain.zip".
3. Add desired Grafcet-Studio .grafcet file into the "shared" directory (make sure to fully delete this file's contents between uses).
4. **If on Windows** run "toolchain_run.bat".

   **If on Linux** run "toolchain_run.sh", ensuring you first have Wine installed.
6. Your IEC 61131-3 compliant files are now ready for use in the "shared" directory.

Additionally, if using CODESYS on a Windows environment, you may call the "Import_project_to_CODESYS.py" script from within CODESYS (under Tools → Scripting menu) instead of performing step 4.
