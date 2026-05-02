#include <Adafruit_TinyUSB.h>
#include <Arduino.h>
#include <bluefruit.h>
#include <cstring>
#include "R300_protocol.h"

// pins
#define MOSFET_GATE_PIN (D8)
#define R300_BAUD 115200

// R300 config
#define R300_ADDRESS 0x01
#define R300_CHANNEL 0x0A
#define PAIR_POWER_DBM 2
#define SCAN_POWER_DBM 25

// timing
#define POWER_SETTLE_MS 100
#define SCAN_TIMEOUT_MS 900

// tag storage
#define EPC_LEN 12
#define MAX_TAGS 20

struct TagRecord
{
    uint8_t epc[EPC_LEN];
    uint8_t rssi;
    bool valid;
};

static TagRecord tags[MAX_TAGS];
static uint8_t tagCount = 0;

// uart rx buffer
#define RX_BUF_SIZE 256
static uint8_t rxBuf[RX_BUF_SIZE];
static size_t rxIdx = 0;

// BLE
BLEService svc("7c6e4014-b6b1-4f7d-9a3d-8f1e2d4c1000");
BLECharacteristic ctrlChar("7c6e4014-b6b1-4f7d-9a3d-8f1e2d4c1001");
BLECharacteristic dataChar("7c6e4014-b6b1-4f7d-9a3d-8f1e2d4c1002");

static const uint8_t CMD_SCAN = 0x01;
static const uint8_t CMD_PAIR = 0x02;

static const uint8_t RESP_SCAN_RESULT = 0x11;
static const uint8_t RESP_PAIR_RESULT = 0x12;
static const uint8_t RESP_ERROR = 0x13;

static const uint8_t ERR_NO_TAG = 0x02;
static const uint8_t ERR_UNKNOWN_CMD = 0xEE;

static const size_t MAX_NOTIFY_SIZE = 180;

// state machine
enum State
{
    STATE_IDLE,
    STATE_POWER_UP,
    STATE_SCANNING,
    STATE_REPORTING,
    STATE_POWER_DOWN
};

static volatile State state = STATE_IDLE;
static volatile uint8_t pendingCmd = 0;

static uint32_t stateTimer = 0;
static uint8_t activePower = 0;
static bool isPairMode = false;

void printHex(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        if (data[i] < 0x10)
        {
            Serial.print('0');
        }

        Serial.print(data[i], HEX);

        if (i + 1 < len)
        {
            Serial.print(' ');
        }
    }
}

const char *stateName(State s)
{
    if (s == STATE_IDLE)
        return "IDLE";
    if (s == STATE_POWER_UP)
        return "POWER_UP";
    if (s == STATE_SCANNING)
        return "SCANNING";
    if (s == STATE_REPORTING)
        return "REPORTING";
    if (s == STATE_POWER_DOWN)
        return "POWER_DOWN";
    return "UNKNOWN";
}

void printStateChange(State oldState, State newState)
{
    if (oldState != newState)
    {
        Serial.print("state: ");
        Serial.print(stateName(oldState));
        Serial.print(" -> ");
        Serial.println(stateName(newState));
    }
}

void mosfetOn()
{
    digitalWrite(MOSFET_GATE_PIN, HIGH);
    Serial.println("mosfet on");
}

void mosfetOff()
{
    digitalWrite(MOSFET_GATE_PIN, LOW);
    Serial.println("mosfet off");
}

void clearTags()
{
    Serial.println("clearing old tags");

    tagCount = 0;

    for (uint8_t i = 0; i < MAX_TAGS; i++)
    {
        tags[i].valid = false;
        tags[i].rssi = 0;
        memset(tags[i].epc, 0, EPC_LEN);
    }
}

