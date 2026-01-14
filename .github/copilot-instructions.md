<!--  Copyright (c) Microsoft Corporation. All rights reserved -->
# Copilot instructions

## Copy Notices

When generating any new file (rust, markdown, toml, etc.) in this repo include a notice comment of the appropriate style:

```rust
// Copyright (c) Microsoft Corporation. All rights reserved
```

For markdown files, insert a comment
```markdown
<!--  Copyright (c) Microsoft Corporation. All rights reserved -->
```

If a file is unexpectedly ignored by copilot it is likely because the copy notice is incorrect. Only files with a proper microsoft copy notice can be accessed by copilot, and a notice from another company will prevent access. If the word or symbol for copy right appears anywhere in the document without being followed by `Microsoft Corporation` the file will be ignored.

## ADO

Source for this project is at https://dev.azure.com/AzureCSI/Woodinville/_git/Kingsgate.MSCP

Work items for this project are at https://azurecsi.visualstudio.com/Dev/_workitems/

## Code Style

Follow the existing code style in the repository, including:

- Appropriate documentation for public APIs
- Consistent naming conventions

## Build Firmware

Check `$env:REPO_APP_TOOLCHAIN == "arm-eabi-aarch"` to determine if the build environment is configured for target firmware. Run `start.ps1` if it is not configured (only once per terminal session). Run `build` to compile the firmware.

## Building and Running Unit Tests

Check `$env:REPO_APP_TOOLCHAIN == "i386-pc-windows-msvc"` to determine if the build environment is configured for unit tests. Run `start.ps1 -Toolchain i386-pc-windows-msvc -Configuration Debug` if it is not configured (only once per terminal session). Run `build` to compile the tests. Run `localunittest` to execute the tests.

