#include "app/RepositoryFactory.h"
#include "repository/MemoryGameRepository.h"
#ifdef BORDERLAND_HAVE_PQXX
#include "repository/PgGameRepository.h"
#endif
#include <cstdlib>
#include <stdexcept>
namespace border {
std::unique_ptr<IGameRepository> openRepository(bool demo) {
    if(demo)return std::make_unique<MemoryGameRepository>();
#ifdef BORDERLAND_HAVE_PQXX
    const char* conn=std::getenv("BORDERLAND_DB");
    if(!conn||!*conn)throw std::runtime_error("Установите BORDERLAND_DB или используйте --demo (без сохранения на диск)");
    return std::make_unique<PgGameRepository>(conn);
#else
    throw std::runtime_error("Приложение собрано без libpqxx. Установите libpqxx-dev и пересоберите; либо используйте --demo (без сохранения на диск)");
#endif
}
} // namespace border
