# Micro-Esp

A simple firmware micro-kernel for ESP32 NodeMCU board with an interactive shell and file system support.

This micro-kernel is built using ESP-IDF tool, specially designed for NodeMCU ESP32 board.

## Features

- Interactive shell with UART serial communication
- SPIFFS file system support
- File management commands (ls, cat, write, rm, edit)
- Real-time command execution
- Support for file editing with append/truncate modes

## Building

```bash
# Configure ESP-IDF
idf.py menuconfig

# Build the project
idf.py build

# Flash to device
idf.py flash

# Monitor serial output
idf.py monitor
```

## Commands

| Command | Usage                                   | Description                                                                                                                                        |
| ------- | --------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------- |
| `ls`    | `ls`                                    | List all files in SPIFFS (no directory support)                                                                                                    |
| `cat`   | `cat <filename>`                        | Display file contents                                                                                                                              |
| `write` | `write <filename> <text>`               | Write text to a file (non-interactive)                                                                                                             |
| `rm`    | `rm <filename>`                         | Remove a file                                                                                                                                      |
| `edit`  | `edit [-a\|-n] <filename>`              | Interactive file editor with flags:<br>- `-a`: Append mode (default)<br>- `-n`: New/Truncate mode                                                  |
| `cp`    | `cp <filename> [destination_filename]`  | copyes file:<br>- `destination provided`: copies to destination<br>- `no destination provided`: New file created with no. added to end of filename |
| `mv`    | `edit <filename> <destination_filename` | renames/moves file:<br>- error given if destination file already exists                                                                            |
| `clear` | `clear`                                 | performs scro;;ing the screen to show clean terminal just like `ctrl`+`l` in linux                                                                 |
| `exit`  | `exit`                                  | Exit the shell and stop the firmware                                                                                                               |

### Edit Command Details

The `edit` command provides an interactive text editor with the following features:

- **Append Mode** (`-a`): Opens existing file and appends new content
- **New Mode** (`-n`): Creates a new file or truncates existing file
- **Shortcuts**:
    - `:w` - Save changes
    - `:q` - Quit without saving
    - `Ctrl+C` - Exit editor immediately

Example:

```bash
edit -a myfile.txt     # Append to existing file
edit -n newfile.txt    # Create or truncate file
edit config.txt        # Default append mode
```

## Configuration

### Flash Size

Configure the flash memory size via menuconfig:

```bash
idf.py menuconfig
# Serial Flasher Config → Flash size
```

Default: 2MB (can be changed to 4MB or 8MB)

### Partition Layout

Edit `partitions.csv` to customize partition sizes:

```csv
nvs,      data, nvs,     0x9000,   0x5000
otadata,  data, ota,     0xe000,  0x2000
app0,     app,  ota_0,   0x10000, 1M
spiffs,   data, spiffs,  0x110000, 1M
```

### UART Configuration

- **Baud Rate**: 115200 (configurable in menuconfig)
- **Port**: UART_NUM_0
- **Buffer Size**: 128 bytes

## Troubleshooting

### Partition Table Too Large

Error: `Partitions tables occupies X bytes which does not fit in configured flash size`

**Solution**: Either increase flash size in menuconfig or reduce partition sizes in `partitions.csv`

### Undefined Reference to `shell_running`

Ensure `shell_running` is declared as a global variable in `micro-esp.c`

## Project Structure

```
micro-esp/
├── main/
│   ├── micro-esp.c      # Main firmware entry point
│   ├── shell/
│   │   ├── commands.c   # Shell commands implementation
│   │   ├── commands.h   # Commands header
│   │   └── parser.c     # Command parser
│   └── fs/
│       ├── fs.c         # SPIFFS file system wrapper
│       └── fs.h         # File system header
├── partitions.csv       # Partition table configuration
├── sdkconfig            # ESP-IDF configuration
└── CMakeLists.txt       # Build configuration
```

## Hardware Requirements

- ESP32 NodeMCU board
- USB-UART adapter (usually built-in)
- Micro USB cable
- 2MB or larger flash memory (4MB+ recommended)
