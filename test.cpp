#include <iostream>

int main() {
#if __cplusplus >= 202002L
    std::cout << "C++20 or newer\n";
#elif __cplusplus >= 201703L
    std::cout << "C++17\n";
#else
    std::cout << "Older than C++17\n";
#endif
}
