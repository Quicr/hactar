# UI host tests

This is a standalone CMake project for the UI's doctest runner. It uses a
native C++20 compiler and fetches doctest v2.5.3 on first configure.

From `firmware/ui`:

```sh
make test
```

The runner currently has zero test cases. To add cases later, list their source
files in the `ui_tests` target and include `<doctest/doctest.h>` in each file.
