/*
 * Spdylay - SPDY Library
 *
 * Copyright (c) 2012 Tatsuhiro Tsujikawa
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "spdylay_frame_test.h"

#include <CUnit/CUnit.h>

#include "spdylay_frame.h"

static const char *headers[] = {
  "method", "GET",
  "scheme", "https",
  "url", "/",
  "x-head", "foo",
  "x-head", "bar",
  "version", "HTTP/1.1",
  NULL
};

void test_spdylay_frame_unpack_nv()
{
  uint8_t out[1024];
  char **nv;
  size_t inlen = spdylay_frame_pack_nv(out, (char**)headers);
  CU_ASSERT(0 == spdylay_frame_unpack_nv(&nv, out, inlen));
  spdylay_frame_nv_free(nv);
  free(nv);
}

void test_spdylay_frame_count_nv_space()
{
  CU_ASSERT(83 == spdylay_frame_count_nv_space((char**)headers));
}

void test_spdylay_frame_pack_ping()
{
  spdylay_frame frame, oframe;
  uint8_t *buf;
  ssize_t buflen;
  spdylay_frame_ping_init(&frame.ping, 1);
  buflen = spdylay_frame_pack_ping(&buf, &frame.ping);
  CU_ASSERT(0 == spdylay_frame_unpack_ping
            (&oframe.ping,
             &buf[0], SPDYLAY_FRAME_HEAD_LENGTH,
             &buf[SPDYLAY_FRAME_HEAD_LENGTH],
             buflen-SPDYLAY_FRAME_HEAD_LENGTH));
  CU_ASSERT(1 == oframe.ping.unique_id);
  free(buf);
  spdylay_frame_ping_free(&oframe.ping);
  spdylay_frame_ping_free(&frame.ping);
}

void test_spdylay_frame_pack_goaway()
{
  spdylay_frame frame, oframe;
  uint8_t *buf;
  ssize_t buflen;
  spdylay_frame_goaway_init(&frame.goaway, 1000000007);
  buflen = spdylay_frame_pack_goaway(&buf, &frame.goaway);
  CU_ASSERT(0 == spdylay_frame_unpack_goaway
            (&oframe.goaway,
             &buf[0], SPDYLAY_FRAME_HEAD_LENGTH,
             &buf[SPDYLAY_FRAME_HEAD_LENGTH],
             buflen-SPDYLAY_FRAME_HEAD_LENGTH));
  CU_ASSERT(1000000007 == oframe.goaway.last_good_stream_id);
  CU_ASSERT(SPDYLAY_PROTO_VERSION == oframe.headers.hd.version);
  CU_ASSERT(SPDYLAY_GOAWAY == oframe.headers.hd.type);
  CU_ASSERT(SPDYLAY_FLAG_NONE == oframe.headers.hd.flags);
  CU_ASSERT(buflen-SPDYLAY_FRAME_HEAD_LENGTH == oframe.ping.hd.length);
  free(buf);
  spdylay_frame_goaway_free(&oframe.goaway);
  spdylay_frame_goaway_free(&frame.goaway);
}

void test_spdylay_frame_pack_headers()
{
  spdylay_zlib deflater, inflater;
  spdylay_frame frame, oframe;
  uint8_t *buf;
  ssize_t buflen;
  spdylay_zlib_deflate_hd_init(&deflater);
  spdylay_zlib_inflate_hd_init(&inflater);
  spdylay_frame_headers_init(&frame.headers, SPDYLAY_FLAG_FIN, 3,
                             spdylay_frame_nv_copy(headers));
  buflen = spdylay_frame_pack_headers(&buf, &frame.headers, &deflater);
  CU_ASSERT(0 == spdylay_frame_unpack_headers
            (&oframe.headers,
             &buf[0], SPDYLAY_FRAME_HEAD_LENGTH,
             &buf[SPDYLAY_FRAME_HEAD_LENGTH],
             buflen-SPDYLAY_FRAME_HEAD_LENGTH,
             &inflater));
  CU_ASSERT(3 == oframe.headers.stream_id);
  CU_ASSERT(SPDYLAY_PROTO_VERSION == oframe.headers.hd.version);
  CU_ASSERT(SPDYLAY_HEADERS == oframe.headers.hd.type);
  CU_ASSERT(SPDYLAY_FLAG_FIN == oframe.headers.hd.flags);
  CU_ASSERT(buflen-SPDYLAY_FRAME_HEAD_LENGTH == oframe.ping.hd.length);
  CU_ASSERT(strcmp("method", oframe.headers.nv[0]) == 0);
  CU_ASSERT(strcmp("GET", oframe.headers.nv[1]) == 0);
  CU_ASSERT(NULL == oframe.headers.nv[12]);
  free(buf);
  spdylay_frame_headers_free(&oframe.headers);
  spdylay_frame_headers_free(&frame.headers);
  spdylay_zlib_inflate_free(&inflater);
  spdylay_zlib_deflate_free(&deflater);
}

void test_spdylay_frame_nv_sort()
{
  char *nv[7];
  nv[0] = (char*)"version";
  nv[1] = (char*)"HTTP/1.1";
  nv[2] = (char*)"method";
  nv[3] = (char*)"GET";
  nv[4] = (char*)"scheme";
  nv[5] = (char*)"https";
  nv[6] = NULL;
  spdylay_frame_nv_sort(nv);
  CU_ASSERT(strcmp("method", nv[0]) == 0);
  CU_ASSERT(strcmp("GET", nv[1]) == 0);
  CU_ASSERT(strcmp("scheme", nv[2]) == 0);
  CU_ASSERT(strcmp("https", nv[3]) == 0);
  CU_ASSERT(strcmp("version", nv[4]) == 0);
  CU_ASSERT(strcmp("HTTP/1.1", nv[5]) == 0);
}