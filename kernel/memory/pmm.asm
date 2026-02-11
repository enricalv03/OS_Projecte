[bits 32]

section .text

global pmm_init
global pmm_alloc_page
global pmm_free_page
global pmm_get_free_pages
global total_pages_value
total_pages_value dd TOTAL_PAGES

BITMAP_BASE equ 0x00101000
MEMORY_BASE equ 0x00100000
PAGE_SIZE equ 4096
TOTAL_PAGES equ 32512
BITMAP_SIZE equ 4096

set_bit:
  push ebx
  mov ebx, eax
  shr eax, 3
  add eax, BITMAP_BASE
  and ebx, 7

  bts [eax], ebx

  pop ebx
  ret

clear_bit:
  push ebx
  mov ebx, eax
  shr eax, 3
  add eax, BITMAP_BASE
  and ebx, 7

  btr [eax], ebx

  pop ebx
  ret

test_bit:
  push ebx
  mov ebx, eax
  shr eax, 3
  add eax, BITMAP_BASE
  and ebx, 7
  
  bt [eax], ebx
  
  pop ebx
  ret

pmm_init:
  push eax
  push ecx
  push edi

  mov edi, BITMAP_BASE
  mov ecx, BITMAP_SIZE / 4
  xor eax, eax
  rep stosd

  mov eax, 1
  call set_bit

  pop edi
  pop ecx
  pop eax
  ret

pmm_alloc_page:
  push ebx
  push ecx
  push edx
  push esi

  mov esi, BITMAP_BASE
  mov ecx, BITMAP_SIZE

.scan_loop:
  mov al, [esi]
  cmp al, 0xFF
  je .next_byte

  mov edx, 0

.bit_loop:
  bt [esi], edx
  jnc .found_free
  inc edx
  cmp edx, 8
  jl .bit_loop

.next_byte:
  inc esi
  loop .scan_loop
  mov eax, 0
  jmp .done

.found_free:
  mov eax, esi
  sub eax, BITMAP_BASE
  shl eax, 3
  add eax, edx

  push eax
  call set_bit
  pop eax

  ; Calculate physical address: page_index * 4096 + MEMORY_BASE
  ; Since PAGE_SIZE = 4096 = 2^12, we can use shift
  shl eax, 12              ; Multiply by 4096
  add eax, MEMORY_BASE

.done:
  pop esi
  pop edx
  pop ecx
  pop ebx
  ret

pmm_free_page:
  push ebx
  push ecx

  ; Validate address range
  cmp eax, MEMORY_BASE
  jb .error

  mov ebx, MEMORY_BASE
  add ebx, TOTAL_PAGES * PAGE_SIZE
  cmp eax, ebx
  jae .error

  ; Validate page alignment
  push eax
  and eax, 0xFFF
  cmp eax, 0
  pop eax
  jne .error

  ; Calculate page index
  mov ecx, eax        ; Save address
  sub eax, MEMORY_BASE
  shr eax, 12         ; eax = page index

  ; Check if page is actually allocated (bit should be set)
  push eax
  call test_bit
  pop eax
  jnc .error          ; If CF=0, bit is not set (page not allocated)

  ; Page is allocated, free it
  call clear_bit

  mov eax, 1
  jmp .done

.error:
  mov eax, 0

.done:
  pop ecx
  pop ebx
  ret

pmm_get_free_pages:
  push ebx
  push ecx
  push edx
  push esi
  push edi

  mov esi, BITMAP_BASE
  mov ecx, BITMAP_SIZE
  xor eax, eax            ; free page counter
  xor edi, edi            ; total bits processed

.count_loop:
  cmp edi, TOTAL_PAGES
  jae .done

  mov bl, [esi]
  cmp bl, 0xFF
  je .skip_bits
  mov edx, 0

.bit_check:
  cmp edi, TOTAL_PAGES
  jae .done
  bt [esi], edx
  jc .bit_used
  inc eax

.bit_used:
  inc edx
  inc edi
  cmp edx, 8
  jl .bit_check
  jmp .next_byte

.skip_bits:
  add edi, 8

.next_byte:
  inc esi
  loop .count_loop

.done:
  pop edi
  pop esi
  pop edx
  pop ecx
  pop ebx
  ret