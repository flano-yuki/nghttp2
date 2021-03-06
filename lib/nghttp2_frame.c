/*
 * nghttp2 - HTTP/2.0 C Library
 *
 * Copyright (c) 2013 Tatsuhiro Tsujikawa
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
#include "nghttp2_frame.h"

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>

#include "nghttp2_helper.h"
#include "nghttp2_net.h"

int nghttp2_frame_is_data_frame(uint8_t *head)
{
  return head[2] == 0;
}

void nghttp2_frame_pack_frame_hd(uint8_t* buf, const nghttp2_frame_hd *hd)
{
  nghttp2_put_uint16be(&buf[0], hd->length);
  buf[2]=  hd->type;
  buf[3] = hd->flags;
  nghttp2_put_uint32be(&buf[4], hd->stream_id);
}

void nghttp2_frame_unpack_frame_hd(nghttp2_frame_hd *hd, const uint8_t* buf)
{
  hd->length = nghttp2_get_uint16(&buf[0]) & NGHTTP2_FRAME_LENGTH_MASK;
  hd->type = buf[2];
  hd->flags = buf[3];
  hd->stream_id = nghttp2_get_uint32(&buf[4]) & NGHTTP2_STREAM_ID_MASK;
}

static void nghttp2_frame_set_hd(nghttp2_frame_hd *hd, uint16_t length,
                                 uint8_t type, uint8_t flags,
                                 int32_t stream_id)
{
  hd->length = length;
  hd->type = type;
  hd->flags = flags;
  hd->stream_id = stream_id;
}

void nghttp2_frame_headers_init(nghttp2_headers *frame,
                                uint8_t flags, int32_t stream_id, int32_t pri,
                                nghttp2_nv *nva, size_t nvlen)
{
  memset(frame, 0, sizeof(nghttp2_headers));
  nghttp2_frame_set_hd(&frame->hd, 0, NGHTTP2_HEADERS, flags, stream_id);
  frame->pri = pri;
  frame->nva = nva;
  frame->nvlen = nvlen;
  frame->cat = NGHTTP2_HCAT_REQUEST;
}

void nghttp2_frame_headers_free(nghttp2_headers *frame)
{
  nghttp2_nv_array_del(frame->nva);
}

void nghttp2_frame_priority_init(nghttp2_priority *frame, int32_t stream_id,
                                 int32_t pri)
{
  memset(frame, 0, sizeof(nghttp2_priority));
  nghttp2_frame_set_hd(&frame->hd, 4, NGHTTP2_PRIORITY, NGHTTP2_FLAG_NONE,
                       stream_id);
  frame->pri = pri;
}

void nghttp2_frame_priority_free(nghttp2_priority *frame)
{}

void nghttp2_frame_rst_stream_init(nghttp2_rst_stream *frame,
                                   int32_t stream_id,
                                   nghttp2_error_code error_code)
{
  memset(frame, 0, sizeof(nghttp2_rst_stream));
  nghttp2_frame_set_hd(&frame->hd, 4, NGHTTP2_RST_STREAM, NGHTTP2_FLAG_NONE,
                       stream_id);
  frame->error_code = error_code;
}

void nghttp2_frame_rst_stream_free(nghttp2_rst_stream *frame)
{}


void nghttp2_frame_settings_init(nghttp2_settings *frame, uint8_t flags,
                                 nghttp2_settings_entry *iv, size_t niv)
{
  memset(frame, 0, sizeof(nghttp2_settings));
  nghttp2_frame_set_hd(&frame->hd, niv * NGHTTP2_FRAME_SETTINGS_ENTRY_LENGTH,
                       NGHTTP2_SETTINGS, flags, 0);
  frame->niv = niv;
  frame->iv = iv;
}

void nghttp2_frame_settings_free(nghttp2_settings *frame)
{
  free(frame->iv);
}

void nghttp2_frame_push_promise_init(nghttp2_push_promise *frame,
                                     uint8_t flags, int32_t stream_id,
                                     int32_t promised_stream_id,
                                     nghttp2_nv *nva, size_t nvlen)
{
  memset(frame, 0, sizeof(nghttp2_push_promise));
  nghttp2_frame_set_hd(&frame->hd, 0, NGHTTP2_PUSH_PROMISE, flags, stream_id);
  frame->promised_stream_id = promised_stream_id;
  frame->nva = nva;
  frame->nvlen = nvlen;
}

void nghttp2_frame_push_promise_free(nghttp2_push_promise *frame)
{
  nghttp2_nv_array_del(frame->nva);
}

void nghttp2_frame_ping_init(nghttp2_ping *frame, uint8_t flags,
                             const uint8_t *opaque_data)
{
  memset(frame, 0, sizeof(nghttp2_ping));
  nghttp2_frame_set_hd(&frame->hd, 8, NGHTTP2_PING, flags, 0);
  if(opaque_data) {
    memcpy(frame->opaque_data, opaque_data, sizeof(frame->opaque_data));
  }
}

void nghttp2_frame_ping_free(nghttp2_ping *frame)
{}

void nghttp2_frame_goaway_init(nghttp2_goaway *frame, int32_t last_stream_id,
                               nghttp2_error_code error_code,
                               uint8_t *opaque_data, size_t opaque_data_len)
{
  memset(frame, 0, sizeof(nghttp2_goaway));
  nghttp2_frame_set_hd(&frame->hd, 8+opaque_data_len, NGHTTP2_GOAWAY,
                       NGHTTP2_FLAG_NONE, 0);
  frame->last_stream_id = last_stream_id;
  frame->error_code = error_code;
  frame->opaque_data = opaque_data;
  frame->opaque_data_len = opaque_data_len;
}

void nghttp2_frame_goaway_free(nghttp2_goaway *frame)
{
  free(frame->opaque_data);
}

void nghttp2_frame_window_update_init(nghttp2_window_update *frame,
                                      uint8_t flags,
                                      int32_t stream_id,
                                      int32_t window_size_increment)
{
  memset(frame, 0, sizeof(nghttp2_window_update));
  nghttp2_frame_set_hd(&frame->hd, 4, NGHTTP2_WINDOW_UPDATE, flags, stream_id);
  frame->window_size_increment = window_size_increment;
}

void nghttp2_frame_window_update_free(nghttp2_window_update *frame)
{}

void nghttp2_frame_data_init(nghttp2_data *frame, nghttp2_private_data *pdata)
{
  frame->hd = pdata->hd;
  frame->padlen = pdata->padlen;
  /* flags may have NGHTTP2_FLAG_END_STREAM or
     NGHTTP2_FLAG_END_SEGMENT even if the sent chunk is not the end of
     the stream */
  if(!pdata->eof) {
    frame->hd.flags &= ~(NGHTTP2_FLAG_END_STREAM | NGHTTP2_FLAG_END_SEGMENT);
  }
}

