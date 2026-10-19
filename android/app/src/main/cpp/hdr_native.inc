/* Android-only helpers in the private SDL EGL translation unit. */
#ifdef SDL_VIDEO_DRIVER_ANDROID
#include <errno.h>
static int (*nvSetDataspace)(ANativeWindow *, int32_t);
static int32_t (*nvGetDataspace)(ANativeWindow *);
static ANativeWindow *nvHdrWindow;
static EGLSurface nvHdrSurface;
static int32_t nvHdrDataspace;
static const char *nvHdrName;
static int nvHdrFirstSwap;

static int nvHdrLoad(void)
{
    static int loaded;
    static void *library;
    const char *error;
    if (loaded) return nvSetDataspace != NULL;
    loaded = 1;
    dlerror();
    nvSetDataspace = (int (*)(ANativeWindow *, int32_t))dlsym(RTLD_DEFAULT, "ANativeWindow_setBuffersDataSpace");
    error = dlerror();
    SDL_Log("[hdr-ui] lookup=RTLD_DEFAULT symbol=setDataspace found=%d error=%s", nvSetDataspace != NULL, error ? error : "none");
    dlerror();
    library = dlopen("libnativewindow.so", RTLD_NOW | RTLD_LOCAL);
    error = dlerror();
    SDL_Log("[hdr-ui] lookup=dlopen library=libnativewindow.so found=%d error=%s", library != NULL, error ? error : "none");
    if (library) {
        dlerror();
        int (*resolved)(ANativeWindow *, int32_t);
        resolved = (int (*)(ANativeWindow *, int32_t))dlsym(library, "ANativeWindow_setBuffersDataSpace");
        error = dlerror();
        if (resolved) nvSetDataspace = resolved;
        SDL_Log("[hdr-ui] lookup=libnativewindow.so symbol=setDataspace found=%d error=%s", nvSetDataspace != NULL, error ? error : "none");
        nvGetDataspace = (int32_t (*)(ANativeWindow *))dlsym(library, "ANativeWindow_getBuffersDataSpace");
    }
    return nvSetDataspace != NULL;
}

static int nvHdrTag(ANativeWindow *window, int32_t space, const char *name, const char *stage)
{
    int result, savedErrno;
    int32_t before = nvGetDataspace ? nvGetDataspace(window) : -1, after;
    errno = 0;
    result = nvSetDataspace(window, space);
    savedErrno = errno;
    after = nvGetDataspace ? nvGetDataspace(window) : -1;
    if (!result && nvGetDataspace && after != space) result = -EINVAL;
    SDL_Log("[hdr-ui] attempt=%s stage=%s dataspace=%d before=%d after=%d rc=%d errno=%d",
            name, stage, space, before, after, result, savedErrno);
    return result;
}

static void nvHdrAfterSwap(EGLSurface surface)
{
    if (SDL_GetHintBoolean("NUVIO_HDR_UI", SDL_FALSE) && surface == nvHdrSurface && nvHdrWindow && !nvHdrFirstSwap) {
        nvHdrFirstSwap = 1;
        nvHdrTag(nvHdrWindow, nvHdrDataspace, nvHdrName, "after-first-swap");
    }
}
#endif
