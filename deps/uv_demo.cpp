#include <uv.h>
#include <iostream>

int main() {
    std::cout << "libuv version: " << uv_version_string() << std::endl;
    return 0;
}


