# Directory structure and what was implemented

This file summarizes the directory support that was added and how to use it.

---

## 1. What was done (you can copy/adapt from the code)

### A. **ramfs.c** – hierarchical filesystem

- **`ramfs_node_t`** now has a **`parent`** pointer (which directory the node belongs to).
- **`ramfs_add_node(parent, name, data, size, disk_sector)`** – adds a **file** under `parent` (root or a dir).
- **`ramfs_add_dir(parent, name)`** – adds a **directory** under `parent`; returns the new `ramfs_node_t*` so you can add children to it.
- **`ramfs_readdir`** – lists only **children of the given directory** (uses `parent`).
- **`ramfs_vfs_lookup(dir, name)`** – finds a **child by name** in that directory.
- **`ramfs_get_vnode_for(ramfs_node_t*)`** – internal helper to get the VFS node for a ramfs node (for parent pointers).

Root is still `ramfs_root_storage`; all other nodes (files and dirs) live in `ramfs_nodes[]` and have `parent` set.

### B. **vfs.c** – multi-component paths

- **`vfs_lookup(path)`** now supports paths with **several components**, e.g.:
  - `"/"` or `""` → root
  - `"/readme.txt"` or `"readme.txt"` → file under root
  - `"/sistem"` → directory under root
  - `"/home/normal"` → directory `normal` inside `home`
- If the path **does not start with `/`**, a `/` is prepended (e.g. `readme.txt` → `/readme.txt`).
- Implementation: split the path by `/`, start at root, and for each component call `dir->ops->lookup(dir, component)`.

### C. **ramfs_init()** – default directory tree

In **`ramfs_init()`** the following directories are created:

| Path            | Meaning                          |
|-----------------|----------------------------------|
| `/`             | Root (already existed)           |
| `/sistem`       | System config (like /etc)        |
| `/admin`        | Root's home (like /root)         |
| `/home`         | User homes                       |
| `/home/normal`  | Normal user profile              |
| `/home/hacking` | Hacking/technical user profile   |
| `/bin`          | System binaries (for later)     |
| `/tmp`          | Temporary files                  |
| `/logs`         | Log files                        |
| `/boot`         | Boot-related files               |

### D. **cat** command – uses full path

- `cat readme.txt`
- `cat /readme.txt`
- `cat /sistem/somefile` (once you add files under `/sistem`)

### E. **ls** command – with optional path

- `ls` lists the current directory (CWD).
- `ls /sistem` or `ls /home` lists the given directory.

### F. **go** command – change directory (cd)

- `go /sistem`, `go /`, `go /home/normal`, `go ..` (parent directory).
- Prompt shows the current path.

### G. **mkdir** command – create directories

- `mkdir /tmp/test` creates `/tmp/test`.
- Works in all languages: `mkdir` (EN), `creadir` (CA/ES).
- Reports "already exists" if the path is taken.

### H. **write** command – create files with full path support

- `write /sistem/passwd root` creates a file at `/sistem/passwd`.
- `write myfile.txt hello` creates the file in the **current directory** (CWD), not always in `/`.

### K. **cp** command – copy files

- `cp test.txt backup.txt` copies a file within the current directory.
- `cp /home/normal/test.txt /tmp/copy.txt` copies between absolute paths.
- Works in all languages: `cp` (EN), `copia` (CA), `copiar` (ES).
- Uses `malloc()` for persistent heap storage of the copied data.

### I. **rm** command – remove files and empty directories

- `rm /tmp/test` removes the directory (with confirmation if it's a directory).

### J. **Bootloader – 120 sector load**

- `stage2.asm` now does two INT 0x13 reads across the CHS track boundary.
- Total capacity: 120 sectors = 60 KB for the kernel binary.

---

## 2. How to test

1. Build and run (e.g. `make all` then `make run` in QEMU).
2. At the shell, run:
   - **`ls`** – see root contents.
   - **`ls /home`** – see `normal`, `hacking`.
   - **`go /tmp`** – change to `/tmp`.
   - **`mkdir test`** – create `/tmp/test`.
   - **`ls`** – see `test`.
   - **`write myfile.txt hello world`** – creates `/tmp/myfile.txt`.
   - **`cat myfile.txt`** – prints `hello world`.
   - **`write /sistem/config system_data`** – creates file in `/sistem` from anywhere.
   - **`cat /sistem/config`** – prints `system_data`.

---

## 3. Completed steps

- **`ls <path>`** — done
- **`cd <path>`** (via `go`) — done
- **`mkdir <path>`** — done
- **`write` with full path and CWD** — done
- **`cp <src> <dest>`** — done
- **`mkdir` / `write` CWD fix** — relative paths now use CWD, not root
- **`mv <src> <dest>`** — done (supports mv file dir → dir/file, plus simple rename)
- **CWD fix for all commands** — `go`, `ls`, `cat`, `rm`, `cp`, `mv` all resolve relative paths via CWD
- **Bootloader 120-sector load** — done

## 4. Next steps

- **`find <path> <name>`** — Search for files by name under a directory tree.
- **Fix `block_init` crash** — ATA driver initialisation causes a page fault; currently disabled. Needs debugging.
- **Arch abstraction** — Introduce `arch_enable_interrupts()` / `arch_disable_interrupts()` / `arch_halt()` to separate CPU-specific code from generic kernel.

---

## 5. File summary

| File              | Changes |
|-------------------|--------|
| `kernel/fs/ramfs.h` | `RAMFS_MAX_FILES` = 48, `ramfs_mkdir_at_path`, `ramfs_get_vfs_root` |
| `kernel/fs/ramfs.c` | Parent pointer, `ramfs_add_dir`, hierarchical readdir/lookup, directory tree in `ramfs_init()`, `ramfs_mkdir_at_path`, `ramfs_add_dir_under` |
| `kernel/fs/vfs.c`   | Multi-component `vfs_lookup(path)`, fallback to ramfs root, CWD init on mount |
| `kernel/commands/commands.asm` | `handle_ls` with path, `handle_go` (cd), `handle_mkdir`, `handle_write` CWD-aware, `handle_rm`, `handle_cp`, `build_cwd_path` helper |
| `kernel/commands/languages/*.asm` | mkdir/creadir, cp/copia/copiar strings in EN, CA, ES |
| `boot/stage2.asm` | Two-read bootloader (120 sectors / 60 KB capacity) |
| `kernel/core/kernel_c.c` | Re-mount VFS root in `kernel_c_init` |
