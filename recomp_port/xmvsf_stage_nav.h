// XMVSF 87.487: pure stage-direction input and bounded 5-column navigation.
// No SDL dependency. The stage picker has its OWN edge latch; other menus
// retain their existing exact one-press-one-action policy.
#pragma once
namespace port::xmvsf_home::stage_nav {
constexpr unsigned Left = 1u, Right = 2u, Up = 4u, Down = 8u;
constexpr unsigned Directions = Left | Right | Up | Down;

inline bool Single(unsigned d) { return d && !(d & (d - 1u)); }

// Disallow simultaneous/opposed/diagonal controls. An ambiguous press is
// neutral, not an arbitrary left/right or up/down preference.
inline unsigned Valid(unsigned raw) {
    const unsigned d = raw & Directions;
    return Single(d) ? d : 0u;
}

struct Edges {
    unsigned previousRaw = 0u;
    unsigned previousValid = 0u;
    bool wasActive = false;

    unsigned Press(unsigned raw, bool active) {
        raw &= Directions;
        const unsigned valid = Valid(raw);
        if (!active || !wasActive) {
            previousRaw = raw;
            previousValid = valid;
            wasActive = active;
            return 0u;  // held button from HOME cannot leak on picker open
        }
        const unsigned fresh = raw & ~previousRaw;
        // A held conflicting/diagonal input must rearm when it becomes
        // unambiguous after one direction is released.
        const unsigned result = valid &&
            ((fresh & valid) || (!previousValid && previousRaw && valid))
            ? valid : 0u;
        previousRaw = raw;
        previousValid = valid;
        return result;
    }
};

inline int Move(int selected, unsigned direction, int count,
                int pageSize = 10, int columns = 5) {
    if (count <= 0 || pageSize <= 0 || columns <= 0 ||
        selected < 0 || selected >= count || !Single(direction))
        return selected;
    const int begin = (selected / pageSize) * pageSize;
    const int inPage = selected - begin;
    const int available = (count - begin < pageSize) ? count - begin : pageSize;
    int row = inPage / columns;
    int col = inPage % columns;
    if (direction == Left) {
        if (col == 0) return selected; // NO five-column wrap
        --col;
    } else if (direction == Right) {
        if (col + 1 >= columns || row * columns + col + 1 >= available)
            return selected; // NO four-panel backwards jump
        ++col;
    } else if (direction == Up) {
        if (row == 0) return selected;
        --row;
    } else if (direction == Down) {
        if ((row + 1) * columns >= available) return selected;
        ++row;
        const int width = available - row * columns;
        if (col >= width) col = width - 1;
    } else return selected;
    return begin + row * columns + col;
}

inline int Page(int selected, bool previous, bool next,
                int count, int pageSize = 10) {
    if (count <= 0 || pageSize <= 0 ||
        selected < 0 || selected >= count || previous == next)
        return selected; // both shoulders = neutral
    const int pages = (count + pageSize - 1) / pageSize;
    if (pages < 2) return selected;
    const int current = selected / pageSize;
    const int nextPage = (current + (next ? 1 : pages - 1)) % pages;
    const int index = nextPage * pageSize + selected % pageSize;
    return index < count ? index : count - 1;
}
} // namespace port::xmvsf_home::stage_nav
