import hashlib
import os
from pathlib import Path
from playsound import playsound
import shutil
import subprocess
import time

PYTHON_CMD = 'python3'

MY_REPO_DIR = os.getcwd()
MY_SHADERS_DIR = f'{MY_REPO_DIR}/assets/shaders/'
MY_UPDATE_BUILD_SCRIPT_SCRIPT_PATH = f'{MY_REPO_DIR}/update_cmakelists.py'

TXP_RENDERER_REPO_DIR = f'{MY_REPO_DIR}/third_party/TXP_renderer'
TXP_RENDERER_COMPILE_SHADERS_SCRIPT_PATH = f'{TXP_RENDERER_REPO_DIR}/compile_shaders.py'
TXP_RENDERER_COMPILE_TEXTURES_SCRIPT_PATH = f'{TXP_RENDERER_REPO_DIR}/compile_textures.py'
TXP_RENDERER_UPDATE_BUILD_SCRIPT_SCRIPT_PATH = f'{TXP_RENDERER_REPO_DIR}/update_cmakelists.py'
TXP_RENDERER_SHADERS_DIR = f'{TXP_RENDERER_REPO_DIR}/assets/shaders/'

WATCH_DIRS_FOR_UPDATE = [f'{MY_REPO_DIR}/assets_raw/', f'{TXP_RENDERER_REPO_DIR}/assets_raw/']
NOTIFY_UPDATE_SOUND_FNAME = f'{MY_REPO_DIR}/assets/audio/i_came.mp3'


def find_existing_files(search_dirs: list[str], search_extensions: list[str]) -> list[Path]:
    
    all_found_files: list[Path] = []
    for search_dir in search_dirs:
        for search_ext in search_extensions:
            # Convert `search_ext` to case-insensitive extension.
            case_insensitive_ext = '*.'
            for ext_char in search_ext:
                case_insensitive_ext += f'[{ext_char.lower()}{ext_char.upper()}]'

            # Search directory for extension.
            files = list(Path(search_dir).rglob(case_insensitive_ext))
            all_found_files.extend(files)

    return all_found_files


def hash_file(file_path: Path):  # @COPYPASTA
    hash_func = hashlib.new('sha512')

    with open(file_path, 'rb') as file:
        # Read the file in chunks of 8192 bytes
        while chunk := file.read(8192):
            hash_func.update(chunk)

    return hash_func.hexdigest()


def copy_built_shaders_to_asset_dir():
    files_to_copy = find_existing_files([TXP_RENDERER_SHADERS_DIR], ['shader', 'shadrefl'])

    for f in files_to_copy:
        if not f.is_file():
            continue

        dest_path = Path(MY_SHADERS_DIR) / f.name
        if dest_path.exists() and hash_file(f) == hash_file(dest_path):
            continue  # skip because nothing changed

        shutil.copyfile(f, dest_path)
        print(f'Copied: {f.name}')


def do_compile_assets():
    print('=' * 80)
    print(' Compile TXP Renderer Shaders')
    print('=' * 80)
    subprocess.call([PYTHON_CMD, TXP_RENDERER_COMPILE_SHADERS_SCRIPT_PATH],
                     cwd=TXP_RENDERER_REPO_DIR)
    print()
    print()

    print('=' * 80)
    print(' Update TXP Renderer Build Script')
    print('=' * 80)
    subprocess.call([PYTHON_CMD, TXP_RENDERER_UPDATE_BUILD_SCRIPT_SCRIPT_PATH],
                    cwd=TXP_RENDERER_REPO_DIR)
    print('  ... done')
    print()
    print()

    print('=' * 80)
    print(' Copy TXP Renderer Shaders to Asset Directory')
    print('=' * 80)
    copy_built_shaders_to_asset_dir()
    print()
    print()

    print('=' * 80)
    print(' Compile Textures (using TXP Renderer\'s script)')
    print('=' * 80)
    subprocess.call([PYTHON_CMD, TXP_RENDERER_COMPILE_TEXTURES_SCRIPT_PATH],
                     cwd=MY_REPO_DIR)
    print()
    print()

    print('=' * 80)
    print(' Update Build Script')
    print('=' * 80)
    subprocess.call([PYTHON_CMD, MY_UPDATE_BUILD_SCRIPT_SCRIPT_PATH],
                    cwd=MY_REPO_DIR)
    print('  ... done')
    print()
    print()


def check_if_changed(last_mod_hashes: dict[Path, str]):
    has_changed = False

    for dir in WATCH_DIRS_FOR_UPDATE:
        for subdir, _, files in os.walk(dir):
            for file in files:
                file_path_str = Path(subdir) / file
                file_hash = hash_file(file_path_str)

                if file_path_str not in last_mod_hashes.keys() or \
                   last_mod_hashes[file_path_str] != file_hash:
                    has_changed = True

                last_mod_hashes[file_path_str] = file_hash

    return has_changed


if __name__ == '__main__':
    do_compile_assets()

    # Watch for any updates/changes.
    try:
        first_iter = True
        last_mod_hashes: dict[Path, str] = {}

        while True:
            has_changed = check_if_changed(last_mod_hashes)

            if not first_iter and has_changed:
                do_compile_assets()
                # check_if_changed(last_mod_hashes)  # to force infinite compilation loop from happening (this would introduce a bug if a save happened during compilation)
                playsound(NOTIFY_UPDATE_SOUND_FNAME)

            time.sleep(1)
            first_iter = False
    except KeyboardInterrupt:
        pass
