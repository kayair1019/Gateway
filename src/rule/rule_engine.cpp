#include "gateway/rule/rule_engine.hpp"

#include <cmath>
#include <utility>

namespace gateway::rule {
namespace {

bool matches(double actual, CompareOperator op, double expected) {
    switch (op) {
    case CompareOperator::less_than:
        return actual < expected;
    case CompareOperator::less_equal:
        return actual <= expected;
    case CompareOperator::greater_than:
        return actual > expected;
    case CompareOperator::greater_equal:
        return actual >= expected;
    case CompareOperator::equal:
        return std::fabs(actual - expected) < 0.000001;
    }

    return false;
}

} // namespace

RuleEngine::RuleEngine(std::vector<Rule> rules) : rules_(std::move(rules)) {}

RuleResult RuleEngine::evaluate(const gateway::core::TelemetryMessage& message) const {
    if (rules_.empty()) {
        return {.allowed = true, .matched_rule = ""};
    }

    for (const auto& rule : rules_) {
        for (const auto& point : message.points) {
            if (point.name != rule.point) {
                continue;
            }

            if (matches(point.value, rule.op, rule.value)) {
                return {
                    .allowed = rule.action == RuleAction::allow,
                    .matched_rule = rule.name,
                };
            }
        }
    }

    return {.allowed = false, .matched_rule = ""};
}

} // namespace gateway::rule
