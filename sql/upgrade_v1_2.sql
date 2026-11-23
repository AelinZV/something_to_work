-- Безопасное обновление существующей БД с версии 1.1 до 1.2.
-- Не удаляет героев, контракты, инвентарь и сохранения.
-- Повторный запуск не выдаёт дополнительные очки.
BEGIN;
DO $$
BEGIN
    IF NOT EXISTS (
        SELECT 1 FROM information_schema.columns
        WHERE table_schema='public' AND table_name='heroes' AND column_name='skill_points'
    ) THEN
        ALTER TABLE heroes ADD COLUMN skill_points INTEGER NOT NULL DEFAULT 0 CHECK(skill_points>=0);
        -- Выдаём ранее заработанные очки уже существующим героям.
        UPDATE heroes SET skill_points=GREATEST(level-1,0);
    END IF;
END $$;
ALTER TABLE heroes ADD COLUMN IF NOT EXISTS sword_skill INTEGER NOT NULL DEFAULT 0 CHECK(sword_skill>=0);
ALTER TABLE heroes ADD COLUMN IF NOT EXISTS vitality_skill INTEGER NOT NULL DEFAULT 0 CHECK(vitality_skill>=0);
ALTER TABLE heroes ADD COLUMN IF NOT EXISTS sign_skill INTEGER NOT NULL DEFAULT 0 CHECK(sign_skill>=0);
COMMIT;
