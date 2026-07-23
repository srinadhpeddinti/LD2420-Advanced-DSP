#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "LD2420_AKF_HMM_NoLimits.hpp"

TEST_CASE("AdaptiveKalmanFilter Initialization", "[Kalman]") {
    UltimateDSP::AdaptiveKalmanFilter kf(0.5);
    REQUIRE(kf.x[0] == 0.5);
    REQUIRE(kf.x[1] == 0.0);
    REQUIRE(kf.P[0] == 100.0);
}

TEST_CASE("Kalman Prediction and Update", "[Kalman]") {
    UltimateDSP::AdaptiveKalmanFilter kf(0.1);
    
    // Simulate reading 100cm at dt=0.1
    kf.predict(0.1);
    kf.update(100.0, 0.1);
    
    // Position should be drawn towards 100
    REQUIRE(kf.x[0] > 10.0);
    REQUIRE(kf.x[0] <= 100.0);
}

TEST_CASE("HMM State Initialization", "[HMM]") {
    UltimateDSP::MarkovActivityEngine hmm;
    REQUIRE_THAT(hmm.A[0][0], Catch::Matchers::WithinAbs(0.950, 0.001));
}

TEST_CASE("IntentPredictor Initialization", "[Intent]") {
    UltimateML::IntentPredictor predictor;
    REQUIRE(predictor.intention_to_leave == false);
    REQUIRE_THAT(predictor.door_zone_range, Catch::Matchers::WithinAbs(500.0, 0.001));
}

TEST_CASE("IntentPredictor Transition to Leave", "[Intent]") {
    UltimateML::IntentPredictor predictor;

    // Conditions for leaving: velocity > 50.0 AND range > (door_zone_range - 150.0 = 350.0)
    predictor.update(350.1, 50.1);
    REQUIRE(predictor.intention_to_leave == true);

    // Edge case: just barely missing the condition
    predictor.intention_to_leave = false;
    predictor.update(350.0, 50.1);
    REQUIRE(predictor.intention_to_leave == false);

    predictor.update(350.1, 50.0);
    REQUIRE(predictor.intention_to_leave == false);
}

TEST_CASE("IntentPredictor Transition to Stay", "[Intent]") {
    UltimateML::IntentPredictor predictor;
    predictor.intention_to_leave = true; // Initial state

    // Condition to stay: velocity <= 0 OR range < (door_zone_range - 200.0 = 300.0)

    // Test velocity <= 0
    predictor.update(400.0, 0.0);
    REQUIRE(predictor.intention_to_leave == false);

    // Test range < 300.0
    predictor.intention_to_leave = true;
    predictor.update(299.9, 10.0);
    REQUIRE(predictor.intention_to_leave == false);

    // Edge case: just missing the condition
    predictor.intention_to_leave = true;
    predictor.update(300.0, 0.1);
    REQUIRE(predictor.intention_to_leave == true); // Should maintain state
}

TEST_CASE("IntentPredictor Maintain State", "[Intent]") {
    UltimateML::IntentPredictor predictor;

    // Default state: intention_to_leave = false
    // Conditions where it shouldn't change to true (needs velocity > 50 AND range > 350)
    // AND it shouldn't explicitly set to false (needs velocity <= 0 OR range < 300)

    // Velocity between 0 and 50, Range between 300 and 350
    predictor.update(325.0, 25.0);
    REQUIRE(predictor.intention_to_leave == false);

    predictor.intention_to_leave = true;
    predictor.update(325.0, 25.0);
    REQUIRE(predictor.intention_to_leave == true);
}
