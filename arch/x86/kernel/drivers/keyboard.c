#include "keyboard.h"

// Corrected Spanish layout for MacBook
// NOTE: High-ASCII characters are encoded using explicit byte values
// so the table compiles cleanly on all compilers. The values match
// the low byte of the UTF-8 encoding that was previously used.
key_mapping_t spanish_layout[] = {
    {0x02, '1', '!', '\xA1'},   // 1 key (AltGr+1 = ¡)
    {0x03, '2', '"', '\xB2'},   // 2 key (AltGr+2 = ²)
    {0x04, '3', '\xB7', '\xB3'},// 3 key (Shift+3 = ·, AltGr+3 = ³)
    {0x05, '4', '$', '\xAC'},   // 4 key (AltGr+4 = € -> 0xAC)
    {0x06, '5', '%', '\xB0'},   // 5 key (AltGr+5 = ‰ -> 0xB0)
    {0x07, '6', '&', '\xAC'},   // 6 key (AltGr+6 = ¬ -> 0xAC)
    {0x08, '7', '/', '|'},      // 7 key
    {0x09, '8', '(', '['},      // 8 key
    {0x0A, '9', ')', ']'},      // 9 key
    {0x0B, '0', '=', '}'},      // 0 key
    {0x0C, '\'', '?', '\xBF'},  // ' key (AltGr+' = ¿)
    {0x0D, '\xA1', '\xBF', '\xA1'}, // ¡ key (variants of ¡ / ¿)
    {0x10, 'q', 'Q', 'q'},      // Q key
    {0x11, 'w', 'W', 'w'},      // W key
    {0x12, 'e', 'E', '\xA9'},   // E key (AltGr+e = é)
    {0x13, 'r', 'R', 'r'},      // R key
    {0x14, 't', 'T', 't'},      // T key
    {0x15, 'y', 'Y', 'y'},      // Y key
    {0x16, 'u', 'U', '\xBA'},   // U key (AltGr+u = ú)
    {0x17, 'i', 'I', '\xAD'},   // I key (AltGr+i = í)
    {0x18, 'o', 'O', '\xB3'},   // O key (AltGr+o = ó)
    {0x19, 'p', 'P', 'p'},      // P key
    {0x1A, '`', '^', '`'},      // ` key
    {0x1B, '+', '*', '~'},      // + key
    {0x1E, 'a', 'A', '\xA1'},   // A key (AltGr+a = á)
    {0x1F, 's', 'S', 's'},      // S key
    {0x20, 'd', 'D', 'd'},      // D key
    {0x21, 'f', 'F', 'f'},      // F key
    {0x22, 'g', 'G', 'g'},      // G key
    {0x23, 'h', 'H', 'h'},      // H key
    {0x24, 'j', 'J', 'j'},      // J key
    {0x25, 'k', 'K', 'k'},      // K key
    {0x26, 'l', 'L', 'l'},      // L key
    {0x27, '\xB1', '\x91', '\xB1'}, // Ñ key (ñ / Ñ / ñ)
    {0x28, '\xB4', '\xA8', '\xB4'}, // ´ key (´ / ¨ / ´)
    {0x29, '\xA7', '\x87', '\xA7'}, // Ç key (ç / Ç / ç)
    {0x2B, '<', '>', '|'},      // < key
    {0x2C, 'z', 'Z', 'z'},      // Z key
    {0x2D, 'x', 'X', 'x'},      // X key
    {0x2E, 'c', 'C', 'c'},      // C key
    {0x2F, 'v', 'V', 'v'},      // V key
    {0x30, 'b', 'B', 'b'},      // B key
    {0x31, 'n', 'N', '\xB1'},   // N key (AltGr+n = ñ)
    {0x32, 'm', 'M', 'm'},      // M key
    {0x33, ',', ';', ';'},      // , key
    {0x34, '.', ':', ':'},      // . key
    {0x35, '-', '_', '_'},      // - key
    {0x39, ' ', ' ', ' '},      // Space key
    {0, 0, 0, 0}                // End marker
};

