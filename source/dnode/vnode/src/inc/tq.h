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

#ifndef _TD_VNODE_TQ_H_
#define _TD_VNODE_TQ_H_

#include "vnodeInt.h"

#include "executor.h"
#include "os.h"
#include "thash.h"
#include "tmsg.h"
#include "tqueue.h"
#include "trpc.h"
#include "ttimer.h"
#include "wal.h"

#ifdef __cplusplus
extern "C" {
#endif

// tqDebug ===================
// clang-format off
#define tqFatal(...) do { if (tqDebugFlag & DEBUG_FATAL) { taosPrintLog("TQ  FATAL ", DEBUG_FATAL, 255, __VA_ARGS__); }}     while(0)
#define tqError(...) do { if (tqDebugFlag & DEBUG_ERROR) { taosPrintLog("TQ  ERROR ", DEBUG_ERROR, 255, __VA_ARGS__); }}     while(0)
#define tqWarn(...)  do { if (tqDebugFlag & DEBUG_WARN)  { taosPrintLog("TQ  WARN ", DEBUG_WARN, 255, __VA_ARGS__); }}       while(0)
#define tqInfo(...)  do { if (tqDebugFlag & DEBUG_INFO)  { taosPrintLog("TQ  ", DEBUG_INFO, 255, __VA_ARGS__); }}            while(0)
#define tqDebug(...) do { if (tqDebugFlag & DEBUG_DEBUG) { taosPrintLog("TQ  ", DEBUG_DEBUG, tqDebugFlag, __VA_ARGS__); }} while(0)
#define tqTrace(...) do { if (tqDebugFlag & DEBUG_TRACE) { taosPrintLog("TQ  ", DEBUG_TRACE, tqDebugFlag, __VA_ARGS__); }} while(0)
// clang-format on

typedef struct STqOffsetCfg   STqOffsetCfg;
typedef struct STqOffsetStore STqOffsetStore;

// tqRead

struct STqReadHandle {
  int64_t           ver;
  const SSubmitReq* pMsg;
  SSubmitBlk*       pBlock;
  SSubmitMsgIter    msgIter;
  SSubmitBlkIter    blkIter;

  SMeta*    pVnodeMeta;
  SHashObj* tbIdHash;
  SArray*   pColIdList;  // SArray<int16_t>

  int32_t         cachedSchemaVer;
  int64_t         cachedSchemaUid;
  SSchemaWrapper* pSchemaWrapper;
  STSchema*       pSchema;
};

// tqPush

typedef struct {
  int64_t  consumerId;
  int32_t  epoch;
  int32_t  skipLogNum;
  int64_t  reqOffset;
  SRWLatch lock;
  SRpcMsg* handle;
} STqPushHandle;

#if 0
typedef struct {
  char    subKey[TSDB_SUBSCRIBE_KEY_LEN];
  int64_t consumerId;
  int32_t epoch;
  int8_t  subType;
  // int8_t        withTbName;
  // int8_t        withSchema;
  // int8_t        withTag;
  char*         qmsg;
  SHashObj*     pDropTbUid;
  STqPushHandle pushHandle;
  // SRWLatch        lock;
  SWalReadHandle* pWalReader;
  // task number should be the same with fetch thread
  STqReadHandle* pExecReader[5];
  qTaskInfo_t    task[5];
} STqExec;
#endif

// tqExec

typedef struct {
  char*       qmsg;
  qTaskInfo_t task[5];
} STqExecCol;

typedef struct {
  int64_t suid;
} STqExecTb;

typedef struct {
  SHashObj* pFilterOutTbUid;
} STqExecDb;

typedef struct {
  int8_t subType;

  STqReadHandle* pExecReader[5];
  union {
    STqExecCol execCol;
    STqExecTb  execTb;
    STqExecDb  execDb;
  } exec;
} STqExecHandle;

typedef struct {
  // info
  char    subKey[TSDB_SUBSCRIBE_KEY_LEN];
  int64_t consumerId;
  int32_t epoch;

  // reader
  SWalReadHandle* pWalReader;

  // push
  STqPushHandle pushHandle;

  // exec
  STqExecHandle execHandle;
} STqHandle;

struct STQ {
  char*     path;
  SHashObj* pushMgr;       // consumerId -> STqHandle*
  SHashObj* handles;       // subKey -> STqHandle
  SHashObj* pStreamTasks;  // taksId -> SStreamTask
  SVnode*   pVnode;
  SWal*     pWal;
  TDB*      pMetaStore;
  TTB*      pExecStore;
};

typedef struct {
  int8_t inited;
  tmr_h  timer;
} STqMgmt;

static STqMgmt tqMgmt = {0};

// init once
int  tqInit();
void tqCleanUp();

// int32_t tEncodeSTqExec(SEncoder* pEncoder, const STqExec* pExec);
// int32_t tDecodeSTqExec(SDecoder* pDecoder, STqExec* pExec);

int32_t tEncodeSTqHandle(SEncoder* pEncoder, const STqHandle* pHandle);
int32_t tDecodeSTqHandle(SDecoder* pDecoder, STqHandle* pHandle);

int64_t tqFetchLog(STQ* pTq, STqHandle* pHandle, int64_t* fetchOffset, SWalHead** pHeadWithCkSum);

int32_t tqDataExec(STQ* pTq, STqExecHandle* pExec, SSubmitReq* pReq, SMqDataBlkRsp* pRsp, int32_t workerId);

// tqOffset
STqOffsetStore* STqOffsetOpen(STqOffsetCfg*);
void            STqOffsetClose(STqOffsetStore*);

int64_t tqOffsetFetch(STqOffsetStore* pStore, const char* subscribeKey);
int32_t tqOffsetCommit(STqOffsetStore* pStore, const char* subscribeKey, int64_t offset);
int32_t tqOffsetPersist(STqOffsetStore* pStore, const char* subscribeKey);
int32_t tqOffsetPersistAll(STqOffsetStore* pStore);

#ifdef __cplusplus
}
#endif

#endif /*_TD_VNODE_TQ_H_*/