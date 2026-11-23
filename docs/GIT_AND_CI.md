# Git и GitHub Actions для «Контрактов Пограничья» v1.5

## Что делает CI

Файл `.github/workflows/ci.yml` запускается на `push`, `pull_request` и вручную через вкладку Actions.

1. **01 | Validate sources** — проверяет структуру проекта и синтаксис Bash-скрипта.
2. **02 | Core build & 50 tests** — собирает C++17 без Qt, запускает 50 независимых тестов CTest и выгружает JUnit XML.
3. **03a | Qt 6 GUI + smoke** — собирает графическую версию, открывает её в `offscreen`, меняет размер окна, выгружает отчет и Linux-бинарники.
4. **03b | PostgreSQL integration** — в отдельном контейнере PostgreSQL 16 создаёт таблицы, запускает CLI, создаёт героя, принимает контракт и проверяет сохранение после повторной загрузки.
5. **04 | Package sources** — запускается только после двух успешных проверок Qt и PostgreSQL, публикует архив исходников.

В графе CI после второго этапа две ветви выполняются параллельно: проверка Qt и интеграция PostgreSQL. Сборка и тесты выполняются на Ubuntu 24.04, поэтому workflow рассчитан на Linux, как WSL. PostgreSQL в CI временный; локальная база пользователя не затрагивается.

> Установка и настройка PostgreSQL на вашем компьютере не требуются для просмотра CI, и пароли в workflow относятся **только к временной тестовой БД**. Для локальных сохранений по-прежнему нужна переменная `BORDERLAND_DB`.

## Git: ключевые смысловые коммиты

Если Git уже ведётся, **не переписывайте сделанные коммиты**. Дальнейшие сообщения для реальных изменений могут быть такими:

- `chore: initialize C++17 project with CMake` — структура, CMake, `.gitignore`;
- `feat(core): implement RPG characters contracts and combat` — предметная модель и `GameService`;
- `feat(storage): add PostgreSQL persistence and demo mode` — PostgreSQL и Memory-репозиторий;
- `feat(ui): add responsive Qt6 interface and quest map` — Qt, карта и CLI;
- `test: cover combat quests skills and map statuses` — 50 тестов;
- `ci: add staged builds Qt smoke and PostgreSQL checks` — этот workflow и интеграционный Bash-скрипт.

Если проект **уже закоммичен**, текущие реальные изменения можно зафиксировать отдельно:

```bash
git status
git add .github/workflows/ci.yml .github/scripts/ci_postgres_smoke.sh docs/GIT_AND_CI.md README.md
git commit -m "ci: add staged builds Qt smoke and PostgreSQL checks"
git push
```

Если проект ещё не находится под Git, честнее всего сначала сохранить текущее работоспособное состояние одним коммитом `Initial working RPG on C++17 Qt6 and PostgreSQL`, а последующие функции фиксировать отдельными коммитами по мере разработки. Не нужно создавать фиктивную историю, будто предыдущие версии написаны в разные дни.

## Как проверить локально

```bash
cmake -S . -B build-qt -DCMAKE_BUILD_TYPE=Release
cmake --build build-qt -j$(nproc)
ctest --test-dir build-qt -j 4 --output-on-failure
QT_QPA_PLATFORM=offscreen ./build-qt/borderland_gui --demo --smoke-test
```

Для локального интеграционного теста PostgreSQL нужен **отдельный пустой тестовый экземпляр/БД** с применённым `sql/init.sql` и установленной `BORDERLAND_DB`. Не запускайте скрипт интеграции против вашей игровой базы: он создаёт тестового героя и изменяет его прогресс.

## Где смотреть результаты на GitHub

В репозитории откройте **Actions → Borderland Contracts | Multi-stage CI → последний запуск**. Зелёные стадии — успешные. Вкладка Artifacts содержит отчёты тестов, Linux-бинарники (не standalone-пакет: для запуска нужны зависимости Qt) и архив исходников.