size_t nghttp2_frame_trail_padlen(nghttp2_frame *frame, size_t padlen)
{
  return padlen
    - ((frame->hd.flags & NGHTTP2_FLAG_PAD_HIGH) > 0)
    - ((frame->hd.flags & NGHTTP2_FLAG_PAD_LOW) > 0);
}

void nghttp2_frame_private_data_init(nghttp2_private_data *frame,
                                     uint8_t flags,
                                     int32_t stream_id,
                                     const nghttp2_data_provider *data_prd)
{
  memset(frame, 0, sizeof(nghttp2_private_data));
  /* At this moment, the length of DATA frame is unknown */
  nghttp2_frame_set_hd(&frame->hd, 0, NGHTTP2_DATA, flags, stream_id);
  frame->data_prd = *data_prd;
}

void nghttp2_frame_private_data_free(nghttp2_private_data *frame)
{}

size_t nghttp2_frame_headers_payload_nv_offset(nghttp2_headers *frame)
{
  if(frame->hd.flags & NGHTTP2_FLAG_PRIORITY) {
    return 4;
  } else {
    return 0;
  }
}

ssize_t nghttp2_frame_pack_headers(nghttp2_buf *buf,
                                   nghttp2_headers *frame,
                                   nghttp2_hd_deflater *deflater)
{
  size_t nv_offset;
  ssize_t rv;

  assert(nghttp2_buf_len(buf) == 0);

  /* Account for possible PAD_HIGH and PAD_LOW */
  buf->pos += 2;

  nv_offset =
    NGHTTP2_FRAME_HDLEN + nghttp2_frame_headers_payload_nv_offset(frame);

  /* If frame->nvlen == 0, nghttp2_buf_len(buf) may be smaller than
     nv_offset */
  rv = nghttp2_buf_pos_reserve(buf, nv_offset);
  if(rv < 0) {
    return rv;
  }

  buf->pos += nv_offset;
  buf->last = buf->pos;

  /* This call will adjust buf->last to the correct position */
  rv = nghttp2_hd_deflate_hd(deflater, buf, frame->nva, frame->nvlen);
  buf->pos -= nv_offset;

  if(rv < 0) {
    return rv;
  }

  frame->hd.length = nghttp2_frame_headers_payload_nv_offset(frame) + rv;
  frame->padlen = 0;

  /* Don't use buf->last, since it already points to the end of the
     frame */
  memset(buf->pos, 0, NGHTTP2_FRAME_HDLEN);

  /* pack frame header after length is determined */
  if(NGHTTP2_MAX_PAYLOADLEN < frame->hd.length) {
    /* Needs CONTINUATION */
    nghttp2_frame_hd hd = frame->hd;

    hd.flags &= ~NGHTTP2_FLAG_END_HEADERS;
    hd.length = NGHTTP2_MAX_PAYLOADLEN;

    nghttp2_frame_pack_frame_hd(buf->pos, &hd);
  } else {
    nghttp2_frame_pack_frame_hd(buf->pos, &frame->hd);
  }

  if(frame->hd.flags & NGHTTP2_FLAG_PRIORITY) {
    nghttp2_put_uint32be(buf->pos + NGHTTP2_FRAME_HDLEN, frame->pri);
  }

  return nghttp2_buf_len(buf);
}

