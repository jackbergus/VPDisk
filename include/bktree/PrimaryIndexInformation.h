//
// Created by giacomo on 02/04/24.
//

#ifndef SIMMATCH_PRIMARYINDEXINFORMATION_H
#define SIMMATCH_PRIMARYINDEXINFORMATION_H

#include <string>

struct PrimaryIndexInformation {
    size_t BKTreeDiskOffset;
    size_t ParentOffset;
};

#endif //SIMMATCH_PRIMARYINDEXINFORMATION_H
