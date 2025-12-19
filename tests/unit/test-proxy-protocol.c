/*
 * Unit tests for Proxy Protocol v2/v1 parsing
 * Tests the PP header detection and parsing logic
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "config.h"
#include "../test_common.h"
#include "libckpool.h"

/* Test: No Proxy Protocol (direct connection) */
static void test_no_proxy_protocol(void)
{
	unsigned char buf[100] = {0};
	char address[128] = {0};
	int port = 0;
	bool pp_pending = false, pp_parsed = false;
	unsigned long discard = 0;

	/* Send JSON directly (no PP) */
	strcpy((char*)buf, "{\"id\":1,\"method\":\"mining.subscribe\"}");
	
	int result = parse_proxy_protocol_peek(buf, strlen((char*)buf),
	                                        address, &port, &pp_pending, &discard, &pp_parsed);
	
	assert_true(result == 0);  /* No PP detected */
	assert_false(pp_parsed);
	assert_false(pp_pending);
	assert_int_equal(discard, 0);
}

/* Test: Valid PPv2 TCP4 (minimal header) */
static void test_ppv2_tcp4_valid(void)
{
	unsigned char buf[100];
	char address[128] = {0};
	int port = 0;
	bool pp_pending = false, pp_parsed = false;
	unsigned long discard = 0;

	/* PPv2 magic + version/cmd + family/proto + len + TCP4 addresses */
	unsigned char ppv2_header[] = {
		0x0D, 0x0A, 0x0D, 0x0A, 0x00, 0x0D, 0x0A, 0x51, 0x55, 0x49, 0x54, 0x0A,  /* magic */
		0x21,                                                                      /* version=2, cmd=PROXY */
		0x11,                                                                      /* family=AF_INET, proto=STREAM */
		0x00, 0x0C,                                                                /* len=12 (4+4+2+2) */
		0xCB, 0x00, 0x71, 0x0A,                                                    /* src 203.0.113.10 */
		0x7F, 0x00, 0x00, 0x01,                                                    /* dst 127.0.0.1 */
		0x9C, 0x40,                                                                /* src port 40000 */
		0x0D, 0x05                                                                 /* dst port 3333 */
	};
	
	memcpy(buf, ppv2_header, sizeof(ppv2_header));
	
	int result = parse_proxy_protocol_peek(buf, sizeof(ppv2_header),
	                                        address, &port, &pp_pending, &discard, &pp_parsed);
	
	assert_true(result == 1);  /* PP detected */
	assert_true(pp_parsed);    /* Valid parse */
	assert_true(pp_pending);   /* Mark for discard */
	assert_string_equal(address, "203.0.113.10");
	assert_int_equal(port, 40000);
	assert_int_equal(discard, 28);  /* 16 + 12 */
}

/* Test: Valid PPv2 TCP6 (IPv6 addresses) */
static void test_ppv2_tcp6_valid(void)
{
	unsigned char buf[100];
	char address[128] = {0};
	int port = 0;
	bool pp_pending = false, pp_parsed = false;
	unsigned long discard = 0;

	/* PPv2 magic + version/cmd + family/proto + len + TCP6 addresses */
	unsigned char ppv2_tcp6_header[] = {
		0x0D, 0x0A, 0x0D, 0x0A, 0x00, 0x0D, 0x0A, 0x51, 0x55, 0x49, 0x54, 0x0A,  /* magic */
		0x21,                                                                      /* version=2, cmd=PROXY */
		0x21,                                                                      /* family=AF_INET6, proto=STREAM */
		0x00, 0x24,                                                                /* len=36 (16+16+2+2) */
		/* src: 2001:db8::1 */
		0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
		/* dst: ::1 */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
		0x9C, 0x40,                                                                /* src port 40000 */
		0x0D, 0x05                                                                 /* dst port 3333 */
	};
	
	memcpy(buf, ppv2_tcp6_header, sizeof(ppv2_tcp6_header));
	
	int result = parse_proxy_protocol_peek(buf, sizeof(ppv2_tcp6_header),
	                                        address, &port, &pp_pending, &discard, &pp_parsed);
	
	assert_true(result == 1);      /* PP detected */
	assert_true(pp_parsed);        /* Valid parse */
	assert_true(pp_pending);       /* Mark for discard */
	assert_string_equal(address, "2001:db8::1");
	assert_int_equal(port, 40000);
	assert_int_equal(discard, 52);  /* 16 + 36 */
}

