/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "rocksdb/c.h"
#include "streamBackendRocksdb.h"
#include "tcommon.h"

int defaultKeyComp(void* state, const char* aBuf, size_t aLen, const char* bBuf, size_t bLen) {
  int ret = memcmp(aBuf, bBuf, aLen);
  if (ret == 0) {
    if (aLen < bLen)
      return -1;
    else if (aLen > bLen)
      return 1;
    else
      return 0;
  } else {
    return ret;
  }
}
int defaultKeyEncode(void* k, char* buf) {
  int len = strlen((char*)k);
  memcpy(buf, (char*)k, len);
  return len;
}
int defaultKeyDecode(void* k, char* buf) {
  int len = strlen(buf);
  memcpy(k, buf, len);
  return len;
}
int defaultKeyToString(void* k, char* buf) {
  // just to debug
  return sprintf(buf, "key: %s", (char*)k);
}
//
//  SStateKey
//  |--groupid--|---ts------|--opNum----|
//  |--uint64_t-|-uint64_t--|--int64_t--|
//
//
//
int stateKeyDBComp(void* state, const char* aBuf, size_t aLen, const char* bBuf, size_t bLen) {
  SStateKey key1, key2;
  memset(&key1, 0, sizeof(key1));
  memset(&key2, 0, sizeof(key2));

  char* p1 = (char*)aBuf;
  char* p2 = (char*)bBuf;

  p1 = taosDecodeFixedU64(p1, &key1.key.groupId);
  p2 = taosDecodeFixedU64(p2, &key2.key.groupId);

  p1 = taosDecodeFixedI64(p1, &key1.key.ts);
  p2 = taosDecodeFixedI64(p2, &key2.key.ts);

  taosDecodeFixedI64(p1, &key1.opNum);
  taosDecodeFixedI64(p2, &key2.opNum);

  return stateKeyCmpr(&key1, sizeof(key1), &key2, sizeof(key2));
}

int stateKeyEncode(void* k, char* buf) {
  SStateKey* key = k;
  int        len = 0;
  len += taosEncodeFixedU64((void**)&buf, key->key.groupId);
  len += taosEncodeFixedI64((void**)&buf, key->key.ts);
  len += taosEncodeFixedI64((void**)&buf, key->opNum);
  return len;
}
int stateKeyDecode(void* k, char* buf) {
  SStateKey* key = k;
  int        len = 0;
  char*      p = buf;
  p = taosDecodeFixedU64(p, &key->key.groupId);
  p = taosDecodeFixedI64(p, &key->key.ts);
  p = taosDecodeFixedI64(p, &key->opNum);
  return p - buf;
}

int stateKeyToString(void* k, char* buf) {
  SStateKey* key = k;
  int        n = 0;
  n += sprintf(buf + n, "[groupId:%" PRId64 ",", key->key.groupId);
  n += sprintf(buf + n, "ts:%" PRIi64 ",", key->key.ts);
  n += sprintf(buf + n, "opNum:%" PRIi64 "]", key->opNum);
  return n;
}

//
// SStateSessionKey
//  |-----------SSessionKey----------|
//  |-----STimeWindow-----|
//  |---skey--|---ekey----|--groupId-|--opNum--|
//  |---int64-|--int64_t--|--uint64--|--int64_t|
// |
//
int stateSessionKeyDBComp(void* state, const char* aBuf, size_t aLen, const char* bBuf, size_t bLen) {
  SStateSessionKey w1, w2;
  memset(&w1, 0, sizeof(w1));
  memset(&w2, 0, sizeof(w2));

  char* p1 = (char*)aBuf;
  char* p2 = (char*)bBuf;

  p1 = taosDecodeFixedI64(p1, &w1.key.win.skey);
  p2 = taosDecodeFixedI64(p2, &w2.key.win.skey);

  p1 = taosDecodeFixedI64(p1, &w1.key.win.ekey);
  p2 = taosDecodeFixedI64(p2, &w2.key.win.ekey);

  p1 = taosDecodeFixedU64(p1, &w1.key.groupId);
  p2 = taosDecodeFixedU64(p2, &w2.key.groupId);

  p1 = taosDecodeFixedI64(p1, &w1.opNum);
  p2 = taosDecodeFixedI64(p2, &w2.opNum);

  return stateSessionKeyCmpr(&w1, sizeof(w1), &w2, sizeof(w2));
}
int stateSessionKeyEncode(void* ses, char* buf) {
  SStateSessionKey* sess = ses;
  int               len = 0;
  len += taosEncodeFixedI64((void**)&buf, sess->key.win.skey);
  len += taosEncodeFixedI64((void**)&buf, sess->key.win.ekey);
  len += taosEncodeFixedU64((void**)&buf, sess->key.groupId);
  len += taosEncodeFixedI64((void**)&buf, sess->opNum);
  return len;
}
int stateSessionKeyDecode(void* ses, char* buf) {
  SStateSessionKey* sess = ses;
  int               len = 0;

  char* p = buf;
  p = taosDecodeFixedI64(p, &sess->key.win.skey);
  p = taosDecodeFixedI64(p, &sess->key.win.ekey);
  p = taosDecodeFixedU64(p, &sess->key.groupId);
  p = taosDecodeFixedI64(p, &sess->opNum);
  return p - buf;
}
int stateSessionKeyToString(void* k, char* buf) {
  SStateSessionKey* key = k;
  int               n = 0;
  n += sprintf(buf + n, "[skey:%" PRIi64 ",", key->key.win.skey);
  n += sprintf(buf + n, "ekey:%" PRIi64 ",", key->key.win.ekey);
  n += sprintf(buf + n, "groupId:%" PRIu64 ",", key->key.groupId);
  n += sprintf(buf + n, "opNum:%" PRIi64 "]", key->opNum);
  return n;
}

/**
 *  SWinKey
 *  |------groupId------|-----ts------|
 *  |------uint64-------|----int64----|
 */
int winKeyDBComp(void* state, const char* aBuf, size_t aLen, const char* bBuf, size_t bLen) {
  SWinKey w1, w2;
  memset(&w1, 0, sizeof(w1));
  memset(&w2, 0, sizeof(w2));

  char* p1 = (char*)aBuf;
  char* p2 = (char*)bBuf;

  p1 = taosDecodeFixedU64(p1, &w1.groupId);
  p2 = taosDecodeFixedU64(p2, &w2.groupId);

  p1 = taosDecodeFixedI64(p1, &w1.ts);
  p2 = taosDecodeFixedI64(p2, &w2.ts);

  return winKeyCmpr(&w1, sizeof(w1), &w2, sizeof(w2));
}

int winKeyEncode(void* k, char* buf) {
  SWinKey* key = k;
  int      len = 0;
  len += taosEncodeFixedU64((void**)&buf, key->groupId);
  len += taosEncodeFixedI64((void**)&buf, key->ts);
  return len;
}

int winKeyDecode(void* k, char* buf) {
  SWinKey* key = k;
  int      len = 0;
  char*    p = buf;
  p = taosDecodeFixedU64(p, &key->groupId);
  p = taosDecodeFixedI64(p, &key->ts);
  return len;
}

int winKeyToString(void* k, char* buf) {
  SWinKey* key = k;
  int      n = 0;
  n += sprintf(buf + n, "[groupId:%" PRIu64 ",", key->groupId);
  n += sprintf(buf + n, "ts:%" PRIi64 "]", key->ts);
  return n;
}
/*
 * STupleKey
 * |---groupId---|---ts---|---exprIdx---|
 * |---uint64--|---int64--|---int32-----|
 */
int tupleKeyDBComp(void* state, const char* aBuf, size_t aLen, const char* bBuf, size_t bLen) {
  STupleKey w1, w2;
  memset(&w1, 0, sizeof(w1));
  memset(&w2, 0, sizeof(w2));

  char* p1 = (char*)aBuf;
  char* p2 = (char*)bBuf;

  p1 = taosDecodeFixedU64(p1, &w1.groupId);
  p2 = taosDecodeFixedU64(p2, &w2.groupId);

  p1 = taosDecodeFixedI64(p1, &w1.ts);
  p2 = taosDecodeFixedI64(p2, &w2.ts);

  p1 = taosDecodeFixedI32(p1, &w1.exprIdx);
  p2 = taosDecodeFixedI32(p2, &w2.exprIdx);

  return STupleKeyCmpr(&w1, sizeof(w1), &w2, sizeof(w2));
}

