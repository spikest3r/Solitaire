#include <fstream>
#include "cardobject.h"

void Save(
    const CardObject(&cards)[52],
    const Pile(&columns)[7],
    const Pile(&bases)[4],
    const std::vector<CardObject>& deck,
    const std::vector<CardObject>& revealedDeck
) {
    std::ofstream out("save.bin", std::ios::binary);
    if (!out) return;

    // cards
    out.write((char*)cards, sizeof(cards));

    // columns
    for (int i = 0; i < 7; i++) {
        size_t sz = columns[i].size();
        out.write((char*)&sz, sizeof(sz));
        out.write((char*)columns[i].data(), sz * sizeof(CardObject));
    }

    // deck
    size_t deckSz = deck.size();
    out.write((char*)&deckSz, sizeof(deckSz));
    out.write((char*)deck.data(), deckSz * sizeof(CardObject));

    // revealedDeck
    size_t revSz = revealedDeck.size();
    out.write((char*)&revSz, sizeof(revSz));
    out.write((char*)revealedDeck.data(), revSz * sizeof(CardObject));

    // bases
    for (int i = 0; i < 4; i++) {
        size_t sz = bases[i].size();
        out.write((char*)&sz, sizeof(sz));
        out.write((char*)bases[i].data(), sz * sizeof(CardObject));
    }
}

void Load(
    CardObject(&cards)[52],
    Pile(&columns)[7],
    Pile(&bases)[4],
    std::vector<CardObject>& deck,
    std::vector<CardObject>& revealedDeck
) {
    std::ifstream in("save.bin", std::ios::binary);
    if (!in) return;

    // cards
    in.read((char*)cards, sizeof(cards));

    // columns
    for (int i = 0; i < 7; i++) {
        size_t sz;
        in.read((char*)&sz, sizeof(sz));
        columns[i].resize(sz);
        in.read((char*)columns[i].data(), sz * sizeof(CardObject));
    }

    // deck
    size_t deckSz;
    in.read((char*)&deckSz, sizeof(deckSz));
    deck.resize(deckSz);
    in.read((char*)deck.data(), deckSz * sizeof(CardObject));

    // revealedDeck
    size_t revSz;
    in.read((char*)&revSz, sizeof(revSz));
    revealedDeck.resize(revSz);
    in.read((char*)revealedDeck.data(), revSz * sizeof(CardObject));

    // bases
    for (int i = 0; i < 4; i++) {
        size_t sz;
        in.read((char*)&sz, sizeof(sz));
        bases[i].resize(sz);
        in.read((char*)bases[i].data(), sz * sizeof(CardObject));
    }
}