bool storeTag(const uint8_t *epc, uint8_t rssi)
{
    if (tagCount >= MAX_TAGS)
    {
        Serial.println("tag buffer full");
        return false;
    }

    for (uint8_t i = 0; i < MAX_TAGS; i++)
    {
        if (!tags[i].valid)
        {
            continue;
        }

        if (memcmp(tags[i].epc, epc, EPC_LEN) == 0)
        {
            Serial.print("duplicate tag: ");
            printHex(epc, EPC_LEN);
            Serial.println();
            return false;
        }
    }

    memcpy(tags[tagCount].epc, epc, EPC_LEN);
    tags[tagCount].rssi = rssi;
    tags[tagCount].valid = true;

    Serial.print("stored tag ");
    Serial.print(tagCount);
    Serial.print(" epc=");
    printHex(epc, EPC_LEN);
    Serial.print(" rssi=");
    Serial.println(rssi);

    tagCount++;

    return true;
}

void flushUart()
{
    int dropped = 0;

    while (Serial1.available())
    {
        Serial1.read();
        dropped++;
    }

    rxIdx = 0;

    Serial.print("uart flushed, dropped ");
    Serial.print(dropped);
    Serial.println(" bytes");
}

bool readFrame(uint8_t *outFrame, size_t *outLen)
{
    while (Serial1.available())
    {
        uint8_t b = (uint8_t)Serial1.read();

        if (rxIdx == 0 && b != R300_HEAD)
        {
            Serial.print("skipping byte: ");
            if (b < 0x10)
                Serial.print('0');
            Serial.println(b, HEX);
            continue;
        }

        if (rxIdx >= RX_BUF_SIZE)
        {
            Serial.println("rx buffer overflow");
            rxIdx = 0;
            return false;
        }

        rxBuf[rxIdx] = b;
        rxIdx++;

        if (rxIdx >= 2)
        {
            size_t totalFrameSize = (size_t)rxBuf[1] + 2;

            if (totalFrameSize > RX_BUF_SIZE)
            {
                Serial.print("bad frame size: ");
                Serial.println(totalFrameSize);
                rxIdx = 0;
                return false;
            }

            if (rxIdx == totalFrameSize)
            {
                Serial.print("r300 frame: ");
                printHex(rxBuf, totalFrameSize);
                Serial.println();

                uint8_t expected = _checksum(rxBuf, totalFrameSize - 1);
                uint8_t actual = rxBuf[totalFrameSize - 1];

                if (expected != actual)
                {
                    Serial.print("bad checksum, expected ");
                    if (expected < 0x10)
                        Serial.print('0');
                    Serial.print(expected, HEX);

                    Serial.print(" got ");
                    if (actual < 0x10)
                        Serial.print('0');
                    Serial.println(actual, HEX);

                    rxIdx = 0;
                    return false;
                }

                Serial.println("checksum ok");

                memcpy(outFrame, rxBuf, totalFrameSize);
                *outLen = totalFrameSize;

                rxIdx = 0;
                return true;
            }
        }
    }

    return false;
}

void r300SetPower(uint8_t power)
{
    uint8_t pkt[R300_CMD_SET_OUTPUT_POWER_LEN + 2U];

    r300_build_set_output_power(R300_ADDRESS, power, pkt);

    Serial.print("setting r300 power to ");
    Serial.print(power);
    Serial.println(" dBm");

    Serial.print("r300 tx power packet: ");
    printHex(pkt, sizeof(pkt));
    Serial.println();

    Serial1.write(pkt, sizeof(pkt));
    Serial1.flush();
}

void r300StartInventory()
{
    flushUart();

    uint8_t pkt[R300_CMD_REAL_TIME_INVENTORY_LEN + 2U];

    r300_build_real_time_inventory(R300_ADDRESS, R300_CHANNEL, pkt);

    Serial.println("starting r300 inventory");

    Serial.print("r300 tx inventory packet: ");
    printHex(pkt, sizeof(pkt));
    Serial.println();

    Serial1.write(pkt, sizeof(pkt));
    Serial1.flush();
}

void extractTag(const uint8_t *frame, size_t frameLen)
{
    Serial.print("trying to parse frame, len=");
    Serial.println(frameLen);

    if (frameLen < 9)
    {
        Serial.println("frame too short, not a tag");
        return;
    }

    size_t parsedEpcLen = frameLen - 9;

    if (parsedEpcLen != EPC_LEN)
    {
        Serial.print("ignoring non-12-byte epc, len=");
        Serial.println(parsedEpcLen);
        return;
    }

    const uint8_t *epc = &frame[7];
    uint8_t rssi = frame[frameLen - 2];

    Serial.print("parsed tag epc=");
    printHex(epc, EPC_LEN);
    Serial.print(" rssi=");
    Serial.println(rssi);

    storeTag(epc, rssi);
}

