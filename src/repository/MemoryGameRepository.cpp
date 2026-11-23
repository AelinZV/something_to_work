#include "repository/MemoryGameRepository.h"
#include <stdexcept>
namespace border {
MemoryGameRepository::MemoryGameRepository() {
    contracts_.emplace_back(1,1,110,80,"То, что дышит в болоте",
        "Рыбаки исчезают в камышах, а лодки возвращаются без людей.",
        "Слизь и обожжённый тростник: огонь — слабость топляка.",
        "Рыбаки вновь выходят на воду.",
        Monster(1,"Топляк",46,9,2,"fire","Мокрая тварь поднимается из трясины."));
    contracts_.emplace_back(2,2,190,130,"Плач у старой мельницы",
        "Ночью на мельнице слышат женский плач.",
        "Дух удерживает нарушенное обещание. Знак ловушки сдержит его.",
        "Старая мельница умолкла.",
        Monster(2,"Мельничная скорбница",68,12,4,"sigil","Бледный силуэт парит над жерновами."));
    contracts_.emplace_back(3,3,320,190,"Король сухих ветвей",
        "Лес зарастает корнями и не пускает путников.",
        "Серебряное масло усиливает удар по стражу.",
        "В лесу открылась тропа.",
        Monster(3,"Корневой страж",94,15,6,"silver","Исполин из корней и сухих ветвей."));
}
void MemoryGameRepository::addContract(const Contract& contract) {
    for(const auto& c:contracts_)
        if(c.id()==contract.id())throw std::invalid_argument("Повторяющийся ID контракта");
    contracts_.push_back(contract);
    for(auto& entry:saves_) {
        Progress p;
        p.contractId=contract.id();
        entry.second.progress.push_back(p);
    }
}
std::vector<Hero> MemoryGameRepository::listHeroes() {
    std::vector<Hero> result;for(const auto& pair:saves_)result.push_back(pair.second.hero);return result;
}
std::optional<Hero> MemoryGameRepository::getHero(int id) {
    auto it=saves_.find(id);if(it==saves_.end())return std::nullopt;return it->second.hero;
}
Hero MemoryGameRepository::createHero(const std::string& name) {
    Hero h(nextId_++,name);
    Save data{h,{},{ItemStack(1,2),ItemStack(2,1),ItemStack(3,5)}};
    for(const auto& c:contracts_) {Progress p;p.contractId=c.id();data.progress.push_back(p);}
    saves_.emplace(h.id(),data);return h;
}
void MemoryGameRepository::deleteHero(int id) {
    if(!saves_.erase(id))throw std::runtime_error("Персонаж не найден");
}
std::vector<Contract> MemoryGameRepository::listContracts() {return contracts_;}
std::vector<Progress> MemoryGameRepository::loadProgress(int heroId) {
    auto it=saves_.find(heroId);if(it==saves_.end())throw std::runtime_error("Персонаж не найден");return it->second.progress;
}
std::vector<ItemStack> MemoryGameRepository::loadInventory(int heroId) {
    auto it=saves_.find(heroId);if(it==saves_.end())throw std::runtime_error("Персонаж не найден");return it->second.items;
}
void MemoryGameRepository::save(const Hero& hero,const std::vector<Progress>& progress,
                                const std::vector<ItemStack>& items) {
    auto it=saves_.find(hero.id());if(it==saves_.end())throw std::runtime_error("Персонаж удалён");
    // Проверяем инварианты транзакции без частичного обновления состояния.
    if(progress.size()!=contracts_.size()||items.size()!=3)throw std::runtime_error("Неполное сохранение");
    it->second=Save{hero,progress,items};
}
} // namespace border
