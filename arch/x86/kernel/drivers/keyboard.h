typedef struct {
  unsigned char scan_code;
  char normal;
  char shift;
  char altgr;
} key_mapping_t;

extern key_mapping_t spanish_layout[];
extern key_mapping_t english_layout[];

__attribute__((externally_visible)) void init_keyboard_layouts(void);
__attribute__((externally_visible)) char get_char_from_scan_code(unsigned char scan_code, unsigned char modifiers);
void switch_layout(unsigned char layout_id);