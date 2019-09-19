#ifndef SFINDER_EXP_CREATE_HPP
#define SFINDER_EXP_CREATE_HPP

#include "lib/create.hpp"

void create() {
    sfexp::createIndexes("../../csv/index.csv", "../../bin/index.bin");
    sfexp::createSolutions(
            "../../csv/indexed_solutions_10x4_SRS7BAG.csv",
            "../../bin/indexed_solutions_10x4_SRS7BAG.bin"
    );
}

#endif //SFINDER_EXP_CREATE_HPP