int tupleKeyEncode(void* k, char* buf) {
  STupleKey* key = k;
  int        len = 0;
  len += taosEncodeFixedU64((void**)&buf, key->groupId);
  len += taosEncodeFixedI64((void**)&buf, key->ts);
  len += taosEncodeFixedI32((void**)&buf, key->exprIdx);
  return len;
}
int tupleKeyDecode(void* k, char* buf) {
  STupleKey* key = k;
  int        len = 0;
  char*      p = buf;
  p = taosDecodeFixedU64(p, &key->groupId);
  p = taosDecodeFixedI64(p, &key->ts);
  p = taosDecodeFixedI32(p, &key->exprIdx);
  return len;
}
int tupleKeyToString(void* k, char* buf) {
  int        n = 0;
  STupleKey* key = k;
  n += sprintf(buf + n, "[groupId:%" PRIu64 ",", key->groupId);
  n += sprintf(buf + n, "ts:%" PRIi64 ",", key->ts);
  n += sprintf(buf + n, "exprIdx:%d]", key->exprIdx);
  return n;
}

int parKeyDBComp(void* state, const char* aBuf, size_t aLen, const char* bBuf, size_t bLen) {
  int64_t w1, w2;
  memset(&w1, 0, sizeof(w1));
  memset(&w2, 0, sizeof(w2));
  char* p1 = (char*)aBuf;
  char* p2 = (char*)bBuf;

  taosDecodeFixedI64(p1, &w1);
  taosDecodeFixedI64(p2, &w2);
  if (w1 == w2) {
    return 0;
  } else {
    return w1 < w2 ? -1 : 1;
  }
}
int parKeyEncode(void* k, char* buf) {
  int64_t* groupid = k;
  int      len = taosEncodeFixedI64((void**)&buf, *groupid);
  return len;
}
int parKeyDecode(void* k, char* buf) {
  char*    p = buf;
  int64_t* groupid = k;

  p = taosDecodeFixedI64(p, groupid);
  return p - buf;
}
int parKeyToString(void* k, char* buf) {
  int64_t* key = k;
  int      n = 0;
  n = sprintf(buf + n, "[groupId:%" PRIi64 "]", *key);
  return n;
}
int stremaValueEncode(void* k, char* buf) {
  int           len = 0;
  SStreamValue* key = k;
  len += taosEncodeFixedI64((void**)&buf, key->unixTimestamp);
  len += taosEncodeFixedI32((void**)&buf, key->len);
  len += taosEncodeBinary((void**)&buf, key->data, key->len);
  return len;
}
int streamValueDecode(void* k, char* buf) {
  SStreamValue* key = k;
  char*         p = buf;
  p = taosDecodeFixedI64(p, &key->unixTimestamp);
  p = taosDecodeFixedI32(p, &key->len);
  p = taosDecodeBinary(p, (void**)&key->data, key->len);
  return p - buf;
}
int32_t streamValueToString(void* k, char* buf) {
  SStreamValue* key = k;
  int           n = 0;
  n += sprintf(buf + n, "[unixTimestamp:%" PRIi64 ",", key->unixTimestamp);
  n += sprintf(buf + n, "len:%d,", key->len);
  n += sprintf(buf + n, "data:%s]", key->data);
  return n;
}
/*1: stale, 0: no stale*/

int32_t streaValueIsStale(void* k, int64_t ts) {
  SStreamValue* key = k;
  if (key->unixTimestamp < ts) {
    return 1;
  }
  return 0;
}

typedef struct {
  void* tableOpt;
  void* lru;  // global or not
  void* filteFactory;

} rocksdbCfParam;
const char* cfName[] = {"default", "state", "fill", "sess", "func", "parname", "partag"};

typedef int (*EncodeFunc)(void* key, char* buf);
typedef int (*DecodeFunc)(void* key, char* buf);
typedef int (*ToStringFunc)(void* key, char* buf);
typedef const char* (*CompareName)(void* statue);
typedef int (*BackendCmpFunc)(void* state, const char* aBuf, size_t aLen, const char* bBuf, size_t bLen);
typedef void (*DestroyFunc)(void* state);
typedef int32_t (*EncodeValueFunc)(void* value, int32_t vlen, int64_t ttl, char** dest);
typedef int32_t (*DecodeValueFunc)(void* value, int32_t vlen, int64_t* ttl, char** dest);

const char* compareDefaultName(void* name);
const char* compareStateName(void* name);
const char* compareWinKeyName(void* name);
const char* compareSessionKeyName(void* name);
const char* compareFuncKeyName(void* name);
const char* compareParKeyName(void* name);
const char* comparePartagKeyName(void* name);

void destroyFunc(void* stata) { return; }

typedef struct {
  const char*     key;
  int32_t         len;
  int             idx;
  BackendCmpFunc  cmpFunc;
  EncodeFunc      enFunc;
  DecodeFunc      deFunc;
  ToStringFunc    toStrFunc;
  CompareName     cmpName;
  DestroyFunc     detroyFunc;
  EncodeValueFunc enValueFunc;
  DecodeValueFunc deValueFunc;

} SCfInit;
int32_t encodeValueFunc(void* value, int32_t vlen, int64_t ttl, char** dest) {
  SStreamValue key = {.unixTimestamp = ttl, .len = vlen, .data = (char*)(value)};

  char*   p = taosMemoryCalloc(1, sizeof(int64_t) + sizeof(int32_t) + key.len);
  char*   buf = p;
  int32_t len = 0;
  len += taosEncodeFixedI64((void**)&buf, key.unixTimestamp);
  len += taosEncodeFixedI32((void**)&buf, key.len);
  len += taosEncodeBinary((void**)&buf, (char*)value, vlen);
  *dest = p;
  return len;
}
int32_t decodeValueFunc(void* value, int32_t vlen, int64_t* ttl, char** dest) {
  SStreamValue key = {0};
  char*        p = value;
  p = taosDecodeFixedI64(p, &key.unixTimestamp);
  p = taosDecodeFixedI32(p, &key.len);
  p = taosDecodeBinary(p, (void**)&(key.data), key.len);

  *ttl = key.unixTimestamp;
  *dest = key.data;
  return key.len;
}
SCfInit ginitDict[] = {
    {"default", 7, 0, defaultKeyComp, defaultKeyEncode, defaultKeyDecode, defaultKeyToString, compareDefaultName,
     destroyFunc, encodeValueFunc, decodeValueFunc},
    {"state", 5, 1, stateKeyDBComp, stateKeyEncode, stateKeyDecode, stateKeyToString, compareStateName, destroyFunc,
     encodeValueFunc, decodeValueFunc},
    {"fill", 4, 2, winKeyDBComp, winKeyEncode, winKeyDecode, winKeyToString, compareWinKeyName, destroyFunc,
     encodeValueFunc, decodeValueFunc},
    {"sess", 4, 3, stateSessionKeyDBComp, stateSessionKeyEncode, stateSessionKeyDecode, stateSessionKeyToString,
     compareSessionKeyName, destroyFunc, encodeValueFunc, decodeValueFunc},
    {"func", 4, 4, tupleKeyDBComp, tupleKeyEncode, tupleKeyDecode, tupleKeyToString, compareFuncKeyName, destroyFunc,
     encodeValueFunc, decodeValueFunc},
    {"parname", 7, 5, parKeyDBComp, parKeyEncode, parKeyDecode, parKeyToString, compareParKeyName, destroyFunc,
     encodeValueFunc, decodeValueFunc},
    {"partag", 6, 6, parKeyDBComp, parKeyEncode, parKeyDecode, parKeyToString, comparePartagKeyName, destroyFunc,
     encodeValueFunc, decodeValueFunc},
};

const char* compareDefaultName(void* name) { return ginitDict[0].key; }
const char* compareStateName(void* name) { return ginitDict[1].key; }
const char* compareWinKeyName(void* name) { return ginitDict[2].key; }
const char* compareSessionKeyName(void* name) { return ginitDict[3].key; }
const char* compareFuncKeyName(void* name) { return ginitDict[4].key; }
const char* compareParKeyName(void* name) { return ginitDict[5].key; }
const char* comparePartagKeyName(void* name) { return ginitDict[6].key; }

typedef struct SCompactFilteFactory {
  void* status;
} SCompactFilteFactory;

void destroyCompactFilteFactory(void* arg) {
  if (arg == NULL) return;
}
const char* compactFilteFactoryName(void* arg) {
  SCompactFilteFactory* state = arg;
  return "stream_compact_filter";
}

void destroyCompactFilte(void* arg) {
  if (arg == NULL) return;
}
unsigned char compactFilte(void* arg, int level, const char* key, size_t klen, const char* val, size_t vlen,
                           char** newval, size_t* newvlen, unsigned char* value_changed) {
  int64_t      unixTime = taosGetTimestampMs();
  SStreamValue value;
  memset(&value, 0, sizeof(value));
  streamValueDecode(&value, (char*)val);
  taosMemoryFree(value.data);
  if (value.unixTimestamp != 0 && value.unixTimestamp < unixTime) {
    return 1;
  }
  return 0;
}
const char* compactFilteName(void* arg) { return "stream_filte"; }

