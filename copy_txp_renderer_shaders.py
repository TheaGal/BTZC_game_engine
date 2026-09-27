from pathlib import Path
import shutil

MY_SHADERS_DIR = './assets/shaders/'
TXP_RENDERER_SHADERS_DIR = './third_party/TXP_renderer/assets/shaders/'


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


if __name__ == '__main__':
    files_to_copy = find_existing_files([TXP_RENDERER_SHADERS_DIR], ['shader', 'shadrefl'])

    for f in files_to_copy:
        if not f.is_file():
            continue

        shutil.copyfile(f, Path(MY_SHADERS_DIR) / f.name)
        print(f'Copied: {f.name}')
