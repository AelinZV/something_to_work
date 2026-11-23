#include "repository/PgGameRepository.h"
#include <stdexcept>

namespace border {
namespace {
Hero readHero(const pqxx::row& r) {
    return Hero(r[0].as<int>(),r[1].as<std::string>(),r[2].as<int>(),r[3].as<int>(),
                r[4].as<int>(),r[5].as<int>(),r[6].as<int>(),r[7].as<int>(),
                r[8].as<int>(),r[9].as<int>(),r[10].as<int>(),r[11].as<int>(),
                r[12].as<int>(),r[13].as<int>());
}
std::string toDb(Status s) {
    switch(s) {
        case Status::Available:return "available";
        case Status::Accepted:return "accepted";
        case Status::Investigated:return "investigated";
        case Status::Completed:return "completed";
    }
    throw std::runtime_error("Недопустимый статус");
}
}
PgGameRepository::PgGameRepository(const std::string& conn):connection_(conn) {
    pqxx::nontransaction tx(connection_);
    tx.exec("SET client_encoding TO 'UTF8'");
}
std::vector<Hero> PgGameRepository::listHeroes() {
    pqxx::read_transaction tx(connection_);
    auto rows=tx.exec("SELECT id,name,level,experience,hp,max_hp,attack,defense,gold,location_id,skill_points,sword_skill,vitality_skill,sign_skill FROM heroes ORDER BY id");
    std::vector<Hero> result;for(const auto& r:rows)result.push_back(readHero(r));return result;
}
std::optional<Hero> PgGameRepository::getHero(int id) {
    pqxx::read_transaction tx(connection_);
    auto rows=tx.exec_params("SELECT id,name,level,experience,hp,max_hp,attack,defense,gold,location_id,skill_points,sword_skill,vitality_skill,sign_skill FROM heroes WHERE id=$1",id);
    if(rows.empty())return std::nullopt;
    return readHero(rows[0]);
}
Hero PgGameRepository::createHero(const std::string& name) {
    pqxx::work tx(connection_);
    auto r=tx.exec_params("INSERT INTO heroes(name) VALUES($1) RETURNING id",name);
    int id=r[0][0].as<int>();
    tx.exec_params("INSERT INTO hero_contracts(hero_id,contract_id) SELECT $1,id FROM contracts",id);
    tx.exec_params("INSERT INTO inventory(hero_id,item_id,quantity) VALUES ($1,1,2),($1,2,1),($1,3,5)",id);
    tx.commit();
    return Hero(id,name);
}
void PgGameRepository::deleteHero(int id) {
    pqxx::work tx(connection_);
    auto r=tx.exec_params("DELETE FROM heroes WHERE id=$1",id);
    if(r.affected_rows()!=1)throw std::runtime_error("Персонаж не существует");
    tx.commit();
}
std::vector<Contract> PgGameRepository::listContracts() {
    pqxx::read_transaction tx(connection_);
    auto rows=tx.exec(
        "SELECT c.id,c.location_id,c.reward,c.experience,c.title,c.story,c.clue,c.resolution,"
        "m.id,m.name,m.max_hp,m.attack,m.defense,m.weakness,m.description "
        "FROM contracts c JOIN monsters m ON c.monster_id=m.id ORDER BY c.id");
    std::vector<Contract> result;
    for(const auto& r:rows) {
        Monster monster(r[8].as<int>(),r[9].as<std::string>(),r[10].as<int>(),
                        r[11].as<int>(),r[12].as<int>(),r[13].as<std::string>(),r[14].as<std::string>());
        result.emplace_back(r[0].as<int>(),r[1].as<int>(),r[2].as<int>(),r[3].as<int>(),
                            r[4].as<std::string>(),r[5].as<std::string>(),r[6].as<std::string>(),
                            r[7].as<std::string>(),std::move(monster));
    }
    return result;
}
std::vector<Progress> PgGameRepository::loadProgress(int heroId) {
    // Контракты можно добавлять после создания героя. При следующей загрузке
    // автоматически регистрируем для него новые заказы, не меняя старый прогресс.
    pqxx::work tx(connection_);
    tx.exec_params(
        "INSERT INTO hero_contracts(hero_id,contract_id) "
        "SELECT $1,c.id FROM contracts c "
        "ON CONFLICT (hero_id,contract_id) DO NOTHING",heroId);
    auto rows=tx.exec_params(
        "SELECT contract_id,status,investigation_choice,fighting,monster_hp,turn,focus,shield,trap,oiled,outcome "
        "FROM hero_contracts WHERE hero_id=$1 ORDER BY contract_id",heroId);
    std::vector<Progress> result;
    for(const auto& r:rows) {
        Progress p;
        p.contractId=r[0].as<int>();p.status=parseStatus(r[1].as<std::string>());
        p.investigationChoice=r[2].as<int>();p.fighting=r[3].as<bool>();
        p.monsterHp=r[4].as<int>();p.turn=r[5].as<int>();p.focus=r[6].as<int>();
        p.shield=r[7].as<int>();p.trap=r[8].as<int>();p.oiled=r[9].as<bool>();
        p.outcome=r[10].as<std::string>();result.push_back(p);
    }
    tx.commit();
    return result;
}
std::vector<ItemStack> PgGameRepository::loadInventory(int heroId) {
    pqxx::read_transaction tx(connection_);
    auto rows=tx.exec_params("SELECT item_id,quantity FROM inventory WHERE hero_id=$1 ORDER BY item_id",heroId);
    std::vector<ItemStack> result;
    for(const auto& r:rows)result.emplace_back(r[0].as<int>(),r[1].as<int>());
    return result;
}
void PgGameRepository::save(const Hero& h,const std::vector<Progress>& progress,
                            const std::vector<ItemStack>& items) {
    pqxx::work tx(connection_);
    auto heroRow=tx.exec_params(
        "UPDATE heroes SET level=$2,experience=$3,hp=$4,max_hp=$5,attack=$6,defense=$7,"
        "gold=$8,location_id=$9,skill_points=$10,sword_skill=$11,vitality_skill=$12,sign_skill=$13 WHERE id=$1",
        h.id(),h.level(),h.experience(),h.hp(),h.maxHp(),h.attack(),h.defense(),h.gold(),h.location(),
        h.skillPoints(),h.swordSkill(),h.vitalitySkill(),h.signSkill());
    if(heroRow.affected_rows()!=1)throw std::runtime_error("Персонаж исчез из базы");
    for(const auto& p:progress) {
        auto r=tx.exec_params(
            "UPDATE hero_contracts SET status=$3,investigation_choice=$4,fighting=$5,monster_hp=$6,"
            "turn=$7,focus=$8,shield=$9,trap=$10,oiled=$11,outcome=$12 WHERE hero_id=$1 AND contract_id=$2",
            h.id(),p.contractId,toDb(p.status),p.investigationChoice,p.fighting,p.monsterHp,
            p.turn,p.focus,p.shield,p.trap,p.oiled,p.outcome);
        if(r.affected_rows()!=1)throw std::runtime_error("Не удалось сохранить состояние контракта");
    }
    for(const auto& item:items) {
        tx.exec_params(
            "INSERT INTO inventory(hero_id,item_id,quantity) VALUES($1,$2,$3) "
            "ON CONFLICT(hero_id,item_id) DO UPDATE SET quantity=EXCLUDED.quantity",
            h.id(),item.id(),item.quantity());
    }
    tx.commit();
}
} // namespace border