// English layout for comparison
key_mapping_t english_layout[] = {
    {0x02, '1', '!', '1'},      // 1 key
    {0x03, '2', '@', '2'},      // 2 key
    {0x04, '3', '#', '3'},      // 3 key
    {0x05, '4', '$', '4'},      // 4 key
    {0x06, '5', '%', '5'},      // 5 key
    {0x07, '6', '^', '6'},      // 6 key
    {0x08, '7', '&', '7'},      // 7 key
    {0x09, '8', '*', '8'},      // 8 key
    {0x0A, '9', '(', '9'},      // 9 key
    {0x0B, '0', ')', '0'},      // 0 key
    {0x0C, '-', '_', '-'},      // - key
    {0x0D, '=', '+', '='},      // = key
    {0x10, 'q', 'Q', 'q'},      // Q key
    {0x11, 'w', 'W', 'w'},      // W key
    {0x12, 'e', 'E', 'e'},      // E key
    {0x13, 'r', 'R', 'r'},      // R key
    {0x14, 't', 'T', 't'},      // T key
    {0x15, 'y', 'Y', 'y'},      // Y key
    {0x16, 'u', 'U', 'u'},      // U key
    {0x17, 'i', 'I', 'i'},      // I key
    {0x18, 'o', 'O', 'o'},      // O key
    {0x19, 'p', 'P', 'p'},      // P key
    {0x1A, '[', '{', '['},      // [ key
    {0x1B, ']', '}', ']'},      // ] key
    {0x1E, 'a', 'A', 'a'},      // A key
    {0x1F, 's', 'S', 's'},      // S key
    {0x20, 'd', 'D', 'd'},      // D key
    {0x21, 'f', 'F', 'f'},      // F key
    {0x22, 'g', 'G', 'g'},      // G key
    {0x23, 'h', 'H', 'h'},      // H key
    {0x24, 'j', 'J', 'j'},      // J key
    {0x25, 'k', 'K', 'k'},      // K key
    {0x26, 'l', 'L', 'l'},      // L key
    {0x27, ';', ':', ';'},      // ; key
    {0x28, '\'', '"', '\''},    // ' key
    {0x2B, '\\', '|', '\\'},    // \ key
    {0x2C, 'z', 'Z', 'z'},      // Z key
    {0x2D, 'x', 'X', 'x'},      // X key
    {0x2E, 'c', 'C', 'c'},      // C key
    {0x2F, 'v', 'V', 'v'},      // V key
    {0x30, 'b', 'B', 'b'},      // B key
    {0x31, 'n', 'N', 'n'},      // N key
    {0x32, 'm', 'M', 'm'},      // M key
    {0x33, ',', '<', ','},      // , key
    {0x34, '.', '>', '.'},      // . key
    {0x35, '/', '?', '/'},      // / key
    {0x39, ' ', ' ', ' '},      // Space key
    {0, 0, 0, 0}                // End marker
};

// Global variables
static key_mapping_t* current_layout = spanish_layout;
static unsigned char current_layout_id = 0; // 0 = Spanish, 1 = English

void init_keyboard_layouts(void) {
    // Set default layout to Spanish
    current_layout = spanish_layout;
    current_layout_id = 0;
}

char get_char_from_scan_code(unsigned char scan_code, unsigned char modifiers) {
    // Find the key mapping for this scan code
    key_mapping_t* layout = current_layout;
    
    while (layout->scan_code != 0) {
        if (layout->scan_code == scan_code) {
            // Return appropriate character based on modifiers
            if (modifiers & 0x02) {  // AltGr pressed
                return layout->altgr;
            } else if (modifiers & 0x01) {  // Shift pressed
                return layout->shift;
            } else {
                return layout->normal;
            }
        }
        layout++;
    }
    
    return 0; // No mapping found
}

void switch_layout(unsigned char layout_id) {
    switch (layout_id) {
        case 0: // Spanish
            current_layout = spanish_layout;
            current_layout_id = 0;
            break;
        case 1: // English
            current_layout = english_layout;
            current_layout_id = 1;
            break;
        default:
            // Default to Spanish
            current_layout = spanish_layout;
            current_layout_id = 0;
            break;
    }
}