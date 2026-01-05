# BinaryViewer

A lightweight hex editor for Windows that allows you to view and edit binary files in hexadecimal format.

## Features

- **Hex Editing**: View and edit binary data in hexadecimal format with a clean, organized layout
- **Dual View**: Displays both hexadecimal values and ASCII representation side-by-side
- **Address Display**: Shows file offsets in hexadecimal for easy navigation
- **File Operations**:
  - Open any file type to view/edit as binary
  - Save modified files
- **Editing Capabilities**:
  - Direct hex value editing (nibble by nibble)
  - Insert new bytes
  - Delete bytes
  - Keyboard navigation (arrow keys, Home, End, Page Up/Down)
  - Mouse click positioning
- **Customization**:
  - Adjustable font size (8pt - 72pt)
  - Customizable cursor color
  - Auto-adjusting window size based on font
- **Smooth Scrolling**: Mouse wheel and scroll bar support
- **Visual Feedback**: Highlighted cursor position for easy tracking

## System Requirements

- Windows 7 or later
- Visual Studio 2022 (for building from source)

## Building

1. Open `BinaryViewer.sln` in Visual Studio 2022
2. Build the solution (F7 or Build → Build Solution)
3. The executable will be generated in the output directory

## Usage

### Opening Files

1. Launch BinaryViewer
2. Click **File → Open** or press the Open menu item
3. Select any file to view its binary content

### Editing Bytes

- **Navigate**: Use arrow keys, Page Up/Down, Home, End, or click with mouse
- **Edit**: Type hexadecimal digits (0-9, A-F) to modify byte values
- **Insert**: Press Insert key to add a new byte at the cursor position
- **Delete**: Press Delete key to remove the byte at the cursor position

### Saving Changes

1. Click **File → Save**
2. Choose the destination file name
3. Confirm if there are any uninitialized bytes (they will be saved as 0x00)

### Customization

- **Change Font Size**: Click **View → Font Size** and select from the available sizes
- **Change Cursor Color**: Click **View → Cursor Color** and pick your preferred color

## Display Format

```
00000000:  48 65 6C 6C 6F 20 57 6F 72 6C 64 21 00 00 00 00  Hello World!....
00000010:  FF FF FF FF 00 00 00 00 12 34 56 78 9A BC DE F0  .........4Vx....
```

- **Address Column**: 8-digit hexadecimal offset
- **Hex Column**: 16 bytes per line in hexadecimal format (space-separated)
- **ASCII Column**: ASCII representation of the bytes (non-printable characters shown as `.`)

## Keyboard Shortcuts

| Key | Action |
|-----|--------|
| Arrow Keys | Move cursor |
| Home | Move to start of current line |
| End | Move to end of current line |
| Ctrl+Home | Jump to file start |
| Ctrl+End | Jump to file end |
| Page Up/Down | Scroll by page |
| Insert | Insert new byte |
| Delete | Delete current byte |
| 0-9, A-F | Edit hexadecimal value |

## Technical Details

- Written in C++ using Win32 API
- Uses double buffering to prevent flicker
- Fixed-width Consolas font for clean alignment
- Supports files of any size (limited by available memory)
- 16 bytes per line display format

## License

See the repository for license information.

## Author

Created by mr11451

## Contributing

This application was written almost entirely by Copilot.