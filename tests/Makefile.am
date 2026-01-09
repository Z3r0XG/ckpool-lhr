# Test suite Makefile

AM_CPPFLAGS = -I$(top_srcdir)/src -I$(top_srcdir)/src/jansson-2.14/src \
              -I$(top_srcdir)/tests

# Link against libckpool and jansson
# JANSSON_LIBS is relative to src/, so we need to prepend src/ path
LDADD = $(top_builddir)/src/libckpool.a $(top_builddir)/src/@JANSSON_LIBS@ @LIBS@ -lm

# Unit tests
check_PROGRAMS = \
	unit/test-difficulty \
	unit/test-sha256 \
	unit/test-encoding \
	unit/test-donation \
	unit/test-useragent \
	unit/test-address \
	unit/test-string-utils \
	unit/test-time-utils \
	unit/test-fulltest \
	unit/test-serialization \
	unit/test-endian \
	unit/test-time-conversion \
	unit/test-base58 \
	unit/test-base64 \
	unit/test-dropidle \
	unit/test-vardiff \
	unit/test-share-params \
	unit/test-config \
	unit/test-low-diff \
	unit/test-password-diff \
	unit/test-fractional-stats \
	unit/test-fractional-vardiff \
	unit/test-fractional-config \
	unit/test-vardiff-comprehensive \
	unit/test-share-orphan-prevention \
	unit/test-adjustment-hysteresis \
	unit/test-network-diff-interactions \
	unit/test-backwards-compatibility \
	unit/test-ua-aggregation \
	unit/test-zombie-cleanup

TESTS = $(check_PROGRAMS)
# UA normalization/aggregation tests
unit_test_ua_aggregation_SOURCES = \
	unit/test-ua-aggregation.c

# Difficulty calculation tests
unit_test_difficulty_SOURCES = \
	unit/test-difficulty.c

# SHA-256 tests
unit_test_sha256_SOURCES = \
	unit/test-sha256.c

# Encoding tests
unit_test_encoding_SOURCES = \
	unit/test-encoding.c

# Donation system tests
unit_test_donation_SOURCES = \
	unit/test-donation.c

# Useragent whitelisting tests
unit_test_useragent_SOURCES = \
	unit/test-useragent.c

# Address encoding tests
unit_test_address_SOURCES = \
	unit/test-address.c

# String utility tests
unit_test_string_utils_SOURCES = \
	unit/test-string-utils.c

# Time utility tests
unit_test_time_utils_SOURCES = \
	unit/test-time-utils.c

# Fulltest (hash validation) tests
unit_test_fulltest_SOURCES = \
	unit/test-fulltest.c

# Number serialization tests
unit_test_serialization_SOURCES = \
	unit/test-serialization.c

# Endian conversion tests
unit_test_endian_SOURCES = \
	unit/test-endian.c

# Time conversion tests
unit_test_time_conversion_SOURCES = \
	unit/test-time-conversion.c

# Base58 decoding tests
unit_test_base58_SOURCES = \
	unit/test-base58.c

# Base64 encoding tests
unit_test_base64_SOURCES = \
	unit/test-base64.c

# dropidle feature tests
unit_test_dropidle_SOURCES = \
	unit/test-dropidle.c

# Vardiff logic tests
unit_test_vardiff_SOURCES = \
	unit/test-vardiff.c

# Share parameter validation tests
unit_test_share_params_SOURCES = \
	unit/test-share-params.c

# Configuration parsing tests
unit_test_config_SOURCES = \
	unit/test-config.c

# Low diff (allow_low_diff) feature tests
unit_test_low_diff_SOURCES = \
	unit/test-low-diff.c

# Password difficulty suggestion tests
unit_test_password_diff_SOURCES = \
	unit/test-password-diff.c

# Fractional statistics accumulation tests
unit_test_fractional_stats_SOURCES = \
	unit/test-fractional-stats.c

# Fractional variable difficulty tests
unit_test_fractional_vardiff_SOURCES = \
	unit/test-fractional-vardiff.c

# Fractional configuration validation tests
unit_test_fractional_config_SOURCES = \
	unit/test-fractional-config.c

# Comprehensive vardiff algorithm tests (real-world scenarios)
unit_test_vardiff_comprehensive_SOURCES = \
	unit/test-vardiff-comprehensive.c

# Share orphan prevention tests
unit_test_share_orphan_prevention_SOURCES = \
	unit/test-share-orphan-prevention.c

# Adjustment hysteresis tests
unit_test_adjustment_hysteresis_SOURCES = \
	unit/test-adjustment-hysteresis.c

# Network difficulty interaction tests
unit_test_network_diff_interactions_SOURCES = \
	unit/test-network-diff-interactions.c

# Backwards compatibility tests
unit_test_backwards_compatibility_SOURCES = \
	unit/test-backwards-compatibility.c

# Zombie cleanup logic tests
unit_test_zombie_cleanup_SOURCES = \
	unit/test-zombie-cleanup.c

# All tests depend on libckpool
$(check_PROGRAMS): $(top_builddir)/src/libckpool.a
