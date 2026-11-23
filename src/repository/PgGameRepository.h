#pragma once
#include "repository/IGameRepository.h"
#include <pqxx/pqxx>
namespace border {
class PgGameRepository : public IGameRepository {
    pqxx::connection connection_;
public:
    explicit PgGameRepository(const std::string& connectionString);
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
