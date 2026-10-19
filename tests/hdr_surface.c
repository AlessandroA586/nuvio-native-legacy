/* Exercise the real SDL injection without Android hardware or EGL libraries. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#define SDL_VIDEO_DRIVER_ANDROID 1
#define SDL_FALSE 0
#define EGL_NO_SURFACE NULL
#define EGL_NONE 0x3038
#define EGL_EXTENSIONS 1
#define EGL_RED_SIZE 2
#define EGL_GREEN_SIZE 3
#define EGL_BLUE_SIZE 4
#define EGL_ALPHA_SIZE 5
#define SDL_EGL_DISPLAY_EXTENSION 0
#define RTLD_DEFAULT NULL
#define ADATASPACE_BT2020_PQ 163971072
#define _THIS Device *_this
typedef int EGLint;
typedef int SDL_bool;
typedef void EGLSurface;
typedef void ANativeWindow;
static int requested, enabled, pq, api, symbol, bits, createFails, dataResult;
static int creates, destroys, tags, attrs[9];
static char mode[32], errorText[160], logText[256];
static int SDL_GetHintBoolean(const char *key, int fallback) {
    (void)fallback;
    return !strcmp(key, "NUVIO_HDR_REQUESTED") ? requested : enabled;
}
static void SDL_SetHint(const char *key, const char *value) { (void)key; snprintf(mode, sizeof mode, "%s", value); }
static void SDL_Log(const char *format, ...) {
    va_list ap; va_start(ap, format); vsnprintf(logText, sizeof logText, format, ap); va_end(ap);
}
static void SDL_SetError(const char *format, ...) {
    va_list ap; va_start(ap, format); vsnprintf(errorText, sizeof errorText, format, ap); va_end(ap);
}
static const char *SDL_GetError(void) { return errorText; }
static void SDL_EGL_SetErrorEx(const char *message, const char *function, int error) {
    SDL_SetError("%s: %s 0x%04x", message, function, error);
}
static int android_get_device_api_level(void) { return api; }
static int setData(ANativeWindow *window, int32_t space) {
    assert(window && space == ADATASPACE_BT2020_PQ); tags++; return dataResult;
}
static void *dlsym(void *handle, const char *name) {
    (void)handle; assert(!strcmp(name, "ANativeWindow_setBuffersDataSpace"));
    return symbol ? (void *)setData : NULL;
}
static const char *extensions(void *display, int key) { (void)display; (void)key; return "EGL_KHR_gl_colorspace"; }
static int config(void *display, void *c, int key, int *value) {
    (void)display; (void)c; *value = key == EGL_ALPHA_SIZE ? 2 : bits; return 1;
}
static EGLSurface *create(void *display, void *c, void *window, const int *a) {
    (void)display; (void)c; (void)window; creates++;
    for (int i = 0; i < 9; i++) { attrs[i] = a[i]; if (a[i] == EGL_NONE) break; }
    return createFails ? NULL : (void *)1;
}
static int destroy(void *display, void *surface) { (void)display; assert(surface); destroys++; return 1; }
static int eglError(void) { return 0x3009; }
static int ANativeWindow_getFormat(void *window) { assert(window); return 43; }
static void Android_SetFormat(int wanted, int got) { assert(wanted == 43 && got == 43); }
typedef struct {
    void *egl_display, *egl_config;
    const char *(*eglQueryString)(void *, int);
    int (*eglGetConfigAttrib)(void *, void *, int, int *);
    EGLSurface *(*eglCreateWindowSurface)(void *, void *, void *, const int *);
    int (*eglDestroySurface)(void *, void *);
    int (*eglGetError)(void);
} Egl;
typedef struct { Egl *egl_data; } Device;
static int SDL_EGL_HasExtension(Device *device, int type, const char *name) {
    (void)device; (void)type; assert(!strcmp(name, "EGL_EXT_gl_colorspace_bt2020_pq")); return pq;
}
static EGLSurface *surface(Device *_this, void *nw) {
    int attribs[9] = { EGL_NONE }, attr = 1, format_wanted = 43, format_got;
    EGLSurface *surface;
#include "../android/app/src/main/cpp/hdr_surface.inc"
    return create(NULL, NULL, nw, attribs);
}
static void reset(void) {
    requested = enabled = symbol = 1; pq = createFails = dataResult = 0; api = 34; bits = 10;
    creates = destroys = tags = 0; mode[0] = errorText[0] = logText[0] = 0;
}
int main(void) {
    Egl egl = {NULL, NULL, extensions, config, create, destroy, eglError}; Device device = {&egl};
    reset(); assert(surface(&device, &device));
    assert(!strcmp(mode, "dataspace") && tags == 1 && attrs[0] == EGL_NONE);
    reset(); pq = 1; assert(surface(&device, &device));
    assert(!strcmp(mode, "pq") && !tags && attrs[0] == 0x309D && attrs[1] == 0x3340);
    reset(); api = 27; assert(!surface(&device, &device)); assert(destroys == 1 && !tags && errorText[0]);
    reset(); symbol = 0; assert(!surface(&device, &device)); assert(destroys == 1 && !strcmp(mode, "none"));
    reset(); dataResult = -22; assert(!surface(&device, &device)); assert(destroys == 1 && strstr(errorText, "-22"));
    reset(); bits = 8; assert(!surface(&device, &device)); assert(!creates && strstr(errorText, "8/8/8/2"));
    reset(); createFails = 1; assert(!surface(&device, &device));
    assert(strstr(logText, "0x3009") && strstr(errorText, "0x3009") && !tags);
    reset(); enabled = requested = 0; assert(surface(&device, &device));
    assert(creates == 1 && !tags && !mode[0] && attrs[0] == EGL_NONE);
    puts("PASS HDR dataspace, PQ extension, API/symbol/config/EGL failures, SDR unchanged");
}
