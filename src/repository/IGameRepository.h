#pragma once
#include "domain/World.h"
#include <optional>
namespace border {
// Сервис зависит от интерфейса, а не от PostgreSQL/libpqxx.
class IGameRepository {
public:
    virtual ~IGameRepository()=default;
    virtual std::vector<Hero> listHeroes()=0;
    virtual std::optional<Hero> getHero(int id)=0;
    virtual Hero createHero(const std::string& name)=0;
    virtual void deleteHero(int id)=0;
    virtual std::vector<Contract> listContracts()=0;
    virtual std::vector<Progress> loadProgress(int heroId)=0;
    virtual std::vector<ItemStack> loadInventory(int heroId)=0;
    // Единственная транзакция для героя, контрактов и предметов.
    virtual void save(const Hero& hero,const std::vector<Progress>& progress,
                      const std::vector<ItemStack>& items)=0;
};
} // namespace border
