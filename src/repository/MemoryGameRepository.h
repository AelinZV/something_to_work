#pragma once
#include "repository/IGameRepository.h"
#include <map>
namespace border {
// Учебный демонстрационный режим и тестовый double. Не заменяет PostgreSQL.
class MemoryGameRepository : public IGameRepository {
    struct Save {
        Hero hero;
        std::vector<Progress> progress;
        std::vector<ItemStack> items;
    };
    std::map<int,Save> saves_;
    int nextId_{1};
    std::vector<Contract> contracts_;
public:
    MemoryGameRepository();
    // Для демо и тестов: добавление контракта без привязки к конкретной локации.
    void addContract(const Contract& contract);
    std::vector<Hero> listHeroes() override;
    std::optional<Hero> getHero(int id) override;
    Hero createHero(const std::string& name) override;
    void deleteHero(int id) override;
    std::vector<Contract> listContracts() override;
    std::vector<Progress> loadProgress(int heroId) override;
    std::vector<ItemStack> loadInventory(int heroId) override;
    void save(const Hero& hero,const std::vector<Progress>& progress,
              const std::vector<ItemStack>& items) override;
};
} // namespace border
