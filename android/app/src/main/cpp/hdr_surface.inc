/* Inserted into SDL_EGL_CreateSurface in the build tree only. */
#ifdef SDL_VIDEO_DRIVER_ANDROID
    if (SDL_GetHintBoolean("NUVIO_HDR_REQUESTED", SDL_FALSE)) {
        const char *ext = _this->egl_data->eglQueryString(_this->egl_data->egl_display, EGL_EXTENSIONS);
        SDL_Log("[hdr-ui] EGL extensions: %s", ext ? ext : "none");
    }
    if (SDL_GetHintBoolean("NUVIO_HDR_UI", SDL_FALSE)) {
        const char *mode = "none";
        EGLint space = 0;
        if (SDL_EGL_HasExtension(_this, SDL_EGL_DISPLAY_EXTENSION, "EGL_EXT_gl_colorspace_bt2020_pq")) {
            space = 0x3340; mode = "pq";
        } else if (SDL_EGL_HasExtension(_this, SDL_EGL_DISPLAY_EXTENSION, "EGL_EXT_gl_colorspace_scrgb_linear")) {
            /* Linear scRGB needs values above 1 for 203 nits. The requested
             * fixed 1010102 buffer clips at 80 nits: reject it rather than
             * mislabel a clipped UI as a working HDR result. */
            SDL_Log("[hdr-ui] scRGB available but requires float storage; using SDR");
            return EGL_NO_SURFACE;
        }
        SDL_SetHint("NUVIO_HDR_EGL", "none");
        EGLint r = 0, g = 0, b = 0, a = 0;
        _this->egl_data->eglGetConfigAttrib(_this->egl_data->egl_display, _this->egl_data->egl_config, EGL_RED_SIZE, &r);
        _this->egl_data->eglGetConfigAttrib(_this->egl_data->egl_display, _this->egl_data->egl_config, EGL_GREEN_SIZE, &g);
        _this->egl_data->eglGetConfigAttrib(_this->egl_data->egl_display, _this->egl_data->egl_config, EGL_BLUE_SIZE, &b);
        _this->egl_data->eglGetConfigAttrib(_this->egl_data->egl_display, _this->egl_data->egl_config, EGL_ALPHA_SIZE, &a);
        if (!space || r != 10 || g != 10 || b != 10 || a != 2) {
            return EGL_NO_SURFACE;
        }
        attribs[attr - 1] = 0x309D; /* EGL_GL_COLORSPACE_KHR */
        attribs[attr++] = space;
        attribs[attr++] = EGL_NONE;
        SDL_SetHint("NUVIO_HDR_EGL", mode);
    }
#endif
