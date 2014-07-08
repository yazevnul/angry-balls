#pragma once

#include "ab/strategy_interface.h"

#include <random>
#include <string>
#include <vector>

#include <cstddef>

namespace ab {

enum class StrategyName
{
    DoNothing,
    MoveToClosest,
    Predictive,
    Drunk,
    Buffalo,
    RandomAcceleration,
};

std::string ToString(const StrategyName name);
bool TryFromString(const std::string& str, StrategyName& name);
StrategyName FromString(const std::string& str);

static const std::vector<StrategyName> StrategyNames = { StrategyName::DoNothing,
                                                         StrategyName::MoveToClosest,
                                                         StrategyName::Predictive,
                                                         StrategyName::Drunk,
                                                         StrategyName::Buffalo,
                                                         StrategyName::RandomAcceleration
                                                       };

static const std::vector<std::string> StrategyNamesStr =
    { ToString(StrategyName::DoNothing),
      ToString(StrategyName::MoveToClosest),
      ToString(StrategyName::Predictive),
      ToString(StrategyName::Drunk),
      ToString(StrategyName::Buffalo),
      ToString(StrategyName::RandomAcceleration)
    };

class DoNothingStrategy : public StrategyInterface
{
public:
    Acceleration GetTurn(const FieldState& state, const PlayerId player_id) override;

    virtual ~DoNothingStrategy() {}
};

class MoveToClosestStrategy : public StrategyInterface
{
public:
    Acceleration GetTurn(const FieldState& state, const PlayerId player_id) override;

    virtual ~MoveToClosestStrategy() {}
};

class PredictiveStrategy : public StrategyInterface
{
public:
    Acceleration GetTurn(const FieldState& state, const PlayerId player_id) override;

    virtual ~PredictiveStrategy() {}
};

class DrunkStrategy : public StrategyInterface
{
public:
    DrunkStrategy();

    Acceleration GetTurn(const FieldState& state, const PlayerId player_id) override;

    virtual ~DrunkStrategy() {}

private:
    size_t turn_index_;
};

class BuffaloStrategy : public StrategyInterface
{
public:
    BuffaloStrategy();

    Acceleration GetTurn(const FieldState& state, const PlayerId player_id) override;

    virtual ~BuffaloStrategy() {}

private:
    std::mt19937 generator_;
};

class RandomAccelerationStrategy : public StrategyInterface
{
public:
    RandomAccelerationStrategy();

    Acceleration GetTurn(const FieldState& state, const PlayerId player_id) override;

    virtual ~RandomAccelerationStrategy() {}

private:
    static constexpr double kTimeToChangeAcceleration = 2000.0;

    double time_until_change_of_acceleration_;
    ab::Acceleration previous_acceleration_;
    std::mt19937 generator_;
};

} // namespace ab
