-- Оригинальный игровой мир «Контракты Пограничья».
-- Скрипт безопасно запускать повторно: сохранения НЕ удаляются.
BEGIN;
CREATE TABLE IF NOT EXISTS monsters (
 id INTEGER PRIMARY KEY,
 name TEXT NOT NULL,
 max_hp INTEGER NOT NULL CHECK(max_hp>0),
 attack INTEGER NOT NULL CHECK(attack>0),
 defense INTEGER NOT NULL CHECK(defense>=0),
 weakness TEXT NOT NULL,
 description TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS contracts (
 id INTEGER PRIMARY KEY,
 title TEXT NOT NULL,
 location_id INTEGER NOT NULL CHECK(location_id BETWEEN 1 AND 3),
 reward INTEGER NOT NULL CHECK(reward>=0),
 experience INTEGER NOT NULL CHECK(experience>=0),
 story TEXT NOT NULL,
 clue TEXT NOT NULL,
 resolution TEXT NOT NULL,
 monster_id INTEGER NOT NULL REFERENCES monsters(id)
);
CREATE TABLE IF NOT EXISTS heroes (
 id INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
 name TEXT NOT NULL CHECK(char_length(name) BETWEEN 1 AND 60),
 level INTEGER NOT NULL DEFAULT 1 CHECK(level>=1),
 experience INTEGER NOT NULL DEFAULT 0 CHECK(experience>=0),
 hp INTEGER NOT NULL DEFAULT 100 CHECK(hp>=0),
 max_hp INTEGER NOT NULL DEFAULT 100 CHECK(max_hp>0),
 attack INTEGER NOT NULL DEFAULT 15 CHECK(attack>0),
 defense INTEGER NOT NULL DEFAULT 4 CHECK(defense>=0),
 gold INTEGER NOT NULL DEFAULT 100 CHECK(gold>=0),
 location_id INTEGER NOT NULL DEFAULT 0 CHECK(location_id BETWEEN 0 AND 3),
 skill_points INTEGER NOT NULL DEFAULT 0 CHECK(skill_points>=0),
 sword_skill INTEGER NOT NULL DEFAULT 0 CHECK(sword_skill>=0),
 vitality_skill INTEGER NOT NULL DEFAULT 0 CHECK(vitality_skill>=0),
 sign_skill INTEGER NOT NULL DEFAULT 0 CHECK(sign_skill>=0),
 created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
 CHECK(hp<=max_hp)
);
CREATE TABLE IF NOT EXISTS hero_contracts (
 hero_id INTEGER NOT NULL REFERENCES heroes(id) ON DELETE CASCADE,
 contract_id INTEGER NOT NULL REFERENCES contracts(id),
 status TEXT NOT NULL DEFAULT 'available' CHECK(status IN ('available','accepted','investigated','completed')),
 investigation_choice INTEGER NOT NULL DEFAULT 0 CHECK(investigation_choice BETWEEN 0 AND 2),
 fighting BOOLEAN NOT NULL DEFAULT FALSE,
 monster_hp INTEGER NOT NULL DEFAULT 0 CHECK(monster_hp>=0),
 turn INTEGER NOT NULL DEFAULT 0 CHECK(turn>=0),
 focus INTEGER NOT NULL DEFAULT 0 CHECK(focus>=0),
 shield INTEGER NOT NULL DEFAULT 0 CHECK(shield>=0),
 trap INTEGER NOT NULL DEFAULT 0 CHECK(trap>=0),
 oiled BOOLEAN NOT NULL DEFAULT FALSE,
 outcome TEXT NOT NULL DEFAULT '',
 PRIMARY KEY(hero_id,contract_id)
);
CREATE TABLE IF NOT EXISTS inventory (
 hero_id INTEGER NOT NULL REFERENCES heroes(id) ON DELETE CASCADE,
 item_id INTEGER NOT NULL CHECK(item_id BETWEEN 1 AND 3),
 quantity INTEGER NOT NULL DEFAULT 0 CHECK(quantity>=0),
 PRIMARY KEY(hero_id,item_id)
);

INSERT INTO monsters(id,name,max_hp,attack,defense,weakness,description) VALUES
 (1,'Топляк',46,9,2,'fire','Поднявшаяся из трясины тварь тянется к свету чужими руками.'),
 (2,'Мельничная скорбница',68,12,4,'sigil','Призрачная женщина кружит над жерновами, не касаясь пола.'),
 (3,'Корневой страж',94,15,6,'silver','Из мха поднимается исполин с рогами из сухого дерева.')
ON CONFLICT (id) DO NOTHING;
INSERT INTO contracts(id,title,location_id,reward,experience,story,clue,resolution,monster_id) VALUES
 (1,'То, что дышит в болоте',1,110,80,
 'Рыбаки исчезают в камышах. Утром лодки возвращаются без людей, а на бортах остаются следы мокрых когтей.',
 'Вы нашли слизь и обожжённый тростник. Это топляк: огонь заставляет его отступить. Вода хранит чужие голоса.',
 'Жители снова могут спускать лодки на воду. У тумана стало одной тайной меньше.',1),
 (2,'Плач у старой мельницы',2,190,130,
 'Ночью слышится женская песня. На мельнице гаснут фонари, а каждый, кто приходит на зов, теряет память.',
 'На жерновах вырезано имя погибшей девушки. Дух удерживает не месть, а нарушенное обещание. Знак ловушки ослабит его.',
 'Мельница умолкла. Вместо плача слышен только ветер среди лопастей.',2),
 (3,'Король сухих ветвей',3,320,190,
 'Лесорубы нашли деревья, проросшие сквозь забытые доспехи. Старый бор не пускает живых в свои владения.',
 'В корнях виден металл прежних охотников. Серебряное масло усиливает удар по древнему стражу.',
 'Чёрный бор раскрывает тропу, и у деревни появляется надежда на спокойную зиму.',3)
ON CONFLICT (id) DO NOTHING;
COMMIT;