int nghttp2_frame_unpack_headers_payload(nghttp2_headers *frame,
                                         const uint8_t *payload,
                                         size_t payloadlen)
{
  if(frame->hd.flags & NGHTTP2_FLAG_PRIORITY) {
    frame->pri = nghttp2_get_uint32(payload) & NGHTTP2_PRIORITY_MASK;
  } else {
    frame->pri = NGHTTP2_PRI_DEFAULT;
  }
  frame->nva = NULL;
  frame->nvlen = 0;
  return 0;
}

ssize_t nghttp2_frame_pack_priority(nghttp2_buf *buf,
                                    nghttp2_priority *frame)
{
  ssize_t framelen= NGHTTP2_FRAME_HDLEN + 4;
  int rv;

  assert(nghttp2_buf_len(buf) == 0);

  rv = nghttp2_buf_pos_reserve(buf, framelen);
  if(rv != 0) {
    return rv;
  }

  memset(buf->last, 0, framelen);

  nghttp2_frame_pack_frame_hd(buf->last, &frame->hd);
  buf->last += NGHTTP2_FRAME_HDLEN;

  nghttp2_put_uint32be(buf->last, frame->pri);
  buf->last += 4;

  return nghttp2_buf_len(buf);
}

void nghttp2_frame_unpack_priority_payload(nghttp2_priority *frame,
                                           const uint8_t *payload,
                                           size_t payloadlen)
{
  frame->pri = nghttp2_get_uint32(payload) & NGHTTP2_PRIORITY_MASK;
}

ssize_t nghttp2_frame_pack_rst_stream(nghttp2_buf *buf,
                                      nghttp2_rst_stream *frame)
{
  ssize_t framelen = NGHTTP2_FRAME_HDLEN + 4;
  int rv;

  assert(nghttp2_buf_len(buf) == 0);

  rv = nghttp2_buf_pos_reserve(buf, framelen);
  if(rv != 0) {
    return rv;
  }

  memset(buf->last, 0, framelen);

  nghttp2_frame_pack_frame_hd(buf->last, &frame->hd);
  buf->last += NGHTTP2_FRAME_HDLEN;

  nghttp2_put_uint32be(buf->last, frame->error_code);
  buf->last += 4;

  return nghttp2_buf_len(buf);
}

void nghttp2_frame_unpack_rst_stream_payload(nghttp2_rst_stream *frame,
                                             const uint8_t *payload,
                                             size_t payloadlen)
{
  frame->error_code = nghttp2_get_uint32(payload);
}

ssize_t nghttp2_frame_pack_settings(nghttp2_buf *buf,
                                    nghttp2_settings *frame)
{
  ssize_t framelen = NGHTTP2_FRAME_HDLEN + frame->hd.length;
  int rv;

  assert(nghttp2_buf_len(buf) == 0);

  rv = nghttp2_buf_pos_reserve(buf, framelen);
  if(rv != 0) {
    return rv;
  }

  memset(buf->last, 0, framelen);

  nghttp2_frame_pack_frame_hd(buf->last, &frame->hd);
  buf->last += NGHTTP2_FRAME_HDLEN;

  buf->last += nghttp2_frame_pack_settings_payload(buf->last,
                                                   frame->iv, frame->niv);

  return nghttp2_buf_len(buf);
}

