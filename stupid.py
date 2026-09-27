AVAIL_CHARS = [#'q',
               'w',
               'p',
               's',
               'g',
               'k',
               'l',
            #    'x',
               'v',
               'b',
               'm']
PATTERN = '*e**s'

if __name__ == '__main__':
    # Find where asterisks are.
    aster_locs = []
    loc = 0
    for c in PATTERN:
        if c == '*':
            aster_locs.append(loc)
        loc += 1

    # Iter thru all combos.
    all_char_combos = []

    char_counters = [0 for _ in aster_locs]
    assert len(char_counters) >= 1

    running = True
    while running:
        # Log combination.
        pcopy = list(PATTERN)
        for i in range(len(aster_locs)):
            this_char = AVAIL_CHARS[char_counters[i]]
            pcopy[aster_locs[i]] = this_char
        all_char_combos.append(''.join(pcopy))

        # Tick counters.
        char_counters[0] += 1

        for i in range(len(char_counters)):
            if char_counters[i] >= len(AVAIL_CHARS):
                if i < len(char_counters) - 1:
                    # Carry over to next one.
                    char_counters[i] = 0
                    char_counters[i + 1] += 1
                else:
                    # This is it! We are done.
                    running = False
                    break
    
    # Print all combos.
    for cc in all_char_combos:
        print(cc)
