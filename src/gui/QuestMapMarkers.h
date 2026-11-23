#pragma once
#include "domain/World.h"
#include <array>
#include <stdexcept>
#include <string>
#include <vector>

// Тестируемая модель маркеров карты. Никаких зависимостей от Qt или PostgreSQL.
namespace border::ui {
struct MapQuest {
    int id{};
    int location{};
    Status status{Status::Available};
    std::string title;
};

// [локация][статус] -> список заказов (в одном месте может быть много заказов).
using MapQuestGroups = std::array<std::array<std::vector<MapQuest>, 4>, 4>;

inline int statusIndex(Status status) {
    switch (status) {
        case Status::Available:     return 0;
        case Status::Accepted:      return 1;
        case Status::Investigated:  return 2;
        case Status::Completed:     return 3;
    }
    throw std::invalid_argument("Неизвестный статус контракта на карте");
}

inline MapQuestGroups groupMapQuests(const std::vector<MapQuest>& quests) {
    MapQuestGroups groups{};
    for (const auto& quest : quests) {
        if (quest.id <= 0 || quest.location < 0 || quest.location >= 4)
            throw std::invalid_argument("Неверный ID контракта или локации на карте");
        groups[static_cast<std::size_t>(quest.location)]
              [static_cast<std::size_t>(statusIndex(quest.status))].push_back(quest);
    }
    return groups;
}
} // namespace border::ui
