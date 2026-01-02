#pragma once

#include <vector>
#include "cardobject.h"

void Save(
    const CardObject(&cards)[52],
    const Pile(&columns)[7],
    const Pile(&bases)[4],
    const std::vector<CardObject>& deck,
    const std::vector<CardObject>& revealedDeck,
    const std::vector<GameState>& previousMoves,
    const int& time
);

void Load(
    CardObject(&cards)[52],
    Pile(&columns)[7],
    Pile(&bases)[4],
    std::vector<CardObject>& deck,
    std::vector<CardObject>& revealedDeck,
    std::vector<GameState>& previousMoves,
    int& time
);