rocksdb_compactionfilter_t* compactFilteFactoryCreateFilter(void* arg, rocksdb_compactionfiltercontext_t* ctx) {
  SCompactFilteFactory*       state = arg;
  rocksdb_compactionfilter_t* filter =
      rocksdb_compactionfilter_create(NULL, destroyCompactFilte, compactFilte, compactFilteName);
  return filter;
}
int streamInitBackend(SStreamState* pState, char* path) {
  rocksdb_env_t* env = rocksdb_create_default_env();  // rocksdb_envoptions_create();
  rocksdb_env_set_low_priority_background_threads(env, 4);
  rocksdb_env_set_high_priority_background_threads(env, 2);

  rocksdb_options_t* opts = rocksdb_options_create();
  rocksdb_options_set_env(opts, env);
  // rocksdb_options_increase_parallelism(opts, 8);
  //  rocksdb_options_optimize_level_style_compaction(opts, 0);
  //   create the DB if it's not already present
  rocksdb_options_set_create_if_missing(opts, 1);
  rocksdb_options_set_create_missing_column_families(opts, 1);
  rocksdb_options_set_write_buffer_size(opts, 128 << 20);

  rocksdb_compactionfilterfactory_t* factory = rocksdb_compactionfilterfactory_create(
      NULL, destroyCompactFilteFactory, compactFilteFactoryCreateFilter, compactFilteFactoryName);
  rocksdb_options_set_compaction_filter_factory(opts, factory);

  char* err = NULL;
  int   cfLen = sizeof(ginitDict) / sizeof(ginitDict[0]);

  rocksdbCfParam*           param = taosMemoryCalloc(cfLen, sizeof(rocksdbCfParam));
  const rocksdb_options_t** cfOpt = taosMemoryCalloc(cfLen, sizeof(rocksdb_options_t*));
  for (int i = 0; i < cfLen; i++) {
    cfOpt[i] = rocksdb_options_create_copy(opts);
    // refactor later
    rocksdb_block_based_table_options_t* tableOpt = rocksdb_block_based_options_create();
    rocksdb_cache_t*                     cache = rocksdb_cache_create_lru(32 << 20);
    rocksdb_block_based_options_set_block_cache(tableOpt, cache);

    rocksdb_filterpolicy_t* filter = rocksdb_filterpolicy_create_bloom(15);
    rocksdb_block_based_options_set_filter_policy(tableOpt, filter);

    rocksdb_options_set_block_based_table_factory((rocksdb_options_t*)cfOpt[i], tableOpt);

    param[i].tableOpt = tableOpt;
    param[i].lru = cache;
  };

  rocksdb_comparator_t** pCompare = taosMemoryCalloc(cfLen, sizeof(rocksdb_comparator_t**));
  for (int i = 0; i < cfLen; i++) {
    SCfInit*              cf = &ginitDict[i];
    rocksdb_comparator_t* compare = rocksdb_comparator_create(NULL, cf->detroyFunc, cf->cmpFunc, cf->cmpName);
    rocksdb_options_set_comparator((rocksdb_options_t*)cfOpt[i], compare);
    pCompare[i] = compare;
  }

  rocksdb_column_family_handle_t** cfHandle = taosMemoryMalloc(cfLen * sizeof(rocksdb_column_family_handle_t*));
  rocksdb_t*                       db = rocksdb_open_column_families(opts, path, cfLen, cfName, cfOpt, cfHandle, &err);

  pState->pTdbState->rocksdb = db;
  pState->pTdbState->pHandle = cfHandle;
  pState->pTdbState->writeOpts = rocksdb_writeoptions_create();
  // rocksdb_writeoptions_
  // rocksdb_writeoptions_set_no_slowdown(pState->pTdbState->writeOpts, 1);
  pState->pTdbState->readOpts = rocksdb_readoptions_create();
  pState->pTdbState->cfOpts = (rocksdb_options_t**)cfOpt;
  pState->pTdbState->pCompare = pCompare;
  pState->pTdbState->dbOpt = opts;
  pState->pTdbState->param = param;
  pState->pTdbState->env = env;
  pState->pTdbState->compactFactory = factory;
  return 0;
}
void streamCleanBackend(SStreamState* pState) {
  if (pState->pTdbState->rocksdb == NULL) {
    qInfo("rocksdb already free");
    return;
  }

  int             cfLen = sizeof(ginitDict) / sizeof(ginitDict[0]);
  rocksdbCfParam* param = pState->pTdbState->param;
  for (int i = 0; i < cfLen; i++) {
    rocksdb_column_family_handle_destroy(pState->pTdbState->pHandle[i]);
  }
  taosMemoryFreeClear(pState->pTdbState->pHandle);

  rocksdb_options_destroy(pState->pTdbState->dbOpt);

  rocksdb_writeoptions_destroy(pState->pTdbState->writeOpts);
  pState->pTdbState->writeOpts = NULL;

  rocksdb_readoptions_destroy(pState->pTdbState->readOpts);
  pState->pTdbState->readOpts = NULL;

  rocksdb_close(pState->pTdbState->rocksdb);
  // wait for all background work to finish
  for (int i = 0; i < cfLen; i++) {
    rocksdb_options_destroy(pState->pTdbState->cfOpts[i]);
    rocksdb_comparator_destroy(pState->pTdbState->pCompare[i]);

    rocksdb_cache_destroy(param[i].lru);
    rocksdb_block_based_options_destroy(param[i].tableOpt);
    // rocksdb_compactionfilterfactory_destroy(param[i].filteFactory);
  }
  taosMemoryFreeClear(pState->pTdbState->cfOpts);
  taosMemoryFree(pState->pTdbState->pCompare);
  taosMemoryFree(pState->pTdbState->param);
  rocksdb_env_destroy(pState->pTdbState->env);
  rocksdb_compactionfilterfactory_destroy(pState->pTdbState->compactFactory);

  pState->pTdbState->rocksdb = NULL;
}

int streamGetInit(const char* funcName) {
  size_t len = strlen(funcName);
  for (int i = 0; i < sizeof(ginitDict) / sizeof(ginitDict[0]); i++) {
    if (len == ginitDict[i].len && strncmp(funcName, ginitDict[i].key, strlen(funcName)) == 0) {
      return i;
    }
  }
  return -1;
}
bool streamStateIterSeekAndValid(rocksdb_iterator_t* iter, char* buf, size_t len) {
  rocksdb_iter_seek(iter, buf, len);
  if (!rocksdb_iter_valid(iter)) {
    rocksdb_iter_seek_for_prev(iter, buf, len);
    if (!rocksdb_iter_valid(iter)) {
      return false;
    }
  }
  return true;
}
rocksdb_iterator_t* streamStateIterCreate(SStreamState* pState, const char* cfName, rocksdb_snapshot_t** snapshot,
                                          rocksdb_readoptions_t** readOpt) {
  int idx = streamGetInit(cfName);

  if (snapshot != NULL) {
    *snapshot = (rocksdb_snapshot_t*)rocksdb_create_snapshot(pState->pTdbState->rocksdb);
  }

  rocksdb_readoptions_t* rOpt = rocksdb_readoptions_create();
  *readOpt = rOpt;

  // rocksdb_readoptions_set_snapshot(rOpt, *snapshot);

  rocksdb_readoptions_set_fill_cache(rOpt, 0);

  return rocksdb_create_iterator_cf(pState->pTdbState->rocksdb, rOpt, pState->pTdbState->pHandle[idx]);
}

#define STREAM_STATE_PUT_ROCKSDB(pState, funcname, key, value, vLen)                                     \
  do {                                                                                                   \
    code = 0;                                                                                            \
    char  buf[128] = {0};                                                                                \
    char* err = NULL;                                                                                    \
    int   i = streamGetInit(funcname);                                                                   \
    if (i < 0) {                                                                                         \
      qWarn("streamState failed to get cf name: %s", funcname);                                          \
      code = -1;                                                                                         \
      break;                                                                                             \
    }                                                                                                    \
    char toString[128] = {0};                                                                            \
    if (qDebugFlag & DEBUG_TRACE) ginitDict[i].toStrFunc((void*)key, toString);                          \
    int32_t                         klen = ginitDict[i].enFunc((void*)key, buf);                         \
    rocksdb_column_family_handle_t* pHandle = pState->pTdbState->pHandle[ginitDict[i].idx];              \
    rocksdb_t*                      db = pState->pTdbState->rocksdb;                                     \
    rocksdb_writeoptions_t*         opts = pState->pTdbState->writeOpts;                                 \
    char*                           ttlV = NULL;                                                         \
    int32_t                         ttlVLen = ginitDict[i].enValueFunc((char*)value, vLen, 0, &ttlV);    \
    rocksdb_put_cf(db, opts, pHandle, (const char*)buf, klen, (const char*)ttlV, (size_t)ttlVLen, &err); \
    if (err != NULL) {                                                                                   \
      taosMemoryFree(err);                                                                               \
      qDebug("streamState str: %s failed to write to %s, err: %s", toString, funcname, err);             \
      code = -1;                                                                                         \
    } else {                                                                                             \
      qDebug("streamState str:%s succ to write to %s, valLen:%d", toString, funcname, vLen);             \
    }                                                                                                    \
    taosMemoryFree(ttlV);                                                                                \
  } while (0);

