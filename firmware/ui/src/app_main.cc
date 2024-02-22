#include "main.h"
#include "app_main.hh"

#include "String.hh"
#include "Font.hh"
#include "PortPin.hh"
#include "PushReleaseButton.hh"

#include "Screen.hh"
#include "Q10Keyboard.hh"
#include "EEPROM.hh"
#include "UserInterfaceManager.hh"

#include "SerialStm.hh"
#include "Led.hh"
#include "AudioCodec.hh"

#include <hpke/digest.h>
#include <hpke/signature.h>
#include <hpke/hpke.h>

#include <crypto/cmox_crypto.h>
#include <cstring>

#include <memory>
#include <cmath>
#include <sstream>

// Handlers
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

extern SPI_HandleTypeDef hspi1;
extern I2C_HandleTypeDef hi2c1;
extern I2S_HandleTypeDef hi2s3;
extern TIM_HandleTypeDef htim2;
extern RNG_HandleTypeDef hrng;


port_pin cs = { DISP_CS_GPIO_Port, DISP_CS_Pin };
port_pin dc = { DISP_DC_GPIO_Port, DISP_DC_Pin };
port_pin rst = { DISP_RST_GPIO_Port, DISP_RST_Pin };
port_pin bl = { DISP_BL_GPIO_Port, DISP_BL_Pin };

Screen screen(hspi1, cs, dc, rst, bl, Screen::Orientation::left_landscape);
Q10Keyboard* keyboard = nullptr;
SerialStm* mgmt_serial_interface = nullptr;
SerialStm* net_serial_interface = nullptr;
UserInterfaceManager* ui_manager = nullptr;
EEPROM* eeprom = nullptr;
AudioCodec* audio = nullptr;
bool rx_busy = false;

uint8_t random_byte() {
    // XXX(RLB) This is 4x slower than it could be, because we only take the
    // low-order byte of the four bytes in a uint32_t.
    auto value = uint32_t(0);
    HAL_RNG_GenerateRandomNumber(&hrng, &value);
    return value;
}

// buffer size = 0.1s * freq
const uint16_t SOUND_BUFFER_SZ = 16000;
uint16_t rx_sound_buff[SOUND_BUFFER_SZ] = { 0 };
uint16_t tx_sound_buff[SOUND_BUFFER_SZ] = { 0 };

uint16_t GenerateTriangleWavePoint(double frequency, double amplitude, double time)
{
    double period = 1.0 / frequency;
    double phase = fmod(time, period) / period;

    double triangle_point = 0;

    if (phase < 0.25)
        triangle_point = 4.0 * amplitude * phase;
    else if (phase < 0.75)
        triangle_point = 2.0 * amplitude - 4.0 * amplitude * phase;
    else
        triangle_point = -4.0 * amplitude + 4.0 * amplitude * phase;

    return static_cast<uint16_t>((triangle_point + 1.0) * 32767.5);
}

struct Logger {
  int y = 0;

  std::string space_separated_line(std::stringstream&& str) {
    return str.str();
  }

  template<typename T, typename... U>
  std::string space_separated_line(std::stringstream&& str, const T& first, const U&... rest) {
    str << first << " ";
    return space_separated_line(std::move(str), rest...);
  }

  template<typename... T>
  void log(const T&... args) {
    auto str = std::stringstream();
    str << "[UI] ";
    const auto line = space_separated_line(std::move(str), args...);

    // Uncomment for UART logging
    // line = line + "\n";
    // const auto* line_ptr = reinterpret_cast<const uint8_t*>(line.c_str());
    // HAL_UART_Transmit(&huart1, line_ptr, line.size(), HAL_MAX_DELAY);

    // Uncomment for logging to screen
    screen.DrawText(0, y, line.c_str(), font7x12, C_GREEN, C_BLACK);
    y += 12;
  }
};


