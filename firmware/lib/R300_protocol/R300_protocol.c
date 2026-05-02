#include "r300_protocol.h"

uint8_t _checksum(const uint8_t *buf, size_t len)
{
    uint8_t i, sum = 0;
    for (i = 0; i < len; i++)
    {
        sum = (uint8_t)(sum + buf[i]);
    }
    sum = (uint8_t)(~sum + 1);
    return sum;
}

uint8_t *r300_build_set_uart_baudrate(
    uint8_t reader_address,
    const r300_baudrate_t baudrate,
    uint8_t *out_frame)
{
    if (!baudrate || !out_frame)
    {
        return 0;
    }

    out_frame[0] = R300_HEAD;
    out_frame[1] = R300_CMD_SET_UART_BAUDRATE_LEN;
    out_frame[2] = reader_address;
    out_frame[3] = R300_CMD_SET_UART_BAUDRATE;
    out_frame[4] = (uint8_t)baudrate;
    out_frame[5] = _checksum(out_frame, R300_CMD_SET_UART_BAUDRATE_LEN + 1U);

    return out_frame;
}

uint8_t *r300_build_set_output_power(
    uint8_t reader_address,
    const uint8_t output_power,
    uint8_t *out_frame)
{
    if (!output_power || !out_frame)
    {
        return 0;
    }

    out_frame[0] = R300_HEAD;
    out_frame[1] = R300_CMD_SET_OUTPUT_POWER_LEN;
    out_frame[2] = reader_address;
    out_frame[3] = R300_CMD_SET_OUTPUT_POWER;
    out_frame[4] = output_power;
    out_frame[5] = _checksum(out_frame, R300_CMD_SET_OUTPUT_POWER_LEN + 1U);

    return out_frame;
}

uint8_t *r300_build_get_output_power(
    uint8_t reader_address,
    uint8_t *out_frame)
{
    if (!out_frame)
    {
        return 0;
    }

    out_frame[0] = R300_HEAD;
    out_frame[1] = R300_CMD_GET_OUTPUT_POWER_LEN;
    out_frame[2] = reader_address;
    out_frame[3] = R300_CMD_GET_OUTPUT_POWER;
    out_frame[4] = _checksum(out_frame, R300_CMD_GET_OUTPUT_POWER_LEN + 1U);

    return out_frame;
}

uint8_t *r300_build_real_time_inventory(
    uint8_t reader_address,
    const uint8_t channel,
    uint8_t *out_frame)
{
    if (!channel || !out_frame)
    {
        return 0;
    }

    out_frame[0] = R300_HEAD;
    out_frame[1] = R300_CMD_REAL_TIME_INVENTORY_LEN;
    out_frame[2] = reader_address;
    out_frame[3] = R300_CMD_REAL_TIME_INVENTORY;
    out_frame[4] = channel;
    out_frame[5] = _checksum(out_frame, R300_CMD_REAL_TIME_INVENTORY_LEN + 1U);

    return out_frame;
}

bool r300_parse_status_response(
    const uint8_t *frame,
    size_t frame_len,
    r300_status_response_t *out_resp)
{
    if (!frame || !out_resp || frame_len < 6)
    {
        return false;
    }

    if (frame[0] != R300_HEAD)
    {
        return false;
    }

    /* Expected:
     * [0] Head
     * [1] Len = 0x04
     * [2] Address
     * [3] Cmd
     * [4] ErrorCode
     * [5] Check
     */
    if (frame[1] != 0x04)
    {
        return false;
    }

    uint8_t expected = _checksum(frame, frame_len - 1);
    if (expected != frame[frame_len - 1])
    {
        return false;
    }

    out_resp->address = frame[2];
    out_resp->cmd = frame[3];
    out_resp->error_code = frame[4];

    return true;
}