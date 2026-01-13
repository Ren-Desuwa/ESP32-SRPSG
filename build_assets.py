import os
import shutil
import gzip
import datetime
import hashlib

try:
    import SCons.Script
    IS_PIO_BUILD = True
except ImportError:
    IS_PIO_BUILD = False

# --- CONFIGURATION ---

# 1. Get the absolute path of THIS script file
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# 2. Resolve the source dir relative to the script
# This moves Up 2 levels from the script's location, then into Websites/braniac
SOURCE_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, '../../Websites/brainiac'))

DATA_DIR = os.path.join(SCRIPT_DIR, 'data')
BACKUP_DIR = os.path.join(SCRIPT_DIR, 'backups')
HASH_FILE = os.path.join(SCRIPT_DIR, '.last_build_hash')
MSG_FILE = os.path.join(SCRIPT_DIR, 'commit_message.txt')

GZIP_EXTENSIONS = {'.html', '.css', '.js', '.json', '.xml', '.svg', '.txt'}
# ---------------------
# ... imports ...

print(f"DEBUG: Current Working Dir: {os.getcwd()}")
abs_source = os.path.abspath(SOURCE_DIR)
print(f"DEBUG: Looking for source at: {abs_source}")
if not os.path.exists(abs_source):
    print("❌ ERROR: Source directory NOT FOUND!")
else:
    print("✅ OK: Source directory found.")
    
def get_dir_hash(directory):
    """Generates a hash based on file contents to detect changes."""
    sha_hash = hashlib.sha1()
    if not os.path.exists(directory):
        return "0"
    
    # Sort to ensure consistent order
    for root, dirs, files in os.walk(directory):
        for names in sorted(files):
            filepath = os.path.join(root, names)
            try:
                # Hash the file content
                with open(filepath, 'rb') as f:
                    while chunk := f.read(8192):
                        sha_hash.update(chunk)
                # Hash the filename too (in case of renames)
                sha_hash.update(filepath.encode())
            except OSError:
                pass
    return sha_hash.hexdigest()

def get_commit_message():
    """Reads the custom message file if it exists."""
    if os.path.exists(MSG_FILE):
        with open(MSG_FILE, 'r') as f:
            msg = f.read().strip()
        # Optional: Delete file after reading so next build is auto?
        # os.remove(MSG_FILE) 
        if msg: return msg
    
    return f"Auto-build {datetime.datetime.now().strftime('%H:%M:%S')}"

def create_backup():
    if not os.path.exists(DATA_DIR):
        return
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_path = os.path.join(BACKUP_DIR, f"backup_{timestamp}")
    print(f"📦 [Asset Pipeline] Changes detected! Backing up to '{backup_path}'...")
    shutil.copytree(DATA_DIR, backup_path)

def process_files(message):
    print(f"🚀 [Asset Pipeline] Rebuilding '{DATA_DIR}' from '{SOURCE_DIR}'...")
    
    # 1. Clear Data Dir
    if os.path.exists(DATA_DIR):
        shutil.rmtree(DATA_DIR)
    os.makedirs(DATA_DIR)

    # 2. Compress/Copy
    for root, dirs, files in os.walk(SOURCE_DIR):
        rel_path = os.path.relpath(root, SOURCE_DIR)
        dest_root = os.path.join(DATA_DIR, rel_path)
        
        if not os.path.exists(dest_root):
            os.makedirs(dest_root)
            
        for file in files:
            src_file = os.path.join(root, file)
            name, ext = os.path.splitext(file)
            
            if ext.lower() in GZIP_EXTENSIONS:
                dest_file = os.path.join(dest_root, file + ".gz")
                with open(src_file, 'rb') as f_in:
                    with gzip.open(dest_file, 'wb') as f_out:
                        shutil.copyfileobj(f_in, f_out)
            else:
                dest_file = os.path.join(dest_root, file)
                shutil.copy2(src_file, dest_file)

    # 3. Create Version File
    with open(os.path.join(DATA_DIR, "version_info.txt"), "w") as f:
        f.write(f"Message: {message}\n")
        f.write(f"Build Date: {datetime.datetime.now()}\n")

def main():
    # 1. Calculate Current Hash
    current_hash = get_dir_hash(SOURCE_DIR)
    
    # 2. Read Last Hash
    last_hash = ""
    if os.path.exists(HASH_FILE):
        with open(HASH_FILE, 'r') as f:
            last_hash = f.read().strip()
            
    # 3. Check for Changes
    # If forced manual run OR hash mismatch -> Build
    if not IS_PIO_BUILD or (current_hash != last_hash):
        if IS_PIO_BUILD:
            print(f"🔎 [Asset Pipeline] Changes detected in '{SOURCE_DIR}'.")
        
        # Get message
        if IS_PIO_BUILD:
            msg = get_commit_message()
        else:
            msg = input("Enter Commit Message: ") or get_commit_message()

        create_backup()       # Backup old data
        process_files(msg)    # Build new data
        
        # Save new hash
        with open(HASH_FILE, 'w') as f:
            f.write(current_hash)
            
    else:
        print("✅ [Asset Pipeline] No changes in web sources. Skipping rebuild.")

if __name__ == "__main__":
    main()
elif IS_PIO_BUILD:
    main()