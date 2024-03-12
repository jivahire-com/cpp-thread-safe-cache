# CMake Guidelines

[[_TOC_]]

## Introduction

### Purpose

This document defines the minimum common coding practices for maintaining the CMake build scripts.

## General Rules

1. Keep it **simple**. A single CMake should build a single target. Create directories for different components.

2. Be **explicit**. Avoid implicit or obscure features of CMake.

3. Be **consistent**. Use the same rules as broadly as possible.

4. Be **thorough** and treat warnings as errors.

## Other Considerations

4. Always use `target` functions (target_sources, target_compile_definitions, etc...) and make sure to add the PUBLIC/PRIVATE/INTERFACE keywords as appropriate. Keyword meanings:
    * PRIVATE: requirement should apply to just this target.
    * PUBLIC: requirement should apply to this target and anything that links to it.
    * INTERFACE: requirement should apply just to things that link to it.

5. All `toolchain` rules and configurations need to be contained in the toolchain CMake script (tools\cmakes\toolchain).

6. There is no public `inc` folders. Include rules are handled through `target_include_directories` statements. This way dependencies need to be properly specified by consumer modules with the proper visibility, which eases up how dependencies are tracked.

7. Add `lint_target` and `format_target` wherever possible, to provide a first line of defense in terms of static analysis and having a consistent code formatting.

8. When in doubt follow modern CMake practices. And contribute to this guide.
    * [Effective modern cmake](https://gist.github.com/mbinna/c61dbb39bca0e4fb7d1f73b0d66a4fd1)
    * [CMake best practices](https://indico.jlab.org/event/420/contributions/7961/attachments/6507/8734/CMakeSandCroundtable.slides.pdf)
    * [Modern cmake](http://cliutils.gitlab.io/modern-cmake/)
    * Always end CMake files with a new line
