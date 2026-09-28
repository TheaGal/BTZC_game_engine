import os
from pathlib import Path
import shutil
import subprocess

PYTHON_CMD = 'python3'

MY_REPO_DIR = os.getcwd()
MY_SHADERS_DIR = f'{MY_REPO_DIR}/assets/shaders/'
MY_UPDATE_BUILD_SCRIPT_SCRIPT_PATH = f'{MY_REPO_DIR}/update_cmakelists.py'

TXP_RENDERER_REPO_DIR = f'{MY_REPO_DIR}/third_party/TXP_renderer'
TXP_RENDERER_COMPILE_SHADERS_SCRIPT_PATH = f'{TXP_RENDERER_REPO_DIR}/compile_shaders.py'
TXP_RENDERER_COMPILE_TEXTURES_SCRIPT_PATH = f'{TXP_RENDERER_REPO_DIR}/compile_textures.py'
TXP_RENDERER_UPDATE_BUILD_SCRIPT_SCRIPT_PATH = f'{TXP_RENDERER_REPO_DIR}/update_cmakelists.py'
TXP_RENDERER_SHADERS_DIR = f'{TXP_RENDERER_REPO_DIR}/assets/shaders/'


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


def copy_built_shaders_to_asset_dir():
    files_to_copy = find_existing_files([TXP_RENDERER_SHADERS_DIR], ['shader', 'shadrefl'])

    for f in files_to_copy:
        if not f.is_file():
            continue

        shutil.copyfile(f, Path(MY_SHADERS_DIR) / f.name)
        print(f'Copied: {f.name}')


if __name__ == '__main__':
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
