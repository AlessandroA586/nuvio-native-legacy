// Lanczos-2/Catmull-Rom cubic resampling for upscaling (GLES2 compatible).
// A cheaper bicubic approximation suitable for GLES2 with mediump.
// uPar = source dimensions
// vUv = existing quad UV
#define NV_LANCZOS_SHADER \
"#if defined(GL_ES) && (!defined(GL_FRAGMENT_PRECISION_HIGH) || defined(NV_LANCZOS_MEDIUMP))\n" \
"#define LZ mediump\n" \
"#else\n" \
"#define LZ highp\n" \
"#endif\n" \
"LZ vec4 cubic(LZ float x) {\n" \
"  LZ float x2 = x * x;\n" \
"  LZ float x3 = x2 * x;\n" \
"  LZ vec4 w;\n" \
"  w.x = -0.5 * x3 + x2 - 0.5 * x;\n" \
"  w.y = 1.5 * x3 - 2.5 * x2 + 1.0;\n" \
"  w.z = -1.5 * x3 + 2.0 * x2 + 0.5 * x;\n" \
"  w.w = 0.5 * x3 - 0.5 * x2;\n" \
"  return w;\n" \
"}\n" \
"vec3 lz_bicubic(sampler2D tex, vec2 uv, vec2 srcDim) {\n" \
"  LZ vec2 d = 1.0 / srcDim;\n" \
"  LZ vec2 p = uv * srcDim - 0.5;\n" \
"  LZ vec2 f = fract(p);\n" \
"  LZ vec2 c = floor(p) + 0.5;\n" \
"  vec4 wx = cubic(f.x);\n" \
"  vec4 wy = cubic(f.y);\n" \
"  vec3 result = vec3(0.0);\n" \
"  LZ float sum = 0.0;\n" \
"  for(int y = -1; y <= 2; y++) {\n" \
"    for(int x = -1; x <= 2; x++) {\n" \
"      LZ vec2 coord = c + vec2(float(x), float(y)) * d;\n" \
"      LZ float w = wx[x + 1] * wy[y + 1];\n" \
"      result += texture2D(tex, coord).rgb * w;\n" \
"      sum += w;\n" \
"    }\n" \
"  }\n" \
"  return result / sum;\n" \
"}\n" \
"void main() {\n" \
"  gl_FragColor = vec4(lz_bicubic(uTex, vUv, uPar.xy), 1.0);\n" \
"}\n"
