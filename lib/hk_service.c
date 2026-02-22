
#include "hk_service.h"
#include "commands.h"
#include "tm_manager.h"
#include "fdir_service.h"
#include <stdio.h>
#include <string.h>

uint32_t telemetry_interval_ms = 10000; // Default 10 seconds

// This is a private "Database" - not visible outside this file
static hk_entry_t s_telemetry_table[HK_MAX_PARAMS];

static hk_limits_t s_limits_table[HK_MAX_PARAMS];


static uint32_t report_interval_ms = 5000; // Default: 5 seconds
static uint32_t last_report_time = 0;


void HK_Init(void){
    // Clear the table: all values to 0, all timestamps to 0, all is_valid to 0
    memset(s_telemetry_table, 0, sizeof(s_telemetry_table));
}

int HK_UpdateParam(hk_param_id_t id, uint32_t value, uint32_t current_time){
    // Safety check: Don't write the array bounds
    if(id >= HK_MAX_PARAMS){
        return -1;
    }

    s_telemetry_table[id].value = value;
    s_telemetry_table[id].timestamp = current_time;
    s_telemetry_table[id].is_valid = 1; // Mark as fresh data
    return 0; // Success
}

int HK_ReadParam(hk_param_id_t id, hk_entry_t *out_entry){
    // Safety check and null pointer protection
    if(id >= HK_MAX_PARAMS || out_entry == NULL){
        return -1;
    }

    // Copy the internal entry to the output pointer
    *out_entry = s_telemetry_table[id];

    return 0;
}

void HK_SetLimits(hk_param_id_t id, uint32_t low, uint32_t high){
    if(id < HK_MAX_PARAMS){
        s_limits_table[id].low_limit = low;
        s_limits_table[id].high_limit = high;
    }

}

int HK_CheckLimits(hk_param_id_t id) {
    // 1. Safety/Freshness Check
    if (id >= HK_MAX_PARAMS) {
        return -2; // Invalid ID error
    }
    
    if (s_telemetry_table[id].is_valid == 0) {
        return -1; // Data is stale or hasn't been sampled yet
    }

    uint32_t val = s_telemetry_table[id].value;
    
    // 2. Limit Comparison
    if (val < s_limits_table[id].low_limit || val > s_limits_table[id].high_limit) {
        return 1; // ALARM: Out of bounds!
    }

    return 0; // NOMINAL: Everything is within range
}

int HK_Serialize(uint8_t *buffer, uint16_t buffer_size){
    if(buffer_size < HK_PACKET_SIZE) return -1;

    // Did not pack time for simplicity

    uint16_t offset = 0;
    for(int i = 0; i < HK_MAX_PARAMS; i++){
        // Pack 32-bit value (Big Endian)
        buffer[offset++] = (s_telemetry_table[i].value >> 24) & 0xFF;
        buffer[offset++] = (s_telemetry_table[i].value >> 16) & 0xFF;
        buffer[offset++] = (s_telemetry_table[i].value >> 8) &  0xFF;
        buffer[offset++] = (s_telemetry_table[i].value) & 0xFF;

        // Pack 8-bit validity status
        buffer[offset++] = s_telemetry_table[i].is_valid;
    }
    return offset; // Return total bytes packed
}


void HK_GenerateReport(void) {
    // Collect data from other systems
    uint8_t hk_data[4];
    hk_data[0] = (uint8_t)FDIR_GetCurrentMode(); // Mode from FDIR
    hk_data[1] = 0xFF; // Mock Battery Level
    hk_data[2] = 0x00; // Mock Temp
    hk_data[3] = 0x01; // Mock Status

    // Send it to Earth via the TM Manager
    TM_SendReport(0x030, hk_data, 4);
}

void HK_ProcessCommand(const uint8_t* payload, uint16_t len) {
    if (len < 1) return;

    uint8_t command_id = payload[0];

    switch (command_id) {
        case HK_CMD_SET_INTERVAL:
            if (len >= 2) {
                // payload[1] is seconds from Earth
                report_interval_ms = payload[1] * 1000;
                printf("HK: Interval updated to %d ms\n", report_interval_ms);
            }
            break;

        case HK_CMD_FORCE_REPORT:
            printf("HK: Manual report requested.\n");
            HK_GenerateReport();
            break;

        default:
            printf("HK Error: Unknown Command 0x%02X\n", command_id);
            break;
    }
}



void HK_RunPeriodic(uint32_t current_time_ms) {
    if (current_time_ms - last_report_time >= report_interval_ms) {
        HK_GenerateReport();
        last_report_time = current_time_ms;
    }
}