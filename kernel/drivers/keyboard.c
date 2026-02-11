#include "keyboard.h"

// Corrected Spanish layout for MacBook
key_mapping_t spanish_layout[] = {
    {0x02, '1', '!', '¡'},      // 1 key
    {0x03, '2', '"', '²'},      // 2 key
    {0x04, '3', '·', '³'},      // 3 key
    {0x05, '4', '$', '€'},      // 4 key
    {0x06, '5', '%', '‰'},      // 5 key
    {0x07, '6', '&', '¬'},      // 6 key
    {0x08, '7', '/', '|'},      // 7 key
    {0x09, '8', '(', '['},      // 8 key
    {0x0A, '9', ')', ']'},      // 9 key
    {0x0B, '0', '=', '}'},      // 0 key
    {0x0C, '\'', '?', '¿'},     // ' key
    {0x0D, '¡', '¿', '¡'},      // ¡ key
    {0x10, 'q', 'Q', 'q'},      // Q key
    {0x11, 'w', 'W', 'w'},      // W key
    {0x12, 'e', 'E', 'é'},      // E key
    {0x13, 'r', 'R', 'r'},      // R key
    {0x14, 't', 'T', 't'},      // T key
    {0x15, 'y', 'Y', 'y'},      // Y key
    {0x16, 'u', 'U', 'ú'},      // U key
    {0x17, 'i', 'I', 'í'},      // I key
    {0x18, 'o', 'O', 'ó'},      // O key
    {0x19, 'p', 'P', 'p'},      // P key
    {0x1A, '`', '^', '`'},      // ` key
    {0x1B, '+', '*', '~'},      // + key
    {0x1E, 'a', 'A', 'á'},      // A key
    {0x1F, 's', 'S', 's'},      // S key
    {0x20, 'd', 'D', 'd'},      // D key
    {0x21, 'f', 'F', 'f'},      // F key
    {0x22, 'g', 'G', 'g'},      // G key
    {0x23, 'h', 'H', 'h'},      // H key
    {0x24, 'j', 'J', 'j'},      // J key
    {0x25, 'k', 'K', 'k'},      // K key
    {0x26, 'l', 'L', 'l'},      // L key
    {0x27, 'ñ', 'Ñ', 'ñ'},      // Ñ key
    {0x28, '´', '¨', '´'},      // ´ key
    {0x29, 'ç', 'Ç', 'ç'},      // Ç key
    {0x2B, '<', '>', '|'},      // < key
    {0x2C, 'z', 'Z', 'z'},      // Z key
    {0x2D, 'x', 'X', 'x'},      // X key
    {0x2E, 'c', 'C', 'c'},      // C key
    {0x2F, 'v', 'V', 'v'},      // V key
    {0x30, 'b', 'B', 'b'},      // B key
    {0x31, 'n', 'N', 'ñ'},      // N key (AltGr+n = ñ)
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