#define STREAM_STATE_GET_ROCKSDB(pState, funcname, key, pVal, vLen)                             \
  do {                                                                                          \
    code = 0;                                                                                   \
    char  buf[128] = {0};                                                                       \
    char* err = NULL;                                                                           \
    int   i = streamGetInit(funcname);                                                          \
    if (i < 0) {                                                                                \
      qWarn("streamState failed to get cf name: %s", funcname);                                 \
      code = -1;                                                                                \
      break;                                                                                    \
    }                                                                                           \
    char toString[128] = {0};                                                                   \
    if (qDebugFlag & DEBUG_TRACE) ginitDict[i].toStrFunc((void*)key, toString);                 \
    int32_t                         klen = ginitDict[i].enFunc((void*)key, buf);                \
    rocksdb_column_family_handle_t* pHandle = pState->pTdbState->pHandle[ginitDict[i].idx];     \
    rocksdb_t*                      db = pState->pTdbState->rocksdb;                            \
    rocksdb_readoptions_t*          opts = pState->pTdbState->readOpts;                         \
    size_t                          len = 0;                                                    \
    char* val = rocksdb_get_cf(db, opts, pHandle, (const char*)buf, klen, (size_t*)&len, &err); \
    if (val == NULL) {                                                                          \
      qDebug("streamState str: %s failed to read from %s, err: not exist", toString, funcname); \
      if (err != NULL) taosMemoryFree(err);                                                     \
      code = -1;                                                                                \
    } else {                                                                                    \
      char *  p = NULL, *end = NULL;                                                            \
      int64_t ttl;                                                                              \
      int32_t vlen = ginitDict[i].deValueFunc(val, len, &ttl, &p);                              \
      if (pVal != NULL) {                                                                       \
        *pVal = p;                                                                              \
      } else {                                                                                  \
        taosMemoryFree(p);                                                                      \
      }                                                                                         \
      if (vLen != NULL) *vLen = vlen;                                                           \
    }                                                                                           \
    if (err != NULL) {                                                                          \
      taosMemoryFree(err);                                                                      \
      qDebug("streamState str: %s failed to read from %s, err: %s", toString, funcname, err);   \
      code = -1;                                                                                \
    } else {                                                                                    \
      if (code == 0) qDebug("streamState str: %s succ to read from %s", toString, funcname);    \
    }                                                                                           \
  } while (0);

#define STREAM_STATE_DEL_ROCKSDB(pState, funcname, key)                                      \
  do {                                                                                       \
    code = 0;                                                                                \
    char  buf[128] = {0};                                                                    \
    char* err = NULL;                                                                        \
    int   i = streamGetInit(funcname);                                                       \
    if (i < 0) {                                                                             \
      qWarn("streamState failed to get cf name: %s", funcname);                              \
      code = -1;                                                                             \
      break;                                                                                 \
    }                                                                                        \
    char toString[128] = {0};                                                                \
    if (qDebugFlag & DEBUG_TRACE) ginitDict[i].toStrFunc((void*)key, toString);              \
    int32_t                         klen = ginitDict[i].enFunc((void*)key, buf);             \
    rocksdb_column_family_handle_t* pHandle = pState->pTdbState->pHandle[ginitDict[i].idx];  \
    rocksdb_t*                      db = pState->pTdbState->rocksdb;                         \
    rocksdb_writeoptions_t*         opts = pState->pTdbState->writeOpts;                     \
    rocksdb_delete_cf(db, opts, pHandle, (const char*)buf, klen, &err);                      \
    if (err != NULL) {                                                                       \
      qDebug("streamState str: %s failed to del from %s, err: %s", toString, funcname, err); \
      taosMemoryFree(err);                                                                   \
      code = -1;                                                                             \
    } else {                                                                                 \
      qDebug("streamState str: %s succ to del from %s", toString, funcname);                 \
    }                                                                                        \
  } while (0);

int32_t streamStateFuncPut_rocksdb(SStreamState* pState, const STupleKey* key, const void* value, int32_t vLen) {
  int code = 0;
  STREAM_STATE_PUT_ROCKSDB(pState, "func", key, (void*)value, vLen);
  return code;
}
int32_t streamStateFuncGet_rocksdb(SStreamState* pState, const STupleKey* key, void** pVal, int32_t* pVLen) {
  int code = 0;
  STREAM_STATE_GET_ROCKSDB(pState, "func", key, pVal, pVLen);
  return 0;
}
int32_t streamStateFuncDel_rocksdb(SStreamState* pState, const STupleKey* key) {
  int code = 0;
  STREAM_STATE_DEL_ROCKSDB(pState, "func", key);
  return 0;
}

int32_t streamStatePut_rocksdb(SStreamState* pState, const SWinKey* key, const void* value, int32_t vLen) {
  int code = 0;

  SStateKey sKey = {.key = *key, .opNum = pState->number};
  STREAM_STATE_PUT_ROCKSDB(pState, "state", &sKey, (void*)value, vLen);
  return code;
}
int32_t streamStatePutBatch_rocksdb(SStreamState* pState, void* pBatch) {
  char* err = NULL;
  rocksdb_write(pState->pTdbState->rocksdb, pState->pTdbState->writeOpts, (rocksdb_writebatch_t*)pBatch, &err);
  if (err != NULL) {
    qError("streamState failed to write batch, err:%s", err);
    taosMemoryFree(err);
    return -1;
  }
  return 0;
}

void* streamStateCreateBatch() {
  rocksdb_writebatch_t* pBatch = rocksdb_writebatch_create();
  return pBatch;
}
int32_t streamStateGetBatchSize(void* pBatch) {
  if (pBatch == NULL) return -1;

  return rocksdb_writebatch_count((rocksdb_writebatch_t*)pBatch);
}

void    streamStateClearBatch(void* pBatch) { rocksdb_writebatch_clear((rocksdb_writebatch_t*)pBatch); }
void    streamStateDestroyBatch(void* pBatch) { rocksdb_writebatch_destroy((rocksdb_writebatch_t*)pBatch); }
int32_t streamStatePutBatch(SStreamState* pState, const char* cfName, rocksdb_writebatch_t* pBatch, void* key,
                            void* val, int32_t vlen) {
  int i = streamGetInit(cfName);

  if (i < 0) {
    qError("streamState failed to put to cf name:%s", cfName);
    return -1;
  }
  char    buf[128] = {0};
  int32_t klen = ginitDict[i].enFunc((void*)key, buf);

  char*                           ttlV = NULL;
  int32_t                         ttlVLen = ginitDict[i].enValueFunc(val, vlen, 0, &ttlV);
  rocksdb_column_family_handle_t* pCf = pState->pTdbState->pHandle[ginitDict[i].idx];
  rocksdb_writebatch_put_cf((rocksdb_writebatch_t*)pBatch, pCf, buf, (size_t)klen, ttlV, (size_t)ttlVLen);
  taosMemoryFree(ttlV);
  return 0;
}

int32_t streamDefaultPut_rocksdb(SStreamState* pState, const void* key, void* pVal, int32_t pVLen) {
  int code = 0;
  STREAM_STATE_PUT_ROCKSDB(pState, "default", &key, pVal, pVLen);
  return code;
}
int32_t streamDefaultGet_rocksdb(SStreamState* pState, const void* key, void** pVal, int32_t* pVLen) {
  int code = 0;
  STREAM_STATE_GET_ROCKSDB(pState, "default", &key, pVal, pVLen);
  return code;
}
int32_t streamDefaultDel_rocksdb(SStreamState* pState, const void* key) {
  int code = 0;
  STREAM_STATE_DEL_ROCKSDB(pState, "default", &key);
  return code;
}

void* streamDefaultIterCreate_rocksdb(SStreamState* pState) {
  SStreamStateCur* pCur = taosMemoryCalloc(1, sizeof(SStreamStateCur));

  pCur->db = pState->pTdbState->rocksdb;
  pCur->iter = streamStateIterCreate(pState, "default", &pCur->snapshot, &pCur->readOpt);
  return pCur;
}
int32_t streamDefaultIterValid_rocksdb(void* iter) {
  SStreamStateCur* pCur = iter;
  bool             val = rocksdb_iter_valid(pCur->iter);

  return val ? 0 : -1;
}
void streamDefaultIterSeek_rocksdb(void* iter, const char* key) {
  SStreamStateCur* pCur = iter;
  rocksdb_iter_seek(pCur->iter, key, strlen(key));
}
void streamDefaultIterNext_rocksdb(void* iter) {
  SStreamStateCur* pCur = iter;
  rocksdb_iter_next(pCur->iter);
}
char* streamDefaultIterKey_rocksdb(void* iter, int32_t* len) {
  SStreamStateCur* pCur = iter;
  return (char*)rocksdb_iter_key(pCur->iter, (size_t*)len);
}
char* streamDefaultIterVal_rocksdb(void* iter, int32_t* len) {
  SStreamStateCur* pCur = iter;
  return (char*)rocksdb_iter_value(pCur->iter, (size_t*)len);
}
// typedef struct {
//   char* start;
//   char* end;
//   void* result;
// } StreamFilterArg;

