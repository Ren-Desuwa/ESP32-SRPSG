Import("env")

# We create a custom target named "upload_all"
# It runs the platformio command line tool twice: once for code, once for files.
env.AddCustomTarget(
    "upload_all", 
    None, 
    [
        "$PYTHONEXE -m platformio run -t upload", 
        "$PYTHONEXE -m platformio run -t uploadfs"
    ], 
    title="Upload All", 
    description="Uploads both Firmware and LittleFS Image"
)