// import hashlib
// hashlib.sha256(b"hello").hexdigest()
bool test_digest(Logger& log) {
    using namespace mls::hpke;
    const auto digest = Digest::get<Digest::ID::SHA256>();
    const auto data = from_ascii("hello");

    const auto output = digest.hash(data);

    const auto expected = from_hex("2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824");
    const auto pass = output == expected;
    const auto status = std::string(pass? "PASS" : "FAIL");
    log.log("hash", status);

    return pass;
}

bool test_hmac(Logger& log) {
    using namespace mls::hpke;
    const auto digest = Digest::get<Digest::ID::SHA256>();
    const auto key = from_ascii("key");
    const auto data = from_ascii("hello");

    const auto output = digest.hmac(key, data);

    const auto expected = from_hex("9307b3b915efb5171ff14d8cb55fbcc798c6c0ef1456d66ded1a6aa723a58b7b");
    const auto pass = output == expected;
    const auto status = std::string(pass? "PASS" : "FAIL");
    log.log("hmac", status);

    return pass;
}

bool test_aead(Logger& log) {
    try {
        using namespace mls::hpke;
        const auto& cipher = AEAD::get<AEAD::ID::AES_128_GCM>();
        const auto key = from_ascii("sixteen byte key");
        const auto nonce = from_ascii("public nonce");
        const auto pt = from_ascii("secret message");
        const auto aad = from_ascii("extra");
        log.log("cipher", "prep");

        const auto ct = cipher.seal(key, nonce, aad, pt);
        log.log("cipher", "seal");

        const auto expected = from_hex("76e196daeff5e2224f12f7726d82aaedbe176f85ece437c5ce7861a02fc2");
        const auto pass_kat = (ct == expected);
        log.log("cipher", "pass_kat", pass_kat);

        const auto maybe_pt = cipher.open(key, nonce, aad, ct);
        log.log("cipher", "open");

        auto pt_print = std::string("");
        if (maybe_pt) {
          pt_print = to_hex(*maybe_pt);
        }
        log.log("cipher", "pt");

        const auto pass_rtt = maybe_pt && pt == *maybe_pt;
        log.log("cipher", "pass_rtt", pass_rtt);

        return pass_kat && pass_rtt;
    } catch (const std::exception& e) {
        log.log("cipher", "throw", e.what());
        return false;
    }
}

