#include "random.h"

KQRandom::KQRandom()
{
    time_t t0 = time(nullptr);
    engine.seed(static_cast<std::minstd_rand::result_type>(t0));
}

int KQRandom::random_range_exclusive(int min_inclusive, int max_exclusive)
{
    std::uniform_int_distribution<int> rnd(min_inclusive, max_exclusive - 1);
    return rnd(engine);
}

int KQRandom::random_range_inclusive(int min_inclusive, int max_inclusive)
{
    std::uniform_int_distribution<int> rnd(min_inclusive, max_inclusive);
    return rnd(engine);
}

std::string KQRandom::kq_get_random_state()
{
    std::ostringstream stm;
    stm << engine;
    return stm.str();
}

void KQRandom::kq_set_random_state(const std::string &new_state)
{
    std::istringstream stm(new_state);
    stm >> engine;
}

// Name a specific engine here rather than default
// just in case different platforms have different
// implementations of default.
static std::minstd_rand engine;

KQRandom* kqrandom = nullptr;
