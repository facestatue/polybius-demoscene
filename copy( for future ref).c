#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <time.h>
#define WIN_WIDTH  600
#define WIN_HEIGHT 480

static Display* dis;
static int         screen;
static Window      win;
static GC          gc;
static XImage* x_image;

static unsigned int pixel_buffer[WIN_WIDTH * WIN_HEIGHT];
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

void nano_delay(long sec, long nsec) {
    struct timespec req = { sec, nsec };
    long ret;

    asm volatile(
        "syscall"
        : "=a" (ret)
        : "a" (35),      // rax: sys_nanosleep
          "D" (&req),    // rdi: pointer to requested time
          "S" (0)        // rsi: NULL (0) for remaining time
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
    XStoreName(dis, win, "spcshp");
    XMapWindow(dis, win);
    
    gc = XCreateGC(dis, win, 0, 0);
    
    x_image = XCreateImage(
        dis, 
        DefaultVisual(dis, screen), 
        DefaultDepth(dis, screen), 
        ZPixmap, 
        0, 
        (char*)pixel_buffer, 
        WIN_WIDTH, 
        WIN_HEIGHT, 
        32, 
        0
    );

    for (int i=0;i<sizeof(keyboard_state)/4;i++) {keyboard_state[i] = 0;}
}

void write_pixel(int x, int y, u_int32_t color) {
    pixel_buffer[(y * WIN_WIDTH) + x] = color;
}

void draw_line_bresenham(int x1, int y1, int x2, int y2, u_int32_t color) {
    int dx, dy, sx, sy, cx, cy, err, err2;
    dx = NOSTD_abs(x2 - x1);
    dy = NOSTD_abs(y2 - y1);
    
    sx = (x1 < x2) ? 1 : -1;
    sy = (y1 < y2) ? 1 : -1;
    
    err = dx - dy; cx = x1; cy = y1;
    
    while (1) {
        write_pixel(cx, cy, color);
        
        if (cx == x2 && cy == y2) { break; }
            
        err2 = 2 * err;
        if (err2 > -dy) {
            err -= dy;
            cx += sx;
        }

        if (err2 < dx) {
            err += dx;
            cy += sy;
        }
    }
}

// Literally the entire game goes here.
void render() {
    static unsigned int frame = 0;
    frame++;

    for (int x=0;x<WIN_WIDTH;x++) { for (int y=0;y<WIN_HEIGHT;y++) {
        write_pixel(x, y, 0);
        draw_line_bresenham(WIN_WIDTH>>1, WIN_HEIGHT>>1, 100+cos_lut[(frame + 0x20) & 127], 200+cos_lut[frame & 127], 0xFFFFFFFF);
    }}

    XPutImage(dis, win, gc, x_image, 0, 0, 0, 0, WIN_WIDTH, WIN_HEIGHT);
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
        nano_delay(0, 16666666);
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

