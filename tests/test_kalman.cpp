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

TEST_CASE("SleepStager Initialization and Stages", "[SleepStager]") {
    UltimateML::SleepStager stager;

    // Initial stage is Awake (0)
    REQUIRE(stager.current_stage == 0);
    REQUIRE(stager.brv_moving_avg == 0.0);

    SECTION("Invalid reading sets stage to Awake") {
        stager.current_stage = 2; // Artificially set to Deep
        stager.update(15.0, false);
        REQUIRE(stager.current_stage == 0);
    }

    SECTION("Steady breathing (15 bpm) leads to Deep Sleep") {
        // brv_moving_avg = 0.9 * avg + 0.1 * |breath_rate - 15|
        // If breath_rate = 15, avg stays 0.
        // Stage should be 2 (Deep) since avg (0.0) <= 1.5
        for (int i = 0; i < 10; ++i) {
            stager.update(15.0, true);
        }
        REQUIRE(stager.current_stage == 2);
        REQUIRE_THAT(stager.brv_moving_avg, Catch::Matchers::WithinAbs(0.0, 0.001));
    }

    SECTION("Slightly erratic breathing (e.g. 17 bpm) leads to Light Sleep") {
        // breath_rate = 17, diff = 2.0.
        // avg will approach 2.0 over time.
        // Once avg > 1.5 and avg <= 3.0, stage should be 1 (Light)
        for (int i = 0; i < 50; ++i) {
            stager.update(17.0, true);
        }
        REQUIRE(stager.current_stage == 1);
        REQUIRE(stager.brv_moving_avg > 1.5);
        REQUIRE(stager.brv_moving_avg <= 3.0);
    }

    SECTION("Highly erratic breathing (e.g. 20 bpm) leads to REM Sleep") {
        // breath_rate = 20, diff = 5.0.
        // avg will approach 5.0 over time.
        // Once avg > 3.0, stage should be 3 (REM)
        for (int i = 0; i < 50; ++i) {
            stager.update(20.0, true);
        }
        REQUIRE(stager.current_stage == 3);
        REQUIRE(stager.brv_moving_avg > 3.0);
    }
}
