/* Inserted into SDL_EGL_CreateSurface in the build tree only. */
#ifdef SDL_VIDEO_DRIVER_ANDROID
    if (SDL_GetHintBoolean("NUVIO_HDR_REQUESTED", SDL_FALSE)) {
        const char *ext = _this->egl_data->eglQueryString(_this->egl_data->egl_display, EGL_EXTENSIONS);
        SDL_Log("[hdr-ui] EGL extensions: %s", ext ? ext : "none");
    }
    if (SDL_GetHintBoolean("NUVIO_HDR_UI", SDL_FALSE)) {
        const SDL_bool pq = SDL_EGL_HasExtension(_this, SDL_EGL_DISPLAY_EXTENSION, "EGL_EXT_gl_colorspace_bt2020_pq");
        EGLint r = 0, g = 0, b = 0, a = 0;
        SDL_SetHint("NUVIO_HDR_EGL", "none");
        _this->egl_data->eglGetConfigAttrib(_this->egl_data->egl_display, _this->egl_data->egl_config, EGL_RED_SIZE, &r);
        _this->egl_data->eglGetConfigAttrib(_this->egl_data->egl_display, _this->egl_data->egl_config, EGL_GREEN_SIZE, &g);
        _this->egl_data->eglGetConfigAttrib(_this->egl_data->egl_display, _this->egl_data->egl_config, EGL_BLUE_SIZE, &b);
        _this->egl_data->eglGetConfigAttrib(_this->egl_data->egl_display, _this->egl_data->egl_config, EGL_ALPHA_SIZE, &a);
        if (r != 10 || g != 10 || b != 10 || a != 2) {
            SDL_SetError("HDR requires RGBA1010102; EGL selected %d/%d/%d/%d", r, g, b, a);
            SDL_Log("[hdr-ui] %s", SDL_GetError());
            return EGL_NO_SURFACE;
        }
        if (pq) {
            attribs[attr - 1] = 0x309D; /* EGL_GL_COLORSPACE_KHR */
            attribs[attr++] = 0x3340; /* EGL_GL_COLORSPACE_BT2020_PQ_EXT */
            attribs[attr++] = EGL_NONE;
        }
        surface = _this->egl_data->eglCreateWindowSurface(
            _this->egl_data->egl_display, _this->egl_data->egl_config, nw, attribs);
        if (surface == EGL_NO_SURFACE) {
            EGLint error = _this->egl_data->eglGetError();
            SDL_Log("[hdr-ui] eglCreateWindowSurface failed: EGL error=0x%04x", error);
            SDL_EGL_SetErrorEx("HDR window surface failed", "eglCreateWindowSurface", error);
            return EGL_NO_SURFACE;
        }
        if (!pq) {
            int (*setDataspace)(ANativeWindow *, int32_t) = NULL;
            int result = -1;
            if (android_get_device_api_level() >= 28)
                setDataspace = (int (*)(ANativeWindow *, int32_t))dlsym(RTLD_DEFAULT, "ANativeWindow_setBuffersDataSpace");
            /* Set after EGL creation: the driver may reset the default dataspace. */
            if (setDataspace) result = setDataspace(nw, ADATASPACE_BT2020_PQ);
            if (result != 0) {
                _this->egl_data->eglDestroySurface(_this->egl_data->egl_display, surface);
                SDL_SetError("HDR BT2020_PQ dataspace unavailable or rejected (%d)", result);
                SDL_Log("[hdr-ui] %s", SDL_GetError());
                return EGL_NO_SURFACE;
            }
        }
        format_got = ANativeWindow_getFormat(nw);
        Android_SetFormat(format_wanted, format_got);
        SDL_SetHint("NUVIO_HDR_EGL", pq ? "pq" : "dataspace");
        return surface;
    }
#endif
