"""
Brainiac Deployment Tool v2.8
-----------------------------
Senior Developer Build: Arch Linux Optimized.
Features: Explicit Wi-Fi Rescan, Smart Sync, High Telemetry.
Rule 2: Logs every action.
Rule 4: No Emoji.
"""

import os, json, gzip, hashlib, requests, time, subprocess, sys

# --- CONFIGURATION ---
SOURCE_DIR = '../../Websites/Braniac 2'
HASH_MANIFEST = '.file_hashes.json'
URL_FILE = 'website_upload_link.txt'
ESP_SSID = "Akbay"
HOME_SSID = "WIFI-0"
HOME_PASS = "password1"
GZIP_EXTENSIONS = {'.html', '.css', '.js', '.json', '.xml', '.svg', '.txt'}

class Log:
    @staticmethod
    def info(msg): print(f"[INFO] {msg}", flush=True)
    @staticmethod
    def success(msg): print(f"[ OK ] {msg}", flush=True)
    @staticmethod
    def warn(msg): print(f"[WARN] {msg}", flush=True)
    @staticmethod
    def error(msg, e=None):
        out = f"[FAIL] {msg}"
        if e: out += f" | {e}"
        print(out, flush=True)

def get_file_hash(filepath):
    print(f"  [HASH-TRACE] Scanning: {os.path.basename(filepath)}".ljust(60), end="\r", flush=True)
    sha1 = hashlib.sha1()
    with open(filepath, 'rb') as f:
        while chunk := f.read(8192): sha1.update(chunk)
    return sha1.hexdigest()

def switch_network(ssid, password=None):
    """
    Smart Switcher with Explicit Scan:
    1. Unblocks radio.
    2. Forces a rescan to find fresh networks (Fixes 'network not found').
    3. Tries to activate existing profile.
    4. Falls back to fresh device connection.
    """
    Log.info(f"Network Switch Request: {ssid}")
    
    # Step 0: Ensure Radio is Unblocked
    try:
        subprocess.run(["rfkill", "unblock", "wifi"], capture_output=True)
        subprocess.run(["nmcli", "radio", "wifi", "on"], capture_output=True)
    except Exception as e:
        Log.warn(f"Radio unblock warning: {e}")

    # Step 1: EXPLICIT SCAN (User Requested Fix)
    # This ensures the ESP32 is actually visible to the OS before we try to join.
    try:
        Log.info("Forcing Wi-Fi rescan to update network list...")
        subprocess.run(["nmcli", "dev", "wifi", "rescan"], check=False)
        # Waiting 1 second for the scan results to populate the OS cache
        time.sleep(1) 
    except Exception as e:
        Log.warn(f"Rescan trigger warning (proceeding anyway): {e}")

    # Give the interface a moment
    time.sleep(1)

    # Strategy 1: Activate Existing Profile (Preferred for Home Networks)
    try:
        # Check if profile exists silently
        check = subprocess.run(
            ["nmcli", "-t", "connection", "show", ssid], 
            capture_output=True, 
            text=True
        )
        
        if check.returncode == 0:
            Log.info(f"Saved profile '{ssid}' found. Activating...")
            subprocess.run(
                ["nmcli", "connection", "up", ssid], 
                check=True, 
                capture_output=True, 
                text=True
            )
            Log.success(f"Restored saved connection: {ssid}")
            time.sleep(3) # Wait for DHCP
            return True
    except subprocess.CalledProcessError as e:
        Log.warn(f"Could not activate saved profile (Error: {e.stderr.strip()}). Attempting fresh connection...")

    # Strategy 2: Fresh Connection (Preferred for ESP32/New Networks)
    try:
        Log.info(f"Attempting device-level connection to {ssid}...")
        
        cmd = ["nmcli", "dev", "wifi", "connect", ssid]
        if password: 
            cmd.extend(["password", password])
        
        Log.info(f"Executing Command: {' '.join(cmd)}")
        subprocess.run(cmd, check=True, capture_output=True, text=True)
        
        Log.success(f"Connected to {ssid}")
        time.sleep(5) 
        return True
    except subprocess.CalledProcessError as e:
        Log.error(f"Connection failed for {ssid}", e.stderr.strip())
        return False
    except Exception as e:
        Log.error(f"Unexpected error switching to {ssid}", e)
        return False

def main():
    print("--- Brainiac 2 Deployment Pipeline: Senior Telemetry Build (v2.8) ---", flush=True)
    
    # 1. Access ESP AP
    if not switch_network(ESP_SSID): 
        return

    with open(URL_FILE, 'r') as f: base_url = f.read().strip()
    Log.info(f"Resolved Target: {base_url}")

    old_hashes = {}
    if os.path.exists(HASH_MANIFEST):
        with open(HASH_MANIFEST, 'r') as f: old_hashes = json.load(f)
    Log.info(f"Differential sync active. Tracking {len(old_hashes)} assets.")

    current_state_hashes = {}
    upload_queue = []
    
    Log.info("Calculating file-level delta...")
    for root, dirs, files in os.walk(SOURCE_DIR):
        if '.git' in root.split(os.sep): continue
        rel_root = os.path.relpath(root, SOURCE_DIR)
        for file in files:
            rel_path = os.path.normpath(os.path.join(rel_root, file))
            src_path = os.path.join(root, file)
            
            f_hash = get_file_hash(src_path)
            current_state_hashes[rel_path] = f_hash
            
            if old_hashes.get(rel_path) != f_hash:
                Log.info(f"  [DELTA] Change: {rel_path}")
                upload_queue.append((src_path, rel_path))
    
    print("\n", flush=True)
    Log.info(f"Scan complete. {len(upload_queue)} files queued.")

    # Pruning (Rule 2: Log every deletion)
    prune_queue = [p for p in old_hashes if p not in current_state_hashes]
    for p in prune_queue:
        target = p + (".gz" if any(p.endswith(e) for e in GZIP_EXTENSIONS) else "")
        Log.warn(f"  [PRUNE] Deleting: {target}")
        try:
            requests.delete(f"{base_url}/upload", headers={'X-File-Path': target}, timeout=5)
        except Exception as e: Log.error(f"  Failed: {target}", e)

    # Syncing (Rule 2: Log every transfer)
    for src_path, rel_path in upload_queue:
        is_gz = any(rel_path.endswith(ext) for ext in GZIP_EXTENSIONS)
        target = rel_path + (".gz" if is_gz else "")
        
        Log.info(f"  [SYNC] Sending {target}...")
        try:
            with open(src_path, 'rb') as f:
                content = gzip.compress(f.read()) if is_gz else f.read()

            r = requests.post(f"{base_url}/upload", files={'file': (target, content)}, headers={'X-File-Path': target}, timeout=15)
            if r.status_code == 200:
                Log.success(f"    Synced {target}")
            else:
                Log.error(f"    Server Rejected {target} (Code: {r.status_code})")
                current_state_hashes[rel_path] = old_hashes.get(rel_path)
            
            time.sleep(0.15) 
        except Exception as e:
            Log.error(f"    Sync failed: {target}", e)
            current_state_hashes[rel_path] = old_hashes.get(rel_path)

    with open(HASH_MANIFEST, 'w') as f: json.dump(current_state_hashes, f, indent=2)
    
    Log.info(f"Reverting network: {HOME_SSID}")
    switch_network(HOME_SSID, HOME_PASS)
    Log.success("Pipeline finished.")

if __name__ == "__main__":
    main()