size_t nghttp2_frame_pack_settings_payload(uint8_t *buf,
                                           const nghttp2_settings_entry *iv,
                                           size_t niv)
{
  size_t i;
  for(i = 0; i < niv; ++i, buf += NGHTTP2_FRAME_SETTINGS_ENTRY_LENGTH) {
    buf[0] = iv[i].settings_id;
    nghttp2_put_uint32be(buf + 1, iv[i].value);
  }
  return NGHTTP2_FRAME_SETTINGS_ENTRY_LENGTH * niv;
}

int nghttp2_frame_unpack_settings_payload(nghttp2_settings *frame,
                                          nghttp2_settings_entry *iv,
                                          size_t niv)
{
  size_t payloadlen = niv * sizeof(nghttp2_settings_entry);

  frame->iv = malloc(payloadlen);
  if(frame->iv == NULL) {
    return NGHTTP2_ERR_NOMEM;
  }
  memcpy(frame->iv, iv, payloadlen);
  frame->niv = niv;
  return 0;
}

void nghttp2_frame_unpack_settings_entry(nghttp2_settings_entry *iv,
                                         const uint8_t *payload)
{
  iv->settings_id = payload[0];
  iv->value = nghttp2_get_uint32(&payload[1]);
}

int nghttp2_frame_unpack_settings_payload2(nghttp2_settings_entry **iv_ptr,
                                           size_t *niv_ptr,
                                           const uint8_t *payload,
                                           size_t payloadlen)
{
  size_t i;
  *niv_ptr = payloadlen / NGHTTP2_FRAME_SETTINGS_ENTRY_LENGTH;
  *iv_ptr = malloc((*niv_ptr)*sizeof(nghttp2_settings_entry));
  if(*iv_ptr == NULL) {
    return NGHTTP2_ERR_NOMEM;
  }
  for(i = 0; i < *niv_ptr; ++i) {
    size_t off = i * NGHTTP2_FRAME_SETTINGS_ENTRY_LENGTH;
    nghttp2_frame_unpack_settings_entry(&(*iv_ptr)[i], &payload[off]);
  }
  return 0;
}

ssize_t nghttp2_frame_pack_push_promise(nghttp2_buf *buf,
                                        nghttp2_push_promise *frame,
                                        nghttp2_hd_deflater *deflater)
{
  size_t nv_offset = NGHTTP2_FRAME_HDLEN + 4;
  ssize_t rv;

  assert(nghttp2_buf_len(buf) == 0);

  /* Account for possible PAD_HIGH and PAD_LOW */
  buf->pos += 2;

  /* If frame->nvlen == 0, nghttp2_buf_len(buf) may be smaller than
     nv_offset */
  rv = nghttp2_buf_pos_reserve(buf, nv_offset);
  if(rv < 0) {
    return rv;
  }

  buf->pos += nv_offset;
  buf->last = buf->pos;

  /* This call will adjust buf->last to the correct position */
  rv = nghttp2_hd_deflate_hd(deflater, buf, frame->nva, frame->nvlen);
  buf->pos -= nv_offset;

  if(rv < 0) {
    return rv;
  }

  frame->hd.length = 4 + rv;
  frame->padlen = 0;

  /* Don't use buf->last, since it already points to the end of the
     frame */
  memset(buf->pos, 0, NGHTTP2_FRAME_HDLEN);

  /* pack frame header after length is determined */
  if(NGHTTP2_MAX_PAYLOADLEN < frame->hd.length) {
    /* Needs CONTINUATION */
    nghttp2_frame_hd hd = frame->hd;

    hd.flags &= ~NGHTTP2_FLAG_END_HEADERS;
    hd.length = NGHTTP2_MAX_PAYLOADLEN;

    nghttp2_frame_pack_frame_hd(buf->pos, &hd);
  } else {
    nghttp2_frame_pack_frame_hd(buf->pos, &frame->hd);
  }

  nghttp2_put_uint32be(buf->pos + NGHTTP2_FRAME_HDLEN,
                       frame->promised_stream_id);

  return nghttp2_buf_len(buf);
}

int nghttp2_frame_unpack_push_promise_payload(nghttp2_push_promise *frame,
                                              const uint8_t *payload,
                                              size_t payloadlen)
{
  frame->promised_stream_id = nghttp2_get_uint32(payload) &
    NGHTTP2_STREAM_ID_MASK;
  frame->nva = NULL;
  frame->nvlen = 0;
  return 0;
}

