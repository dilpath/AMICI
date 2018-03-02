#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <testfunctions.h>
#include <include/amici_hdf5.h>
#include <include/amici_interface_cpp.h>
#include <cstring>
#include "wrapfunctions.h"

TEST_GROUP(groupDirac)
{
    void setup() {

    }

    void teardown() {

    }
};

TEST(groupDirac, testSimulation) {
    amici::simulateAndVerifyFromFile("/model_dirac/nosensi/");
}

TEST(groupDirac, testSimulationExpData) {

}

TEST(groupDirac, testSensitivityForward) {
   amici::simulateAndVerifyFromFile("/model_dirac/sensiforward/");
}

TEST(groupDirac, testSensitivityState) {

}

TEST(groupDirac, testSensitivityAdjoint) {

}

