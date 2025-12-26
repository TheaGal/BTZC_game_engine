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
    line_idx = 0
    for line in buffer_lines:
        # Helper func.
        def get_char_safe(idx: int):
            if idx < 0 or idx >= len(line):
                return ''
            else:
                return line[idx]

        # Scan.
        c_idx = 0
        found_non_ws = False
        while True:
            cm_char = get_char_safe(c_idx - 1)
            c0_char = get_char_safe(c_idx + 0)
            c1_char = get_char_safe(c_idx + 1)

            # Mode switch.
            if scan_mode == 0:
                # Code mode.
                if c0_char == '\'':
                    erase_regions.append({})
                    erase_regions[-1]["from_line"] = line_idx
                    erase_regions[-1]["from_idx"]  = c_idx
                    scan_mode = 1
                    c_idx += 1
                elif c0_char == "\"":
                    erase_regions.append({})
                    erase_regions[-1]["from_line"] = line_idx
                    erase_regions[-1]["from_idx"]  = c_idx
                    scan_mode = 2
                    c_idx += 1
                elif c0_char == "#" and not found_non_ws:
                    erase_regions.append({})
                    erase_regions[-1]["from_line"] = line_idx
                    erase_regions[-1]["from_idx"]  = c_idx
                    scan_mode = 3
                    c_idx += 1
                elif c0_char == '/' and c1_char == '*':
                    erase_regions.append({})
                    erase_regions[-1]["from_line"] = line_idx
                    erase_regions[-1]["from_idx"]  = c_idx
                    scan_mode = 4
                    c_idx += 2
                elif c0_char == '/' and c1_char == '/':
                    erase_regions.append({})
                    erase_regions[-1]["from_line"] = line_idx
                    erase_regions[-1]["from_idx"]  = c_idx
                    scan_mode = 5
                    c_idx += 2
                else:
                    c_idx += 1

            elif scan_mode == 1:
                # Single str mode.
                if c0_char == '\\':
                    # Escape c1 char.
                    c_idx += 2
                elif c0_char == '\'':
                    erase_regions[-1]["to_line"] = line_idx
                    erase_regions[-1]["to_idx"]  = c_idx
                    scan_mode = 0
                    c_idx += 1
                else:
                    c_idx += 1

            elif scan_mode == 2:
                # Double str mode.
                if c0_char == '\\':
                    # Escape c1 char.
                    c_idx += 2
                elif c0_char == '\"':
                    erase_regions[-1]["to_line"] = line_idx
                    erase_regions[-1]["to_idx"]  = c_idx
                    scan_mode = 0
                    c_idx += 1
                else:
                    c_idx += 1

            elif scan_mode == 3:
                # Preprocessor mode.
                if c0_char == '':
                    # End of line.
                    if cm_char == '\\':
                        # Continue to next line of preprocessor mode.
                        pass
                    else:
                        # End preprocessor mode.
                        erase_regions[-1]["to_line"] = line_idx
                        erase_regions[-1]["to_idx"]  = c_idx
                        scan_mode = 0
                else:
                    c_idx += 1

            elif scan_mode == 4:
                # Block comment mode.
                if c0_char == '*' and c1_char == '/':
                    # End block comment mode.
                    erase_regions[-1]["to_line"] = line_idx
                    erase_regions[-1]["to_idx"]  = c_idx + 2
                    scan_mode = 0
                    c_idx += 2
                else:
                    c_idx += 1

            elif scan_mode == 5:
                # End comment mode.
                erase_regions[-1]["to_line"] = erase_regions[-1]["from_line"]
                erase_regions[-1]["to_idx"]  = len(line) - 1
                scan_mode = 0
                c_idx = len(line)

            else:
                # Unknown.
                assert False
                import sys; sys.exit(1)

            # Check if c0 is a non-whitespace char.
            if len(c0_char.strip()) > 0:
                found_non_ws = True

            # Exit if ran to end of line.
            if c0_char == '':
                assert scan_mode != 1
                assert scan_mode != 2
                assert scan_mode != 5
                break

        # Next line!
        line_idx += 1

    # Process erase regions.
    for region in erase_regions:
        if region["from_line"] == region["to_line"]:
            # Erase within single line.
            replacement_line = buffer_lines[region["from_line"]][:region["from_idx"]]
            replacement_line += " "  # To prevent token mixing.
            replacement_line += buffer_lines[region["to_line"]][region["to_idx"]:]
            buffer_lines[region["from_line"]] = replacement_line
        else:
            # Replace "from" line.
            replacement_line = buffer_lines[region["from_line"]][:region["from_idx"]]
            buffer_lines[region["from_line"]] = replacement_line

            # Replace "to" line.
            replacement_line = buffer_lines[region["to_line"]][region["to_idx"]:]
            buffer_lines[region["to_line"]] = replacement_line

            # Empty between lines.
            for i in range(region["from_line"] + 1, region["to_line"]):
                buffer_lines[i] = ""

    return buffer_lines


def strip_empty_lines(buffer_lines: list[str]) -> list[str]:
    non_empty_lines = []

    for line in buffer_lines:
        if len(line.strip()) > 0:
            non_empty_lines.append(line)

    return non_empty_lines


def extract_modules(buffer_lines: list[str]) -> list[Module]:
    modules = []
    buffer_lines = strip_unnec_parts(buffer_lines)
    buffer_lines = strip_empty_lines(buffer_lines)

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