ssize_t nghttp2_frame_pack_ping(nghttp2_buf *buf, nghttp2_ping *frame)
{
  ssize_t framelen = NGHTTP2_FRAME_HDLEN + 8;
  int rv;

  assert(nghttp2_buf_len(buf) == 0);

  rv = nghttp2_buf_pos_reserve(buf, framelen);
  if(rv != 0) {
    return rv;
  }

  memset(buf->last, 0, framelen);

  nghttp2_frame_pack_frame_hd(buf->last, &frame->hd);
  buf->last += NGHTTP2_FRAME_HDLEN;

  memcpy(buf->last, frame->opaque_data, sizeof(frame->opaque_data));
  buf->last += sizeof(frame->opaque_data);

  return nghttp2_buf_len(buf);
}

void nghttp2_frame_unpack_ping_payload(nghttp2_ping *frame,
                                       const uint8_t *payload,
                                       size_t payloadlen)
{
  memcpy(frame->opaque_data, payload, sizeof(frame->opaque_data));
}

ssize_t nghttp2_frame_pack_goaway(nghttp2_buf *buf, nghttp2_goaway *frame)
{
  ssize_t framelen = NGHTTP2_FRAME_HDLEN + frame->hd.length;
  int rv;

  assert(nghttp2_buf_len(buf) == 0);

  rv = nghttp2_buf_pos_reserve(buf, framelen);
  if(rv != 0) {
    return rv;
  }

  memset(buf->last, 0, framelen);

  nghttp2_frame_pack_frame_hd(buf->last, &frame->hd);
  buf->last += NGHTTP2_FRAME_HDLEN;

  nghttp2_put_uint32be(buf->last, frame->last_stream_id);
  buf->last += 4;

  nghttp2_put_uint32be(buf->last, frame->error_code);
  buf->last += 4;

  memcpy(buf->last, frame->opaque_data, frame->opaque_data_len);
  buf->last += frame->opaque_data_len;

  return nghttp2_buf_len(buf);
}

void nghttp2_frame_unpack_goaway_payload(nghttp2_goaway *frame,
                                         const uint8_t *payload,
                                         size_t payloadlen)
{
  frame->last_stream_id = nghttp2_get_uint32(payload) & NGHTTP2_STREAM_ID_MASK;
  frame->error_code = nghttp2_get_uint32(payload+4);
  /* TODO Currently we don't buffer debug data */
  frame->opaque_data = NULL;
  frame->opaque_data_len = 0;
}

ssize_t nghttp2_frame_pack_window_update(nghttp2_buf *buf,
                                         nghttp2_window_update *frame)
{
  ssize_t framelen = NGHTTP2_FRAME_HDLEN + 4;
  int rv;

  assert(nghttp2_buf_len(buf) == 0);

  rv = nghttp2_buf_pos_reserve(buf, framelen);
  if(rv != 0) {
    return rv;
  }

  memset(buf->last, 0, framelen);

  nghttp2_frame_pack_frame_hd(buf->last, &frame->hd);
  buf->last += NGHTTP2_FRAME_HDLEN;

  nghttp2_put_uint32be(buf->last, frame->window_size_increment);
  buf->last += 4;

  return nghttp2_buf_len(buf);
}

void nghttp2_frame_unpack_window_update_payload(nghttp2_window_update *frame,
                                                const uint8_t *payload,
                                                size_t payloadlen)
{
  frame->window_size_increment = nghttp2_get_uint32(payload) &
    NGHTTP2_WINDOW_SIZE_INCREMENT_MASK;
}

nghttp2_settings_entry* nghttp2_frame_iv_copy(const nghttp2_settings_entry *iv,
                                              size_t niv)
{
  nghttp2_settings_entry *iv_copy;
  size_t len = niv*sizeof(nghttp2_settings_entry);
  iv_copy = malloc(len);
  if(iv_copy == NULL) {
    return NULL;
  }
  memcpy(iv_copy, iv, len);
  return iv_copy;
}

int nghttp2_nv_equal(const nghttp2_nv *a, const nghttp2_nv *b)
{
  return a->namelen == b->namelen && a->valuelen == b->valuelen &&
    memcmp(a->name, b->name, a->namelen) == 0 &&
    memcmp(a->value, b->value, a->valuelen) == 0;
}

void nghttp2_nv_array_del(nghttp2_nv *nva)
{
  free(nva);
}

