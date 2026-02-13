# X16 Unilib Font Editor

A desktop application for creating and editing Unicode bitmap fonts in the X16 Unilib Font (`.ulf`) format for the [Commander X16](https://www.commanderx16.com/) retro computer.

![Built with Qt](https://img.shields.io/badge/Built%20with-Qt%206-green)
![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)

## Features

- **Pixel-level glyph editing** with zoomable grid (8x-48x magnification)
  - Base glyphs: 8x16 pixels, 1-bit (black/white)
  - Overlay glyphs: 8x16 pixels, 2-bit (4 color levels including transparency)
- **Live composite preview** showing how base and overlay glyphs combine
- **Unicode mapping editor** with tree-based block/entry management
  - Per-entry transformation flags: reverse, horizontal flip, vertical flip
  - 24-bit codepoint support for full Unicode coverage
  - Automatic character name lookup via ICU
- **Real-time text preview** rendering arbitrary text with the current font
- **Customizable color palette** for background, foreground, and overlay colors
- **Full undo/redo** for pixel edits, glyph operations, and map changes

## Building

### Prerequisites

- CMake 3.20+
- Qt 6 (Widgets)
- ICU (International Components for Unicode)
- macOS: CoreText, CoreFoundation, CoreGraphics frameworks

### Build

```bash
mkdir build && cd build
cmake ..
make
```

## Usage

```bash
# Start with an empty font
./x16unifontedit

# Open an existing font file
./x16unifontedit path/to/font.ulf
```

A sample font file is included at `testdata/unilib.ulf`.

## ULF File Format

The `.ulf` format is a headerless binary layout:

| Offset | Size | Contents |
|--------|------|----------|
| `0x0000` | 32 KB | 1024 overlay glyphs (8x16, 2bpp, 32 bytes each) |
| `0x8000` | 4 KB | 256 base glyphs (8x16, 1bpp, 16 bytes each) |
| `0x9000` | Variable | Unicode map blocks |

Each Unicode map block contains a 3-byte starting codepoint, an entry count, and per-entry records mapping codepoints to base/overlay glyph indices with transformation flags.

## UI Layout

The editor is organized into four main areas:

1. **Unicode Map** (left) -- Tree view of Unicode blocks and their glyph mappings
2. **Glyph Editors** (top right) -- Side-by-side base editor, overlay editor, composite preview, and reference info
3. **Glyph Grids** (middle) -- Thumbnail grids for browsing all base (16x16) and overlay (64x16) glyphs
4. **Text Preview** (bottom) -- Type text to see it rendered live with the current font