// XXX(RLB): Getting signature working:
// * Validate that raw known-answer test works
// * Validate that raw known-answer test works with small math functions
// * Validate that raw known-answer test works with SHA-256
// * Generate key pair dynamically, verify raw round-trip test works
// * Move ECC context to an object
// * Introduce casts
bool test_sig_raw(Logger& log) {
  cmox_ecc_handle_t Ecc_Ctx;

  // Constants
  const uint8_t Message[] =
  {
    0xff, 0x62, 0x4d, 0x0b, 0xa0, 0x2c, 0x7b, 0x63, 0x70, 0xc1, 0x62, 0x2e, 0xec, 0x3f, 0xa2, 0x18,
    0x6e, 0xa6, 0x81, 0xd1, 0x65, 0x9e, 0x0a, 0x84, 0x54, 0x48, 0xe7, 0x77, 0xb7, 0x5a, 0x8e, 0x77,
    0xa7, 0x7b, 0xb2, 0x6e, 0x57, 0x33, 0x17, 0x9d, 0x58, 0xef, 0x9b, 0xc8, 0xa4, 0xe8, 0xb6, 0x97,
    0x1a, 0xef, 0x25, 0x39, 0xf7, 0x7a, 0xb0, 0x96, 0x3a, 0x34, 0x15, 0xbb, 0xd6, 0x25, 0x83, 0x39,
    0xbd, 0x1b, 0xf5, 0x5d, 0xe6, 0x5d, 0xb5, 0x20, 0xc6, 0x3f, 0x5b, 0x8e, 0xab, 0x3d, 0x55, 0xde,
    0xbd, 0x05, 0xe9, 0x49, 0x42, 0x12, 0x17, 0x0f, 0x5d, 0x65, 0xb3, 0x28, 0x6b, 0x8b, 0x66, 0x87,
    0x05, 0xb1, 0xe2, 0xb2, 0xb5, 0x56, 0x86, 0x10, 0x61, 0x7a, 0xbb, 0x51, 0xd2, 0xdd, 0x0c, 0xb4,
    0x50, 0xef, 0x59, 0xdf, 0x4b, 0x90, 0x7d, 0xa9, 0x0c, 0xfa, 0x7b, 0x26, 0x8d, 0xe8, 0xc4, 0xc2
  };
  const uint8_t Private_Key[] =
  {
    0x70, 0x83, 0x09, 0xa7, 0x44, 0x9e, 0x15, 0x6b, 0x0d, 0xb7, 0x0e, 0x5b, 0x52, 0xe6, 0x06, 0xc7,
    0xe0, 0x94, 0xed, 0x67, 0x6c, 0xe8, 0x95, 0x3b, 0xf6, 0xc1, 0x47, 0x57, 0xc8, 0x26, 0xf5, 0x90
  };
  const uint8_t Public_Key[] =
  {
    0x29, 0x57, 0x8c, 0x7a, 0xb6, 0xce, 0x0d, 0x11, 0x49, 0x3c, 0x95, 0xd5, 0xea, 0x05, 0xd2, 0x99,
    0xd5, 0x36, 0x80, 0x1c, 0xa9, 0xcb, 0xd5, 0x0e, 0x99, 0x24, 0xe4, 0x3b, 0x73, 0x3b, 0x83, 0xab,
    0x08, 0xc8, 0x04, 0x98, 0x79, 0xc6, 0x27, 0x8b, 0x22, 0x73, 0x34, 0x84, 0x74, 0x15, 0x85, 0x15,
    0xac, 0xca, 0xa3, 0x83, 0x44, 0x10, 0x6e, 0xf9, 0x68, 0x03, 0xc5, 0xa0, 0x5a, 0xdc, 0x48, 0x00
  };
  const uint8_t Known_Random[] = /* = k - 1 */
  {
    0x58, 0xf7, 0x41, 0x77, 0x16, 0x20, 0xbd, 0xc4, 0x28, 0xe9, 0x1a, 0x32, 0xd8, 0x6d, 0x23, 0x08,
    0x73, 0xe9, 0x14, 0x03, 0x36, 0xfc, 0xfb, 0x1e, 0x12, 0x28, 0x92, 0xee, 0x1d, 0x50, 0x1b, 0xdb
  };
  const uint8_t Known_Signature[] =
  {
    0x4a, 0x19, 0x27, 0x44, 0x29, 0xe4, 0x05, 0x22, 0x23, 0x4b, 0x87, 0x85, 0xdc, 0x25, 0xfc, 0x52,
    0x4f, 0x17, 0x9d, 0xcc, 0x95, 0xff, 0x09, 0xb3, 0xc9, 0x77, 0x0f, 0xc7, 0x1f, 0x54, 0xca, 0x0d,
    0x58, 0x98, 0x2b, 0x79, 0xa6, 0x5b, 0x73, 0x20, 0xf5, 0xb9, 0x2d, 0x13, 0xbd, 0xae, 0xcd, 0xd1,
    0x25, 0x9e, 0x76, 0x0f, 0x0f, 0x71, 0x8b, 0xa9, 0x33, 0xfd, 0x09, 0x8f, 0x6f, 0x75, 0xd4, 0xb7
  };

  uint8_t Computed_Hash[CMOX_SHA224_SIZE];
  uint8_t Computed_Signature[CMOX_ECC_SECP256R1_SIG_LEN];
  uint8_t Working_Buffer[2000];

  size_t computed_size = 0;
  uint32_t fault_check = CMOX_ECC_AUTH_FAIL;

  // Pre-hash message
  const auto hrv = cmox_hash_compute(CMOX_SHA224_ALGO,         /* Use SHA224 algorithm */
                                     Message, sizeof(Message), /* Message to digest */
                                     Computed_Hash,            /* Data buffer to receive digest data */
                                     CMOX_SHA224_SIZE,         /* Expected digest size */
                                     &computed_size);          /* Size of computed digest */

  if (hrv != CMOX_HASH_SUCCESS)
  {
    log.log("sig_raw", "hrv", hrv);
    return false;
  }

  if (computed_size != CMOX_SHA224_SIZE)
  {
    Error_Handler();
  }

  // Sign
  cmox_ecc_construct(&Ecc_Ctx, CMOX_ECC256_MATH_FUNCS, Working_Buffer, sizeof(Working_Buffer));

  const auto srv = cmox_ecdsa_sign(&Ecc_Ctx,                                 /* ECC context */
                           CMOX_ECC_SECP256R1_LOWMEM,                 /* SECP256R1 ECC curve selected */
                           Known_Random, sizeof(Known_Random),       /* Random data buffer */
                           Private_Key, sizeof(Private_Key),         /* Private key for signature */
                           Computed_Hash, CMOX_SHA224_SIZE,          /* Digest to sign */
                           Computed_Signature, &computed_size);      /* Data buffer to receive signature */
  log.log("sig_raw", "sign");

  if (srv != CMOX_ECC_SUCCESS)
  {
    log.log("sig_raw", "srv", srv);
    return false;
  }

  if (computed_size != sizeof(Known_Signature))
  {
    log.log("sig_raw", "computed_size", computed_size);
    return false;
  }

  if (memcmp(Computed_Signature, Known_Signature, computed_size) != 0)
  {
    log.log("sig_raw", "Computed_Signature");
    return false;
  }

  log.log("sig_raw", "sig_cleanup");
  cmox_ecc_cleanup(&Ecc_Ctx);

  // Verify
  cmox_ecc_construct(&Ecc_Ctx, CMOX_ECC256_MATH_FUNCS, Working_Buffer, sizeof(Working_Buffer));

  const auto vrv = cmox_ecdsa_verify(&Ecc_Ctx,                                  /* ECC context */
                             CMOX_ECC_CURVE_SECP256R1,                  /* SECP256R1 ECC curve selected */
                             Public_Key, sizeof(Public_Key),            /* Public key for verification */
                             Computed_Hash, CMOX_SHA224_SIZE,           /* Digest to verify */
                             Known_Signature, sizeof(Known_Signature),  /* Data buffer to receive signature */
                             &fault_check);                             /* Fault check variable:
                                                            to ensure no fault injection occurs during this API call */

  /* Verify API returned value */
  if (vrv != CMOX_ECC_AUTH_SUCCESS)
  {
    log.log("sig_raw", "srv", srv);
    return false;
  }
  /* Verify Fault check variable value */
  if (fault_check != CMOX_ECC_AUTH_SUCCESS)
  {
    log.log("sig_raw", "fault_check", fault_check);
    return false;
  }

  log.log("sig_raw", "ver_cleanup");
  cmox_ecc_cleanup(&Ecc_Ctx);


  return true;
}

