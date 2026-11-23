#pragma once
#include "repository/IGameRepository.h"
#include <memory>
namespace border {
std::unique_ptr<IGameRepository> openRepository(bool demo);
}
