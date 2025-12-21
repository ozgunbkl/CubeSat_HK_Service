#include "unity.h"
#include "hk_service.h"

void setUp(void){
    HK_Init(); // Reset the "vault" before every single test
}

void tearDown(void){}

void test_HK_InitialStateIsInvalid(void){
    hk_entry_t entry;
    // Read the battery voltage (ID 0) before any update
    int resut = HK_ReadParam(HK_ID_BATT_VOLTAGE, &entry);

    TEST_ASSERT_EQUAL_INT(0, resut);
    TEST_ASSERT_EQUAL_UINT8(0, entry.is_valid); // Should be 0 (invalid)
}

void test_HK_UpdateAndReadSuccess(void){
    hk_entry_t entry;
    uint32_t test_val = 3700;
    uint32_t test_time = 500;

    // 1. Update the value
    int update_res = HK_UpdateParam(HK_ID_BATT_VOLTAGE, test_val, test_time);
    TEST_ASSERT_EQUAL_INT(0, update_res);

    // 2. Read it back
    int read_res = HK_ReadParam(HK_ID_BATT_VOLTAGE, &entry);
    TEST_ASSERT_EQUAL_INT(0, read_res);

    //3. Verify the "Photocopy" matches the data we sent
    TEST_ASSERT_EQUAL_UINT32(test_val, entry.value);
    TEST_ASSERT_EQUAL_UINT32(test_time, entry.timestamp);
    TEST_ASSERT_EQUAL_UINT8(1, entry.is_valid); // Should be valid now
}

void test_HK_RejectsInvalidIDs(void){
    hk_entry_t entry;
    // Try to update an ID that does not exist (HK_MAX_PARAMS is the limit)
    int result = HK_UpdateParam(HK_MAX_PARAMS + 1, 999, 10);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_HK_CheckLimitsReturnsErrorForStaleData(void) {
    // I haven't updated HK_ID_ALTITUDE yet
    int status = HK_CheckLimits(HK_ID_ALTITUDE);
    
    // It should NOT return 0 (Nominal)
    TEST_ASSERT_EQUAL_INT(-1, status); 
}

void test_HK_DifferenceBetweenNominalAndStale(void) {
    // 1. Set limits for Battery (3000mV to 4200mV)
    HK_SetLimits(HK_ID_BATT_VOLTAGE, 3000, 4200);

    // 2. Scenario A: The sensor is disconnected (Stale/Invalid)
    // Even though the default value is 0 (which is out of limits),
    // it should return -1 because it's NOT VALID.
    TEST_ASSERT_EQUAL_INT(-1, HK_CheckLimits(HK_ID_BATT_VOLTAGE));

    // 3. Scenario B: The sensor is connected and 3700mV (Nominal)
    HK_UpdateParam(HK_ID_BATT_VOLTAGE, 3700, 1000);
    TEST_ASSERT_EQUAL_INT(0, HK_CheckLimits(HK_ID_BATT_VOLTAGE));

    // 4. Scenario C: The sensor is connected but 2500mV (Alarm)
    HK_UpdateParam(HK_ID_BATT_VOLTAGE, 2500, 1100);
    TEST_ASSERT_EQUAL_INT(1, HK_CheckLimits(HK_ID_BATT_VOLTAGE));
}

void test_HK_SerializationPacksMultipleParams(void) {
    uint8_t buffer[HK_PACKET_SIZE];
    
    // 1. Set some specific values
    // 3700 = 0x00000E74
    HK_UpdateParam(HK_ID_BATT_VOLTAGE, 3700, 100); 
    // 25 = 0x00000019
    HK_UpdateParam(HK_ID_BATT_TEMP, 25, 100);     

    // 2. Serialize
    int bytes_packed = HK_Serialize(buffer, sizeof(buffer));
    
    // 3. Verify total size (5 bytes per param * HK_MAX_PARAMS)
    TEST_ASSERT_EQUAL_INT(HK_PACKET_SIZE, bytes_packed);

    // 4. Verify Parameter 0 (Voltage)
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer[1]);
    TEST_ASSERT_EQUAL_HEX8(0x0E, buffer[2]);
    TEST_ASSERT_EQUAL_HEX8(0x74, buffer[3]);
    TEST_ASSERT_EQUAL_HEX8(0x01, buffer[4]); // Valid flag

    // 5. Verify Parameter 1 (Temperature) - Starts at buffer[5]
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer[5]);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer[6]);
    TEST_ASSERT_EQUAL_HEX8(0x00, buffer[7]);
    TEST_ASSERT_EQUAL_HEX8(0x19, buffer[8]);
    TEST_ASSERT_EQUAL_HEX8(0x01, buffer[9]); // Valid flag
}

int main (void){
    UNITY_BEGIN();

    RUN_TEST(test_HK_InitialStateIsInvalid);
    RUN_TEST(test_HK_UpdateAndReadSuccess);
    RUN_TEST(test_HK_RejectsInvalidIDs);
    RUN_TEST(test_HK_CheckLimitsReturnsErrorForStaleData);
    RUN_TEST(test_HK_DifferenceBetweenNominalAndStale);
    RUN_TEST(test_HK_SerializationPacksMultipleParams);

    return UNITY_END();
}
