## 2024-07-23 - Catch2 C++ Linker Issue Side-note
**Learning:** While focusing on UX, I encountered a C++ linker error (`undefined reference to UltimateDSP::MarkovActivityEngine::A_INIT`) when running the Catch2 test suite. Catch2 odr-uses static constexpr members when asserting against them (like `REQUIRE_THAT`), which requires a definition outside the class declaration to prevent linker errors.
**Action:** When working on C++ repositories, ensure static constexpr arrays/variables that are asserted against in Catch2 have explicit definitions in the corresponding .cpp files.
