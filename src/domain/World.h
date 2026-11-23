#pragma once
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace border {

enum class Status { Available, Accepted, Investigated, Completed };
enum class Action { Strike, Heavy, Igni, Quen, Yrden, Potion, Oil, Ritual, Flee };

std::string statusName(Status s);
Status parseStatus(const std::string& s);
std::string locationName(int id);
std::string itemName(int id);
int itemPrice(int id);

class Hero {
    int id_{}, level_{1}, experience_{0}, hp_{100}, maxHp_{100};
    int attack_{15}, defense_{4}, gold_{100}, location_{0};
    int skillPoints_{0}, swordSkill_{0}, vitalitySkill_{0}, signSkill_{0};
    std::string name_;
public:
    Hero(int id, std::string name, int level=1, int experience=0, int hp=100,
         int maxHp=100, int attack=15, int defense=4, int gold=100, int location=0,
         int skillPoints=0, int swordSkill=0, int vitalitySkill=0, int signSkill=0);
    int id() const {return id_;}
    const std::string& name() const {return name_;}
    int level() const {return level_;}
    int experience() const {return experience_;}
    int hp() const {return hp_;}
    int maxHp() const {return maxHp_;}
    int attack() const {return attack_;}
    int defense() const {return defense_;}
    int gold() const {return gold_;}
    int location() const {return location_;}
    int skillPoints() const {return skillPoints_;}
    int swordSkill() const {return swordSkill_;}
    int vitalitySkill() const {return vitalitySkill_;}
    int signSkill() const {return signSkill_;}
    void learnSkill(int skillId);
    void travel(int location);
    void damage(int damage);
    void heal(int amount);
    void reviveAfterDefeat();
    void earn(int amount);
    void pay(int amount);
    int gainExperience(int amount);
};

class Monster {
    int id_, hp_, attack_, defense_;
    std::string name_, weakness_, description_;
public:
    Monster(int id, std::string name, int hp, int attack, int defense,
            std::string weakness, std::string description);
    int id() const {return id_;}
    const std::string& name() const {return name_;}
    int hp() const {return hp_;}
    int attack() const {return attack_;}
    int defense() const {return defense_;}
    const std::string& weakness() const {return weakness_;}
    const std::string& description() const {return description_;}
};

class Contract {
    int id_, location_, reward_, experience_;
    std::string title_, story_, clue_, resolution_;
    Monster monster_;
public:
    Contract(int id, int location, int reward, int experience,
             std::string title, std::string story, std::string clue,
             std::string resolution, Monster monster);
    int id() const {return id_;}
    int location() const {return location_;}
    int reward() const {return reward_;}
    int experience() const {return experience_;}
    const std::string& title() const {return title_;}
    const std::string& story() const {return story_;}
    const std::string& clue() const {return clue_;}
    const std::string& resolution() const {return resolution_;}
    const Monster& monster() const {return monster_;}
};

class ItemStack {
    int id_, quantity_;
public:
    ItemStack(int id, int quantity=0);
    int id() const {return id_;}
    int quantity() const {return quantity_;}
    void add(int n);
    void take(int n);
};

// Состояние одного контракта: сохраняется в базе после каждого действия.
class Progress {
public:
    int contractId{};
    Status status{Status::Available};
    int investigationChoice{0}; // 1 = следы, 2 = свидетели
    bool fighting{false};
    int monsterHp{0};
    int turn{0};
    int focus{0};
    int shield{0};
    int trap{0};
    bool oiled{false};
    std::string outcome;
};

} // namespace border
