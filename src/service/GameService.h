#pragma once
#include "repository/IGameRepository.h"
#include <optional>

namespace border {
class GameService {
    IGameRepository& repo_;
    std::optional<Hero> hero_;
    std::vector<Contract> contracts_;
    std::vector<Progress> progress_;
    std::vector<ItemStack> items_;
    Progress& progressFor(int contractId);
    const Progress& progressFor(int contractId) const;
    ItemStack& itemFor(int id);
    const Contract& contractFor(int id) const;
    void requireHero() const;
    void persist();
    bool hasActiveBattle() const;
public:
    explicit GameService(IGameRepository& repo);
    std::vector<Hero> listHeroes();
    void createHero(const std::string& name);
    void selectHero(int id);
    void deleteHero(int id);
    bool selected() const {return hero_.has_value();}
    const Hero& hero() const;
    const std::vector<Contract>& contracts() const {return contracts_;}
    const std::vector<Progress>& progress() const {return progress_;}
    const std::vector<ItemStack>& inventory() const {return items_;}
    int countItem(int id) const;
    const Contract& getContract(int id) const;
    const Progress& getProgress(int id) const;
    void travel(int location);
    std::string acceptContract(int id);
    std::string investigate(int id, int choice);
    std::string startBattle(int id);
    std::string turn(int id, Action action);
    std::string purchase(int itemId);
    std::string craft(int itemId);
    std::string drinkPotion(); // Лечение вне боя, +40 HP
    std::string rest();        // Отдых в Сероводье, 10 крон, полное HP
    std::string learnSkill(int skillId);
    bool storyCompleted() const;
};
} // namespace border