// typedef int (*streamfilterFunc)(StreamFilterArg* arg);

// int32_t streamDefaultIterFilter_rocksdb(SStreamState* pState, streamfilterFunc filterFunc, StreamFilterArg* arg) {
//   int   code = 0;
//   char* err = NULL;

//   rocksdb_snapshot_t*    snapshot = NULL;
//   rocksdb_readoptions_t* readopts = NULL;
//   rocksdb_iterator_t*    pIter = streamStateIterCreate(pState, "default", &snapshot, &readopts);
//   if (pIter == NULL) {
//     return -1;
//   }
//   char*   start = arg->start;
//   char*   end = arg->end;
//   SArray* result = arg->result;

//   rocksdb_iter_seek(pIter, start, strlen(start));
//   while (rocksdb_iter_valid(pIter)) {
//     const char* key = rocksdb_iter_key(pIter, NULL);
//     if (end != NULL && strcmp(key, end) > 0) {
//       break;
//     }
//     if (strncmp(key, start, strlen(start)) == 0 && strlen(key) >= strlen(start) + 1) {
//       int64_t checkPoint = 0;
//       // if (sscanf(key + strlen(key), ":%" PRId64 "", &checkPoint) == 1) {
//       //   taosArrayPush(result, &checkPoint);
//       // }
//     } else {
//       break;
//     }
//     rocksdb_iter_next(pIter);
//   }
//   rocksdb_release_snapshot(pState->pTdbState->rocksdb, snapshot);
//   rocksdb_readoptions_destroy(readopts);
//   rocksdb_iter_destroy(pIter);
//   return code;
// }
int32_t streamDefaultIterGet_rocksdb(SStreamState* pState, const void* start, const void* end, SArray* result) {
  int   code = 0;
  char* err = NULL;

  rocksdb_snapshot_t*    snapshot = NULL;
  rocksdb_readoptions_t* readopts = NULL;
  rocksdb_iterator_t*    pIter = streamStateIterCreate(pState, "default", &snapshot, &readopts);
  if (pIter == NULL) {
    return -1;
  }

  rocksdb_iter_seek(pIter, start, strlen(start));
  while (rocksdb_iter_valid(pIter)) {
    const char* key = rocksdb_iter_key(pIter, NULL);
    if (end != NULL && strcmp(key, end) > 0) {
      break;
    }
    if (strncmp(key, start, strlen(start)) == 0 && strlen(key) >= strlen(start) + 1) {
      int64_t checkPoint = 0;
      if (sscanf(key + strlen(key), ":%" PRId64 "", &checkPoint) == 1) {
        taosArrayPush(result, &checkPoint);
      }
    } else {
      break;
    }
    rocksdb_iter_next(pIter);
  }
  rocksdb_release_snapshot(pState->pTdbState->rocksdb, snapshot);
  rocksdb_readoptions_destroy(readopts);
  rocksdb_iter_destroy(pIter);
  return code;
}

int32_t streamStateGet_rocksdb(SStreamState* pState, const SWinKey* key, void** pVal, int32_t* pVLen) {
  int       code = 0;
  SStateKey sKey = {.key = *key, .opNum = pState->number};
  STREAM_STATE_GET_ROCKSDB(pState, "state", &sKey, pVal, pVLen);
  return code;
}
// todo refactor
int32_t streamStateDel_rocksdb(SStreamState* pState, const SWinKey* key) {
  int       code = 0;
  SStateKey sKey = {.key = *key, .opNum = pState->number};
  STREAM_STATE_DEL_ROCKSDB(pState, "state", &sKey);
  return code;
}

// todo refactor
int32_t streamStateFillPut_rocksdb(SStreamState* pState, const SWinKey* key, const void* value, int32_t vLen) {
  int code = 0;

  STREAM_STATE_PUT_ROCKSDB(pState, "fill", key, value, vLen);
  return code;
}

int32_t streamStateFillGet_rocksdb(SStreamState* pState, const SWinKey* key, void** pVal, int32_t* pVLen) {
  int code = 0;
  STREAM_STATE_GET_ROCKSDB(pState, "fill", key, pVal, pVLen);
  return code;
}
int32_t streamStateFillDel_rocksdb(SStreamState* pState, const SWinKey* key) {
  int code = 0;
  STREAM_STATE_DEL_ROCKSDB(pState, "fill", key);
  return code;
}

// todo refactor
int32_t streamStateClear_rocksdb(SStreamState* pState) {
  qDebug("streamStateClear_rocksdb");

  SStateKey sKey = {.key = {.ts = 0, .groupId = 0}, .opNum = pState->number};
  SStateKey eKey = {.key = {.ts = INT64_MAX, .groupId = UINT64_MAX}, .opNum = pState->number};
  char      sKeyStr[128] = {0};
  char      eKeyStr[128] = {0};

  int sLen = stateKeyEncode(&sKey, sKeyStr);
  int eLen = stateKeyEncode(&eKey, eKeyStr);

  char toStringStart[128] = {0};
  char toStringEnd[128] = {0};
  if (qDebugFlag & DEBUG_TRACE) {
    stateKeyToString(&sKey, toStringStart);
    stateKeyToString(&eKey, toStringEnd);
  }

  char* err = NULL;
  rocksdb_delete_range_cf(pState->pTdbState->rocksdb, pState->pTdbState->writeOpts, pState->pTdbState->pHandle[1],
                          sKeyStr, sLen, eKeyStr, eLen, &err);
  // rocksdb_compact_range_cf(pState->pTdbState->rocksdb, pState->pTdbState->pHandle[0], sKeyStr, sLen, eKeyStr, eLen);
  if (err != NULL) {
    qWarn(
        "failed to delete range cf(state) err: %s, "
        "start: %s, end:%s",
        err, toStringStart, toStringEnd);
    taosMemoryFree(err);
  }

  // del one by one

  // char      buf[128] = {0};
  // char*     s = "null";
  // SWinKey   key = {.ts = 0, .groupId = 0};
  // SStateKey skey = {.key = key, .opNum = pState->number};
  // int       sLen = stateKeyEncode(&skey, buf);

  // streamStatePut_rocksdb(pState, &key, s, strlen(s));

  // rocksdb_readoptions_t* opt = NULL;
  // rocksdb_iterator_t*    iter = streamStateIterCreate(pState, "state", NULL, &opt);
  // rocksdb_iter_seek(iter, buf, sLen);

  // char* err = NULL;
  // while (rocksdb_iter_valid(iter)) {
  //   int32_t kLen = 0;
  //   char*   key = (char*)rocksdb_iter_key(iter, (size_t*)&kLen);

  //   SStateKey ckey = {0};
  //   stateKeyDecode((void*)&ckey, key);
  //   if (ckey.opNum != pState->number) {
  //     break;
  //   }
  //   if (stateKeyCmpr(&skey, sizeof(skey), &ckey, sizeof(ckey)) > 0) {
  //     break;
  //   }

  //   rocksdb_delete_cf(pState->pTdbState->rocksdb, pState->pTdbState->writeOpts, pState->pTdbState->pHandle[0], key,
  //                     kLen, &err);
  //   if (err != NULL) {
  //     taosMemoryFree(err);
  //   }
  //   if (rocksdb_iter_valid(iter)) rocksdb_iter_next(iter);
  // }
  // rocksdb_iter_destroy(iter);

  return 0;
}

