/* SPDX-License-Identifier: GPL-2.0-or-later */

/***************************************************************************
 *   Cryptography support for Memory VFS                                   *
 *   Copyright (C) 2024 OpenOCD Contributors                               *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "vfs_crypto.h"
#include "../helper/log.h"
#include "aes.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Global crypto configuration */
static struct vfs_crypto_config *g_crypto_config = NULL;
static bool g_crypto_initialized = false;

/**
 * Helper function: Convert hex character to integer
 */
static int hex_char_to_int(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

/**
 * Helper function: Decode hex string to binary
 * Returns the number of bytes decoded, or -1 on error
 */
static int hex_decode(const unsigned char *hex_str, size_t hex_len,
                      unsigned char **binary_out, size_t *binary_len)
{
	if (!hex_str || !binary_out || !binary_len)
		return -1;

	/* Hex string must be even length */
	if (hex_len % 2 != 0) {
		LOG_ERROR("Crypto: Hex string length must be even");
		return -1;
	}

	size_t out_len = hex_len / 2;
	unsigned char *out = malloc(out_len);
	if (!out) {
		LOG_ERROR("Crypto: Failed to allocate memory for hex decode");
		return -1;
	}

	for (size_t i = 0; i < hex_len; i += 2) {
		int high = hex_char_to_int(hex_str[i]);
		int low = hex_char_to_int(hex_str[i + 1]);

		if (high < 0 || low < 0) {
			LOG_ERROR("Crypto: Invalid hex character at position %zu", i);
			free(out);
			return -1;
		}

		out[i / 2] = (high << 4) | low;
	}

	*binary_out = out;
	*binary_len = out_len;
	return 0;
}

/**
 * Helper function: Remove zero-byte padding from the end
 */
static void remove_zero_padding(unsigned char *data, size_t *len)
{
	if (!data || !len || *len == 0)
		return;

	/* Remove trailing zeros */
	while (*len > 0 && data[*len - 1] == 0)
		(*len)--;
}

/**
 * Initialize crypto subsystem
 */
int vfs_crypto_init(struct vfs_crypto_config *config)
{
	if (g_crypto_initialized) {
		LOG_WARNING("Crypto: Already initialized, cleaning up first");
		vfs_crypto_cleanup();
	}

	if (!config) {
		/* No encryption */
		g_crypto_config = NULL;
		g_crypto_initialized = true;
		return 0;
	}

	/* Allocate and copy config */
	g_crypto_config = malloc(sizeof(struct vfs_crypto_config));
	if (!g_crypto_config) {
		LOG_ERROR("Crypto: Failed to allocate config");
		return -1;
	}

	memcpy(g_crypto_config, config, sizeof(struct vfs_crypto_config));

	/* Duplicate string fields */
	if (config->env_var_name) {
		g_crypto_config->env_var_name = strdup(config->env_var_name);
		if (!g_crypto_config->env_var_name) {
			free(g_crypto_config);
			g_crypto_config = NULL;
			return -1;
		}
	}

	if (config->key_file_path) {
		g_crypto_config->key_file_path = strdup(config->key_file_path);
		if (!g_crypto_config->key_file_path) {
			if (g_crypto_config->env_var_name)
				free((void *)g_crypto_config->env_var_name);
			free(g_crypto_config);
			g_crypto_config = NULL;
			return -1;
		}
	}

	/* Duplicate key and IV if provided */
	if (config->key && config->key_len > 0) {
		g_crypto_config->key = malloc(config->key_len);
		if (!g_crypto_config->key) {
			if (g_crypto_config->env_var_name)
				free((void *)g_crypto_config->env_var_name);
			if (g_crypto_config->key_file_path)
				free((void *)g_crypto_config->key_file_path);
			free(g_crypto_config);
			g_crypto_config = NULL;
			return -1;
		}
		memcpy(g_crypto_config->key, config->key, config->key_len);
	}

	if (config->iv && config->iv_len > 0) {
		g_crypto_config->iv = malloc(config->iv_len);
		if (!g_crypto_config->iv) {
			if (g_crypto_config->key)
				free(g_crypto_config->key);
			if (g_crypto_config->env_var_name)
				free((void *)g_crypto_config->env_var_name);
			if (g_crypto_config->key_file_path)
				free((void *)g_crypto_config->key_file_path);
			free(g_crypto_config);
			g_crypto_config = NULL;
			return -1;
		}
		memcpy(g_crypto_config->iv, config->iv, config->iv_len);
	}

	g_crypto_initialized = true;
	LOG_INFO("Crypto: Initialized with algorithm: %s",
	         vfs_crypto_algorithm_name(config->algorithm));

	return 0;
}

/**
 * Cleanup crypto subsystem
 */
void vfs_crypto_cleanup(void)
{
	if (!g_crypto_initialized)
		return;

	if (g_crypto_config) {
		if (g_crypto_config->key) {
			/* Zero out key before freeing for security */
			memset(g_crypto_config->key, 0, g_crypto_config->key_len);
			free(g_crypto_config->key);
		}
		if (g_crypto_config->iv) {
			memset(g_crypto_config->iv, 0, g_crypto_config->iv_len);
			free(g_crypto_config->iv);
		}
		if (g_crypto_config->env_var_name)
			free((void *)g_crypto_config->env_var_name);
		if (g_crypto_config->key_file_path)
			free((void *)g_crypto_config->key_file_path);

		free(g_crypto_config);
		g_crypto_config = NULL;
	}

	g_crypto_initialized = false;
	LOG_DEBUG("Crypto: Cleaned up");
}

/**
 * Set encryption configuration
 */
int vfs_crypto_set_config(struct vfs_crypto_config *config)
{
	vfs_crypto_cleanup();
	return vfs_crypto_init(config);
}

/**
 * Check if encryption is enabled
 */
bool vfs_crypto_is_enabled(void)
{
	return g_crypto_initialized && g_crypto_config != NULL &&
	       g_crypto_config->algorithm != VFS_CRYPTO_NONE;
}

/**
 * Decrypt content using AES-128-CBC
 * Handles double-hex encoding: hex_decode -> AES_decrypt -> remove_padding -> hex_decode
 */
int vfs_crypto_decrypt(const unsigned char *encrypted, size_t encrypted_len,
                       unsigned char **decrypted_out, size_t *decrypted_len)
{
	if (!encrypted || !decrypted_out || !decrypted_len) {
		LOG_ERROR("Crypto: Invalid parameters");
		return -1;
	}

	if (!vfs_crypto_is_enabled()) {
		LOG_ERROR("Crypto: Encryption not enabled");
		return -1;
	}

	if (!g_crypto_config->key || g_crypto_config->key_len == 0) {
		LOG_ERROR("Crypto: No key configured");
		return -1;
	}

	unsigned char *step1_data = NULL;
	size_t step1_len = 0;
	unsigned char *step2_data = NULL;
	size_t step2_len = 0;
	unsigned char *step3_data = NULL;
	size_t step3_len = 0;
	int ret = -1;

	LOG_DEBUG("Crypto: Decrypting %zu bytes", encrypted_len);

	/* Step 1: First hex decode (encrypted file is hex encoded) */
	if (hex_decode(encrypted, encrypted_len, &step1_data, &step1_len) != 0) {
		LOG_ERROR("Crypto: First hex decode failed");
		goto cleanup;
	}

	LOG_DEBUG("Crypto: After first hex decode: %zu bytes", step1_len);

	/* Step 2: AES-128-CBC decrypt */
	if (g_crypto_config->algorithm == VFS_CRYPTO_AES_128_CBC) {
		/* Check that data length is multiple of AES block size */
		if (step1_len % AES_BLOCKLEN != 0) {
			LOG_ERROR("Crypto: Encrypted data length (%zu) not multiple of AES block size (%d)",
			          step1_len, AES_BLOCKLEN);
			goto cleanup;
		}

		/* Allocate buffer for decrypted data */
		step2_data = malloc(step1_len);
		if (!step2_data) {
			LOG_ERROR("Crypto: Failed to allocate decryption buffer");
			goto cleanup;
		}

		/* Copy encrypted data to output buffer (AES decrypts in-place) */
		memcpy(step2_data, step1_data, step1_len);
		step2_len = step1_len;

		/* Prepare AES context */
		struct AES_ctx ctx;
		
		/* Use IV from config, or use key as IV (like Python script) */
		unsigned char *iv_to_use = g_crypto_config->iv;
		if (!iv_to_use) {
			/* Use key as IV (matches Python script behavior) */
			iv_to_use = g_crypto_config->key;
		}

		/* Initialize AES context with key and IV */
		AES_init_ctx_iv(&ctx, g_crypto_config->key, iv_to_use);

		/* Decrypt in-place */
		AES_CBC_decrypt_buffer(&ctx, step2_data, step2_len);

		LOG_DEBUG("Crypto: AES-128-CBC decrypted: %zu bytes", step2_len);

		/* Step 3: Remove zero-byte padding */
		remove_zero_padding(step2_data, &step2_len);
		LOG_DEBUG("Crypto: After removing padding: %zu bytes", step2_len);

		/* Step 4: Second hex decode (original content was hex encoded) */
		if (hex_decode(step2_data, step2_len, &step3_data, &step3_len) != 0) {
			LOG_ERROR("Crypto: Second hex decode failed");
			goto cleanup;
		}

		LOG_DEBUG("Crypto: After second hex decode: %zu bytes (final)", step3_len);

		/* Success */
		*decrypted_out = step3_data;
		*decrypted_len = step3_len;
		step3_data = NULL; /* Prevent cleanup from freeing it */
		ret = 0;

	} else {
		LOG_ERROR("Crypto: Unsupported algorithm: %d", g_crypto_config->algorithm);
		goto cleanup;
	}

cleanup:
	if (step1_data)
		free(step1_data);
	if (step2_data)
		free(step2_data);
	if (step3_data)
		free(step3_data);

	return ret;
}

/**
 * Detect if content is encrypted (heuristic check)
 * Checks if data looks like hex-encoded content
 */
bool vfs_crypto_is_encrypted(const unsigned char *data, size_t data_len)
{
	if (!data || data_len == 0)
		return false;

	/* Check if data looks like hex (all printable hex characters) */
	size_t hex_chars = 0;
	for (size_t i = 0; i < data_len && i < 100; i++) {
		char c = data[i];
		if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
			hex_chars++;
	}

	/* If more than 95% are hex characters, likely encrypted */
	size_t sample_size = data_len < 100 ? data_len : 100;
	return (hex_chars * 100 / sample_size) > 95;
}

/**
 * Set encryption key from string
 */
int vfs_crypto_set_key_string(const char *key_string, enum vfs_crypto_algorithm algorithm)
{
	if (!key_string) {
		LOG_ERROR("Crypto: Key string is NULL");
		return -1;
	}

	size_t key_len = strlen(key_string);
	if (key_len == 0) {
		LOG_ERROR("Crypto: Key string is empty");
		return -1;
	}

	/* Validate key length based on algorithm */
	if (algorithm == VFS_CRYPTO_AES_128_CBC && key_len != 16) {
		LOG_ERROR("Crypto: AES-128 requires 16-byte key, got %zu bytes", key_len);
		return -1;
	}

	struct vfs_crypto_config config = {
		.algorithm = algorithm,
		.key_source = VFS_KEY_SOURCE_LITERAL,
		.key = (unsigned char *)key_string,
		.key_len = key_len,
		.iv = NULL,  /* Will use key as IV */
		.iv_len = 0,
		.env_var_name = NULL,
		.key_file_path = NULL,
		.key_callback = NULL,
		.callback_user_data = NULL
	};

	return vfs_crypto_init(&config);
}

/**
 * Set encryption key from environment variable
 */
int vfs_crypto_set_key_env(const char *env_var_name, enum vfs_crypto_algorithm algorithm)
{
	if (!env_var_name) {
		LOG_ERROR("Crypto: Environment variable name is NULL");
		return -1;
	}

	const char *key_string = getenv(env_var_name);
	if (!key_string) {
		LOG_ERROR("Crypto: Environment variable '%s' not set", env_var_name);
		return -1;
	}

	LOG_DEBUG("Crypto: Using key from environment variable '%s'", env_var_name);
	return vfs_crypto_set_key_string(key_string, algorithm);
}

/**
 * Set encryption key from file
 */
int vfs_crypto_set_key_file(const char *key_file, enum vfs_crypto_algorithm algorithm)
{
	if (!key_file) {
		LOG_ERROR("Crypto: Key file path is NULL");
		return -1;
	}

	FILE *fp = fopen(key_file, "rb");
	if (!fp) {
		LOG_ERROR("Crypto: Failed to open key file: %s", key_file);
		return -1;
	}

	/* Read key from file */
	unsigned char key_buffer[256];
	size_t key_len = fread(key_buffer, 1, sizeof(key_buffer), fp);
	fclose(fp);

	if (key_len == 0) {
		LOG_ERROR("Crypto: Key file is empty: %s", key_file);
		return -1;
	}

	/* Remove trailing newline if present */
	while (key_len > 0 && (key_buffer[key_len - 1] == '\n' || key_buffer[key_len - 1] == '\r'))
		key_len--;

	/* Validate key length */
	if (algorithm == VFS_CRYPTO_AES_128_CBC && key_len != 16) {
		LOG_ERROR("Crypto: AES-128 requires 16-byte key, got %zu bytes from file", key_len);
		return -1;
	}

	/* Create config */
	unsigned char *key_copy = malloc(key_len);
	if (!key_copy) {
		LOG_ERROR("Crypto: Failed to allocate key buffer");
		return -1;
	}
	memcpy(key_copy, key_buffer, key_len);

	struct vfs_crypto_config config = {
		.algorithm = algorithm,
		.key_source = VFS_KEY_SOURCE_FILE,
		.key = key_copy,
		.key_len = key_len,
		.iv = NULL,
		.iv_len = 0,
		.env_var_name = NULL,
		.key_file_path = key_file,
		.key_callback = NULL,
		.callback_user_data = NULL
	};

	int ret = vfs_crypto_init(&config);
	
	/* Zero out and free temporary key buffer */
	memset(key_buffer, 0, sizeof(key_buffer));
	if (ret != 0)
		free(key_copy);

	LOG_INFO("Crypto: Loaded key from file: %s", key_file);
	return ret;
}

/**
 * Get algorithm name
 */
const char *vfs_crypto_algorithm_name(enum vfs_crypto_algorithm algorithm)
{
	switch (algorithm) {
	case VFS_CRYPTO_AES_128_CBC:
		return "AES-128-CBC";
	case VFS_CRYPTO_AES_256_CBC:
		return "AES-256-CBC";
	case VFS_CRYPTO_AES_128_CTR:
		return "AES-128-CTR";
	case VFS_CRYPTO_NONE:
		return "None";
	default:
		return "Unknown";
	}
}