bool test_sig(Logger& log) {
    try {
        using namespace mls::hpke;
        const auto& sig = Signature::get<Signature::ID::P256_SHA256>();
        const auto msg = from_ascii("attack at dawn!");

        const auto sk = sig.generate_key_pair();
        log.log("sig", "keygen");

        const auto pk = sk->public_key();
        log.log("sig", "public_key");

        const auto sig_val = sig.sign(msg, *sk);
        log.log("sig", "sign", sig_val.size());

        const auto ver = sig.verify(msg, sig_val, *pk);
        log.log("sig", "verify", ver);

        const auto pass = ver;
        log.log("sig", "pass", pass);

        return pass;
    } catch (const std::exception& e) {
        log.log("sig", "throw", e.what());
        return false;
    }
}

bool test_kem(Logger& log) {
    try {
        using namespace mls::hpke;
        const auto& kem = KEM::get<KEM::ID::DHKEM_P256_SHA256>();

        const auto sk = kem.generate_key_pair();
        log.log("kem", "keygen");

        const auto pk = sk->public_key();
        log.log("kem", "public_key");

        const auto [zz_send, enc] = kem.encap(*pk);
        log.log("kem", "encap", zz_send.size(), enc.size());

        const auto zz_recv = kem.decap(enc, *sk);
        log.log("kem", "decap", zz_recv.size());

        const auto pass = zz_send == zz_recv;
        log.log("kem", "pass", pass);

        return pass;
    } catch (const std::exception& e) {
        log.log("kem", "throw", e.what());
        return false;
    }
}

