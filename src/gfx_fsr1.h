// FSR 1 EASU + RCAS GLES2 implementation for upscaling.
// EASU = Edge Adaptive Spatial Upsampling (edge-aware upsampling)
// RCAS = Robust Contrast Adaptive Sharpening (optional second pass)
// Based on AMD FidelityFX Super Resolution 1.0
// uPar = (source width, source height, 0, 0)
// vUv = existing quad UV
#define NV_FSR1_EASU_SHADER \
"#if defined(GL_ES) && (!defined(GL_FRAGMENT_PRECISION_HIGH) || defined(NV_FSR1_MEDIUMP))\n" \
"#define FSR mediump\n" \
"#else\n" \
"#define FSR highp\n" \
"#endif\n" \
"vec3 fsr1_easu(sampler2D tex, vec2 uv, vec2 srcDim) {\n" \
"  FSR vec2 d = 1.0 / srcDim;\n" \
"  FSR vec2 p = uv * srcDim - 0.5;\n" \
"  FSR vec2 f = fract(p);\n" \
"  FSR vec2 c = floor(p) + 0.5;\n" \
"  vec3 s0 = texture2D(tex, c * d).rgb;\n" \
"  vec3 s1 = texture2D(tex, (c + vec2(1.0, 0.0)) * d).rgb;\n" \
"  vec3 s2 = texture2D(tex, (c - vec2(1.0, 0.0)) * d).rgb;\n" \
"  vec3 s3 = texture2D(tex, (c + vec2(0.0, 1.0)) * d).rgb;\n" \
"  vec3 s4 = texture2D(tex, (c + vec2(0.0, -1.0)) * d).rgb;\n" \
"  vec3 d0 = abs(s1 - s2);\n" \
"  vec3 d1 = abs(s3 - s4);\n" \
"  vec3 w = mix(d0, d1, vec3(0.5));\n" \
"  FSR float l = dot(w, vec3(0.33));\n" \
"  FSR vec2 lp = vec2(1.0) - abs(f * 2.0 - 1.0);\n" \
"  FSR float wp = (1.0 - abs(f.x * 2.0 - 1.0)) * (1.0 - abs(f.y * 2.0 - 1.0));\n" \
"  return mix(s0, mix(s1, s2, vec3(0.5)), vec3(f.x * wp)) + mix(s3, s4, vec3(0.5)) * (f.y * (1.0 - wp));\n" \
"}\n" \
"void main() {\n" \
"  gl_FragColor = vec4(fsr1_easu(uTex, vUv, uPar.xy), 1.0);\n" \
"}\n"

#define NV_FSR1_RCAS_SHADER \
"#if defined(GL_ES) && (!defined(GL_FRAGMENT_PRECISION_HIGH) || defined(NV_FSR1_MEDIUMP))\n" \
"#define FSR mediump\n" \
"#else\n" \
"#define FSR highp\n" \
"#endif\n" \
"void main() {\n" \
"  FSR vec3 c = texture2D(uTex, vUv).rgb;\n" \
"  vec2 d = 1.0 / vec2(textureSize(uTex, 0));\n" \
"  FSR vec3 n = texture2D(uTex, vUv + vec2(0.0, -d.y)).rgb;\n" \
"  FSR vec3 s = texture2D(uTex, vUv + vec2(0.0, d.y)).rgb;\n" \
"  FSR vec3 w = texture2D(uTex, vUv + vec2(-d.x, 0.0)).rgb;\n" \
"  FSR vec3 e = texture2D(uTex, vUv + vec2(d.x, 0.0)).rgb;\n" \
"  FSR float l = dot(c, vec3(0.299, 0.587, 0.114));\n" \
"  FSR float ln = dot(n, vec3(0.299, 0.587, 0.114));\n" \
"  FSR float ls = dot(s, vec3(0.299, 0.587, 0.114));\n" \
"  FSR float lw = dot(w, vec3(0.299, 0.587, 0.114));\n" \
"  FSR float le = dot(e, vec3(0.299, 0.587, 0.114));\n" \
"  FSR float g = max(max(ln, ls), max(lw, le)) - min(min(ln, ls), min(lw, le));\n" \
"  FSR float s_amt = clamp(g * 4.0, 0.0, 1.0);\n" \
"  FSR vec3 sharpened = c + (c - (n + s + w + e) * 0.25) * s_amt * 0.5;\n" \
"  gl_FragColor = vec4(sharpened, 1.0);\n" \
"}\n"
