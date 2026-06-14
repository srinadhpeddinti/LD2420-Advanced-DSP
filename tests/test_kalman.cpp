#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "LD2420_AKF_HMM_NoLimits.hpp"

TEST_CASE("AdaptiveKalmanFilter Initialization", "[Kalman]") {
    UltimateDSP::AdaptiveKalmanFilter kf(0.5);
    REQUIRE(kf.x[0] == 0.0);
    REQUIRE(kf.x[1] == 0.0);
    REQUIRE(kf.P[0][0] == 1000.0);
}

TEST_CASE("Kalman Prediction and Update", "[Kalman]") {
    UltimateDSP::AdaptiveKalmanFilter kf(0.1);
    
    // Simulate reading 100cm at dt=0.1
    kf.predict(0.1);
    kf.update(100.0);
    
    // Position should be drawn towards 100
    REQUIRE(kf.x[0] > 10.0);
    REQUIRE(kf.x[0] <= 100.0);
}

TEST_CASE("HMM State Initialization", "[HMM]") {
    UltimateDSP::MarkovActivityEngine hmm;
    REQUIRE_THAT(hmm.A[0][0], Catch::Matchers::WithinAbs(0.950, 0.001));
}
