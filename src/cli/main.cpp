#include "app/RepositoryFactory.h"
#include "service/GameService.h"
#include <clocale>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>

using namespace border;
namespace {
std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    if(!std::getline(std::cin,line)) {std::cout<<"\n";std::exit(0);}
    return line;
}
int readInt(const std::string& prompt) {
    std::string value=readLine(prompt);
    std::istringstream in(value);int n;char extra;
    if(!(in>>n)||(in>>extra))throw std::runtime_error("Нужно ввести целое число");
    return n;
}
void line() {std::cout<<"\n======================================================\n";}
void printHero(const Hero& h) {
    std::cout << "\n"<<h.name()<<" | Ур. "<<h.level()<<" | XP "<<h.experience()<<"/"<<h.level()*100
      <<" | HP "<<h.hp()<<"/"<<h.maxHp()<<" | Атака "<<h.attack()<<" | Защита "<<h.defense()
      <<" | "<<h.gold()<<" крон | Очки навыков: "<<h.skillPoints()<<"\n"<<locationName(h.location())<<"\n";
}
void printInventory(GameService& game) {
    std::cout<<"\n--- СУМКА ОХОТНИКА ---\n";
    for(int i=1;i<=3;++i)std::cout<<i<<". "<<itemName(i)<<" x "<<game.countItem(i)<<"\n";
}
void printContracts(GameService& game) {
    std::cout<<"\n--- ЖУРНАЛ КОНТРАКТОВ ---\n";
    for(const auto& c:game.contracts()) {
        const auto& p=game.getProgress(c.id());
        std::cout<<"\n["<<c.id()<<"] "<<c.title()<<" ["<<statusName(p.status)<<"]\n"
                 <<"Место: "<<locationName(c.location())<<" | Награда: "<<c.reward()<<" крон\n"
                 <<c.story()<<"\n";
        if(p.status==Status::Investigated||p.status==Status::Completed)
            std::cout<<"Противник: "<<c.monster().name()<<" | Слабость: "<<c.monster().weakness()<<"\n";
        if(p.fighting)std::cout<<"Бой в процессе: HP противника "<<p.monsterHp<<"\n";
        if(p.status==Status::Completed)std::cout<<"Итог: "<<p.outcome<<"\n";
    }
}
void showMap(GameService& game) {
    std::cout<<"\n                 [3] ЧЁРНЫЙ БОР\n"
                "                       |\n"
                "[1] ТОПКАЯ НИЗИНА -- [0] СЕРОВОДЬЕ -- [2] СТАРАЯ МЕЛЬНИЦА\n"
                "                       |\n"
                "                  ТРАКТ К ЮГУ\n"
                "\nВаша локация: "<<locationName(game.hero().location())<<"\n";
}
void battle(GameService& game) {
    int id=readInt("ID контракта: ");
    std::cout<<game.startBattle(id)<<"\n";
    while(game.getProgress(id).fighting) {
        const auto& h=game.hero();const auto& p=game.getProgress(id);
        const auto& m=game.getContract(id).monster();
        line();
        std::cout<<"БОЙ: "<<h.name()<<" HP "<<h.hp()<<"/"<<h.maxHp()<<" | "
                 <<m.name()<<" HP "<<p.monsterHp<<"/"<<m.hp()
                 <<" | Сосредоточение "<<p.focus<<"\n"
                 <<"1. Удар мечом    2. Сильный удар    3. Огненный знак\n"
                 <<"4. Защитный знак 5. Знак ловушки    6. Эликсир (+40 HP)\n"
                 <<"7. Серебряное масло    8. Мирный ритуал (дух)    9. Отступить\n"
                 <<"0. Вернуться в меню (бой сохранится)\n";
        int v;
        try {v=readInt("Действие: ");}catch(const std::exception& e){std::cout<<e.what()<<"\n";continue;}
        if(v==0)return;
        if(v<1||v>9) {std::cout<<"Нет такого действия\n";continue;}
        Action a[]={Action::Strike,Action::Heavy,Action::Igni,Action::Quen,
                    Action::Yrden,Action::Potion,Action::Oil,Action::Ritual,Action::Flee};
        try {std::cout<<game.turn(id,a[v-1])<<"\n";}
        catch(const std::exception& e) {std::cout<<"Ошибка: "<<e.what()<<"\n";}
    }
    if(game.storyCompleted())std::cout<<"\n*** ВСЕ КОНТРАКТЫ ИСПОЛНЕНЫ. ПОГРАНИЧЬЕ ВЫЖИЛО. ***\n";
}
void shop(GameService& game) {
    while(true) {
        printInventory(game);
        std::cout<<"Кроны: "<<game.hero().gold()<<"\n"
                 <<"1. Купить эликсир (25)  2. Купить масло (30)  3. Купить травы (8)\n"
                 <<"4. Сварить эликсир (2 травы)  5. Создать масло (3 травы)  0. Назад\n";
        int v=readInt("Выбор: "); if(v==0)return;
        try {
            if(v>=1&&v<=3)std::cout<<game.purchase(v)<<"\n";
            else if(v==4||v==5)std::cout<<game.craft(v==4?1:2)<<"\n";
            else std::cout<<"Нет пункта\n";
        }catch(const std::exception& e){std::cout<<"Ошибка: "<<e.what()<<"\n";}
    }
}
void heroSelection(GameService& game) {
    while(true) {
        line(); std::cout<<"КОНТРАКТЫ ПОГРАНИЧЬЯ — выбор сохранения\n";
        for(const auto& h:game.listHeroes())
            std::cout<<"["<<h.id()<<"] "<<h.name()<<" — уровень "<<h.level()<<"\n";
        std::cout<<"1. Создать нового охотника\n2. Загрузить по ID\n3. Удалить сохранение\n0. Выход\n";
        try {
            int v=readInt("Выбор: ");
            if(v==0)std::exit(0);
            if(v==1) {game.createHero(readLine("Имя охотника: "));return;}
            if(v==2) {game.selectHero(readInt("ID: "));return;}
            if(v==3) {
                int id=readInt("ID для удаления: ");
                if(readLine("Напишите УДАЛИТЬ для подтверждения: ")=="УДАЛИТЬ")
                    game.deleteHero(id);
            }
        }catch(const std::exception& e){std::cout<<"Ошибка: "<<e.what()<<"\n";}
    }
}
void skillsMenu(GameService& game) {
    const auto& h=game.hero();
    std::cout<<"\n--- НАВЫКИ (свободно: "<<h.skillPoints()<<") ---\n"
             <<"1. Мастерство меча ["<<h.swordSkill()<<"] (+3 атаки)\n"
             <<"2. Живучесть ["<<h.vitalitySkill()<<"] (+20 HP)\n"
             <<"3. Мощь знаков ["<<h.signSkill()<<"] (+5 огонь/щит, +2 ловушка)\n"
             <<"0. Назад\n";
    int n=readInt("Изучить навык: ");
    if(n!=0)std::cout<<game.learnSkill(n)<<"\n";
}
void gameLoop(GameService& game) {
    while(true) {
        line();std::cout<<"       КОНТРАКТЫ ПОГРАНИЧЬЯ\n";
        printHero(game.hero());
        std::cout<<"\n1. Карта     2. Доска заказов / журнал\n"
                 <<"3. Принять заказ    4. Отправиться в локацию\n"
                 <<"5. Расследовать     6. Сражаться / продолжить бой\n"
                 <<"7. Сумка            8. Алхимия и торговец\n"
                 <<"9. Сменить персонажа    10. Навыки\n"
                 <<"11. Отдых (10 крон)      12. Выпить эликсир вне боя (+40 HP)\n"
                 <<"0. Сохранить и выйти\n";
        try {
            int v=readInt("Выбор: ");
            if(v==0)return;
            if(v==1)showMap(game);
            else if(v==2)printContracts(game);
            else if(v==3)std::cout<<game.acceptContract(readInt("ID контракта: "))<<"\n";
            else if(v==4) {showMap(game);game.travel(readInt("ID локации 0–3: "));std::cout<<"Вы прибыли: "<<locationName(game.hero().location())<<"\n";}
            else if(v==5) {
                int id=readInt("ID контракта: ");
                std::cout<<"1. Осмотреть следы (бонус первых атак)\n2. Поговорить с жителями (доплата к награде)\n";
                std::cout<<game.investigate(id,readInt("Выбор: "))<<"\n";
            }
            else if(v==6)battle(game);
            else if(v==7)printInventory(game);
            else if(v==8)shop(game);
            else if(v==9){heroSelection(game);}
            else if(v==10)skillsMenu(game);
            else if(v==11)std::cout<<game.rest()<<"\n";
            else if(v==12)std::cout<<game.drinkPotion()<<"\n";
            else std::cout<<"Нет такого пункта\n";
        }catch(const std::exception& e) {std::cerr<<"Ошибка: "<<e.what()<<"\n";}
    }
}
} // namespace
int main(int argc,char** argv) {
    std::setlocale(LC_ALL, "");
    const bool demo=argc==2 && std::string(argv[1])=="--demo";
    if(argc>1&&!demo){std::cerr<<"Использование: borderland_cli [--demo]\n";return 2;}
    try {
        auto repo=openRepository(demo);
        GameService game(*repo);
        if(demo)std::cout<<"ДЕМО-РЕЖИМ: прогресс существует только до закрытия программы.\n";
        heroSelection(game);
        gameLoop(game);
    }catch(const std::exception& e) {
        std::cerr<<"Ошибка запуска: "<<e.what()<<"\nПроверьте PostgreSQL, sql/init.sql и BORDERLAND_DB.\n";
        return 2;
    }
    std::cout<<(demo?"Демо завершено (без сохранения).\n":"Прогресс сохранён. До следующей охоты.\n");
    return 0;
}