int32_t streamStateSessionPut_rocksdb(SStreamState* pState, const SSessionKey* key, const void* value, int32_t vLen) {
  int              code = 0;
  SStateSessionKey sKey = {.key = *key, .opNum = pState->number};
  STREAM_STATE_PUT_ROCKSDB(pState, "sess", &sKey, value, vLen);
  if (code == -1) {
  }
  return code;
}
SStreamStateCur* streamStateSessionSeekKeyCurrentPrev_rocksdb(SStreamState* pState, const SSessionKey* key) {
  qDebug("streamStateSessionSeekKeyCurrentPrev_rocksdb");
  SStreamStateCur* pCur = taosMemoryCalloc(1, sizeof(SStreamStateCur));
  if (pCur == NULL) {
    return NULL;
  }
  pCur->number = pState->number;
  pCur->db = pState->pTdbState->rocksdb;
  pCur->iter = streamStateIterCreate(pState, "sess", &pCur->snapshot, &pCur->readOpt);

  char             buf[128] = {0};
  SStateSessionKey sKey = {.key = *key, .opNum = pState->number};
  int              len = stateSessionKeyEncode(&sKey, buf);
  if (!streamStateIterSeekAndValid(pCur->iter, buf, len)) {
    streamStateFreeCur(pCur);
    return NULL;
  }

  int32_t          c = 0;
  size_t           klen;
  const char*      iKey = rocksdb_iter_key(pCur->iter, &klen);
  SStateSessionKey curKey = {0};
  stateSessionKeyDecode(&curKey, (char*)iKey);
  if (stateSessionKeyCmpr(&sKey, sizeof(sKey), &curKey, sizeof(curKey)) >= 0) return pCur;

  rocksdb_iter_prev(pCur->iter);
  if (!rocksdb_iter_valid(pCur->iter)) {
    // qWarn("streamState failed to seek key prev
    // %s", toString);
    streamStateFreeCur(pCur);
    return NULL;
  }
  return pCur;
}
SStreamStateCur* streamStateSessionSeekKeyCurrentNext_rocksdb(SStreamState* pState, SSessionKey* key) {
  qDebug("streamStateSessionSeekKeyCurrentNext_rocksdb");
  SStreamStateCur* pCur = taosMemoryCalloc(1, sizeof(SStreamStateCur));
  if (pCur == NULL) {
    return NULL;
  }
  pCur->db = pState->pTdbState->rocksdb;
  pCur->iter = streamStateIterCreate(pState, "sess", &pCur->snapshot, &pCur->readOpt);
  pCur->number = pState->number;

  char buf[128] = {0};

  SStateSessionKey sKey = {.key = *key, .opNum = pState->number};
  int              len = stateSessionKeyEncode(&sKey, buf);
  if (!streamStateIterSeekAndValid(pCur->iter, buf, len)) {
    streamStateFreeCur(pCur);
    return NULL;
  }
  size_t           klen;
  const char*      iKey = rocksdb_iter_key(pCur->iter, &klen);
  SStateSessionKey curKey = {0};
  stateSessionKeyDecode(&curKey, (char*)iKey);
  if (stateSessionKeyCmpr(&sKey, sizeof(sKey), &curKey, sizeof(curKey)) <= 0) return pCur;

  rocksdb_iter_next(pCur->iter);
  if (!rocksdb_iter_valid(pCur->iter)) {
    streamStateFreeCur(pCur);
    return NULL;
  }
  return pCur;
}

SStreamStateCur* streamStateSessionSeekKeyNext_rocksdb(SStreamState* pState, const SSessionKey* key) {
  qDebug("streamStateSessionSeekKeyNext_rocksdb");
  SStreamStateCur* pCur = taosMemoryCalloc(1, sizeof(SStreamStateCur));
  if (pCur == NULL) {
    return NULL;
  }
  pCur->db = pState->pTdbState->rocksdb;
  pCur->iter = streamStateIterCreate(pState, "sess", &pCur->snapshot, &pCur->readOpt);
  pCur->number = pState->number;

  SStateSessionKey sKey = {.key = *key, .opNum = pState->number};

  char buf[128] = {0};
  int  len = stateSessionKeyEncode(&sKey, buf);
  if (!streamStateIterSeekAndValid(pCur->iter, buf, len)) {
    streamStateFreeCur(pCur);
    return NULL;
  }

  size_t           klen;
  const char*      iKey = rocksdb_iter_key(pCur->iter, &klen);
  SStateSessionKey curKey = {0};
  stateSessionKeyDecode(&curKey, (char*)iKey);
  if (stateSessionKeyCmpr(&sKey, sizeof(sKey), &curKey, sizeof(curKey)) < 0) return pCur;

  rocksdb_iter_next(pCur->iter);
  if (!rocksdb_iter_valid(pCur->iter)) {
    streamStateFreeCur(pCur);
    return NULL;
  }
  return pCur;
}

int32_t streamStateAddIfNotExist_rocksdb(SStreamState* pState, const SWinKey* key, void** pVal, int32_t* pVLen) {
  qDebug("streamStateAddIfNotExist_rocksdb");
  int32_t size = *pVLen;
  if (streamStateGet_rocksdb(pState, key, pVal, pVLen) == 0) {
    return 0;
  }
  *pVal = taosMemoryMalloc(size);
  memset(*pVal, 0, size);
  return 0;
}
SStreamStateCur* streamStateSeekToLast_rocksdb(SStreamState* pState, const SWinKey* key) {
  qDebug("streamStateGetCur_rocksdb");
  int32_t         code = 0;
  const SStateKey maxStateKey = {.key = {.groupId = UINT64_MAX, .ts = INT64_MAX}, .opNum = INT64_MAX};
  STREAM_STATE_PUT_ROCKSDB(pState, "state", &maxStateKey, "", 0);
  char    buf[128] = {0};
  int32_t klen = stateKeyEncode((void*)&maxStateKey, buf);

  SStreamStateCur* pCur = taosMemoryCalloc(1, sizeof(SStreamStateCur));
  if (pCur == NULL) return NULL;
  pCur->db = pState->pTdbState->rocksdb;
  pCur->iter = streamStateIterCreate(pState, "state", &pCur->snapshot, &pCur->readOpt);
  rocksdb_iter_seek(pCur->iter, buf, (size_t)klen);
  rocksdb_iter_prev(pCur->iter);

  STREAM_STATE_DEL_ROCKSDB(pState, "state", &maxStateKey);
  return pCur;
}
SStreamStateCur* streamStateGetCur_rocksdb(SStreamState* pState, const SWinKey* key) {
  qDebug("streamStateGetCur_rocksdb");
  SStreamStateCur* pCur = taosMemoryCalloc(1, sizeof(SStreamStateCur));

  if (pCur == NULL) return NULL;
  pCur->db = pState->pTdbState->rocksdb;
  pCur->iter = streamStateIterCreate(pState, "state", &pCur->snapshot, &pCur->readOpt);

  SStateKey sKey = {.key = *key, .opNum = pState->number};
  char      buf[128] = {0};
  int       len = stateKeyEncode((void*)&sKey, buf);

  rocksdb_iter_seek(pCur->iter, buf, len);
  if (rocksdb_iter_valid(pCur->iter)) {
    SStateKey curKey;
    size_t    kLen = 0;
    char*     keyStr = (char*)rocksdb_iter_key(pCur->iter, &kLen);
    stateKeyDecode((void*)&curKey, keyStr);

    if (stateKeyCmpr(&sKey, sizeof(sKey), &curKey, sizeof(curKey)) == 0) {
      pCur->number = pState->number;
      return pCur;
    }
  }
  streamStateFreeCur(pCur);
  return NULL;
}

