#pragma once
#include <glm/glm.hpp>
#include <vector>

enum CardPosition {BASE,DECK,ROW};

struct CardObject {
	int rank, suit;
	bool revealed;
	glm::vec3 draggingPos;

	bool operator==(const CardObject& other) const {
		return rank == other.rank && suit == other.suit;
	}
};

using Pile = std::vector<CardObject>;

struct GameState {
	Pile columns[7];
	std::vector<CardObject> deck;
	std::vector<CardObject> revealedDeck;
	Pile bases[4];
};