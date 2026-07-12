#include "gateway/rule/rule_engine.hpp"

#include <gtest/gtest.h>

namespace {

TEST(RuleEngineTest, AllowsMessageWhenNoRulesConfigured) {
    gateway::rule::RuleEngine engine;
    gateway::core::TelemetryMessage msg;
    msg.device_id = "device_1";
    msg.points.push_back({"point_1", 1, 10.0, gateway::core::PointQuality::good});
    
    auto result = engine.evaluate(msg);
    EXPECT_TRUE(result.allowed);
    EXPECT_TRUE(result.matched_rule.empty());
}

TEST(RuleEngineTest, AllowsMessageWhenThresholdRuleMatches) {
    GTEST_SKIP() << "Not implemented yet.";
}

TEST(RuleEngineTest, DropsMessageWhenNoConfiguredRuleMatches) {
    GTEST_SKIP() << "Not implemented yet.";
}

} // namespace
