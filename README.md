# DataConverter

Учебный C++17-проект для преобразования данных между CSV, JSON и YAML.

## Локальная сборка

```bash
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Запуск

```bash
./build/dataconv examples/people.csv result.json
```

На Windows исполняемый файл после Release-сборки обычно находится в `build/Release/`.

## CI

Workflow находится в `.github/workflows/ci.yml`.

При `push` и `pull_request` проект автоматически:

- собирается на Ubuntu, macOS и Windows;
- запускает одинаковый набор тестов на всех трёх ОС;
- использует кэш для зависимостей CMake;
- сохраняет XML-отчёт о тестах как artifact.

Для эксперимента с матрицей значение `strategy.fail-fast` можно менять между `false` и `true`.
