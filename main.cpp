#include <iostream>
#include "core/factory.hpp"

int main() {
    std::cout << "Hello, World!" << std::endl;
    auto factory = core::Factory::create();
    std::cout << factory.get(core::PieceType::I).name << std::endl;
    return 0;
}