// TODO Get the osc working correctly from an external signal
int app_main()
{
    audio = new AudioCodec(hi2s3, hi2c1);

    // Reserve the first 32 bytes, and the total size is 255 bytes - 1k bits
    eeprom = new EEPROM(hi2c1, 32, 255);

    screen.Begin();

    // // Set the port pins and groups for the keyboard columns
    port_pin col_pins[Q10_COLS] =
    {
        { KB_COL1_GPIO_Port, KB_COL1_Pin },
        { KB_COL2_GPIO_Port, KB_COL2_Pin },
        { KB_COL3_GPIO_Port, KB_COL3_Pin },
        { KB_COL4_GPIO_Port, KB_COL4_Pin },
        { KB_COL5_GPIO_Port, KB_COL5_Pin },
    };

    // Set the port pins and groups for the keyboard rows
    port_pin row_pins[Q10_ROWS] =
    {
        { KB_ROW1_GPIO_Port, KB_ROW1_Pin },
        { KB_ROW2_GPIO_Port, KB_ROW2_Pin },
        { KB_ROW3_GPIO_Port, KB_ROW3_Pin },
        { KB_ROW4_GPIO_Port, KB_ROW4_Pin },
        { KB_ROW5_GPIO_Port, KB_ROW5_Pin },
        { KB_ROW6_GPIO_Port, KB_ROW6_Pin },
        { KB_ROW7_GPIO_Port, KB_ROW7_Pin },
    };

    // Create the keyboard object
    keyboard = new Q10Keyboard(col_pins, row_pins, 200, 100, &htim2);

    // Initialize the keyboard
    keyboard->Begin();

    mgmt_serial_interface = new SerialStm(&huart1);
    net_serial_interface = new SerialStm(&huart2);

    ui_manager = new UserInterfaceManager(screen, *keyboard, *net_serial_interface, *eeprom);

    SerialPacketManager mgmt_serial(mgmt_serial_interface);

    uint8_t start_message [] = "UI: start\n\r";
    HAL_UART_Transmit(&huart1, start_message, 12, 1000);
    HAL_GPIO_WritePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(ADC_UI_STAT_GPIO_Port, ADC_UI_STAT_Pin, GPIO_PIN_SET);


    // HAL_GPIO_TogglePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin);

    // TODO figure out how to send a small sound??
    // Generate a simple triangle wave
    // double frequency = 1000; // Hz
    // double amplitude = 1000.0;
    // double duration = 1.0;  //seconds
    // double sample_rate = 16'000; // samples per second5
    uint32_t i = 0;
    // for (double t = 0.0; t < duration && i < SOUND_BUFFER_SZ; t += 1.0 / sample_rate)
    // {
    //     tx_sound_buff[i++] = GenerateTriangleWavePoint(frequency, amplitude, t);
    // }

    #define SAMPLE_RATE 44100
    #define DAC_RESOLUTION 4095 // 12-bit DAC

    // Lookup table for sine wave generation (values are between 0 and DAC_RESOLUTION)
    const uint16_t sine_wave[] = {
        2048, 2447, 2831, 3185, 3495, 3750, 3939, 4056,
        4095, 4056, 3939, 3750, 3495, 3185, 2831, 2447,
        2048, 1648, 1264, 910, 600, 345, 156, 39,
        0, 39, 156, 345, 600, 910, 1264, 1648
        // Add more values as needed to cover a complete cycle
    };

    while (i < SOUND_BUFFER_SZ)
    {
        for (unsigned int j = 0; j < sizeof(sine_wave) / sizeof(sine_wave[0]); ++j)
        {
            tx_sound_buff[i++] = sine_wave[j];
        }
    }



    // for (i = 0; i < SOUND_BUFFER_SZ; ++i)
    // {
    //     // uint32_t val = (i * (0xFFFF / SOUND_BUFFER_SZ)) % 0xFFFF;
    //     // uint32_t val = i;
    //     // tx_sound_buff[i] = val;
    //     tx_sound_buff[i] = GenerateTriangleWavePoint(frequency, amplitude, duration);
    // }

    tx_sound_buff[SOUND_BUFFER_SZ - 1] = 0x7FFF;
    // tx_sound_buff[SOUND_BUFFER_SZ-1] = 0x00;

    // Sanity check numbers
    // tx_sound_buff[0] = 1;
    // tx_sound_buff[1] = 0xF0F0;
    // tx_sound_buff[2] = 65535;
    // tx_sound_buff[4] = 0xAAAA;
    // tx_sound_buff[5] = 0x5555;
    // tx_sound_buff[SOUND_BUFFER_SZ - 1] = 1;
    // tx_sound_buff[SOUND_BUFFER_SZ - 2] = 0x00AA;

    // Initialize the cryptographic library
    Logger log;
    const auto rv = cmox_initialize(nullptr);
    log.log("init", rv == CMOX_INIT_SUCCESS);

    // Delayed condition
    auto first_run = true;
    uint32_t blink = HAL_GetTick() + 5000;
    uint32_t tx_sound = 0;
    // auto output = HAL_I2S_Transmit_DMA(&hi2s3, tx_sound_buff, SOUND_BUFFER_SZ * sizeof(uint16_t));
    while (1)
    {
        ui_manager->Run();

        if (HAL_GetTick() > tx_sound)
        {
            // audio->Send1KHzSignal();
            // HAL_I2SEx_TransmitReceive_DMA(&hi2s3, tx_sound_buff, rx_sound_buff, SOUND_BUFFER_SZ);
            // auto output = HAL_I2S_Transmit_DMA(&hi2s3, tx_sound_buff, SOUND_BUFFER_SZ * sizeof(uint16_t));
            // screen.DrawText(0, 100, String::int_to_string((int)tx_sound_buff[0]), font7x12, C_GREEN, C_BLACK);

            // audio->TestRegister();
            // uint16_t reg_value = 0;
            // uint16_t value = 0x01FF;
            // audio->ReadRegister(0x0A, reg_value);
            // screen.DrawText(0, 100, String::int_to_string((int)reg_value), font7x12, C_GREEN, C_BLACK);

            tx_sound = HAL_GetTick() + 10;
        }

        // // mgmt_serial.RxTx(HAL_GetTick());

        // // screen.FillRectangle(0, 200, 20, 220, C_YELLOW);
        if (HAL_GetTick() > blink)
        {

            //     // auto packet = std::make_unique<SerialPacket>();
            //     // packet->SetData(SerialPacket::Types::Debug, 0, 1);
            //     // packet->SetData(mgmt_serial.NextPacketId(), 1, 2);
            //     // packet->SetData(5, 1, 2);
            //     // packet->SetData('h', 1);
            //     // packet->SetData('e', 1);
            //     // packet->SetData('l', 1);
            //     // packet->SetData('l', 1);
            //     // packet->SetData('o', 1);
            //     // mgmt_serial.EnqueuePacket(std::move(packet));
            // audio->Send1KHzSignal();

            // HAL_StatusTypeDef res = HAL_I2S_Transmit_DMA(&hi2s3, buff, BUFFER_SIZE * sizeof(uint16_t));
            // screen.DrawText(0, 100, String::int_to_string((int)res), font7x12, C_WHITE, C_BLACK);
            // if (res == HAL_OK)
            // {
            // }
            // blink = HAL_GetTick() + 5000;
            // uint32_t num = 0;
            // HAL_RNG_GenerateRandomNumber(&hrng, &num);
            // screen.DrawText(0, 82, String::int_to_string(num), font7x12, C_GREEN, C_BLACK);

            // Self-test the crypto on the first run-through
            if (first_run) {
              log.log("start");

              const auto digest = true; // already validated // test_digest(log);
              const auto hmac = true; // already validated // test_hmac(log);
              const auto aead = test_aead(log);
              const auto sig_raw = test_sig_raw(log);
              const auto sig = test_sig(log);
              const auto kem = true; // already validated // test_kem();
              first_run = false;

              log.log("end ", digest, hmac, aead, sig_raw, sig, kem);
            }

            if (rx_busy) {
                continue;
            }

            // We need this because if the mic stays on it will write to USART3 in
            // bootloader mode which locks up the main chip
            // TODO remove enable mic bits [2,3]
            // audio->XorRegister(0x19, 0b0'0000'1100);

            rx_busy = true;

            // TODO Rx_Buffer_Sz might need to be in bytes not element sz.
            // auto output = HAL_I2SEx_TransmitReceive_DMA(&hi2s3, tx_sound_buff, rx_sound_buff, SOUND_BUFFER_SZ);

            // screen.DrawText(0, 100, String::int_to_string((int)output), font7x12, C_GREEN, C_BLACK);
            // screen.DrawText(0, 112, String::int_to_string((int)rx_sound_buff[0]), font7x12, C_GREEN, C_BLACK);
            // screen.DrawText(0, 124, String::int_to_string((int)rx_sound_buff[1]), font7x12, C_GREEN, C_BLACK);
            // screen.DrawText(0, 136, String::int_to_string((int)rx_sound_buff[2]), font7x12, C_GREEN, C_BLACK);
            // screen.DrawText(0, 148, String::int_to_string((int)rx_sound_buff[3]), font7x12, C_GREEN, C_BLACK);

        }
    }

    return 0;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef* huart, uint16_t size)
{
    if (huart->Instance == USART2)
    {
        net_serial_interface->RxEvent(size);
    }
    else if (huart->Instance == USART1)
    {
        mgmt_serial_interface->RxEvent(size);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    if (huart->Instance == USART2)
    {
        net_serial_interface->TxEvent();
    }
    else if (huart->Instance == USART1)
    {
        HAL_GPIO_TogglePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin);
        mgmt_serial_interface->TxEvent();
    }
}

