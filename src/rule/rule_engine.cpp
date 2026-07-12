#include "gateway/rule/rule_engine.hpp"

#include <cmath>
#include <utility>

namespace gateway::rule {
namespace {

auto matches(double actual, CompareOperator op, double expected) -> bool {
    return {};
}

} // namespace

RuleEngine::RuleEngine(std::vector<Rule> rules) : rules_(std::move(rules)) {
  // 如果 rules_ 为空，直接放行
    if (rules_.empty()) {
        return RuleResult{.allowed = true, .matched_rule = ""};
    }
    
    // TODO: 后面再实现遍历规则的逻辑
    return RuleResult{};
}

auto RuleEngine::evaluate(const gateway::core::TelemetryMessage& message) const -> RuleResult {
    return {};
}

} // namespace gateway::rule
