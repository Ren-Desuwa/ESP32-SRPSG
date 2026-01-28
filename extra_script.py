# [2026-01-28 10:15 am - batch 1.5.1]
Import("env")

# 1. THE OLD "UPLOAD ALL" (Firmware + LittleFS/Filesystem)
env.AddCustomTarget(
    "upload_all", 
    None, 
    [
        "$PYTHONEXE -m platformio run -t upload", 
        "$PYTHONEXE -m platformio run -t uploadfs"
    ], 
    title="Upload All", 
    description="Uploads both Firmware and Filesystem Image"
)

# 2. UPLOAD TO WEBSITE (via Wi-Fi)
# This runs your new deployment script with the '1' choice (Web)
env.AddCustomTarget(
    "deploy_web",
    None,
    'python -u deploy_brainiac.py --mode web', # Added -u here
    title="Deploy: Web Upload",
    description="Compresses assets and pushes to ESP32 over Wi-Fi"
)

# 3. UPLOAD TO SD CARD (Local Mount)
# This runs your new deployment script with the '2' choice (SD)
env.AddCustomTarget(
    "deploy_sd",
    None,
    'python deploy_brainiac.py --mode sd',
    title="Deploy: SD Card",
    description="Compresses assets and copies directly to mounted SD card"
)