#pragma once

#include <vector>

#include "gateway/core/message.hpp"
#include "gateway/rule/rule.hpp"

namespace gateway::rule {

class RuleEngine {
public:
    RuleEngine() = default;
    explicit RuleEngine(std::vector<Rule> rules);

    RuleResult evaluate(const gateway::core::TelemetryMessage& message) const;

private:
    std::vector<Rule> rules_;
};

} // namespace gateway::rule
