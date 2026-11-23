#include "service/GameService.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace border {
GameService::GameService(IGameRepository& repo):repo_(repo) {contracts_=repo_.listContracts();}
std::vector<Hero> GameService::listHeroes() {return repo_.listHeroes();}
void GameService::requireHero() const {if(!hero_)throw std::runtime_error("Сначала создайте или выберите персонажа");}
const Hero& GameService::hero() const {requireHero();return *hero_;}
void GameService::createHero(const std::string& name) {
    if(name.empty()||name.size()>60)throw std::invalid_argument("Имя должно содержать от 1 до 60 байт");
    for(char c:name)if(static_cast<unsigned char>(c)<32)throw std::invalid_argument("Недопустимый символ имени");
    const Hero newHero=repo_.createHero(name);
    selectHero(newHero.id());
}
void GameService::selectHero(int id) {
    auto h=repo_.getHero(id);
    if(!h)throw std::runtime_error("Персонаж не найден");
    auto progress=repo_.loadProgress(id);
    auto items=repo_.loadInventory(id);
    hero_=std::move(h); progress_=std::move(progress); items_=std::move(items);
    for(const auto& c:contracts_) {progressFor(c.id());}
    for(int i=1;i<=3;++i) {itemFor(i);}
}
void GameService::deleteHero(int id) {
    repo_.deleteHero(id);
    if(hero_ && hero_->id()==id) {hero_.reset(); progress_.clear(); items_.clear();}
}
const Contract& GameService::contractFor(int id) const {
    auto it=std::find_if(contracts_.begin(),contracts_.end(),[&](const Contract& c){return c.id()==id;});
    if(it==contracts_.end())throw std::invalid_argument("Контракт не найден");
    return *it;
}
const Contract& GameService::getContract(int id) const {return contractFor(id);}
Progress& GameService::progressFor(int id) {
    auto it=std::find_if(progress_.begin(),progress_.end(),[&](const Progress& p){return p.contractId==id;});
    if(it==progress_.end())throw std::runtime_error("В базе нет прогресса по контракту");
    return *it;
}
const Progress& GameService::progressFor(int id) const {
    auto it=std::find_if(progress_.begin(),progress_.end(),[&](const Progress& p){return p.contractId==id;});
    if(it==progress_.end())throw std::runtime_error("Прогресс контракта не найден");
    return *it;
}
const Progress& GameService::getProgress(int id) const {requireHero();return progressFor(id);}
ItemStack& GameService::itemFor(int id) {
    auto it=std::find_if(items_.begin(),items_.end(),[&](const ItemStack& s){return s.id()==id;});
    if(it==items_.end())throw std::runtime_error("Отсутствует запись предмета в инвентаре");
    return *it;
}
int GameService::countItem(int id) const {
    requireHero();
    auto it=std::find_if(items_.begin(),items_.end(),[&](const ItemStack& s){return s.id()==id;});
    return it==items_.end()?0:it->quantity();
}
bool GameService::hasActiveBattle() const {
    return std::any_of(progress_.begin(),progress_.end(),[](const Progress& p){return p.fighting;});
}
void GameService::persist() {
    requireHero();
    try {repo_.save(*hero_,progress_,items_);}
    catch(...) {
        // БД откатила транзакцию; перечитываем состояние перед следующим действием.
        int id=hero_->id();
        try {selectHero(id);} catch(...) {hero_.reset();progress_.clear();items_.clear();}
        throw;
    }
}
void GameService::travel(int location) {
    requireHero();
    if(location<0||location>3)throw std::invalid_argument("Неверная точка на карте");
    if(hasActiveBattle())throw std::runtime_error("Нельзя путешествовать во время боя: сначала отступите");
    hero_->travel(location);persist();
}
std::string GameService::acceptContract(int id) {
    requireHero();const auto& c=contractFor(id);auto& p=progressFor(id);
    if(hero_->location()!=0)throw std::runtime_error("Заказы принимаются на доске в Сероводье");
    if(p.status!=Status::Available)throw std::runtime_error("Этот заказ уже взят или завершён");
    p.status=Status::Accepted;persist();
    return "Контракт принят: "+c.title()+". Место охоты: "+locationName(c.location());
}
std::string GameService::investigate(int id,int choice) {
    requireHero();const auto& c=contractFor(id);auto& p=progressFor(id);
    if(p.status!=Status::Accepted)throw std::runtime_error("Для расследования сначала возьмите контракт");
    if(hero_->location()!=c.location())throw std::runtime_error("Нужно прибыть к месту происшествия: "+locationName(c.location()));
    if(choice!=1&&choice!=2)throw std::invalid_argument("Выберите 1 (осмотреть следы) или 2 (опросить свидетелей)");
    p.investigationChoice=choice;p.status=Status::Investigated;persist();
    return c.clue() + (choice==1 ? "\nВы изучили следы: первые атаки будут точнее." :
                                    "\nВы помогли свидетелям: община добавит 15 крон к награде.");
}
std::string GameService::startBattle(int id) {
    requireHero();const auto& c=contractFor(id);auto& p=progressFor(id);
    if(hero_->location()!=c.location())throw std::runtime_error("Противник находится в другом месте");
    if(p.status!=Status::Investigated)throw std::runtime_error("Сначала примите и расследуйте контракт");
    if(hasActiveBattle()) {
        if(p.fighting)return "Бой уже начат. Противник: "+c.monster().name();
        throw std::runtime_error("У вас уже идёт другой бой");
    }
    p.fighting=true;p.monsterHp=c.monster().hp();p.turn=0;p.focus=6;
    p.shield=0;p.trap=0;p.oiled=false;
    persist();
    return "В тумане появляется "+c.monster().name()+". "+c.monster().description()+
           "\nУправление: удар, сильный удар, знаки, эликсир, масло, ритуал или отступление.";
}
std::string GameService::turn(int id,Action action) {
    requireHero();const auto& c=contractFor(id);auto& m=c.monster();auto& p=progressFor(id);
    if(!p.fighting || p.status!=Status::Investigated)throw std::runtime_error("Сначала начните бой");
    if(hero_->location()!=c.location())throw std::runtime_error("Персонаж не на месте боя");
    std::ostringstream out;
    if(action==Action::Flee) {
        p.fighting=false;p.monsterHp=0;p.turn=0;p.focus=0;p.shield=0;p.trap=0;p.oiled=false;
        hero_->travel(0);persist();
        return "Вы отступили в Сероводье. Расследование сохранено, монстр восстановит здоровье к следующей встрече.";
    }
    if(action==Action::Ritual) {
        // Для новых контрактов ритуал определяется типом слабости чудовища,
        // а не жёстко заданным ID контракта.
        if(m.weakness()!="sigil")throw std::runtime_error("Мирный ритуал возможен только против призрачного противника");
        if(p.monsterHp>25)throw std::runtime_error("Ослабьте духа до 25 HP или ниже");
        if(p.focus<2)throw std::runtime_error("Не хватает сосредоточения: нужно 2");
        p.focus-=2;p.fighting=false;p.monsterHp=0;p.status=Status::Completed;
        p.outcome="Проклятие снято ритуалом";
        hero_->earn(c.reward()/2 + (p.investigationChoice==2?15:0));
        int up=hero_->gainExperience(c.experience());
        persist();
        out << "Вы произнесли слова обряда, и дух рассеялся без насилия. " << c.resolution()
            << "\nНаграда: " << c.reward()/2 << " крон, опыт: " << c.experience();
        if(up)out<<". Повышение уровня: +"<<up;
        return out.str();
    }
    ++p.turn;
    int damage=0;
    bool heavy=false;
    switch(action) {
        case Action::Strike:
        case Action::Heavy: {
            heavy=(action==Action::Heavy);
            damage=std::max(1,(heavy?hero_->attack()*2:hero_->attack())-m.defense());
            if(p.investigationChoice==1 && p.turn<=3)damage+=4;
            if(p.oiled)damage+=6+(m.weakness()=="silver"?6:0);
            if(p.trap>0&&m.weakness()=="sigil")damage+=10;
            out << (heavy?"Сильный удар":"Удар мечом") << ": -" << damage << " HP врагу. ";
            break;
        }
        case Action::Igni:
            if(p.focus<2)throw std::runtime_error("Не хватает сосредоточения для огненного знака");
            p.focus-=2;damage=10+hero_->level()*2+(m.weakness()=="fire"?12:0)+hero_->signSkill()*5;
            out<<"Огненный знак обжигает цель: -"<<damage<<" HP. ";break;
        case Action::Quen:
            if(p.focus<2)throw std::runtime_error("Не хватает сосредоточения для защитного знака");
            p.focus-=2;p.shield=17+hero_->level()*2+hero_->signSkill()*5;
            out<<"Защитный знак создал щит ("<<p.shield<<" HP). ";break;
        case Action::Yrden:
            if(p.focus<1)throw std::runtime_error("Не хватает сосредоточения для ловушки");
            --p.focus;p.trap=3;
            out<<"Магическая ловушка замедляет противника на три хода. ";break;
        case Action::Potion:
            if(hero_->hp()>=hero_->maxHp())throw std::runtime_error("Здоровье уже полное: эликсир не потрачен");
            itemFor(1).take(1);hero_->heal(40);
            out<<"Вы выпили красный эликсир: здоровье "<<hero_->hp()<<"/"<<hero_->maxHp()<<". ";break;
        case Action::Oil:
            if(p.oiled)throw std::runtime_error("Меч уже смазан маслом");
            itemFor(2).take(1);p.oiled=true;
            out<<"Серебряное масло нанесено на клинок. ";break;
        case Action::Ritual:case Action::Flee:break;
    }
    p.monsterHp=std::max(0,p.monsterHp-damage);
    if(p.monsterHp==0) {
        p.fighting=false;p.status=Status::Completed;p.outcome="Чудовище побеждено";
        int gold=c.reward()+(p.investigationChoice==2?15:0);
        hero_->earn(gold);
        int up=hero_->gainExperience(c.experience());
        out<<"\nПОБЕДА! "<<c.resolution()<<"\nНаграда: "<<gold<<" крон; опыт: "<<c.experience()<<".";
        if(up)out<<" Повышение уровня на "<<up<<"!";
        persist();return out.str();
    }
    int enemyDamage=std::max(1,m.attack()-hero_->defense()+(heavy?4:0)+(p.turn%3==0?3:0));
    if(p.trap>0) {enemyDamage=std::max(0,enemyDamage-6-hero_->signSkill()*2);--p.trap;}
    int absorbed=std::min(p.shield,enemyDamage);p.shield-=absorbed;enemyDamage-=absorbed;
    hero_->damage(enemyDamage);
    out<<"\n"<<m.name()<<" отвечает: -"<<enemyDamage<<" HP";
    if(absorbed)out<<" (щит поглотил "<<absorbed<<")";
    out<<".\nВаше HP: "<<hero_->hp()<<"/"<<hero_->maxHp()
       <<", HP врага: "<<p.monsterHp<<"/"<<m.hp()<<", сосредоточение: "<<p.focus;
    if(hero_->hp()==0) {
        p.fighting=false;p.monsterHp=0;p.focus=0;p.shield=0;p.trap=0;p.oiled=false;
        hero_->reviveAfterDefeat();
        out<<"\nПОРАЖЕНИЕ. Вас вытащили из чащи и доставили в Сероводье. Потеряно до 20 крон.";
    }
    persist();return out.str();
}
std::string GameService::purchase(int itemId) {
    requireHero();if(hero_->location()!=0)throw std::runtime_error("Торговец находится в деревне");
    int price=itemPrice(itemId);
    hero_->pay(price);itemFor(itemId).add(1);persist();
    return "Куплено: "+itemName(itemId)+". Уплачено "+std::to_string(price)+" крон.";
}
std::string GameService::craft(int itemId) {
    requireHero();if(hero_->location()!=0)throw std::runtime_error("Алхимический стол находится в деревне");
    if(itemId!=1&&itemId!=2)throw std::runtime_error("Из трав можно изготовить только эликсир или масло");
    int cost=itemId==1?2:3;
    itemFor(3).take(cost);itemFor(itemId).add(1);persist();
    return "Создано: "+itemName(itemId)+". Потрачено трав: "+std::to_string(cost)+".";
}
std::string GameService::drinkPotion() {
    requireHero();
    if(hasActiveBattle())throw std::runtime_error("Во время боя используйте кнопку «Выпить эликсир» для текущего контракта");
    if(hero_->hp()>=hero_->maxHp())throw std::runtime_error("Здоровье уже полное: эликсир не потрачен");
    itemFor(1).take(1);
    hero_->heal(40);
    persist();
    return "Выпит эликсир вне боя: здоровье " + std::to_string(hero_->hp()) + "/" + std::to_string(hero_->maxHp());
}
std::string GameService::rest() {
    requireHero();
    if(hasActiveBattle())throw std::runtime_error("Нельзя отдыхать во время боя");
    if(hero_->location()!=0)throw std::runtime_error("Отдых доступен только в трактире Сероводья");
    if(hero_->hp()>=hero_->maxHp())throw std::runtime_error("Здоровье уже полное: платить за отдых не нужно");
    hero_->pay(10);
    hero_->heal(hero_->maxHp());
    persist();
    return "Отдых в трактире: здоровье полностью восстановлено, потрачено 10 крон";
}
std::string GameService::learnSkill(int skillId) {
    requireHero();
    if(hasActiveBattle())throw std::runtime_error("Навыки нельзя изучать во время боя");
    hero_->learnSkill(skillId);
    persist();
    switch(skillId) {
        case 1:return "Мастерство меча улучшено: +3 к атаке";
        case 2:return "Живучесть улучшена: +20 к максимуму HP и текущему здоровью";
        case 3:return "Сила знаков улучшена: огонь +5 урона, щит +5, ловушка ослабляет противника сильнее";
    }
    throw std::logic_error("Неизвестный навык");
}
bool GameService::storyCompleted() const {
    requireHero();return !progress_.empty() &&
        std::all_of(progress_.begin(),progress_.end(),[](const Progress& p){return p.status==Status::Completed;});
}
} // namespace border
