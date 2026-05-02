#ifndef R300_PROTOCOL_H
#define R300_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* =========================================================
     * Core protocol constants
     * ========================================================= */

#define R300_HEAD 0xA0u
#define R300_PUBLIC_ADDRESS 0xFFu
#define R300_MIN_READER_ADDRESS 0x00u
#define R300_MAX_READER_ADDRESS 0xFEu

#define R300_MAX_FRAME_SIZE 512u
#define R300_MAX_EPC_BYTES 62u
#define R300_ACCESS_PASSWORD_LEN 4u
#define R300_KILL_PASSWORD_LEN 4u

    /* =========================================================
     * Command IDs
     * ========================================================= */

    typedef enum
    {
        /* Reader control commands */
        R300_CMD_RESET = 0x70,
        R300_CMD_SET_UART_BAUDRATE = 0x71,
        R300_CMD_GET_FIRMWARE_VERSION = 0x72,
        R300_CMD_SET_READER_ADDRESS = 0x73,
        R300_CMD_SET_WORK_ANTENNA = 0x74,
        R300_CMD_GET_WORK_ANTENNA = 0x75,
        R300_CMD_SET_OUTPUT_POWER = 0x76,
        R300_CMD_GET_OUTPUT_POWER = 0x77,
        R300_CMD_SET_FREQUENCY_REGION = 0x78,
        R300_CMD_GET_FREQUENCY_REGION = 0x79,
        R300_CMD_SET_BEEPER_MODE = 0x7A,
        R300_CMD_GET_READER_TEMPERATURE = 0x7B,
        R300_CMD_SET_DRM_MODE = 0x7C,
        R300_CMD_GET_DRM_MODE = 0x7D,
        R300_CMD_READ_GPIO_VALUE = 0x60,
        R300_CMD_WRITE_GPIO_VALUE = 0x61,
        R300_CMD_SET_ANT_CONNECTION_DETECTOR = 0x62,
        R300_CMD_GET_ANT_CONNECTION_DETECTOR = 0x63,

        /* EPC C1G2 commands */
        R300_CMD_INVENTORY = 0x80,
        R300_CMD_READ = 0x81,
        R300_CMD_WRITE = 0x82,
        R300_CMD_LOCK = 0x83,
        R300_CMD_KILL = 0x84,
        R300_CMD_SET_ACCESS_EPC_MATCH = 0x85,
        R300_CMD_GET_ACCESS_EPC_MATCH = 0x86,
        R300_CMD_REAL_TIME_INVENTORY = 0x89,
        R300_CMD_FAST_SWITCH_ANT_INVENTORY = 0x8A,

        /* ISO18000-6B commands listed in command table */
        R300_CMD_ISO18000_6B_INVENTORY = 0xB0,
        R300_CMD_ISO18000_6B_READ = 0xB1,
        R300_CMD_ISO18000_6B_WRITE = 0xB2,
        R300_CMD_ISO18000_6B_LOCK = 0xB3,
        R300_CMD_ISO18000_6B_QUERY_LOCK = 0xB4,

        /* Buffer operations */
        R300_CMD_GET_INVENTORY_BUFFER = 0x90,
        R300_CMD_GET_AND_RESET_INVENTORY_BUFFER = 0x91,
        R300_CMD_GET_INVENTORY_BUFFER_TAG_COUNT = 0x92,
        R300_CMD_RESET_INVENTORY_BUFFER = 0x93,
        R300_CMD_SET_BUFFER_DATA_FRAME_INTERVAL = 0x94,
        R300_CMD_GET_BUFFER_DATA_FRAME_INTERVAL = 0x95
    } r300_cmd_t;

    /* =========================================================
     * Command Lengths
     * ========================================================= */

    typedef enum
    {
        R300_CMD_RESET_LEN = 0x03,
        R300_CMD_SET_UART_BAUDRATE_LEN = 0x04,
        R300_CMD_SET_READER_ADDRESS_LEN = 0x04,
        R300_CMD_SET_WORK_ANTENNA_LEN = 0x04,
        R300_CMD_GET_WORK_ANTENNA_LEN = 0x03,
        R300_CMD_SET_OUTPUT_POWER_LEN = 0x04,
        R300_CMD_GET_OUTPUT_POWER_LEN = 0x03,
        R300_CMD_SET_BEEPER_MODE_LEN = 0x04,
        R300_CMD_GET_READER_TEMPERATURE_LEN = 0x03,
        R300_CMD_SET_DRM_MODE_LEN = 0x04,
        R300_CMD_GET_DRM_MODE_LEN = 0x03,
        R300_CMD_INVENTORY_LEN = 0x04,
        R300_CMD_READ_LEN = 0x06,
        R300_CMD_REAL_TIME_INVENTORY_LEN = 0x04

    } r300_cmd_len_t;

    /* =========================================================
     * Error codes
     * ========================================================= */

    typedef enum
    {
        R300_ERR_COMMAND_SUCCESS = 0x10,
        R300_ERR_COMMAND_FAIL = 0x11,
        R300_ERR_MCU_RESET_ERROR = 0x20,
        R300_ERR_CW_ON_ERROR = 0x21,
        R300_ERR_ANTENNA_MISSING = 0x22,
        R300_ERR_WRITE_FLASH_ERROR = 0x23,
        R300_ERR_READ_FLASH_ERROR = 0x24,
        R300_ERR_SET_OUTPUT_POWER_ERROR = 0x25,
        R300_ERR_TAG_INVENTORY_ERROR = 0x31,
        R300_ERR_TAG_READ_ERROR = 0x32,
        R300_ERR_TAG_WRITE_ERROR = 0x33,
        R300_ERR_TAG_LOCK_ERROR = 0x34,
        R300_ERR_TAG_KILL_ERROR = 0x35,
        R300_ERR_NO_TAG_ERROR = 0x36,
        R300_ERR_INVENTORY_OK_BUT_ACCESS_FAIL = 0x37,
        R300_ERR_BUFFER_IS_EMPTY = 0x38,
        R300_ERR_ACCESS_OR_PASSWORD_ERROR = 0x40,
        R300_ERR_PARAMETER_INVALID = 0x41,
        R300_ERR_PARAMETER_INVALID_WORDCNT_TOO_LONG = 0x42,
        R300_ERR_PARAMETER_INVALID_MEMBANK_OUT_OF_RANGE = 0x43,
        R300_ERR_PARAMETER_INVALID_LOCK_REGION_OUT_OF_RANGE = 0x44,
        R300_ERR_PARAMETER_INVALID_LOCK_ACTION_OUT_OF_RANGE = 0x45,
        R300_ERR_PARAMETER_READER_ADDRESS_INVALID = 0x46,
        R300_ERR_PARAMETER_INVALID_ANTENNA_ID_OUT_OF_RANGE = 0x47,
        R300_ERR_PARAMETER_INVALID_OUTPUT_POWER_OUT_OF_RANGE = 0x48,
        R300_ERR_PARAMETER_INVALID_FREQUENCY_REGION_OUT_OF_RANGE = 0x49,
        R300_ERR_PARAMETER_INVALID_BAUDRATE_OUT_OF_RANGE = 0x4A,
        R300_ERR_PARAMETER_BEEPER_MODE_OUT_OF_RANGE = 0x4B,
        R300_ERR_PARAMETER_EPC_MATCH_LEN_TOO_LONG = 0x4C,
        R300_ERR_PARAMETER_EPC_MATCH_LEN_ERROR = 0x4D,
        R300_ERR_PARAMETER_INVALID_EPC_MATCH_MODE = 0x4E,
        R300_ERR_PARAMETER_INVALID_FREQUENCY_RANGE = 0x4F,
        R300_ERR_FAIL_TO_GET_RN16_FROM_TAG = 0x50,
        R300_ERR_PARAMETER_INVALID_DRM_MODE = 0x51,
        R300_ERR_PLL_LOCK_FAIL = 0x52,
        R300_ERR_RF_CHIP_FAIL_TO_RESPONSE = 0x53,
        R300_ERR_FAIL_TO_ACHIEVE_DESIRED_OUTPUT_POWER = 0x54,
        R300_ERR_COPYRIGHT_AUTHENTICATION_FAIL = 0x55,
        R300_ERR_SPECTRUM_REGULATION_ERROR = 0x56,
        R300_ERR_OUTPUT_POWER_TOO_LOW = 0x57
    } r300_error_t;

    /* =========================================================
     * Common enums / parameter definitions
     * ========================================================= */

    typedef enum
    {
        R300_BAUD_9600 = 0x01,
        R300_BAUD_19200 = 0x02,
        R300_BAUD_38400 = 0x03,
        R300_BAUD_115200 = 0x04
    } r300_baudrate_t;

    typedef enum
    {
        R300_ANTENNA_1 = 0x00,
        R300_ANTENNA_2 = 0x01,
        R300_ANTENNA_3 = 0x02,
        R300_ANTENNA_4 = 0x03
    } r300_antenna_id_t;

    typedef enum
    {
        R300_REGION_FCC = 0x01,
        R300_REGION_ETSI = 0x02,
        R300_REGION_CHN = 0x03
    } r300_region_t;

    typedef enum
    {
        R300_BEEPER_QUIET = 0x00,
        R300_BEEPER_AFTER_INVENTORY_ROUND_IF_TAG = 0x01,
        R300_BEEPER_AFTER_EVERY_TAG = 0x02
    } r300_beeper_mode_t;

    typedef enum
    {
        R300_DRM_OFF = 0x00,
        R300_DRM_ON = 0x01
    } r300_drm_mode_t;

    typedef enum
    {
        R300_GPIO_LOW = 0x00,
        R300_GPIO_HIGH = 0x01
    } r300_gpio_level_t;

    typedef enum
    {
        R300_GPIO3 = 0x03,
        R300_GPIO4 = 0x04
    } r300_writable_gpio_t;

    typedef enum
    {
        R300_ANT_DETECTOR_OFF = 0x00,
        R300_ANT_DETECTOR_ON = 0x01
    } r300_ant_detector_t;

    typedef enum
    {
        R300_MEMBANK_RESERVED = 0x00,
        R300_MEMBANK_EPC = 0x01,
        R300_MEMBANK_TID = 0x02,
        R300_MEMBANK_USER = 0x03
    } r300_memory_bank_t;

    typedef enum
    {
        R300_LOCK_REGION_USER_MEMORY = 0x01,
        R300_LOCK_REGION_TID_MEMORY = 0x02,
        R300_LOCK_REGION_EPC_MEMORY = 0x03,
        R300_LOCK_REGION_ACCESS_PASSWORD = 0x04,
        R300_LOCK_REGION_KILL_PASSWORD = 0x05
    } r300_lock_region_t;

    typedef enum
    {
        R300_LOCK_OPEN = 0x00,
        R300_LOCK_LOCK = 0x01,
        R300_LOCK_PERMA_OPEN = 0x02,
        R300_LOCK_PERMA_LOCK = 0x03
    } r300_lock_action_t;

    typedef enum
    {
        R300_EPC_MATCH_ENABLE = 0x00,
        R300_EPC_MATCH_CLEAR = 0x01
    } r300_epc_match_mode_t;

    typedef enum
    {
        R300_PLUS = 0x00,
        R300_MINUS = 0x01
    } r300_sign_t;

    /* =========================================================
     * Generic frame / common responses
     * ========================================================= */

    typedef struct
    {
        uint8_t head;
        uint8_t len;
        uint8_t address;
        uint8_t payload[R300_MAX_FRAME_SIZE - 4u];
        uint8_t check;
    } r300_frame_t;

    /* Common status response:
     * Head Len Address Cmd ErrorCode Check
     */
    typedef struct
    {
        uint8_t address;
        uint8_t cmd;
        uint8_t error_code;
    } r300_status_response_t;

    uint8_t _checksum(const uint8_t *buf, size_t len);
    uint8_t *r300_build_set_uart_baudrate(uint8_t reader_address,
                                          const r300_baudrate_t baudrate,
                                          uint8_t *out_frame);
    uint8_t *r300_build_set_output_power(uint8_t reader_address,
                                         const uint8_t output_power,
                                         uint8_t *out_frame);
    uint8_t *r300_build_get_output_power(uint8_t reader_address,
                                         uint8_t *out_frame);
    uint8_t *r300_build_real_time_inventory(uint8_t reader_address,
                                            const uint8_t channel,
                                            uint8_t *out_frame);

#ifdef __cplusplus
}
#endif

#endif /* R300_PROTOCOL_H */