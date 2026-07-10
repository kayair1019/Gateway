#pragma once

#include <string>

namespace gateway::rule {

enum class CompareOperator {
    less_than,
    less_equal,
    greater_than,
    greater_equal,
    equal
};

enum class RuleAction {
    allow,
    drop
};

struct Rule {
    std::string name;
    std::string point;
    CompareOperator op{CompareOperator::less_than};
    double value{0.0};
    RuleAction action{RuleAction::allow};
};

struct RuleResult {
    bool allowed{true};
    std::string matched_rule;
};

} // namespace gateway::rule