static int bytes_compar(const uint8_t *a, size_t alen,
                        const uint8_t *b, size_t blen)
{
  if(alen == blen) {
    return memcmp(a, b, alen);
  } else if(alen < blen) {
    int rv = memcmp(a, b, alen);
    if(rv == 0) {
      return -1;
    } else {
      return rv;
    }
  } else {
    int rv = memcmp(a, b, blen);
    if(rv == 0) {
      return 1;
    } else {
      return rv;
    }
  }
}

int nghttp2_nv_compare_name(const nghttp2_nv *lhs, const nghttp2_nv *rhs)
{
  return bytes_compar(lhs->name, lhs->namelen, rhs->name, rhs->namelen);
}

static int nv_compar(const void *lhs, const void *rhs)
{
  const nghttp2_nv *a = (const nghttp2_nv*)lhs;
  const nghttp2_nv *b = (const nghttp2_nv*)rhs;
  int rv;
  rv = bytes_compar(a->name, a->namelen, b->name, b->namelen);
  if(rv == 0) {
    return bytes_compar(a->value, a->valuelen, b->value, b->valuelen);
  }
  return rv;
}

void nghttp2_nv_array_sort(nghttp2_nv *nva, size_t nvlen)
{
  qsort(nva, nvlen, sizeof(nghttp2_nv), nv_compar);
}

ssize_t nghttp2_nv_array_copy(nghttp2_nv **nva_ptr,
                              const nghttp2_nv *nva, size_t nvlen)
{
  size_t i;
  uint8_t *data;
  size_t buflen = 0;
  nghttp2_nv *p;
  for(i = 0; i < nvlen; ++i) {
    buflen += nva[i].namelen + nva[i].valuelen;
  }
  /* If all name/value pair is 0-length, remove them */
  if(nvlen == 0 || buflen == 0) {
    *nva_ptr = NULL;
    return 0;
  }
  buflen += sizeof(nghttp2_nv)*nvlen;
  *nva_ptr = malloc(buflen);
  if(*nva_ptr == NULL) {
    return NGHTTP2_ERR_NOMEM;
  }
  p = *nva_ptr;
  data = (uint8_t*)(*nva_ptr) + sizeof(nghttp2_nv)*nvlen;

  for(i = 0; i < nvlen; ++i) {
    memcpy(data, nva[i].name, nva[i].namelen);
    p->name = data;
    p->namelen = nva[i].namelen;
    nghttp2_downcase(p->name, p->namelen);
    data += nva[i].namelen;
    memcpy(data, nva[i].value, nva[i].valuelen);
    p->value = data;
    p->valuelen = nva[i].valuelen;
    data += nva[i].valuelen;
    ++p;
  }
  return nvlen;
}

int nghttp2_iv_check(const nghttp2_settings_entry *iv, size_t niv)
{
  size_t i;
  for(i = 0; i < niv; ++i) {
    switch(iv[i].settings_id) {
    case NGHTTP2_SETTINGS_HEADER_TABLE_SIZE:
    case NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS:
      break;
    case NGHTTP2_SETTINGS_ENABLE_PUSH:
      if(iv[i].value != 0 && iv[i].value != 1) {
        return 0;
      }
      break;
    case NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE:
      if(iv[i].value > (uint32_t)NGHTTP2_MAX_WINDOW_SIZE) {
        return 0;
      }
      break;
    default:
      return 0;
    }
  }
  return 1;
}

void nghttp2_frame_set_pad(nghttp2_buf *buf, uint8_t *flags_ptr, size_t padlen)
{
  size_t trail_padlen = 0;

  if(padlen > 256) {
    uint8_t *p;

    trail_padlen = padlen - 2;
    *flags_ptr |= NGHTTP2_FLAG_PAD_HIGH | NGHTTP2_FLAG_PAD_LOW;

    assert(nghttp2_buf_pos_offset(buf) >= 2);

    /* Consume previous 2 bytes, shifting 2 bytes to the left */
    nghttp2_buf_shift_left(buf, 2);

    p = buf->pos + NGHTTP2_FRAME_HDLEN;
    *p++ = trail_padlen >> 8;
    *p = trail_padlen & 0xff;

  } else if(padlen > 0) {
    assert(nghttp2_buf_pos_offset(buf) >= 1);

    /* Consume previous 1 byte, shifting 1 bytes to the left */
    nghttp2_buf_shift_left(buf, 1);

    trail_padlen = padlen - 1;
    *flags_ptr |= NGHTTP2_FLAG_PAD_LOW;

    *(buf->pos + NGHTTP2_FRAME_HDLEN) = trail_padlen;
  }
}