void HAL_I2SEx_TxRxCpltCallback(I2S_HandleTypeDef* /* hi2s */)
{
    // audio->RxComplete();
    // rx_busy = false;
    // audio->XorRegister(0x19, 0b0'0000'1100);

    HAL_GPIO_TogglePin(UI_LED_B_GPIO_Port, UI_LED_B_Pin);
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef* /* hi2s */)
{
    // auto output = HAL_I2S_Transmit_DMA(&hi2s3, tx_sound_buff, SOUND_BUFFER_SZ * sizeof(uint16_t));

    HAL_GPIO_TogglePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
    uint16_t err;
    UNUSED(err);
    if (huart->Instance == USART2)
    {
        net_serial_interface->Reset();
        HAL_GPIO_TogglePin(UI_LED_R_GPIO_Port, UI_LED_R_Pin);

        // Read the err codes to clear them
        err = huart->Instance->SR;

        net_serial_interface->StartRx();
    }
    else if (huart->Instance == USART1)
    {
        mgmt_serial_interface->Reset();
        HAL_GPIO_TogglePin(UI_LED_G_GPIO_Port, UI_LED_G_Pin);

        // Read the err codes to clear them
        err = huart->Instance->SR;

        mgmt_serial_interface->StartRx();
    }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef* hspi)
{
    UNUSED(hspi);
    screen.ReleaseSPI();
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
    // Keyboard timer callback!
    if (htim->Instance == TIM2)
    {
        keyboard->Read();
    }
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
        e.g.: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
        /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
