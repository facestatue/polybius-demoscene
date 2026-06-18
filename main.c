#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <time.h>
#define WIN_WIDTH  600
#define WIN_HEIGHT 480

static Display* dis;
static int         screen;
static Window      win;
static GC          gc, gc_white, gc_black;
static XImage* x_image;

static unsigned int keyboard_state[256];

// thanks to https://ppelikan.github.io/drlut/ for the LUT generator
static const int8_t cos_lut[128] = {
 127, 127, 126, 126, 125, 123, 122, 120, 117, 115,
 112, 109, 106, 102,  98,  94,  90,  85,  81,  76,
  71,  65,  60,  54,  49,  43,  37,  31,  25,  19,
  12,   6,   0,  -6, -12, -19, -25, -31, -37, -43,
 -49, -54, -60, -65, -71, -76, -81, -85, -90, -94,
 -98,-102,-106,-109,-112,-115,-117,-120,-122,-123,
-125,-126,-126,-127,-127,-127,-126,-126,-125,-123,
-122,-120,-117,-115,-112,-109,-106,-102, -98, -94,
 -90, -85, -81, -76, -71, -65, -60, -54, -49, -43,
 -37, -31, -25, -19, -12,  -6,   0,   6,  12,  19,
  25,  31,  37,  43,  49,  54,  60,  65,  71,  76,
  81,  85,  90,  94,  98, 102, 106, 109, 112, 115,
 117, 120, 122, 123, 125, 126, 126, 127 };

int NOSTD_abs(int x) {
    return ((x + (x >> 31)) ^ (x >> 31));
}

void nano_delay(long nsec) {
    struct timespec req = {0, nsec};
    long ret; // say wsyscall bro say wsyscall

    asm volatile(
        "syscall"
        : "=a" (ret)
        : "a" (35),
          "D" (&req)  
        : "rcx", "r11", "memory"
    );
}

void init_x11() {
    dis = XOpenDisplay(0);
    if (!dis) {
        return;
    }
    
    screen = DefaultScreen(dis);
    
    // 2. Create the window
    win = XCreateSimpleWindow(
        dis, 
        RootWindow(dis, screen), 
        10, 10, WIN_WIDTH, WIN_HEIGHT, 
        1, 
        BlackPixel(dis, screen), 
        BlackPixel(dis, screen)
    );
    XSelectInput(dis, win, ExposureMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask);
    XStoreName(dis, win, "POLYBIUS");
    XMapWindow(dis, win);

    gc = XCreateGC(dis, win, 0, 0);

    for (int i=0;i<sizeof(keyboard_state)/4;i++) {keyboard_state[i] = 0;}
}

// Literally the entire game goes here.
void render() {
    static unsigned int frame = 0;
    frame++;

    XClearWindow(dis, win);

    XSetForeground(dis, gc, 0xFFFFFF);
    XDrawLine(dis, win, gc, WIN_WIDTH>>1, WIN_HEIGHT>>1, 200 + cos_lut[(frame + 0x20) & 127] >> 2, 200 + cos_lut[frame & 127] >> 2);
    XFlush(dis);
}

void _start() {
    __asm__("and $-16, %rsp\n");
    init_x11();
    
    int running = 1;
    XEvent event;
    
    while (running) {
        while (XPending(dis)) {
            XNextEvent(dis, &event);
            keyboard_state[event.xkey.keycode] = (event.type < 4) ? (event.type-2)^1 : 0;
            running = event.type - 17;
        }
        render();
        nano_delay(50000000); //this is 200 FPS but whatever
    }
    
    XDestroyImage(x_image);
    XFreeGC(dis, gc);
    XDestroyWindow(dis, win);
    XCloseDisplay(dis);
    
    __asm__(
        "mov $60, %rax\n"
        "xor %rdi, %rdi\n"
        "syscall"
    );
}

