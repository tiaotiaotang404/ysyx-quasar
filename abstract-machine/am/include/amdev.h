#ifndef __AMDEV_H__
#define __AMDEV_H__

// **MAY SUBJECT TO CHANGE IN THE FUTURE**

#define AM_DEVR_(id, R_, perm, ...) \
  enum { AM_##R_ = (id) }; \
  typedef struct { __VA_ARGS__; } AM_##R_##_T;

AM_DEVR_( 1, UART_CONFIG,  RD, bool present);
AM_DEVR_( 2, UART_TX,      WR, char data);
AM_DEVR_( 3, UART_RX,      RD, char data);
AM_DEVR_( 4, TIMER_CONFIG, RD, bool present, has_rtc);
AM_DEVR_( 5, TIMER_RTC,    RD, int year, month, day, hour, minute, second);
AM_DEVR_( 6, TIMER_UPTIME, RD, uint64_t us);
AM_DEVR_( 7, INPUT_CONFIG, RD, bool present);
AM_DEVR_( 8, INPUT_KEYBRD, RD, bool keydown; int keycode);
AM_DEVR_( 9, GPU_CONFIG,   RD, bool present, has_accel; int width, height, vmemsz);
AM_DEVR_(10, GPU_STATUS,   RD, bool ready);
AM_DEVR_(11, GPU_FBDRAW,   WR, int x, y; void *pixels; int w, h; bool sync);
AM_DEVR_(12, GPU_MEMCPY,   WR, uint32_t rd; void *src; int size);
AM_DEVR_(13, GPU_RENDER,   WR, uint32_t root);
AM_DEVR_(14, AUDIO_CONFIG, RD, bool present; int bufsize);
AM_DEVR_(15, AUDIO_CTRL,   WR, int freq, channels, samples);
AM_DEVR_(16, AUDIO_STATUS, RD, int count);
AM_DEVR_(17, AUDIO_PLAY,   WR, Area buf);
AM_DEVR_(18, DISK_CONFIG,  RD, bool present; int blksz, blkcnt);
AM_DEVR_(19, DISK_STATUS,  RD, bool ready);
AM_DEVR_(20, DISK_BLKIO,   WR, bool write; void *buf; int blkno, blkcnt);
AM_DEVR_(21, NET_CONFIG,   RD, bool present);
AM_DEVR_(22, NET_STATUS,   RD, int rx_len, tx_len);
AM_DEVR_(23, NET_TX,       WR, Area buf);
AM_DEVR_(24, NET_RX,       WR, Area buf);

// Input

#define AM_KEYS(_) \
  _(ESCAPE) _(F1) _(F2) _(F3) _(F4) _(F5) _(F6) _(F7) _(F8) _(F9) _(F10) _(F11) _(F12) \
  _(GRAVE) _(1) _(2) _(3) _(4) _(5) _(6) _(7) _(8) _(9) _(0) _(MINUS) _(EQUALS) _(BACKSPACE) \
  _(TAB) _(Q) _(W) _(E) _(R) _(T) _(Y) _(U) _(I) _(O) _(P) _(LEFTBRACKET) _(RIGHTBRACKET) _(BACKSLASH) \
  _(CAPSLOCK) _(A) _(S) _(D) _(F) _(G) _(H) _(J) _(K) _(L) _(SEMICOLON) _(APOSTROPHE) _(RETURN) \
  _(LSHIFT) _(Z) _(X) _(C) _(V) _(B) _(N) _(M) _(COMMA) _(PERIOD) _(SLASH) _(RSHIFT) \
  _(LCTRL) _(APPLICATION) _(LALT) _(SPACE) _(RALT) _(RCTRL) \
  _(UP) _(DOWN) _(LEFT) _(RIGHT) _(INSERT) _(DELETE) _(HOME) _(END) _(PAGEUP) _(PAGEDOWN)

#define AM_KEY_NAMES(key) AM_KEY_##key,
enum {
  AM_KEY_NONE = 0,
  AM_KEYS(AM_KEY_NAMES)
};

// GPU

#define AM_GPU_TEXTURE  1
#define AM_GPU_SUBTREE  2
#define AM_GPU_NULL     0xffffffff

typedef uint32_t gpuptr_t;

struct gpu_texturedesc {
  uint16_t w, h;
  gpuptr_t pixels;
} __attribute__((packed));

struct gpu_canvas {
  uint16_t type, w, h, x1, y1, w1, h1;
  gpuptr_t sibling;
  union {
    gpuptr_t child;
    struct gpu_texturedesc texture;
  };
} __attribute__((packed));

#endif
