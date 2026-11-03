## 2026-11-03 - CI Build Architecture Header Issue
**Learning:** When building for RP2040 or STM32 architectures (e.g., in `arduino/compile-sketches` GitHub actions), hardware-specific CMSIS headers like `<arm_math.h>` can cause fatal 'No such file or directory' compilation errors if their functions are not used and the DSP libraries are not explicitly linked.
**Action:** Remove unused architecture-specific headers to prevent unnecessary compilation dependencies and failures.
