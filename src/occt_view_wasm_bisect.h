#pragma once

// WASM / TKOpenGles bisection toggles (Emscripten only). All OFF by default.
//
// To isolate OCCT 8 wasm shading bugs: uncomment ONE line below, rebuild build-em, retest.
//
//   DISABLE_VIEW_SHADOWS       - RenderingParams.IsShadowEnabled=false
//   DISABLE_LIGHT_CAST_SHADOWS - skip SetCastShadows on directional lights
//   DISABLE_MSAA               - NbMsaaSamples=0
//   DISABLE_RESOLUTION_SCALE   - RenderResolutionScale=1.0 (default wasm uses 2.0)
//   DEPTH_PEELING_OIT          - TransparencyMethod depth peeling instead of BLEND_UNORDERED
//   DISABLE_OPAQUE_ALPHA       - OpenGl_GraphicDriver buffersOpaqueAlpha=false

#ifdef __EMSCRIPTEN__

// #define WASM_BISECT_DISABLE_VIEW_SHADOWS
// #define WASM_BISECT_DISABLE_LIGHT_CAST_SHADOWS
// #define WASM_BISECT_DISABLE_MSAA
// #define WASM_BISECT_DISABLE_RESOLUTION_SCALE
// #define WASM_BISECT_DEPTH_PEELING_OIT
// #define WASM_BISECT_DISABLE_OPAQUE_ALPHA

#endif
