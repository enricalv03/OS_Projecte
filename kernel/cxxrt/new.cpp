typedef long unsigned int size_t;

extern "C" {
    void* malloc(unsigned int size);
    void  free(void* ptr);
}

void* operator new(size_t size)   { return malloc((unsigned int)size); }
void* operator new[](size_t size) { return malloc((unsigned int)size); }
void  operator delete(void* p)          noexcept { free(p); }
void  operator delete[](void* p)        noexcept { free(p); }
void  operator delete(void* p, size_t)  noexcept { free(p); }
void  operator delete[](void* p, size_t) noexcept { free(p); }