/* Test: PPv2 with oversized len (should still discard) */
static void test_ppv2_oversized_len(void)
{
	unsigned char buf[100];
	char address[128] = {0};
	int port = 0;
	bool pp_pending = false, pp_parsed = false;
	unsigned long discard = 0;

	/* PPv2 with len=2000 (way beyond peek buffer) */
	unsigned char ppv2_header[] = {
		0x0D, 0x0A, 0x0D, 0x0A, 0x00, 0x0D, 0x0A, 0x51, 0x55, 0x49, 0x54, 0x0A,
		0x21, 0x11,
		0x07, 0xD0   /* len=2000 */
	};
	
	memcpy(buf, ppv2_header, sizeof(ppv2_header));
	
	int result = parse_proxy_protocol_peek(buf, sizeof(ppv2_header),
	                                        address, &port, &pp_pending, &discard, &pp_parsed);
	
	assert_true(result == 1);         /* PP detected */
	assert_false(pp_parsed);          /* NOT parsed (too large) */
	assert_true(pp_pending);          /* But still mark for discard */
	assert_int_equal(discard, 2016);  /* 16 + 2000 */
}

/* Test: Valid PPv1 TCP4 with CRLF */
static void test_ppv1_tcp4_valid(void)
{
	unsigned char buf[200];
	char address[128] = {0};
	int port = 0;
	bool pp_pending = false, pp_parsed = false;
	unsigned long discard = 0;

	const char *ppv1_line = "PROXY TCP4 203.0.113.10 127.0.0.1 40000 3333\r\n";
	strcpy((char*)buf, ppv1_line);
	
	int result = parse_proxy_protocol_peek(buf, strlen(ppv1_line),
	                                        address, &port, &pp_pending, &discard, &pp_parsed);
	
	assert_true(result == 1);  /* PP detected */
	assert_true(pp_parsed);    /* Valid parse */
	assert_true(pp_pending);   /* Mark for discard */
	assert_string_equal(address, "203.0.113.10");
	assert_int_equal(port, 40000);
	assert_int_equal(discard, strlen(ppv1_line));
}

/* Test: PPv1 without CRLF (incomplete) */
static void test_ppv1_no_crlf(void)
{
	unsigned char buf[200];
	char address[128] = {0};
	int port = 0;
	bool pp_pending = false, pp_parsed = false;
	unsigned long discard = 0;

	const char *ppv1_partial = "PROXY TCP4 203.0.113.10 127.0.0.1 40000 3333";  /* no \r\n */
	strcpy((char*)buf, ppv1_partial);
	
	int result = parse_proxy_protocol_peek(buf, strlen(ppv1_partial),
	                                        address, &port, &pp_pending, &discard, &pp_parsed);
	
	assert_true(result == 1);      /* PP detected */
	assert_false(pp_parsed);       /* NOT parsed (no CRLF yet) */
	assert_true(pp_pending);       /* But mark pending */
	assert_int_equal(discard, 0);  /* discard_remaining=0 triggers newline-seek */
}

/* Test: PPv1 with UNKNOWN proto (valid but no address) */
static void test_ppv1_unknown_proto(void)
{
	unsigned char buf[200];
	char address[128] = {0};
	int port = 0;
	bool pp_pending = false, pp_parsed = false;
	unsigned long discard = 0;

	const char *ppv1_unknown = "PROXY UNKNOWN\r\n";
	strcpy((char*)buf, ppv1_unknown);
	
	int result = parse_proxy_protocol_peek(buf, strlen(ppv1_unknown),
	                                        address, &port, &pp_pending, &discard, &pp_parsed);
	
	assert_true(result == 1);     /* PP detected */
	assert_false(pp_parsed);      /* UNKNOWN doesn't set parsed */
	assert_true(pp_pending);      /* But still mark for discard */
	assert_int_equal(discard, strlen(ppv1_unknown));
}

