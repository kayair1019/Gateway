#include "gateway/rule/rule_engine.hpp"

#include <gtest/gtest.h>

namespace {

auto make_message(double temperature) -> gateway::core::TelemetryMessage {
    gateway::core::TelemetryMessage message;
    message.device_id = "device-001";
    message.points.push_back({
        .name = "temperature",
        .address = 0,
        .value = temperature,
        .quality = gateway::core::PointQuality::good,
    });
    return message;
}

TEST(RuleEngineTest, AllowsMessageWhenNoRulesConfigured) {
    const gateway::rule::RuleEngine engine;

    const auto result = engine.evaluate(make_message(100.0));

    EXPECT_TRUE(result.allowed);
}

TEST(RuleEngineTest, AllowsMessageWhenThresholdRuleMatches) {
    const gateway::rule::RuleEngine engine({
        {
            .name = "temperature_limit",
            .point = "temperature",
            .op = gateway::rule::CompareOperator::less_than,
            .value = 80.0,
            .action = gateway::rule::RuleAction::allow,
        },
    });

    const auto result = engine.evaluate(make_message(25.0));

    EXPECT_TRUE(result.allowed);
    EXPECT_EQ(result.matched_rule, "temperature_limit");
}

TEST(RuleEngineTest, DropsMessageWhenNoConfiguredRuleMatches) {
    const gateway::rule::RuleEngine engine({
        {
            .name = "temperature_limit",
            .point = "temperature",
            .op = gateway::rule::CompareOperator::less_than,
            .value = 80.0,
            .action = gateway::rule::RuleAction::allow,
        },
    });

    const auto result = engine.evaluate(make_message(100.0));

    EXPECT_FALSE(result.allowed);
}

} // namespace
