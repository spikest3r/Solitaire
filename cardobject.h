#pragma once
#include <glm/glm.hpp>
#include <vector>

struct CardAnimation {
	bool reveal;
	bool hide; // inverse to reveal

	float xFrom, yFrom;
	float xTo, yTo;
	bool move;

	float timeStep = 0.0f; // from 0 to 1
};

enum CardPosition {BASE,DECK,ROW};

struct CardObject {
	int rank, suit;
	bool revealed;
	glm::vec3 draggingPos;
	CardAnimation* anim = nullptr;
	float scale = 1.0f;

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