/* Test: PPv2 truncated (exactly 16 bytes - header only, no payload) */
static void test_ppv2_incomplete_header(void)
{
	unsigned char buf[100];
	char address[128] = {0};
	int port = 0;
	bool pp_pending = false, pp_parsed = false;
	unsigned long discard = 0;

	/* PPv2 with magic, version/cmd, family/proto, but len field says 12 more bytes */
	unsigned char ppv2_incomplete[] = {
		0x0D, 0x0A, 0x0D, 0x0A, 0x00, 0x0D, 0x0A, 0x51, 0x55, 0x49, 0x54, 0x0A,  /* magic */
		0x21,                                                                      /* version=2, cmd=PROXY */
		0x11,                                                                      /* family=AF_INET, proto=STREAM */
		0x00, 0x0C                                                                 /* len=12 (but we only have header, no addresses) */
	};
	
	memcpy(buf, ppv2_incomplete, sizeof(ppv2_incomplete));
	
	int result = parse_proxy_protocol_peek(buf, sizeof(ppv2_incomplete),
	                                        address, &port, &pp_pending, &discard, &pp_parsed);
	
	assert_true(result == 1);       /* PP detected */
	assert_false(pp_parsed);        /* NOT parsed (incomplete data) */
	assert_true(pp_pending);        /* Mark pending for retry */
	assert_int_equal(discard, 0);   /* Don't discard yet (incomplete) */
}

/* Test: PPv2 truncated (only 8 bytes) */
static void test_ppv2_truncated(void)
{
	unsigned char buf[100];
	char address[128] = {0};
	int port = 0;
	bool pp_pending = false, pp_parsed = false;
	unsigned long discard = 0;

	unsigned char ppv2_truncated[] = {
		0x0D, 0x0A, 0x0D, 0x0A, 0x00, 0x0D, 0x0A, 0x51  /* only 8 bytes of magic */
	};
	
	memcpy(buf, ppv2_truncated, sizeof(ppv2_truncated));
	
	int result = parse_proxy_protocol_peek(buf, sizeof(ppv2_truncated),
	                                        address, &port, &pp_pending, &discard, &pp_parsed);
	
	assert_true(result == 0);  /* Not enough bytes for full magic */
	assert_false(pp_parsed);
	assert_false(pp_pending);
}

/* Test: PPv1 with invalid IP (should not parse) */
static void test_ppv1_invalid_ip(void)
{
	unsigned char buf[200];
	char address[128] = {0};
	int port = 0;
	bool pp_pending = false, pp_parsed = false;
	unsigned long discard = 0;

	const char *ppv1_bad_ip = "PROXY TCP4 999.999.999.999 127.0.0.1 40000 3333\r\n";
	strcpy((char*)buf, ppv1_bad_ip);
	
	int result = parse_proxy_protocol_peek(buf, strlen(ppv1_bad_ip),
	                                        address, &port, &pp_pending, &discard, &pp_parsed);
	
	assert_true(result == 1);      /* PP detected */
	assert_false(pp_parsed);       /* But inet_pton fails, so not parsed */
	assert_true(pp_pending);       /* Still mark for discard */
}

/* Test: Empty/null buffer */
static void test_empty_buffer(void)
{
	unsigned char buf[100] = {0};
	char address[128] = {0};
	int port = 0;
	bool pp_pending = false, pp_parsed = false;
	unsigned long discard = 0;

	int result = parse_proxy_protocol_peek(buf, 0,
	                                        address, &port, &pp_pending, &discard, &pp_parsed);
	
	assert_true(result == 0);  /* No PP in empty buffer */
	assert_false(pp_parsed);
	assert_false(pp_pending);
}

/* Main test runner */
int main(void)
{
	test_no_proxy_protocol();
	test_ppv2_tcp4_valid();
	test_ppv2_tcp6_valid();
	test_ppv2_oversized_len();
	test_ppv2_incomplete_header();
	test_ppv1_tcp4_valid();
	test_ppv1_no_crlf();
	test_ppv1_unknown_proto();
	test_ppv2_truncated();
	test_ppv1_invalid_ip();
	test_empty_buffer();

	printf("All Proxy Protocol tests passed!\n");
	return 0;
}

