===========
CMake usage
===========

Using ``scnlib`` with CMake is pretty easy. Just import it in the way of your
liking (``find_package``, ``add_subdirectory`` etc) and add ``scn::scn`` (or
``scn::scn-header-only``) to your ``target_link_libraries``.

CMake configuration options
---------------------------

These default to ``OFF``, unless ``scnlib`` is built as a standalone project.

 * ``SCN_TESTS``: Build tests
 * ``SCN_EXAMPLES``: Build examples
 * ``SCN_BENCHMARKS``: Build benchmarks
 * ``SCN_DOCS``: Build docs
 * ``SCN_INSTALL`` Generate ``install`` target
 * ``SCN_PEDANTIC``: Enable stricter warning levels

These default to ``OFF``, but can be turned on if you want to:

 * ``SCN_USE_NATIVE_ARCH``: Add ``-march=native`` to build flags
   (gcc or clang only). Useful for increasing performance,
   but makes your binary non-portable.
 * ``SCN_USE_ASAN``, ``SCN_USE_UBSAN``, ``SCN_USE_MSAN``:
   Enable sanitizers, clang only

These default to ``ON``:

 * ``SCN_USE_EXCEPTIONS``, ``SCN_USE_RTTI``: self-explanatory

These default to ``OFF``, and should only be turned on if necessary:

 * ``SCN_WERROR``: Stops compilation on compiler warnings
 * ``SCN_USE_32BIT``: Compile as 32-bit (gcc or clang only)
 * ``SCN_COVERAGE``: Generate code coverage report
 * ``SCN_BLOAT``: Generate bloat test target
 * ``SCN_BUILD_FUZZING``: Build fuzzer
 * ``SCN_BUILD_LOCALIZED_TEST``: Build localization tests, requires en_US.UTF-8 and fi_FI.UTF-8 locales
 * ``SCN_BUILD_BLOAT``: Build code bloat benchmarks
 * ``SCN_BUILD_BUILDTIME``: Build build time benchmarks
