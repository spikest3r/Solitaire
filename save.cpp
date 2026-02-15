#include <fstream>
#include "cardobject.h"

void Save(
    std::string path,
    const CardObject(&cards)[52],
    const Pile(&columns)[7],
    const Pile(&bases)[4],
    const std::vector<CardObject>& deck,
    const std::vector<CardObject>& revealedDeck,
    const std::vector<GameState>& previousMoves,
    const int& time,
    const int& moves,
    unsigned int currentSeed
) {
    std::ofstream out(path, std::ios::binary);
    if (!out) return;

    // time
    out.write((char*)&time, sizeof(int));

    // moves
    out.write((char*)&moves, sizeof(int));

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

    if (previousMoves.empty()) {
        size_t pmSz = 0;
        out.write((char*)&pmSz, sizeof(pmSz));
        return;
    }
    
    // previousMoves
    size_t pmSz = previousMoves.size();
    out.write((char*)&pmSz, sizeof(pmSz));

    for (const GameState& gs : previousMoves) {

        // columns
        for (int i = 0; i < 7; ++i) {
            size_t sz = gs.columns[i].size();
            out.write((char*)&sz, sizeof(sz));
            out.write((char*)gs.columns[i].data(), sz * sizeof(CardObject));
        }

        // bases
        for (int i = 0; i < 4; ++i) {
            size_t sz = gs.bases[i].size();
            out.write((char*)&sz, sizeof(sz));
            out.write((char*)gs.bases[i].data(), sz * sizeof(CardObject));
        }

        // deck
        size_t deckSz = gs.deck.size();
        out.write((char*)&deckSz, sizeof(deckSz));
        out.write((char*)gs.deck.data(), deckSz * sizeof(CardObject));

        // revealedDeck
        size_t revSz = gs.revealedDeck.size();
        out.write((char*)&revSz, sizeof(revSz));
        out.write((char*)gs.revealedDeck.data(), revSz * sizeof(CardObject));
    }

    out.write((char*)&currentSeed, sizeof(unsigned int));
}

void Load(
    std::string path,
    CardObject(&cards)[52],
    Pile(&columns)[7],
    Pile(&bases)[4],
    std::vector<CardObject>& deck,
    std::vector<CardObject>& revealedDeck,
    std::vector<GameState>& previousMoves,
    int& time,
    int& moves,
    unsigned int& currentSeed
) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return;

    // time
    in.read((char*)&time, sizeof(int));

    // moves
    in.read((char*)&moves, sizeof(int));

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

    size_t pmSz;
    in.read((char*)&pmSz, sizeof(pmSz));
    previousMoves.clear();
    previousMoves.resize(pmSz);

    for (size_t p = 0; p < pmSz; ++p) {
        GameState& gs = previousMoves[p];

        // columns
        for (int i = 0; i < 7; ++i) {
            size_t sz;
            in.read((char*)&sz, sizeof(sz));
            gs.columns[i].resize(sz);
            in.read((char*)gs.columns[i].data(), sz * sizeof(CardObject));
        }

        // bases
        for (int i = 0; i < 4; ++i) {
            size_t sz;
            in.read((char*)&sz, sizeof(sz));
            gs.bases[i].resize(sz);
            in.read((char*)gs.bases[i].data(), sz * sizeof(CardObject));
        }

        // deck
        size_t deckSz;
        in.read((char*)&deckSz, sizeof(deckSz));
        gs.deck.resize(deckSz);
        in.read((char*)gs.deck.data(), deckSz * sizeof(CardObject));

        // revealedDeck
        size_t revSz;
        in.read((char*)&revSz, sizeof(revSz));
        gs.revealedDeck.resize(revSz);
        in.read((char*)gs.revealedDeck.data(), revSz * sizeof(CardObject));
    }

    if (in.peek() != EOF) {
        in.read((char*)&currentSeed, sizeof(unsigned int));
    }
    else {
        // Compatibility mode: old file detected
        // You can set it to 0 or generate a new one
        currentSeed = 0;
    }
}
