#include "domain/World.h"
#include <utility>

namespace border {
std::string statusName(Status s) {
    switch(s) {
        case Status::Available: return "Доступен";
        case Status::Accepted: return "Принят";
        case Status::Investigated: return "Расследован";
        case Status::Completed: return "Завершён";
    }
    return "Неизвестно";
}
Status parseStatus(const std::string& s) {
    if(s=="available") return Status::Available;
    if(s=="accepted") return Status::Accepted;
    if(s=="investigated") return Status::Investigated;
    if(s=="completed") return Status::Completed;
    throw std::runtime_error("Неизвестный статус из базы: " + s);
}
std::string locationName(int id) {
    switch(id) {
        case 0: return "Сероводье — трактир и доска заказов";
        case 1: return "Топкая низина";
        case 2: return "Старая мельница";
        case 3: return "Чёрный бор";
        default: return "Неизвестное место";
    }
}
std::string itemName(int id) {
    switch(id) {case 1:return "Красный эликсир";case 2:return "Серебряное масло";case 3:return "Сбор трав";default:return "Неизвестный предмет";}
}
int itemPrice(int id) {
    switch(id) {case 1:return 25;case 2:return 30;case 3:return 8;default:throw std::invalid_argument("Нет такого предмета");}
}
Hero::Hero(int id, std::string name, int level, int experience, int hp, int maxHp,
           int attack, int defense, int gold, int location,
           int skillPoints, int swordSkill, int vitalitySkill, int signSkill)
: id_(id), level_(level), experience_(experience), hp_(hp), maxHp_(maxHp),
  attack_(attack), defense_(defense), gold_(gold), location_(location),
  skillPoints_(skillPoints), swordSkill_(swordSkill),
  vitalitySkill_(vitalitySkill), signSkill_(signSkill), name_(std::move(name)) {
    if(name_.empty() || name_.size()>60 || level<1 || experience<0 || maxHp<1 ||
       hp<0 || hp>maxHp || attack<1 || defense<0 || gold<0 || location<0 || location>3 ||
       skillPoints<0 || swordSkill<0 || vitalitySkill<0 || signSkill<0)
        throw std::invalid_argument("Недопустимые данные персонажа");
}
void Hero::learnSkill(int skillId) {
    if(skillPoints_<=0)throw std::runtime_error("Нет свободных очков навыков. Выполняйте контракты для повышения уровня");
    switch(skillId) {
        case 1: ++swordSkill_; attack_+=3; break;
        case 2: ++vitalitySkill_; maxHp_+=20; hp_+=20; break;
        case 3: ++signSkill_; break;
        default: throw std::invalid_argument("Выберите навык 1, 2 или 3");
    }
    --skillPoints_;
}
void Hero::travel(int location) {if(location<0||location>3) throw std::invalid_argument("Неизвестная локация");location_=location;}
void Hero::damage(int amount) {hp_=std::max(0,hp_-std::max(0,amount));}
void Hero::heal(int amount) {hp_=std::min(maxHp_,hp_+std::max(0,amount));}
void Hero::reviveAfterDefeat() {hp_=std::max(1,maxHp_/2);location_=0;gold_=std::max(0,gold_-20);}
void Hero::earn(int amount) {if(amount<0)throw std::invalid_argument("Отрицательная награда");gold_+=amount;}
void Hero::pay(int amount) {if(amount<0||amount>gold_)throw std::runtime_error("Недостаточно крон");gold_-=amount;}
int Hero::gainExperience(int amount) {
    if(amount<0)throw std::invalid_argument("Отрицательный опыт");
    experience_+=amount;
    int levels=0;
    while(experience_>=level_*100) {
        experience_-=level_*100;
        ++level_; ++levels; ++skillPoints_;
        maxHp_+=12; hp_=maxHp_; attack_+=3; defense_+=1;
    }
    return levels;
}
Monster::Monster(int id,std::string name,int hp,int attack,int defense,
                 std::string weakness,std::string description)
: id_(id),hp_(hp),attack_(attack),defense_(defense),name_(std::move(name)),
  weakness_(std::move(weakness)),description_(std::move(description)) {
    if(id<1||hp<1||attack<1||defense<0)throw std::invalid_argument("Данные чудовища некорректны");
}
Contract::Contract(int id,int location,int reward,int experience,
                   std::string title,std::string story,std::string clue,
                   std::string resolution,Monster monster)
: id_(id),location_(location),reward_(reward),experience_(experience),
  title_(std::move(title)),story_(std::move(story)),clue_(std::move(clue)),
  resolution_(std::move(resolution)),monster_(std::move(monster)) {
    if(id<1||location<1||location>3||reward<0||experience<0)throw std::invalid_argument("Данные контракта некорректны");
}
ItemStack::ItemStack(int id,int quantity):id_(id),quantity_(quantity) {
    if(id<1||id>3||quantity<0)throw std::invalid_argument("Неверный предмет");
}
void ItemStack::add(int n) {if(n<0)throw std::invalid_argument("Количество меньше нуля");quantity_+=n;}
void ItemStack::take(int n) {if(n<0||n>quantity_)throw std::runtime_error("Недостаточно предметов: "+itemName(id_));quantity_-=n;}
} // namespace border