SStreamStateCur* streamStateGetAndCheckCur_rocksdb(SStreamState* pState, SWinKey* key) {
  qDebug("streamStateGetAndCheckCur_rocksdb");
  SStreamStateCur* pCur = streamStateFillGetCur_rocksdb(pState, key);
  if (pCur) {
    int32_t code = streamStateGetGroupKVByCur_rocksdb(pCur, key, NULL, 0);
    if (code == 0) return pCur;
    streamStateFreeCur(pCur);
  }
  return NULL;
}
int32_t streamStateGetKVByCur_rocksdb(SStreamStateCur* pCur, SWinKey* pKey, const void** pVal, int32_t* pVLen) {
  qDebug("streamStateGetKVByCur_rocksdb");
  if (!pCur) return -1;
  SStateKey  tkey;
  SStateKey* pKtmp = &tkey;

  if (rocksdb_iter_valid(pCur->iter)) {
    size_t tlen;
    char*  keyStr = (char*)rocksdb_iter_key(pCur->iter, &tlen);
    stateKeyDecode((void*)pKtmp, keyStr);
    if (pKtmp->opNum != pCur->number) {
      return -1;
    }
    size_t vlen = 0;
    if (pVal != NULL) *pVal = (char*)rocksdb_iter_value(pCur->iter, &vlen);
    if (pVLen != NULL) *pVLen = vlen;
    *pKey = pKtmp->key;
    return 0;
  }
  return -1;
}
SStreamStateCur* streamStateFillGetCur_rocksdb(SStreamState* pState, const SWinKey* key) {
  qDebug("streamStateFillGetCur_rocksdb");
  SStreamStateCur* pCur = taosMemoryCalloc(1, sizeof(SStreamStateCur));

  if (pCur == NULL) return NULL;

  pCur->db = pState->pTdbState->rocksdb;
  pCur->iter = streamStateIterCreate(pState, "fill", &pCur->snapshot, &pCur->readOpt);

  char buf[128] = {0};
  int  len = winKeyEncode((void*)key, buf);
  if (!streamStateIterSeekAndValid(pCur->iter, buf, len)) {
    streamStateFreeCur(pCur);
    return NULL;
  }

  if (rocksdb_iter_valid(pCur->iter)) {
    size_t  kLen;
    SWinKey curKey;
    char*   keyStr = (char*)rocksdb_iter_key(pCur->iter, &kLen);
    winKeyDecode((void*)&curKey, keyStr);
    if (winKeyCmpr(key, sizeof(*key), &curKey, sizeof(curKey)) == 0) {
      return pCur;
    }
  }

  streamStateFreeCur(pCur);
  return NULL;
}
int32_t streamStateFillGetKVByCur_rocksdb(SStreamStateCur* pCur, SWinKey* pKey, const void** pVal, int32_t* pVLen) {
  qDebug("streamStateFillGetKVByCur_rocksdb");
  if (!pCur) {
    return -1;
  }
  SWinKey winKey;
  if (rocksdb_iter_valid(pCur->iter)) {
    size_t tlen;
    char*  keyStr = (char*)rocksdb_iter_key(pCur->iter, &tlen);
    winKeyDecode(&winKey, keyStr);

    size_t      vlen = 0;
    const char* valStr = rocksdb_iter_value(pCur->iter, &vlen);
    if (pVal != NULL) {
      char* dst = taosMemoryCalloc(1, vlen);
      memcpy(dst, valStr, vlen);
      *pVal = dst;
    }
    if (pVLen != NULL) *pVLen = vlen;

  } else {
    return -1;
  }
  *pKey = winKey;
  return 0;
}
int32_t streamStateGetGroupKVByCur_rocksdb(SStreamStateCur* pCur, SWinKey* pKey, const void** pVal, int32_t* pVLen) {
  qDebug("streamStateGetGroupKVByCur_rocksdb");
  if (!pCur) {
    return -1;
  }
  uint64_t groupId = pKey->groupId;

  int32_t code = streamStateFillGetKVByCur_rocksdb(pCur, pKey, pVal, pVLen);
  if (code == 0) {
    if (pKey->groupId == groupId) {
      return 0;
    }
  }
  return -1;
}
int32_t streamStateGetFirst_rocksdb(SStreamState* pState, SWinKey* key) {
  qDebug("streamStateGetFirst_rocksdb");
  SWinKey tmp = {.ts = 0, .groupId = 0};
  streamStatePut_rocksdb(pState, &tmp, NULL, 0);
  SStreamStateCur* pCur = streamStateSeekKeyNext_rocksdb(pState, &tmp);
  int32_t          code = streamStateGetKVByCur_rocksdb(pCur, key, NULL, 0);
  streamStateFreeCur(pCur);
  streamStateDel_rocksdb(pState, &tmp);
  return code;
}
int32_t streamStateSessionGetKVByCur_rocksdb(SStreamStateCur* pCur, SSessionKey* pKey, void** pVal, int32_t* pVLen) {
  qDebug("streamStateSessionGetKVByCur_rocksdb");
  if (!pCur) {
    return -1;
  }
  SStateSessionKey ktmp = {0};
  size_t           kLen = 0, vLen = 0;

  if (!rocksdb_iter_valid(pCur->iter)) {
    return -1;
  }
  const char* curKey = rocksdb_iter_key(pCur->iter, (size_t*)&kLen);
  stateSessionKeyDecode((void*)&ktmp, (char*)curKey);

  SStateSessionKey* pKTmp = &ktmp;
  const char*       val = rocksdb_iter_value(pCur->iter, (size_t*)&vLen);
  if (pVal != NULL) {
    *pVal = (char*)val;
  }
  if (pVLen != NULL) *pVLen = vLen;

  if (pKTmp->opNum != pCur->number) {
    return -1;
  }
  if (pKey->groupId != 0 && pKey->groupId != pKTmp->key.groupId) {
    return -1;
  }
  *pKey = pKTmp->key;
  return 0;
}

SStreamStateCur* streamStateSeekKeyNext_rocksdb(SStreamState* pState, const SWinKey* key) {
  qDebug("streamStateSeekKeyNext_rocksdb");
  SStreamStateCur* pCur = taosMemoryCalloc(1, sizeof(SStreamStateCur));
  if (pCur == NULL) {
    return NULL;
  }
  pCur->number = pState->number;
  pCur->db = pState->pTdbState->rocksdb;
  pCur->iter = streamStateIterCreate(pState, "state", &pCur->snapshot, &pCur->readOpt);

  SStateKey sKey = {.key = *key, .opNum = pState->number};
  char      buf[128] = {0};
  int       len = stateKeyEncode((void*)&sKey, buf);
  if (!streamStateIterSeekAndValid(pCur->iter, buf, len)) {
    streamStateFreeCur(pCur);
    return NULL;
  }

  if (rocksdb_iter_valid(pCur->iter)) {
    SStateKey curKey;
    size_t    kLen;
    char*     keyStr = (char*)rocksdb_iter_key(pCur->iter, &kLen);
    stateKeyDecode((void*)&curKey, keyStr);
    if (stateKeyCmpr(&sKey, sizeof(sKey), &curKey, sizeof(curKey)) > 0) {
      return pCur;
    }
    rocksdb_iter_next(pCur->iter);
    return pCur;
  }
  streamStateFreeCur(pCur);
  return NULL;
}
SStreamStateCur* streamStateFillSeekKeyNext_rocksdb(SStreamState* pState, const SWinKey* key) {
  qDebug("streamStateFillSeekKeyNext_rocksdb");
  SStreamStateCur* pCur = taosMemoryCalloc(1, sizeof(SStreamStateCur));
  if (!pCur) {
    return NULL;
  }

  pCur->db = pState->pTdbState->rocksdb;
  pCur->iter = streamStateIterCreate(pState, "fill", &pCur->snapshot, &pCur->readOpt);

  char buf[128] = {0};
  int  len = winKeyEncode((void*)key, buf);
  if (!streamStateIterSeekAndValid(pCur->iter, buf, len)) {
    streamStateFreeCur(pCur);
    return NULL;
  }

  {
    SWinKey curKey;
    size_t  kLen = 0;
    char*   keyStr = (char*)rocksdb_iter_key(pCur->iter, &kLen);
    winKeyDecode((void*)&curKey, keyStr);
    if (winKeyCmpr(key, sizeof(*key), &curKey, sizeof(curKey)) > 0) {
      return pCur;
    }
    rocksdb_iter_next(pCur->iter);
    return pCur;
  }
  streamStateFreeCur(pCur);
  return NULL;
}
SStreamStateCur* streamStateFillSeekKeyPrev_rocksdb(SStreamState* pState, const SWinKey* key) {
  qDebug("streamStateFillSeekKeyPrev_rocksdb");
  SStreamStateCur* pCur = taosMemoryCalloc(1, sizeof(SStreamStateCur));
  if (pCur == NULL) {
    return NULL;
  }

  pCur->db = pState->pTdbState->rocksdb;
  pCur->iter = streamStateIterCreate(pState, "fill", &pCur->snapshot, &pCur->readOpt);

  char buf[128] = {0};
  int  len = winKeyEncode((void*)key, buf);
  if (!streamStateIterSeekAndValid(pCur->iter, buf, len)) {
    streamStateFreeCur(pCur);
    return NULL;
  }

  {
    SWinKey curKey;
    size_t  kLen = 0;
    char*   keyStr = (char*)rocksdb_iter_key(pCur->iter, &kLen);
    winKeyDecode((void*)&curKey, keyStr);
    if (winKeyCmpr(key, sizeof(*key), &curKey, sizeof(curKey)) < 0) {
      return pCur;
    }
    rocksdb_iter_prev(pCur->iter);
    return pCur;
  }
  streamStateFreeCur(pCur);
  return NULL;
}
int32_t streamStateCurPrev_rocksdb(SStreamState* pState, SStreamStateCur* pCur) {
  qDebug("streamStateCurPrev_rocksdb");
  if (!pCur) return -1;

  rocksdb_iter_prev(pCur->iter);
  return 0;
}
int32_t streamStateCurNext_rocksdb(SStreamState* pState, SStreamStateCur* pCur) {
  if (!pCur) {
    return -1;
  }
  rocksdb_iter_next(pCur->iter);
  return 0;
}
int32_t streamStateSessionGetKeyByRange_rocksdb(SStreamState* pState, const SSessionKey* key, SSessionKey* curKey) {
  qDebug("streamStateSessionGetKeyByRange_rocksdb");
  SStreamStateCur* pCur = taosMemoryCalloc(1, sizeof(SStreamStateCur));
  if (pCur == NULL) {
    return -1;
  }
  pCur->number = pState->number;
  pCur->db = pState->pTdbState->rocksdb;
  pCur->iter = streamStateIterCreate(pState, "sess", &pCur->snapshot, &pCur->readOpt);

  SStateSessionKey sKey = {.key = *key, .opNum = pState->number};
  int32_t          c = 0;
  char             buf[128] = {0};
  int              len = stateSessionKeyEncode(&sKey, buf);
  if (!streamStateIterSeekAndValid(pCur->iter, buf, len)) {
    streamStateFreeCur(pCur);
    return -1;
  }

  size_t           kLen;
  const char*      iKeyStr = rocksdb_iter_key(pCur->iter, (size_t*)&kLen);
  SStateSessionKey iKey = {0};
  stateSessionKeyDecode(&iKey, (char*)iKeyStr);

  c = stateSessionKeyCmpr(&sKey, sizeof(sKey), &iKey, sizeof(iKey));

  SSessionKey resKey = *key;
  int32_t     code = streamStateSessionGetKVByCur_rocksdb(pCur, &resKey, NULL, 0);
  if (code == 0 && sessionRangeKeyCmpr(key, &resKey) == 0) {
    *curKey = resKey;
    streamStateFreeCur(pCur);
    return code;
  }

  if (c > 0) {
    streamStateCurNext_rocksdb(pState, pCur);
    code = streamStateSessionGetKVByCur_rocksdb(pCur, &resKey, NULL, 0);
    if (code == 0 && sessionRangeKeyCmpr(key, &resKey) == 0) {
      *curKey = resKey;
      streamStateFreeCur(pCur);
      return code;
    }
  } else if (c < 0) {
    streamStateCurPrev(pState, pCur);
    code = streamStateSessionGetKVByCur_rocksdb(pCur, &resKey, NULL, 0);
    if (code == 0 && sessionRangeKeyCmpr(key, &resKey) == 0) {
      *curKey = resKey;
      streamStateFreeCur(pCur);
      return code;
    }
  }

  streamStateFreeCur(pCur);
  return -1;
}

