#include "service/GameService.h"
#include "repository/MemoryGameRepository.h"
#include "gui/ResponsiveLayout.h"
#include "gui/QuestMapMarkers.h"
#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
using namespace border;
namespace {
void check(bool ok,const std::string& message="Проверка не пройдена") {if(!ok)throw std::runtime_error(message);}
template<class F> bool fails(F f) {try{f();}catch(const std::exception&){return true;}return false;}
void ready(GameService& g,int id=1,int choice=1) {
    g.acceptContract(id);g.travel(g.getContract(id).location());g.investigate(id,choice);g.startBattle(id);
}
void win(GameService& g,int id) {
    for(int n=0;n<30&&g.getProgress(id).fighting;++n) {
        if(g.hero().hp()<40&&g.countItem(1)>0)g.turn(id,Action::Potion);
        else g.turn(id,Action::Heavy);
    }
    check(g.getProgress(id).status==Status::Completed,"Бой не выигран");
}
}
int main(int argc,char** argv) {
    if(argc!=2){std::cerr<<"Укажите название теста\n";return 2;}
    const std::string test=argv[1];
    try {
        MemoryGameRepository repo;
        GameService g(repo);
        auto create=[&](){g.createHero("Каэрн");};
        if(test=="create_hero") {create();check(g.hero().name()=="Каэрн"&&g.hero().gold()==100);}
        else if(test=="reject_empty_name") {check(fails([&]{g.createHero("");}));}
        else if(test=="separate_saves") {
            create();int a=g.hero().id();g.createHero("Альда");int b=g.hero().id();check(a!=b);
            g.selectHero(a);g.acceptContract(1);g.selectHero(b);
            check(g.getProgress(1).status==Status::Available);
        }
        else if(test=="accept_order") {create();g.acceptContract(1);check(g.getProgress(1).status==Status::Accepted);}
        else if(test=="investigate_requires_travel") {create();g.acceptContract(1);check(fails([&]{g.investigate(1,1);}));}
        else if(test=="investigate_choice") {create();g.acceptContract(2);g.travel(2);g.investigate(2,2);check(g.getProgress(2).investigationChoice==2);}
        else if(test=="travel_during_battle") {create();ready(g);check(fails([&]{g.travel(0);}));}
        else if(test=="starting_battle") {create();ready(g);check(g.getProgress(1).monsterHp==46&&g.getProgress(1).fighting);}
        else if(test=="combat_survives_reload") {
            create();int id=g.hero().id();ready(g);g.turn(1,Action::Strike);
            int monsterHp=g.getProgress(1).monsterHp;
            GameService restarted(repo);restarted.selectHero(id);
            check(restarted.getProgress(1).fighting&&restarted.getProgress(1).monsterHp==monsterHp);
        }
        else if(test=="potion_heals") {
            create();ready(g);g.turn(1,Action::Heavy);
            int before=g.hero().hp();g.turn(1,Action::Potion);
            check(g.countItem(1)==1&&g.hero().hp()>before);
        }
        else if(test=="oil_consumption") {create();ready(g);g.turn(1,Action::Oil);check(g.countItem(2)==0&&g.getProgress(1).oiled);}
        else if(test=="crafting_uses_herbs") {create();g.craft(1);check(g.countItem(1)==3&&g.countItem(3)==3);}
        else if(test=="insufficient_funds") {create();for(int i=0;i<12;++i)g.purchase(3);check(fails([&]{g.purchase(2);}));}
        else if(test=="cannot_accept_twice") {create();g.acceptContract(1);check(fails([&]{g.acceptContract(1);}));}
        else if(test=="cannot_investigate_twice") {create();g.acceptContract(1);g.travel(1);g.investigate(1,1);check(fails([&]{g.investigate(1,1);}));}
        else if(test=="flee_resets_battle") {create();ready(g);g.turn(1,Action::Flee);check(!g.getProgress(1).fighting&&g.hero().location()==0&&g.getProgress(1).status==Status::Investigated);}
        else if(test=="defeat_revives") {
            create();ready(g,3);
            for(int i=0;i<20&&g.getProgress(3).fighting;++i)g.turn(3,Action::Strike);
            check(!g.getProgress(3).fighting&&g.hero().hp()>0&&g.hero().location()==0);
        }
        else if(test=="victory_rewards") {create();ready(g);win(g,1);check(g.hero().gold()==210&&g.hero().experience()==80);}
        else if(test=="no_double_reward") {create();ready(g);win(g,1);int gold=g.hero().gold();check(fails([&]{g.startBattle(1);}));check(g.hero().gold()==gold);}
        else if(test=="ritual_branch") {
            create();ready(g,2,2);
            g.turn(2,Action::Heavy);g.turn(2,Action::Heavy);g.turn(2,Action::Ritual);
            check(g.getProgress(2).status==Status::Completed&&g.hero().gold()==210);
        }
        else if(test=="level_up") {
            create();ready(g,1);win(g,1);g.travel(0);ready(g,2);win(g,2);
            check(g.hero().level()==2&&g.hero().maxHp()==112);
        }
        else if(test=="story_completed") {
            create();for(int i=1;i<=3;++i) {ready(g,i);win(g,i);if(i!=3)g.travel(0);}
            check(g.storyCompleted());
        }
        else if(test=="delete_hero") {create();int id=g.hero().id();g.deleteHero(id);check(g.listHeroes().empty()&&!g.selected());}
        else if(test=="inventory_persists") {
            create();int id=g.hero().id();g.craft(2);
            GameService restarted(repo);restarted.selectHero(id);
            check(restarted.countItem(2)==2&&restarted.countItem(3)==2);
        }
        else if(test=="skill_points_on_level") {
            create();ready(g,1);win(g,1);g.travel(0);ready(g,2);win(g,2);
            check(g.hero().level()==2&&g.hero().skillPoints()==1);
        }
        else if(test=="skill_melee") {
            create();ready(g,1);win(g,1);g.travel(0);ready(g,2);win(g,2);
            int attack=g.hero().attack();g.learnSkill(1);
            check(g.hero().swordSkill()==1&&g.hero().attack()==attack+3&&g.hero().skillPoints()==0);
        }
        else if(test=="skill_vitality") {
            create();ready(g,1);win(g,1);g.travel(0);ready(g,2);win(g,2);
            int hp=g.hero().maxHp();g.learnSkill(2);
            check(g.hero().vitalitySkill()==1&&g.hero().maxHp()==hp+20&&g.hero().skillPoints()==0);
        }
        else if(test=="skill_sign") {
            create();ready(g,1);win(g,1);g.travel(0);ready(g,2);win(g,2);
            g.learnSkill(3);check(g.hero().signSkill()==1&&g.hero().skillPoints()==0);
        }
        else if(test=="skill_saved") {
            create();int id=g.hero().id();ready(g,1);win(g,1);g.travel(0);ready(g,2);win(g,2);
            g.learnSkill(1);
            GameService reloaded(repo);reloaded.selectHero(id);
            check(reloaded.hero().swordSkill()==1&&reloaded.hero().attack()==g.hero().attack());
        }
        else if(test=="skill_without_points") {create();check(fails([&]{g.learnSkill(1);}));}
        else if(test=="skill_invalid_choice") {
            create();ready(g,1);win(g,1);g.travel(0);ready(g,2);win(g,2);
            check(fails([&]{g.learnSkill(42);})&&g.hero().skillPoints()==1);
        }
        else if(test=="rest_heals") {
            create();ready(g,1);g.turn(1,Action::Strike);g.turn(1,Action::Flee);
            int before=g.hero().hp(),gold=g.hero().gold();
            check(before<g.hero().maxHp());g.rest();
            check(g.hero().hp()==g.hero().maxHp()&&g.hero().gold()==gold-10);
        }
        else if(test=="rest_invalid_location") {
            create();g.travel(1);check(fails([&]{g.rest();}));
        }
        else if(test=="rest_full_health") {
            create();check(fails([&]{g.rest();})&&g.hero().gold()==100);
        }
        else if(test=="potion_outside_battle") {
            create();ready(g,1);g.turn(1,Action::Strike);g.turn(1,Action::Flee);
            int before=g.hero().hp();g.drinkPotion();
            check(g.hero().hp()>before&&g.countItem(1)==1);
        }
        else if(test=="potion_full_health") {
            create();check(fails([&]{g.drinkPotion();})&&g.countItem(1)==2);
        }
        else if(test=="rest_during_battle") {
            create();ready(g,1);check(fails([&]{g.rest();}));
        }
        else if(test=="skill_during_battle") {
            create();ready(g,1);check(fails([&]{g.learnSkill(1);}));
        }
        else if(test=="potion_during_battle_rejected") {
            create();ready(g,1);check(fails([&]{g.drinkPotion();}));
        }
        else if(test=="multiple_contracts_one_location") {
            create();
            repo.addContract(Contract(4,1,130,70,"Следы в камышах","Новый заказ",
                                      "Огонь поможет","Болото спокойно",
                                      Monster(4,"Камышовый хищник",38,10,2,"fire","Новый враг")));
            GameService reloaded(repo);reloaded.selectHero(g.hero().id());
            check(reloaded.contracts().size()==4);
            check(reloaded.getContract(1).location()==reloaded.getContract(4).location());
            reloaded.acceptContract(1);reloaded.acceptContract(4);
            reloaded.travel(1);
            reloaded.investigate(1,1);reloaded.investigate(4,2);
            reloaded.startBattle(4);
            check(fails([&]{reloaded.startBattle(1);})); // два боя одновременно запрещены
            win(reloaded,4);
            check(reloaded.getProgress(1).status==Status::Investigated);
            check(reloaded.getProgress(4).status==Status::Completed);
        }
        else if(test=="new_contract_existing_hero") {
            create();int id=g.hero().id();g.acceptContract(1);
            repo.addContract(Contract(4,1,130,70,"Второй заказ","Новая история",
                                      "Улики","Итог",Monster(4,"Чудище",40,10,2,"fire","Описание")));
            GameService reloaded(repo);reloaded.selectHero(id);
            check(reloaded.getProgress(1).status==Status::Accepted);
            check(reloaded.getProgress(4).status==Status::Available);
            reloaded.acceptContract(4);
            check(reloaded.getProgress(4).status==Status::Accepted);
        }
        else if(test=="ritual_new_ghost_contract") {
            create();
            repo.addContract(Contract(4,2,90,90,"Голос в тумане","Сказание",
                                      "Следы духа","Спокойствие",
                                      Monster(4,"Ночной дух",48,8,0,"sigil","Призрак")));
            GameService reloaded(repo);reloaded.selectHero(g.hero().id());
            ready(reloaded,4);
            reloaded.turn(4,Action::Heavy);
            reloaded.turn(4,Action::Ritual);
            check(reloaded.getProgress(4).status==Status::Completed);
        }
        else if(test=="reject_duplicate_contract_id") {
            check(fails([&]{repo.addContract(Contract(1,1,100,50,"Дубликат","История",
                "Улика","Итог",Monster(4,"Зверь",30,8,1,"fire","Описание")));}));
        }
        else if(test=="map_status_markers") {
            using border::ui::MapQuest;
            auto groups=border::ui::groupMapQuests({
                {1,1,Status::Available,"Первый"},
                {2,1,Status::Accepted,"Второй"},
                {3,1,Status::Investigated,"Третий"},
                {4,1,Status::Completed,"Четвёртый"}});
            for(int status=0;status<4;++status)
                check(groups[1][static_cast<std::size_t>(status)].size()==1);
            check(groups[1][3][0].title=="Четвёртый");
            check(groups[2][0].empty());
        }
        else if(test=="map_multiple_contracts_same_status") {
            auto groups=border::ui::groupMapQuests({
                {4,1,Status::Available,"Огни"},
                {1,1,Status::Available,"Болото"},
                {2,2,Status::Completed,"Мельница"}});
            check(groups[1][0].size()==2 && groups[1][0][0].id==4 && groups[1][0][1].id==1);
            check(groups[2][3].size()==1 && groups[2][3][0].id==2);
            check(groups[1][3].empty());
        }
        else if(test=="map_markers_follow_progress") {
            create();
            auto build=[&]() {
                std::vector<border::ui::MapQuest> input;
                for(const auto& c:g.contracts())
                    input.push_back({c.id(),c.location(),g.getProgress(c.id()).status,c.title()});
                return border::ui::groupMapQuests(input);
            };
            check(build()[1][0].size()==1);
            g.acceptContract(1);
            check(build()[1][0].empty() && build()[1][1].size()==1);
            g.travel(1);g.investigate(1,1);
            check(build()[1][1].empty() && build()[1][2].size()==1);
            g.startBattle(1);win(g,1);
            check(build()[1][2].empty() && build()[1][3].size()==1);
        }
        else if(test=="map_invalid_quest_data") {
            using border::ui::groupMapQuests;
            check(fails([&]{groupMapQuests({{0,1,Status::Available,"Invalid"}});}));
            check(fails([&]{groupMapQuests({{10,5,Status::Available,"Invalid"}});}));
            check(fails([&]{groupMapQuests({{10,-1,Status::Available,"Invalid"}});}));
        }
        else if(test=="responsive_layout_minimum"||test=="responsive_layout_regular"||test=="responsive_layout_wide") {
            int width=1280,height=850;
            if(test=="responsive_layout_minimum") {width=760;height=560;}
            if(test=="responsive_layout_wide") {width=1800;height=1100;}
            const auto policy=border::ui::policyFor(width,height);
            check(policy.compact==(width<1120));
            check(policy.coverHeight>=108 && policy.coverHeight<=205);
            check(policy.mapMinimumHeight>=150 && policy.mapMinimumHeight<=270);
            check(policy.coverHeight<height/2);
            if (width==760) check(policy.compact);
            if (width==1800) check(!policy.compact);
        }
        else throw std::runtime_error("Неизвестный тест: "+test);
        std::cout<<"PASS: "<<test<<"\n";return 0;
    } catch(const std::exception& e) {
        std::cerr<<"FAIL: "<<test<<" — "<<e.what()<<"\n";return 1;
    }
}
