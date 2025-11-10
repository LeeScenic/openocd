/* SPDX-License-Identifier: GPL-2.0-or-later */

/***************************************************************************
 *   Cryptography support for Memory VFS                                   *
 *   Copyright (C) 2024 OpenOCD Contributors                               *
 ***************************************************************************/

#ifndef OPENOCD_HELPER_VFS_CRYPTO_H
#define OPENOCD_HELPER_VFS_CRYPTO_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @file vfs_crypto.h
 * @brief Cryptography support for encrypted VFS content
 *
 * This module provides encryption/decryption capabilities for the memory VFS.
 * Supports multiple encryption algorithms and flexible key management.
 */

/* Encryption algorithms */
enum vfs_crypto_algorithm {
	VFS_CRYPTO_AES_128_CBC = 0,  /* AES-128-CBC (default, matches Python script) */
	VFS_CRYPTO_AES_256_CBC,      /* AES-256-CBC (more secure) */
	VFS_CRYPTO_AES_128_CTR,      /* AES-128-CTR mode */
	VFS_CRYPTO_NONE,             /* No encryption */
};

/* Key source types */
enum vfs_crypto_key_source {
	VFS_KEY_SOURCE_NONE = 0,     /* No key configured */
	VFS_KEY_SOURCE_LITERAL,      /* Key provided directly (not recommended for production) */
	VFS_KEY_SOURCE_ENV,          /* Key from environment variable */
	VFS_KEY_SOURCE_FILE,         /* Key from file */
	VFS_KEY_SOURCE_CALLBACK,     /* Key from callback function */
};

/* Crypto configuration */
struct vfs_crypto_config {
	enum vfs_crypto_algorithm algorithm;
	enum vfs_crypto_key_source key_source;
	
	/* Key data */
	unsigned char *key;          /* Encryption key */
	size_t key_len;              /* Key length in bytes */
	
	/* IV (Initialization Vector) */
	unsigned char *iv;           /* IV (NULL = use key as IV, like Python script) */
	size_t iv_len;               /* IV length */
	
	/* Key source specific data */
	const char *env_var_name;    /* For KEY_SOURCE_ENV */
	const char *key_file_path;   /* For KEY_SOURCE_FILE */
	
	/* Callback for dynamic key retrieval */
	int (*key_callback)(unsigned char **key, size_t *key_len, void *user_data);
	void *callback_user_data;
};

/**
 * Initialize crypto subsystem
 *
 * @param config  Crypto configuration (NULL = no encryption)
 * @return 0 on success, -1 on error
 */
int vfs_crypto_init(struct vfs_crypto_config *config);

/**
 * Cleanup crypto subsystem
 */
void vfs_crypto_cleanup(void);

/**
 * Set encryption configuration
 *
 * @param config  New configuration
 * @return 0 on success, -1 on error
 */
int vfs_crypto_set_config(struct vfs_crypto_config *config);

/**
 * Check if encryption is enabled
 *
 * @return true if encryption is configured
 */
bool vfs_crypto_is_enabled(void);

/**
 * Decrypt content
 *
 * @param encrypted     Encrypted data
 * @param encrypted_len Length of encrypted data
 * @param decrypted_out Output buffer for decrypted data (allocated by function)
 * @param decrypted_len Output length of decrypted data
 * @return 0 on success, -1 on error
 */
int vfs_crypto_decrypt(const unsigned char *encrypted, size_t encrypted_len,
                       unsigned char **decrypted_out, size_t *decrypted_len);

/**
 * Detect if content is encrypted (heuristic check)
 *
 * @param data      Data to check
 * @param data_len  Data length
 * @return true if data appears to be encrypted
 */
bool vfs_crypto_is_encrypted(const unsigned char *data, size_t data_len);

/**
 * Set encryption key from string
 * Convenience function for command-line usage
 *
 * @param key_string  Key as string
 * @param algorithm   Algorithm to use
 * @return 0 on success, -1 on error
 */
int vfs_crypto_set_key_string(const char *key_string, enum vfs_crypto_algorithm algorithm);

/**
 * Set encryption key from environment variable
 *
 * @param env_var_name  Environment variable name
 * @param algorithm     Algorithm to use
 * @return 0 on success, -1 on error
 */
int vfs_crypto_set_key_env(const char *env_var_name, enum vfs_crypto_algorithm algorithm);

/**
 * Set encryption key from file
 *
 * @param key_file      Path to key file
 * @param algorithm     Algorithm to use
 * @return 0 on success, -1 on error
 */
int vfs_crypto_set_key_file(const char *key_file, enum vfs_crypto_algorithm algorithm);

/**
 * Get algorithm name
 *
 * @param algorithm  Algorithm enum
 * @return Algorithm name string
 */
const char *vfs_crypto_algorithm_name(enum vfs_crypto_algorithm algorithm);

#endif /* OPENOCD_HELPER_VFS_CRYPTO_H */
