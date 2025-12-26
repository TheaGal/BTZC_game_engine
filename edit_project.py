from pathlib import Path


def find_src_entries() -> list[str]:
    CMAKELISTS_FNAME = './CMakeLists.txt'
    START_BLOCK = 'set(MAIN_SOURCES'
    END_BLOCK = ')'
    SRC_FILE_ENTRY_START = '${CMAKE_CURRENT_SOURCE_DIR}'

    block_process = 0  # 0:before; 1:within; 2:after;
    found_source_files = []
    with open(CMAKELISTS_FNAME) as f:
        for line in f:
            if block_process == 0:
                if line.strip() == START_BLOCK:
                    # Found start of block.
                    block_process = 1
            elif block_process == 1:
                if line.strip().startswith(SRC_FILE_ENTRY_START):
                    # Add file entry.
                    entry_start_str_len = len(SRC_FILE_ENTRY_START)+1  # +1 for '/' after.
                    found_source_files.append(line.strip()[entry_start_str_len:])
                elif line.strip() == END_BLOCK:
                    # Found end of block.
                    block_process = 2
            elif block_process == 2:
                # Exit reading file.
                break

    return found_source_files


def find_existing_files() -> list[Path]:
    SEARCH_DIRS = ['./src/']
    SEARCH_EXTENSIONS = ['h',
                         'hpp',
                         'ixx',  # I think this is for modules???
                         'c',
                         'cxx',
                         'cpp']
    all_found_files = []
    for search_dir in SEARCH_DIRS:
        for search_ext in SEARCH_EXTENSIONS:
            # Convert `search_ext` to case-insensitive extension.
            case_insensitive_ext = '*.'
            for ext_char in search_ext:
                case_insensitive_ext += f'[{ext_char.lower()}{ext_char.upper()}]'

            # Search directory for extension.
            files = list(Path(search_dir).rglob(case_insensitive_ext))
            all_found_files.extend(files)

    return all_found_files


def find_missing_src_entry_in_src_entries(src_entries: list[str],
                                          existing_files: list[Path]):
    missing_entries = []  # If the file exists but not the entry in cmake, then append!
    src_entry_paths = [Path(x) for x in src_entries]
    for existing_file in existing_files:
        if existing_file not in src_entry_paths:
            # Found missing entry.
            missing_entry = str(existing_file).replace('\\', '/')
            missing_entries.append(missing_entry)

    missing_entries.sort()
    return missing_entries


class Module:
    m_type: str
    VALID_TYPES = ["namespace", "func", "enum", "struct", "class"]

    def __init__(self, type: str):
        self.m_type = type
        assert self.m_type in self.VALID_TYPES


def strip_unnec_parts(buffer_lines: list[str]) -> list[str]:
    scan_mode = 0  # 0:code  1:sing-str  2:dbl-str  3:preproc  4:block-comment  5:comment

    erase_regions = []  # from_line, from_idx, to_line, to_idx, leave_space

    # Scan for erase regions.
    for line in buffer_lines:
        # Helper func.
        def get_char_safe(idx: int):
            if idx < 0 or idx >= len(line):
                return ''
            else:
                return line[idx]

        # Scan.
        c_idx = 0
        while True:
            c_char = get_char_safe(c_idx)
            c1_char = get_char_safe(c_idx + 1)
            c2_char = get_char_safe(c_idx + 2)

            # Exit if ran end of line.
            if c_char == '':
                break
            
            # Mode switch.
            if scan_mode == 0:
                # Code mode.
                pass
            elif scan_mode == 1:
                # Single str mode.
                pass
            elif scan_mode == 2:
                # Double str mode.
                pass
            elif scan_mode == 3:
                # Preprocessor mode.
                pass
            elif scan_mode == 4:
                # Block comment mode.
                pass
            elif scan_mode == 5:
                # Comment mode.
                erase_regions[-1][]
            else:
                # Unknown.
                assert False
                import sys; sys.exit(1)


def strip_block_comments(buffer_lines: list[str]) -> list[str]:
    is_block_comm_mode = False
    block_comm_regions = []

    # Gather block comments.
    line_idx = 0
    for line in buffer_lines:
        while True:
            if not is_block_comm_mode:
                start_idx = line.find("/*")
                if start_idx >= 0:
                    is_block_comm_mode = True
                    block_comm_regions.append({ "start_line": line_idx,
                                                "start_idx": start_idx, })
                else:
                    break  # Get out of while-true.
            else:
                end_idx = line.find("*/")
                if end_idx >= 0:
                    is_block_comm_mode = False
                    block_comm_regions[-1]["end_line"] = line_idx
                    block_comm_regions[-1]["end_idx"] = end_idx + 2  # 2 for "*/" chars.
                else:
                    break  # Get out of while-true.
        line_idx += 1

    # Remove block comments.
    for region in block_comm_regions:
        if region["start_line"] == region["end_line"]:
            # Cut out block comment from single line.
            replacement_line = buffer_lines[region["start_line"]][:region["start_idx"]]
            replacement_line += " "  # To prevent token mixing.
            replacement_line += buffer_lines[region["end_line"]][region["end_idx"]:]
            buffer_lines[region["start_line"]] = replacement_line
        else:
            # Replace begin line.
            replacement_line = buffer_lines[region["start_line"]][:region["start_idx"]]
            buffer_lines[region["start_line"]] = replacement_line

            # Replace end line.
            replacement_line = buffer_lines[region["end_line"]][region["end_idx"]:]
            buffer_lines[region["end_line"]] = replacement_line

            # Empty between lines.
            for i in range(region["start_line"] + 1, region["end_line"]):
                buffer_lines[i] = ""

    return buffer_lines


