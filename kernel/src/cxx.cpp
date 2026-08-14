#include <choc/cxx.h>
#include <choc/kprintf.h>

typedef void (*ctor_fn)(void);

extern "C" ctor_fn __init_array_start[];
extern "C" ctor_fn __init_array_end[];

extern "C" void cxx_call_constructors(void) {
    for (ctor_fn *fn = __init_array_start; fn != __init_array_end; fn++) {
        (*fn)();
    }
}

class Banner {
public:
    Banner() : ready_(true) {}
    void print() const {
        kprintf("C++ runtime: constructors ran (%s)\n", ready_ ? "ok" : "no");
    }
private:
    bool ready_;
};

static Banner g_banner;

extern "C" void cxx_banner(void) {
    g_banner.print();
}
