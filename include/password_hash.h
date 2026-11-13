#pragma once
#include <Arduino.h>
#include <esp_system.h>
#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>
#include <mbedtls/version.h>

namespace bridge {
inline String randomHex(unsigned bytes) {
  String result;
  result.reserve(bytes * 2);
  static const char hex[] = "0123456789abcdef";
  for (unsigned i = 0; i < bytes; ++i) {
    const uint8_t value = esp_random();
    result += hex[value >> 4]; result += hex[value & 15];
  }
  return result;
}
inline bool equalSecret(const String& a, const String& b) {
  if (a.length() != b.length()) return false;
  volatile uint8_t difference = 0;
  for (unsigned i = 0; i < a.length(); ++i) difference |= a[i] ^ b[i];
  return difference == 0;
}
inline String passwordHash(const String& password, const String& salt) {
  uint8_t derived[32];
#if MBEDTLS_VERSION_MAJOR >= 3
  const int result = mbedtls_pkcs5_pbkdf2_hmac_ext(MBEDTLS_MD_SHA256,
    reinterpret_cast<const unsigned char*>(password.c_str()), password.length(),
    reinterpret_cast<const unsigned char*>(salt.c_str()), salt.length(), 10000,
    sizeof(derived), derived);
#else
  mbedtls_md_context_t context;
  mbedtls_md_init(&context);
  const auto* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  int result = mbedtls_md_setup(&context, info, 1);
  if (!result) result = mbedtls_pkcs5_pbkdf2_hmac(&context,
    reinterpret_cast<const unsigned char*>(password.c_str()), password.length(),
    reinterpret_cast<const unsigned char*>(salt.c_str()), salt.length(), 10000,
    sizeof(derived), derived);
  mbedtls_md_free(&context);
#endif
  if (result) return "";
  String hash;
  char byte[3];
  for (auto value : derived) { snprintf(byte, sizeof(byte), "%02x", value); hash += byte; }
  memset(derived, 0, sizeof(derived));
  return hash;
}
}
