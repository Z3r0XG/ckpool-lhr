/*
 * Unit tests for LOGMSGSIZ chunking logic in libckpool.h
 *
 * LOGMSGSIZ splits messages longer than DEFLOGBUFSIZ-2 (510) bytes into
 * multiple chunks, each delivered via logmsg(). This test verifies that
 * long messages are fully delivered; no trailing content is dropped.
 *
 * Bug context: the original loop used `LEN -= OFFSET` (cumulative) instead
 * of `LEN -= CPY` (bytes copied this pass), causing messages over ~1020
 * bytes to be silently truncated after the second chunk.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>

#include "../test_common.h"
#include "libckpool.h"

/* ── Capture infrastructure ─────────────────────────────────────────────── */

#define CAPTURE_BUF_SIZE (1024 * 64)

static char  g_captured[CAPTURE_BUF_SIZE];
static int   g_captured_len = 0;
static int   g_chunk_count  = 0;

static void reset_capture(void)
{
	memset(g_captured, 0, sizeof(g_captured));
	g_captured_len = 0;
	g_chunk_count  = 0;
}

/* Strong override of the weak logmsg() in libckpool.c.
 * Each call appends the formatted string to g_captured. */
void logmsg(int loglevel, const char *fmt, ...)
{
	(void)loglevel;
	va_list ap;
	char *buf = NULL;

	va_start(ap, fmt);
	if (vasprintf(&buf, fmt, ap) < 0)
		buf = NULL;
	va_end(ap);

	if (!buf)
		return;

	int len = strlen(buf);
	if (g_captured_len + len < CAPTURE_BUF_SIZE - 1) {
		memcpy(g_captured + g_captured_len, buf, len);
		g_captured_len += len;
		g_captured[g_captured_len] = '\0';
	}
	g_chunk_count++;
	free(buf);
}

/* ── Helpers ─────────────────────────────────────────────────────────────── */

/* Build a deterministic string of exactly `len` printable chars (no NUL). */
static char *make_message(int len)
{
	char *msg = malloc(len + 1);
	assert_non_null(msg);
	for (int i = 0; i < len; i++)
		msg[i] = 'a' + (i % 26);
	msg[len] = '\0';
	return msg;
}

/* ── Tests ───────────────────────────────────────────────────────────────── */

/*
 * Short message (< DEFLOGBUFSIZ-2 = 510 bytes).
 * Expect: 1 chunk, full content delivered.
 */
static void test_short_message_complete(void)
{
	char *msg = make_message(100);

	reset_capture();
	LOGNOTICE("%s", msg);

	assert_int_equal(g_chunk_count, 1);
	assert_string_equal(g_captured, msg);

	free(msg);
	printf("  ✓ short message (100 bytes): 1 chunk, complete\n");
}

/*
 * Medium message (510–1020 bytes).
 * Expect: 2 chunks, full content delivered.
 */
static void test_medium_message_complete(void)
{
	char *msg = make_message(700);

	reset_capture();
	LOGNOTICE("%s", msg);

	assert_int_equal(g_chunk_count, 2);
	assert_string_equal(g_captured, msg);

	free(msg);
	printf("  ✓ medium message (700 bytes): 2 chunks, complete\n");
}

/*
 * Long message (> 1020 bytes).
 * This is the case that the LEN -= OFFSET bug broke; it would exit the
 * loop after chunk 2, silently dropping everything beyond ~1020 bytes.
 * Expect: 3+ chunks, full content delivered.
 */
static void test_long_message_complete(void)
{
	char *msg = make_message(1500);

	reset_capture();
	LOGNOTICE("%s", msg);

	assert_true(g_chunk_count >= 3);
	assert_string_equal(g_captured, msg);

	free(msg);
	printf("  ✓ long message (1500 bytes): %d chunks, complete\n", g_chunk_count);
}

/*
 * Extra-long message (>> 1020 bytes, typical pool UA JSON size).
 * Simulates the real-world Pool UA status JSON that triggered the bug.
 */
static void test_extra_long_message_complete(void)
{
	char *msg = make_message(4000);

	reset_capture();
	LOGNOTICE("%s", msg);

	assert_true(g_chunk_count >= 8);
	assert_string_equal(g_captured, msg);

	free(msg);
	printf("  ✓ extra-long message (4000 bytes): %d chunks, complete\n", g_chunk_count);
}

/*
 * Exact boundary: message exactly DEFLOGBUFSIZ-2 (510) bytes.
 * Should fit in exactly 1 chunk.
 */
static void test_exact_chunk_boundary(void)
{
	char *msg = make_message(DEFLOGBUFSIZ - 2);

	reset_capture();
	LOGNOTICE("%s", msg);

	assert_int_equal(g_chunk_count, 1);
	assert_string_equal(g_captured, msg);

	free(msg);
	printf("  ✓ exact boundary (%d bytes): 1 chunk, complete\n", DEFLOGBUFSIZ - 2);
}

/*
 * One byte over the chunk boundary (511 bytes).
 * Should spill into a second chunk.
 */
static void test_one_over_boundary(void)
{
	char *msg = make_message(DEFLOGBUFSIZ - 1);

	reset_capture();
	LOGNOTICE("%s", msg);

	assert_int_equal(g_chunk_count, 2);
	assert_string_equal(g_captured, msg);

	free(msg);
	printf("  ✓ one-over boundary (%d bytes): 2 chunks, complete\n", DEFLOGBUFSIZ - 1);
}

/* ── main ────────────────────────────────────────────────────────────────── */

int main(void)
{
	printf("Running LOGMSGSIZ chunking tests...\n");

	run_test(test_short_message_complete);
	run_test(test_medium_message_complete);
	run_test(test_long_message_complete);
	run_test(test_extra_long_message_complete);
	run_test(test_exact_chunk_boundary);
	run_test(test_one_over_boundary);

	printf("All LOGMSGSIZ tests passed.\n");
	return 0;
}