def strip_preprocessors(buffer_lines: list[str]) -> list[str]:
    leftover_lines = []

    is_preprocessor_line = False
    is_next_line_preprocessor_line = False

    for line in buffer_lines:
        line_stripped = line.strip()

        # Check whether current and/or next line(s) are preprocessor line(s).
        if len(line_stripped) > 0 and line_stripped[0] == '#':
            is_preprocessor_line = True

        if is_preprocessor_line and len(line_stripped) > 0 and line_stripped[-1] == '\\':
            is_next_line_preprocessor_line = True

        # Ensure preprocessor lines get removed.
        if not is_preprocessor_line:
            leftover_lines.append(line)
        
        # Move to next line.
        is_preprocessor_line = False
        if is_next_line_preprocessor_line:
            is_preprocessor_line = True
            is_next_line_preprocessor_line = False

    return leftover_lines


def strip_comments(buffer_lines: list[str]) -> list[str]:
    for line in buffer_lines:
        pass


def extract_modules(buffer_lines: list[str]) -> list[Module]:
    modules = []
    buffer_lines = strip_block_comments(buffer_lines)
    buffer_lines = strip_preprocessors(buffer_lines)
    buffer_lines = strip_comments(buffer_lines)
    for l in buffer_lines:
        print(l)
    import sys; sys.exit(1)


    return modules


def build_module_database(existing_files: list[Path]) -> list[Module]:
    modules = []
    for existing_file in existing_files:
        with open(existing_file, 'r', encoding='utf-8') as f:
            buffer_lines = f.readlines()
            modules.extend(extract_modules(buffer_lines))

    return modules


def print_quit_help():
    print("[quit/q]")
    print("  Exits the program.")


def print_help_help():
    print("[help/h]")
    print("  Displays this prompt.")


def print_list_help():
    print("[list/l]")
    print("  Lists all modules.")


def print_module_name_example():
    print("    EXAMPLE OF MODULE NAME: BT.world.c-Scene_loader.f-load_scene")
    print("      No prefix : namespace")
    print("      f-        : function or method")
    print("      e-        : enum or enum class")
    print("      s-        : struct")
    print("      c-        : class")


def print_view_help():
    print("[view/v] module_name")
    print("  Views properties of a module. If module name is not an exact match, " \
          "similar ones are suggested.")
    print_module_name_example()


def print_new_help():
    print("[new/n] module_name")
    print("  Creates a new module. If module name is an exact match with another, " \
          "raises an error message.")
    print_module_name_example()


def print_all_help():
    print_quit_help()
    print()
    print_help_help()
    print()
    print_list_help()
    print()
    print_view_help()
    print()
    print_new_help()


def check_token_exists(token_name: str) -> bool:
    pass


def token_view_interactive_mode(token_name: str):
    pass


def token_create_interactive_mode(token_name: str):
    pass


def search_for_token_and_print_results(query: str):
    pass


def interactive_mode(proj_files: list[Path]):
    # Loop for commands in interactive mode.
    while True:
        print()
        user_ans = input("コマンド⊳ ").split()  # Split on whitespace.
        print()
        
        # Check if input is valid.
        if len(user_ans) == 0:
            print("Enter 'help' or 'h' to view list of commands, or enter a command.")
            continue

        # 'quit'
        if user_ans[0].lower() in ['q', 'quit']:
            break

        # 'help'
        if user_ans[0].lower() in ['h', 'help']:
            print_all_help()
            continue

        # 'list'
        if user_ans[0].lower() in ['l', 'list']:
            search_for_token_and_print_results('')
            continue

        # 'view'
        if user_ans[0].lower() in ['v', 'view']:
            if len(user_ans) != 2:
                print_view_help()
                continue

            if check_token_exists(user_ans[1]):
                token_view_interactive_mode(user_ans[1])
            else:
                search_for_token_and_print_results(user_ans[1])
            continue

        # 'new'
        if user_ans[0].lower() in ['n', 'new']:
            if len(user_ans) != 2:
                print_new_help()
                continue

            if check_token_exists(user_ans[1]):
                print("ERROR: Token already exists.")
            else:
                token_create_interactive_mode(user_ans[1])
            continue


if __name__ == '__main__':
    print("STARTUP: Finding project files... ", end='', flush=True)

    src_entries = find_src_entries()
    existing_files = find_existing_files()

    files_missing_in_src_entries = \
        find_missing_src_entry_in_src_entries(src_entries, existing_files)

    print("DONE")


    print("STARTUP: Building module database... ", end='', flush=True)
    all_modules = build_module_database(existing_files)
    print("DONE")


    print("STARTUP finished.")
    print()
    print("Entering interactive mode!")

    interactive_mode(all_modules)







    # Print result.
    if len(files_missing_in_src_entries) > 0:
        print('==== MISSING ENTRIES ============================================')
    for missing_file in files_missing_in_src_entries:
        print(missing_file)

    # Ask to update cmakelists.
    user_ans = input("Update CMakeLists file? (Y/n): ").lower()
    if len(user_ans) == 0 or user_ans[0] == "y":
        print("Starting update CMakeLists script.")
        exec(open('./update_cmakelists.py').read()) 
