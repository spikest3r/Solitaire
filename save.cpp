#include <fstream>
#include "cardobject.h"

#include <fstream>
#include <vector>
#include <iostream>
#include "cardobject.h"

// Helper to save/load a single CardObject
void saveCard(std::ofstream& out, const CardObject& c) {
    out.write((char*)&c.rank, sizeof(c.rank));
    out.write((char*)&c.suit, sizeof(c.suit));
    out.write((char*)&c.revealed, sizeof(c.revealed));
    // anim and scale are ignored
}

bool loadCard(std::ifstream& in, CardObject& c) {
    // Mandatory fields in all formats
    if (!in.read((char*)&c.rank, sizeof(c.rank))) return false;
    if (!in.read((char*)&c.suit, sizeof(c.suit))) return false;
    if (!in.read((char*)&c.revealed, sizeof(c.revealed))) return false;

    // Skip any extra bytes from old files (draggingPos, scale, anim)
    // Old CardObject size minus what we just read
    size_t readSize = sizeof(c.rank) + sizeof(c.suit) + sizeof(c.revealed);
    size_t totalSize = sizeof(CardObject); // might include anim, scale, draggingPos
    if (totalSize > readSize && in.peek() != EOF) {
        in.seekg(totalSize - readSize, std::ios::cur);
    }

    // Reset runtime-only fields
    c.anim = nullptr;
    c.scale = 1.0f;
    c.draggingPos = glm::vec3(0.0f);
    return true;
}

// Save function
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
    if (!out) {
        std::cerr << "Failed to open file for saving: " << path << "\n";
        return;
    }

    out.write((char*)&time, sizeof(time));
    out.write((char*)&moves, sizeof(moves));

    for (int i = 0; i < 52; ++i) saveCard(out, cards[i]);

    for (int i = 0; i < 7; ++i) {
        size_t sz = columns[i].size();
        out.write((char*)&sz, sizeof(sz));
        for (auto& c : columns[i]) saveCard(out, c);
    }

    size_t deckSz = deck.size();
    out.write((char*)&deckSz, sizeof(deckSz));
    for (auto& c : deck) saveCard(out, c);

    size_t revSz = revealedDeck.size();
    out.write((char*)&revSz, sizeof(revSz));
    for (auto& c : revealedDeck) saveCard(out, c);

    for (int i = 0; i < 4; ++i) {
        size_t sz = bases[i].size();
        out.write((char*)&sz, sizeof(sz));
        for (auto& c : bases[i]) saveCard(out, c);
    }

    size_t pmSz = previousMoves.size();
    out.write((char*)&pmSz, sizeof(pmSz));

    for (const GameState& gs : previousMoves) {
        for (int i = 0; i < 7; ++i) {
            size_t sz = gs.columns[i].size();
            out.write((char*)&sz, sizeof(sz));
            for (auto& c : gs.columns[i]) saveCard(out, c);
        }
        for (int i = 0; i < 4; ++i) {
            size_t sz = gs.bases[i].size();
            out.write((char*)&sz, sizeof(sz));
            for (auto& c : gs.bases[i]) saveCard(out, c);
        }
        size_t dSz = gs.deck.size();
        out.write((char*)&dSz, sizeof(dSz));
        for (auto& c : gs.deck) saveCard(out, c);
        size_t rSz = gs.revealedDeck.size();
        out.write((char*)&rSz, sizeof(rSz));
        for (auto& c : gs.revealedDeck) saveCard(out, c);
    }

    // Seed goes last
    out.write((char*)&currentSeed, sizeof(currentSeed));
}

// Load function
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
    if (!in) {
        std::cerr << "Failed to open file for loading: " << path << "\n";
        return;
    }

    if (!in.read((char*)&time, sizeof(time))) return;
    if (!in.read((char*)&moves, sizeof(moves))) return;

    for (int i = 0; i < 52; ++i)
        if (!loadCard(in, cards[i])) return;

    for (int i = 0; i < 7; ++i) {
        size_t sz;
        if (!in.read((char*)&sz, sizeof(sz))) return;
        columns[i].resize(sz);
        for (auto& c : columns[i])
            if (!loadCard(in, c)) return;
    }

    size_t deckSz;
    if (!in.read((char*)&deckSz, sizeof(deckSz))) return;
    deck.resize(deckSz);
    for (auto& c : deck)
        if (!loadCard(in, c)) return;

    size_t revSz;
    if (!in.read((char*)&revSz, sizeof(revSz))) return;
    revealedDeck.resize(revSz);
    for (auto& c : revealedDeck)
        if (!loadCard(in, c)) return;

    for (int i = 0; i < 4; ++i) {
        size_t sz;
        if (!in.read((char*)&sz, sizeof(sz))) return;
        bases[i].resize(sz);
        for (auto& c : bases[i])
            if (!loadCard(in, c)) return;
    }

    size_t pmSz;
    if (!in.read((char*)&pmSz, sizeof(pmSz))) pmSz = 0;
    previousMoves.clear();
    previousMoves.resize(pmSz);

    for (size_t p = 0; p < pmSz; ++p) {
        GameState& gs = previousMoves[p];
        for (int i = 0; i < 7; ++i) {
            size_t sz;
            if (!in.read((char*)&sz, sizeof(sz))) return;
            gs.columns[i].resize(sz);
            for (auto& c : gs.columns[i])
                if (!loadCard(in, c)) return;
        }
        for (int i = 0; i < 4; ++i) {
            size_t sz;
            if (!in.read((char*)&sz, sizeof(sz))) return;
            gs.bases[i].resize(sz);
            for (auto& c : gs.bases[i])
                if (!loadCard(in, c)) return;
        }
        size_t dSz;
        if (!in.read((char*)&dSz, sizeof(dSz))) return;
        gs.deck.resize(dSz);
        for (auto& c : gs.deck)
            if (!loadCard(in, c)) return;

        size_t rSz;
        if (!in.read((char*)&rSz, sizeof(rSz))) return;
        gs.revealedDeck.resize(rSz);
        for (auto& c : gs.revealedDeck)
            if (!loadCard(in, c)) return;
    }

    // Seed: compatibility mode
    if (in.peek() != EOF) {
        in.read((char*)&currentSeed, sizeof(currentSeed));
    }
    else {
        currentSeed = 0;
    }
}