void sendError(uint8_t code)
{
    if (!Bluefruit.connected())
    {
        Serial.println("not connected, cannot send error");
        return;
    }

    uint8_t pkt[2];
    size_t o = 0;

    pkt[o++] = RESP_ERROR;
    pkt[o++] = code;

    Serial.print("ble error packet: ");
    printHex(pkt, o);
    Serial.println();

    dataChar.notify(pkt, o);
}

void sendPairResult()
{
    if (!Bluefruit.connected())
    {
        Serial.println("not connected, cannot send pair result");
        return;
    }

    if (tagCount == 0)
    {
        Serial.println("no tag found for pair");
        sendError(ERR_NO_TAG);
        return;
    }

    const TagRecord &t = tags[0];

    uint8_t pkt[2 + EPC_LEN + 1];
    size_t o = 0;

    pkt[o++] = RESP_PAIR_RESULT;
    pkt[o++] = 1;

    memcpy(&pkt[o], t.epc, EPC_LEN);
    o += EPC_LEN;

    pkt[o++] = t.rssi;

    Serial.print("sending pair result, tag=");
    printHex(t.epc, EPC_LEN);
    Serial.print(" rssi=");
    Serial.println(t.rssi);

    Serial.print("ble pair packet: ");
    printHex(pkt, o);
    Serial.println();

    dataChar.notify(pkt, o);
}

void sendScanResult()
{
    if (!Bluefruit.connected())
    {
        Serial.println("not connected, cannot send scan result");
        return;
    }

    uint8_t pkt[MAX_NOTIFY_SIZE];
    size_t o = 0;

    pkt[o++] = RESP_SCAN_RESULT;

    size_t countIndex = o;
    pkt[o++] = 0;

    uint8_t count = 0;

    if (tagCount == 0)
    {
        Serial.println("sending scan result, no tags");

        Serial.print("ble scan packet: ");
        printHex(pkt, o);
        Serial.println();

        dataChar.notify(pkt, o);
        return;
    }

    Serial.print("sending scan result, tags=");
    Serial.println(tagCount);

    for (uint8_t i = 0; i < tagCount; i++)
    {
        if (!tags[i].valid)
        {
            continue;
        }

        size_t tagSize = EPC_LEN + 1;

        if (o + tagSize > MAX_NOTIFY_SIZE)
        {
            Serial.println("scan packet full, stopping here");
            break;
        }

        Serial.print("adding tag ");
        Serial.print(i);
        Serial.print(" to scan result: ");
        printHex(tags[i].epc, EPC_LEN);
        Serial.println();

        memcpy(&pkt[o], tags[i].epc, EPC_LEN);
        o += EPC_LEN;

        pkt[o++] = tags[i].rssi;

        count++;
    }

    pkt[countIndex] = count;

    Serial.print("ble scan packet: ");
    printHex(pkt, o);
    Serial.println();

    dataChar.notify(pkt, o);
}

// --- BLE callback ---

void onUserInput(uint16_t conn_hdl, BLECharacteristic *chr,
                 uint8_t *data, uint16_t len)
{
    (void)conn_hdl;
    (void)chr;

    Serial.print("ble command received: ");
    printHex(data, len);
    Serial.println();

    if (len == 0)
    {
        return;
    }

    if (state != STATE_IDLE)
    {
        Serial.println("busy, ignoring command");
        return;
    }

    uint8_t cmd = data[0];

    if (cmd == CMD_SCAN || cmd == CMD_PAIR)
    {
        pendingCmd = cmd;

        if (cmd == CMD_SCAN)
        {
            Serial.println("scan command queued");
        }
        else
        {
            Serial.println("pair command queued");
        }
    }
    else
    {
        Serial.println("unknown command");
        sendError(ERR_UNKNOWN_CMD);
    }
}