int32_t streamStateSessionGet_rocksdb(SStreamState* pState, SSessionKey* key, void** pVal, int32_t* pVLen) {
  qDebug("streamStateSessionGet_rocksdb");
  int              code = 0;
  SStreamStateCur* pCur = streamStateSessionSeekKeyCurrentNext_rocksdb(pState, key);
  SSessionKey      resKey = *key;
  void*            tmp = NULL;
  int32_t          vLen = 0;
  code = streamStateSessionGetKVByCur_rocksdb(pCur, &resKey, &tmp, &vLen);
  if (code == 0) {
    if (pVLen != NULL) *pVLen = vLen;

    if (key->win.skey != resKey.win.skey) {
      code = -1;
    } else {
      *key = resKey;
      *pVal = taosMemoryCalloc(1, *pVLen);
      memcpy(*pVal, tmp, *pVLen);
    }
  }
  streamStateFreeCur(pCur);
  // impl later
  return code;
}

int32_t streamStateSessionDel_rocksdb(SStreamState* pState, const SSessionKey* key) {
  int              code = 0;
  SStateSessionKey sKey = {.key = *key, .opNum = pState->number};
  STREAM_STATE_DEL_ROCKSDB(pState, "sess", &sKey);
  return code;
}
int32_t streamStateSessionAddIfNotExist_rocksdb(SStreamState* pState, SSessionKey* key, TSKEY gap, void** pVal,
                                                int32_t* pVLen) {
  qDebug("streamStateSessionAddIfNotExist_rocksdb");
  // todo refactor
  int32_t     res = 0;
  SSessionKey originKey = *key;
  SSessionKey searchKey = *key;
  searchKey.win.skey = key->win.skey - gap;
  searchKey.win.ekey = key->win.ekey + gap;
  int32_t valSize = *pVLen;

  void* tmp = taosMemoryMalloc(valSize);

  SStreamStateCur* pCur = streamStateSessionSeekKeyCurrentPrev_rocksdb(pState, key);
  if (pCur == NULL) {
  }
  int32_t code = streamStateSessionGetKVByCur_rocksdb(pCur, key, pVal, pVLen);

  if (code == 0) {
    if (sessionRangeKeyCmpr(&searchKey, key) == 0) {
      memcpy(tmp, *pVal, valSize);
      streamStateSessionDel_rocksdb(pState, key);
      goto _end;
    }
    streamStateCurNext_rocksdb(pState, pCur);
  } else {
    *key = originKey;
    streamStateFreeCur(pCur);
    pCur = streamStateSessionSeekKeyNext_rocksdb(pState, key);
  }

  code = streamStateSessionGetKVByCur_rocksdb(pCur, key, pVal, pVLen);
  if (code == 0) {
    if (sessionRangeKeyCmpr(&searchKey, key) == 0) {
      memcpy(tmp, *pVal, valSize);
      streamStateSessionDel_rocksdb(pState, key);
      goto _end;
    }
  }

  *key = originKey;
  res = 1;
  memset(tmp, 0, valSize);

_end:

  *pVal = tmp;
  streamStateFreeCur(pCur);
  return res;
}
int32_t streamStateStateAddIfNotExist_rocksdb(SStreamState* pState, SSessionKey* key, char* pKeyData,
                                              int32_t keyDataLen, state_key_cmpr_fn fn, void** pVal, int32_t* pVLen) {
  qDebug("streamStateStateAddIfNotExist_rocksdb");
  // todo refactor
  int32_t     res = 0;
  SSessionKey tmpKey = *key;
  int32_t     valSize = *pVLen;
  void*       tmp = taosMemoryMalloc(valSize);
  // tdbRealloc(NULL, valSize);
  if (!tmp) {
    return -1;
  }

  SStreamStateCur* pCur = streamStateSessionSeekKeyCurrentPrev_rocksdb(pState, key);
  int32_t          code = streamStateSessionGetKVByCur_rocksdb(pCur, key, pVal, pVLen);
  if (code == 0) {
    if (key->win.skey <= tmpKey.win.skey && tmpKey.win.ekey <= key->win.ekey) {
      memcpy(tmp, *pVal, valSize);
      streamStateSessionDel_rocksdb(pState, key);
      goto _end;
    }

    void* stateKey = (char*)(*pVal) + (valSize - keyDataLen);
    if (fn(pKeyData, stateKey) == true) {
      memcpy(tmp, *pVal, valSize);
      streamStateSessionDel_rocksdb(pState, key);
      goto _end;
    }

    streamStateCurNext_rocksdb(pState, pCur);
  } else {
    *key = tmpKey;
    streamStateFreeCur(pCur);
    pCur = streamStateSessionSeekKeyNext_rocksdb(pState, key);
  }

  code = streamStateSessionGetKVByCur_rocksdb(pCur, key, pVal, pVLen);
  if (code == 0) {
    void* stateKey = (char*)(*pVal) + (valSize - keyDataLen);
    if (fn(pKeyData, stateKey) == true) {
      memcpy(tmp, *pVal, valSize);
      streamStateSessionDel_rocksdb(pState, key);
      goto _end;
    }
  }

  *key = tmpKey;
  res = 1;
  memset(tmp, 0, valSize);

_end:

  *pVal = tmp;
  streamStateFreeCur(pCur);
  return res;
}

int32_t streamStateSessionClear_rocksdb(SStreamState* pState) {
  qDebug("streamStateSessionClear_rocksdb");
  SSessionKey      key = {.win.skey = 0, .win.ekey = 0, .groupId = 0};
  SStreamStateCur* pCur = streamStateSessionSeekKeyCurrentNext_rocksdb(pState, &key);

  while (1) {
    SSessionKey delKey = {0};
    void*       buf = NULL;
    int32_t     size = 0;
    int32_t     code = streamStateSessionGetKVByCur_rocksdb(pCur, &delKey, &buf, &size);
    if (code == 0 && size > 0) {
      memset(buf, 0, size);
      streamStateSessionPut_rocksdb(pState, &delKey, buf, size);
    } else {
      break;
    }
    streamStateCurNext_rocksdb(pState, pCur);
  }
  streamStateFreeCur(pCur);
  return -1;
}
int32_t streamStatePutParTag_rocksdb(SStreamState* pState, int64_t groupId, const void* tag, int32_t tagLen) {
  int code = 0;
  STREAM_STATE_PUT_ROCKSDB(pState, "partag", &groupId, tag, tagLen);
  return code;
}

int32_t streamStateGetParTag_rocksdb(SStreamState* pState, int64_t groupId, void** tagVal, int32_t* tagLen) {
  int code = 0;
  STREAM_STATE_GET_ROCKSDB(pState, "partag", &groupId, tagVal, tagLen);
  return code;
}

int32_t streamStatePutParName_rocksdb(SStreamState* pState, int64_t groupId, const char tbname[TSDB_TABLE_NAME_LEN]) {
  int code = 0;
  STREAM_STATE_PUT_ROCKSDB(pState, "parname", &groupId, (char*)tbname, TSDB_TABLE_NAME_LEN);
  return code;
}
int32_t streamStateGetParName_rocksdb(SStreamState* pState, int64_t groupId, void** pVal) {
  int    code = 0;
  size_t tagLen;
  STREAM_STATE_GET_ROCKSDB(pState, "parname", &groupId, pVal, &tagLen);
  return code;
}

void streamStateDestroy_rocksdb(SStreamState* pState) {
  // only close db
  streamCleanBackend(pState);
}