-- Пример расширения игрового мира: второй контракт в Топкой низине (location_id=1).
-- Скрипт можно запускать повторно; уже существующие сохранения не удаляются.
-- Запускайте на СУЩЕСТВУЮЩЕЙ базе PostgreSQL, затем перезапускайте игру.
BEGIN;
INSERT INTO monsters(id,name,max_hp,attack,defense,weakness,description)
VALUES (4,'Камышовый хищник',62,11,3,'fire',
        'Под треск болотных огней из камышей выбирается покрытый тиной хищник.')
ON CONFLICT (id) DO NOTHING;

INSERT INTO contracts(id,title,location_id,reward,experience,story,clue,resolution,monster_id)
VALUES (4,'Огни над трясиной',1,160,105,
        'У старой переправы по ночам горят зелёные огни. Путники слышат зов, сходят с тропы и исчезают.',
        'На камышах остались чёрные следы когтей. Тварь боится открытого огня.',
        'Огни над трясиной погасли, и путники снова находят дорогу.',4)
ON CONFLICT (id) DO NOTHING;

-- Существующие персонажи сразу получают новый контракт в статусе available.
-- Аналогичную проверку автоматически выполняет PgGameRepository при загрузке героя.
INSERT INTO hero_contracts(hero_id,contract_id)
SELECT h.id,4 FROM heroes h
ON CONFLICT (hero_id,contract_id) DO NOTHING;
COMMIT;