void runStateMachine()
{
    State oldState = state;

    switch (state)
    {
    case STATE_IDLE:
        if (pendingCmd != 0)
        {
            if (pendingCmd == CMD_PAIR)
            {
                Serial.println("starting pair mode");

                activePower = PAIR_POWER_DBM;
                isPairMode = true;
            }
            else
            {
                Serial.println("starting scan mode");

                activePower = SCAN_POWER_DBM;
                isPairMode = false;
            }

            Serial.print("power will be ");
            Serial.print(activePower);
            Serial.println(" dBm");

            pendingCmd = 0;

            mosfetOn();
            clearTags();
            stateTimer = millis();
            state = STATE_POWER_UP;
        }
        break;

    case STATE_POWER_UP:
        if (millis() - stateTimer >= POWER_SETTLE_MS)
        {
            Serial.println("power is settled");

            r300SetPower(activePower);
            delay(100);
            r300StartInventory();

            stateTimer = millis();
            state = STATE_SCANNING;
        }
        break;

    case STATE_SCANNING:
    {
        uint8_t frame[RX_BUF_SIZE];
        size_t frameLen = 0;

        if (readFrame(frame, &frameLen))
        {
            if (frameLen > 3 && frame[3] != R300_CMD_REAL_TIME_INVENTORY)
            {
                Serial.print("not inventory frame, cmd=");
                Serial.println(frame[3], HEX);
                break;
            }

            bool isSummary = (frame[1] == 0x08 && frameLen == 10) ||
                             (frame[1] == 0x0A && frameLen == 12);

            bool isError = (frame[1] == 0x04 && frameLen == 6);

            if (isError)
            {
                Serial.println("r300 sent error frame");
            }
            else if (isSummary)
            {
                Serial.println("r300 sent summary frame");
            }
            else
            {
                Serial.println("r300 sent tag frame");
            }

            if (isError || isSummary)
            {
                state = STATE_REPORTING;
            }
            else
            {
                uint8_t oldCount = tagCount;

                extractTag(frame, frameLen);

                if (isPairMode && tagCount > oldCount)
                {
                    Serial.println("pair mode got one tag");
                    state = STATE_REPORTING;
                }
            }
        }

        if (millis() - stateTimer >= SCAN_TIMEOUT_MS)
        {
            Serial.println("scan timeout");
            state = STATE_REPORTING;
        }

        break;
    }

    case STATE_REPORTING:
        Serial.print("reporting, tags found=");
        Serial.println(tagCount);

        if (isPairMode)
        {
            sendPairResult();
        }
        else
        {
            sendScanResult();
        }

        isPairMode = false;
        state = STATE_POWER_DOWN;
        break;

    case STATE_POWER_DOWN:
        mosfetOff();
        state = STATE_IDLE;
        break;
    }

    printStateChange(oldState, state);
}

void setup()
{
    Serial.begin(115200);
    while (!Serial && millis() < 2000)
    {
        delay(10);
    }

    Serial.println();
    Serial.println("firmware started");

    pinMode(MOSFET_GATE_PIN, OUTPUT);
    mosfetOff();

    Serial1.setPins(D9, D10);
    Serial1.begin(R300_BAUD);

    Serial.println("r300 uart started");

    Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
    Bluefruit.begin();
    Bluefruit.setName("Nanotrace Pro");

    svc.begin();

    ctrlChar.setProperties(CHR_PROPS_WRITE | CHR_PROPS_WRITE_WO_RESP);
    ctrlChar.setPermission(SECMODE_OPEN, SECMODE_OPEN);
    ctrlChar.setMaxLen(20);
    ctrlChar.setWriteCallback(onUserInput);
    ctrlChar.begin();

    dataChar.setProperties(CHR_PROPS_NOTIFY | CHR_PROPS_READ);
    dataChar.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
    dataChar.setMaxLen(MAX_NOTIFY_SIZE);
    dataChar.begin();

    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();
    Bluefruit.Advertising.addService(svc);
    Bluefruit.ScanResponse.addName();
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.start(0);

    Serial.println("ble advertising started");
}

void loop()
{
    runStateMachine();
    delay(1);
}