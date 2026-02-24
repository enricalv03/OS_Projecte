#include "../fs/vfs.h"

static int tab_cycle_index = 0;

void tab_reset(void) {
    tab_cycle_index = 0;
}

static int starts_with(const char* str, const char* prefix, int prefix_len) {
    for (int i = 0; i < prefix_len; i++) {
        if (str[i] == 0 || str[i] != prefix[i])
            return 0;
    }
    return 1;
}

static int tc_strlen(const char* s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

static void tc_strcpy(char* dst, const char* src) {
    while (*src) *dst++ = *src++;
    *dst = 0;
}

/*
 * tab_complete(buffer, pos)
 *
 * Finds the word being typed (after the last space), matches it against
 * directory entries in the relevant directory, and overwrites the word
 * with the match.  Returns new pos (or original pos if no match).
 *
 * Cycling: each call advances tab_cycle_index.  The caller must call
 * tab_reset() on any non-Tab keypress.
 */
int tab_complete(char* buffer, int pos) {
    if (pos <= 0) return pos;

    /* Find start of the current word (after last space) */
    int word_start = pos;
    while (word_start > 0 && buffer[word_start - 1] != ' ')
        word_start--;

    char* word = &buffer[word_start];
    int word_len = pos - word_start;
    if (word_len <= 0) return pos;

    /*
     * Split the word into a directory part and a name prefix.
     * E.g. "pro"         → dir = CWD,             prefix = "pro"
     *      "/tmp/pro"    → dir = lookup("/tmp"),   prefix = "pro"
     *      "sub/pro"     → dir = lookup(CWD+"/sub"), prefix = "pro"
     */
    int last_slash = -1;
    for (int i = 0; i < word_len; i++)
        if (word[i] == '/') last_slash = i;

    vfs_node_t* dir;
    const char* prefix;
    int prefix_len;

    if (last_slash < 0) {
        dir = vfs_get_cwd();
        prefix = word;
        prefix_len = word_len;
    } else {
        prefix = &word[last_slash + 1];
        prefix_len = word_len - last_slash - 1;

        if (last_slash == 0) {
            dir = vfs_get_root();
        } else {
            /* Build directory path: copy chars before last_slash, NUL-term */
            char dir_path[64];
            const char* cwd_path = vfs_get_cwd_path();

            if (word[0] == '/') {
                int j = 0;
                for (int i = 0; i < last_slash && j < 63; i++, j++)
                    dir_path[j] = word[i];
                dir_path[j] = 0;
            } else {
                int j = 0;
                for (int i = 0; cwd_path[i] && j < 62; i++, j++)
                    dir_path[j] = cwd_path[i];
                if (j > 0 && dir_path[j - 1] != '/') dir_path[j++] = '/';
                for (int i = 0; i < last_slash && j < 63; i++, j++)
                    dir_path[j] = word[i];
                dir_path[j] = 0;
            }

            dir = vfs_lookup(dir_path);
        }
    }

    if (!dir || dir->type != VFS_NODE_DIR) return pos;

    /* Count matching entries and find the one at tab_cycle_index */
    vfs_dir_entry_t entry;
    unsigned int idx = 0;
    int match_count = 0;
    int target = tab_cycle_index;
    char matched_name[32];
    int matched_type = 0;
    int found = 0;

    while (vfs_readdir(dir, idx, &entry) == 0) {
        if (prefix_len == 0 || starts_with(entry.name, prefix, prefix_len)) {
            if (match_count == target) {
                tc_strcpy(matched_name, entry.name);
                matched_type = entry.type;
                found = 1;
            }
            match_count++;
        }
        idx++;
    }

    /* Wrap around if we passed the last match */
    if (!found && match_count > 0) {
        tab_cycle_index = 0;
        target = 0;
        idx = 0;
        while (vfs_readdir(dir, idx, &entry) == 0) {
            if (prefix_len == 0 || starts_with(entry.name, prefix, prefix_len)) {
                if (0 == target) {
                    tc_strcpy(matched_name, entry.name);
                    matched_type = entry.type;
                    found = 1;
                    break;
                }
                target--;
            }
            idx++;
        }
    }

    if (!found) return pos;

    tab_cycle_index++;

    /* Build the completed buffer:
     * Keep buffer[0..word_start-1], then the directory prefix (up to and
     * including last_slash), then the matched name. */
    int name_len = tc_strlen(matched_name);
    int dir_prefix_len = (last_slash >= 0) ? last_slash + 1 : 0;
    int new_pos = word_start + dir_prefix_len + name_len;

    /* Append '/' for directories */
    int append_slash = (matched_type == VFS_NODE_DIR) ? 1 : 0;

    if (new_pos + append_slash >= 63) return pos;

    /* The directory prefix (e.g. "/tmp/" or "sub/") is already in
     * buffer[word_start..word_start+dir_prefix_len-1], so we only
     * need to write the matched name after it. */
    int dst = word_start + dir_prefix_len;
    for (int i = 0; i < name_len && dst < 63; i++, dst++)
        buffer[dst] = matched_name[i];
    if (append_slash) buffer[dst++] = '/';
    buffer[dst] = 0;

    return dst;
}
