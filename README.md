# API Set Resolver

![C++](https://img.shields.io/badge/C%2B%2B-17%2B-blue.svg) ![License](https://img.shields.io/badge/License-MIT-green.svg)

> A lightweight C++ library to resolve Windows API Set mappings to their actual DLL names.

## Usage

```cpp
#include "ApiSetResolver.h"
#include <iostream>

int main() {
    std::string resolvedDll = ApiSetResolver::Resolve("api-ms-win-core-file-l1-1-0.dll");
    std::cout << "Resolved DLL: " << resolvedDll << std::endl;
    return 0;
}
```

## Building

Build using CMake:
```sh
mkdir build && cd build
cmake ..
make
```

or just add 
```cmake
FetchContent_Declare(
    api_set_resolver
    GIT_REPOSITORY https://github.com/error56/ApiSetResolver
    GIT_TAG        [current git commit tag]
)
```

to your CMakeLists.txt.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.