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

#include "tsdb.h"

enum { RETENTION_NO = 0, RETENTION_EXPIRED = 1, RETENTION_MIGRATE = 2 };

#if 1
#define MIGRATE_MIN_FSIZE (1048576 << 9)  // 512 MB
#define MIGRATE_MAX_SPEED (1048576 << 5)  // 32 MB
#define MIGRATE_MIN_COST  (5)             // second
#else
#define MIGRATE_MIN_FSIZE (1048576 << 5)  // 32 MB
#define MIGRATE_MAX_SPEED (1048576 << 2)  // 4  MB
#define MIGRATE_MIN_COST  (5)             // second
#endif

static bool    tsdbShouldDoMigrate(STsdb *pTsdb);
static int32_t tsdbShouldDoRetention(STsdb *pTsdb, int64_t now);
static int32_t tsdbProcessExpire(STsdb *pTsdb, int64_t now, int32_t retention);
static int32_t tsdbProcessMigrate(STsdb *pTsdb, int64_t now, int32_t maxSpeed, int32_t retention);

static bool tsdbShouldDoMigrate(STsdb *pTsdb) {
  if (tfsGetLevel(pTsdb->pVnode->pTfs) < 2) {
    return false;
  }

  STsdbKeepCfg *keepCfg = &pTsdb->keepCfg;
  if (keepCfg->keep0 == keepCfg->keep1 && keepCfg->keep1 == keepCfg->keep2) {
    return false;
  }
  return true;
}

static int32_t tsdbShouldDoRetention(STsdb *pTsdb, int64_t now) {
  int32_t retention = RETENTION_NO;
  if (taosArrayGetSize(pTsdb->fs.aDFileSet) == 0) {
    return retention;
  }

  SDFileSet *pSet = (SDFileSet *)taosArrayGet(pTsdb->fs.aDFileSet, 0);
  if (tsdbFidLevel(pSet->fid, &pTsdb->keepCfg, now) < 0) {
    retention |= RETENTION_EXPIRED;
  }

  if (!tsdbShouldDoMigrate(pTsdb)) {
    return retention;
  }

  for (int32_t iSet = 0; iSet < taosArrayGetSize(pTsdb->fs.aDFileSet); ++iSet) {
    pSet = (SDFileSet *)taosArrayGet(pTsdb->fs.aDFileSet, iSet);
    int32_t expLevel = tsdbFidLevel(pSet->fid, &pTsdb->keepCfg, now);

    if (expLevel == pSet->diskId.level) continue;

    if (expLevel > 0) {
      retention |= RETENTION_MIGRATE;
      break;
    }
  }

  return retention;
}

static int32_t tsdbProcessExpire(STsdb *pTsdb, int64_t now, int32_t retention) {
  int32_t code = 0;
  int32_t nLoops = 0;
  int32_t maxFid = INT32_MIN;
  STsdbFS fs = {0};
  STsdbFS fsLatest = {0};

  if (!(retention & RETENTION_EXPIRED)) {
    goto _exit;
  }

  code = tsdbFSCopy(pTsdb, &fs);
  if (code) goto _exit;

  int32_t fsSize = taosArrayGetSize(fs.aDFileSet);
  for (int32_t iSet = 0; iSet < fsSize; iSet++) {
    SDFileSet *pSet = (SDFileSet *)taosArrayGet(fs.aDFileSet, iSet);
    int32_t    expLevel = tsdbFidLevel(pSet->fid, &pTsdb->keepCfg, now);
    SDiskID    did;

    if (expLevel < 0) {
      SET_DFSET_EXPIRED(pSet);
      if (pSet->fid > maxFid) maxFid = pSet->fid;
    } else {
      break;
    }
  }

  if (maxFid == INT32_MIN) goto _exit;

_wait_commit_end:
  while (atomic_load_32(&pTsdb->trimHdl.minCommitFid) <= maxFid) {
    if (++nLoops > 1000) {
      nLoops = 0;
      sched_yield();
    }
  }
  if (atomic_val_compare_exchange_8(&pTsdb->trimHdl.state, 0, 1) == 0) {
    if (atomic_load_32(&pTsdb->trimHdl.minCommitFid) <= maxFid) {
      atomic_store_8(&pTsdb->trimHdl.state, 0);
      goto _wait_commit_end;
    }
    atomic_store_32(&pTsdb->trimHdl.maxRetentFid, maxFid);
    atomic_store_8(&pTsdb->trimHdl.state, 0);
  } else {
    goto _wait_commit_end;
  }

_merge_fs:
  taosThreadRwlockWrlock(&pTsdb->rwLock);
  if ((code = tsdbFSCopy(pTsdb, &fsLatest))) {
    taosThreadRwlockUnlock(&pTsdb->rwLock);
    goto _exit;
  }
  // 1) merge tsdbFSNew and pTsdb->fs
  if ((code = tsdbFSUpdDel(pTsdb, &fsLatest, &fs, maxFid))) {
    taosThreadRwlockUnlock(&pTsdb->rwLock);
    goto _exit;
  }
  // 2) save CURRENT
  if ((code = tsdbFSCommit1(pTsdb, &fsLatest))) {
    taosThreadRwlockUnlock(&pTsdb->rwLock);
    goto _exit;
  }
  // 3) apply the tsdbFS to pTsdb->fs
  if ((code = tsdbFSCommit2(pTsdb, &fsLatest))) {
    taosThreadRwlockUnlock(&pTsdb->rwLock);
    goto _exit;
  }
  taosThreadRwlockUnlock(&pTsdb->rwLock);

_exit:
  tsdbFSDestroy(&fs);
  tsdbFSDestroy(&fsLatest);
  if (code != 0) {
    tsdbError("vgId:%d, tsdb do retention(expire) failed since %s", TD_VID(pTsdb->pVnode), tstrerror(code));
    ASSERT(0);
  }
  return code;
}

/**
 * @brief
 *
 * @param pTsdb
 * @param now
 * @param retention
 * @return int32_t
 */
static int32_t tsdbProcessMigrate(STsdb *pTsdb, int64_t now, int32_t maxSpeed, int32_t retention) {
  int32_t code = 0;
  int32_t nBatch = 0;
  int32_t nLoops = 0;
  int32_t maxFid = INT32_MIN;
  int64_t fSize = 0;
  STsdbFS fs = {0};
  STsdbFS fsLatest = {0};

  if (!(retention & RETENTION_MIGRATE)) {
    goto _exit;
  }

_migrate_loop:
  // reset
  maxFid = INT32_MIN;
  fSize = 0;
  tsdbFSDestroy(&fs);
  tsdbFSDestroy(&fsLatest);

  code = tsdbFSCopy(pTsdb, &fs);
  if (code) goto _exit;

  int32_t fsSize = taosArrayGetSize(fs.aDFileSet);
  for (int32_t iSet = 0; iSet < fsSize; ++iSet) {
    SDFileSet *pSet = (SDFileSet *)taosArrayGet(fs.aDFileSet, iSet);
    int32_t    expLevel = tsdbFidLevel(pSet->fid, &pTsdb->keepCfg, now);
    SDiskID    did;

    if (pSet->diskId.level == expLevel) continue;

    if (expLevel > 0) {
      ASSERT(pSet->fid > maxFid);
      maxFid = pSet->fid;
      fSize += (pSet->pDataF->size + pSet->pHeadF->size + pSet->pSmaF->size);
      if (fSize / MIGRATE_MAX_SPEED > MIGRATE_MIN_COST) {
        break;
      }
      for (int32_t iStt = 0; iStt < pSet->nSttF; ++iStt) {
        fSize += pSet->aSttF[iStt]->size;
      }
      if (fSize / MIGRATE_MAX_SPEED > MIGRATE_MIN_COST) {
        tsdbInfo("vgId:%d migrate loop[%d] with maxFid:%d", TD_VID(pTsdb->pVnode), nBatch, maxFid);
        break;
      }
    }
  }

  if (maxFid == INT32_MIN) goto _exit;

_wait_commit_end:
  while (atomic_load_32(&pTsdb->trimHdl.minCommitFid) <= maxFid) {
    if (++nLoops > 1000) {
      nLoops = 0;
      sched_yield();
    }
  }
  if (atomic_val_compare_exchange_8(&pTsdb->trimHdl.state, 0, 1) == 0) {
    if (atomic_load_32(&pTsdb->trimHdl.minCommitFid) <= maxFid) {
      atomic_store_8(&pTsdb->trimHdl.state, 0);
      goto _wait_commit_end;
    }
    atomic_store_32(&pTsdb->trimHdl.maxRetentFid, maxFid);
    atomic_store_8(&pTsdb->trimHdl.state, 0);
  } else {
    goto _wait_commit_end;
  }

  // migrate
  for (int32_t iSet = 0; iSet < fsSize; ++iSet) {
    SDFileSet *pSet = (SDFileSet *)taosArrayGet(fs.aDFileSet, iSet);
    int32_t    expLevel = tsdbFidLevel(pSet->fid, &pTsdb->keepCfg, now);
    SDiskID    did;

    if (pSet->fid > maxFid) break;

    tsdbInfo("vgId:%d migrate loop[%d] with maxFid:%d, fid:%d, did:%d, level:%d, expLevel:%d", TD_VID(pTsdb->pVnode),
             nBatch, maxFid, pSet->fid, pSet->diskId.id, pSet->diskId.level, expLevel);

    if (expLevel < 0) {
      SET_DFSET_EXPIRED(pSet);
    } else {
      if (expLevel == pSet->diskId.level) continue;

      if (tfsAllocDisk(pTsdb->pVnode->pTfs, expLevel, &did) < 0) {
        code = terrno;
        goto _exit;
      }

      if (did.level == pSet->diskId.level) continue;

      // copy file to new disk
      SDFileSet fSet = *pSet;
      fSet.diskId = did;

      code = tsdbDFileSetCopy(pTsdb, pSet, &fSet, maxSpeed);
      if (code) goto _exit;

      code = tsdbFSUpsertFSet(&fs, &fSet);
      if (code) goto _exit;
    }
  }

_merge_fs:
  taosThreadRwlockWrlock(&pTsdb->rwLock);

  if ((code = tsdbFSCopy(pTsdb, &fsLatest))) {
    taosThreadRwlockUnlock(&pTsdb->rwLock);
    goto _exit;
  }
  // 1) merge tsdbFSNew and pTsdb->fs
  if ((code = tsdbFSUpdDel(pTsdb, &fsLatest, &fs, maxFid))) {
    taosThreadRwlockUnlock(&pTsdb->rwLock);
    goto _exit;
  }
  // 2) save CURRENT
  if ((code = tsdbFSCommit1(pTsdb, &fsLatest))) {
    taosThreadRwlockUnlock(&pTsdb->rwLock);
    goto _exit;
  }
  // 3) apply the tsdbFS to pTsdb->fs
  if ((code = tsdbFSCommit2(pTsdb, &fsLatest))) {
    taosThreadRwlockUnlock(&pTsdb->rwLock);
    goto _exit;
  }
  taosThreadRwlockUnlock(&pTsdb->rwLock);

  ++nBatch;
  goto _migrate_loop;

_exit:
  tsdbFSDestroy(&fs);
  tsdbFSDestroy(&fsLatest);
  if (code != 0) {
    tsdbError("vgId:%d, tsdb do retention(migrate) failed since %s", TD_VID(pTsdb->pVnode), tstrerror(code));
    ASSERT(0);
  }
  return code;
}

/**
 * @brief Data migration between multi-tier storage, including remove expired data.
 *  1) firstly, remove expired DFileSet;
 *  2) partition the tsdbFS by the expLevel and the estimated cost(e.g. 5s) to copy, and migrate
 * DFileSet groups between multi-tier storage;
 *  3) update the tsdbFS and CURRENT in the same transaction;
 *  4) finish the migration
 * @param pTsdb
 * @param now
 * @param maxSpeed
 * @return int32_t
 */
int32_t tsdbDoRetention(STsdb *pTsdb, int64_t now, int32_t maxSpeed) {
  int32_t code = 0;
  int32_t retention = RETENTION_NO;

  retention = tsdbShouldDoRetention(pTsdb, now);
  if (retention == RETENTION_NO) {
    goto _exit;
  }

  // step 1: process expire
  code = tsdbProcessExpire(pTsdb, now, retention);
  if (code < 0) goto _exit;

  // step 2: process multi-tier migration
  code = tsdbProcessMigrate(pTsdb, now, maxSpeed, retention);
  if (code < 0) goto _exit;

_exit:
  pTsdb->trimHdl.maxRetentFid = INT32_MIN;
  if (code != 0) {
    tsdbError("vgId:%d, tsdb do retention:%d failed since %s", TD_VID(pTsdb->pVnode), retention, tstrerror(code));
    ASSERT(0);
    // tsdbFSRollback(pTsdb->pFS);
  } else {
    tsdbInfo("vgId:%d, tsdb do retention:%d succeed", TD_VID(pTsdb->pVnode), retention);
  }
  return code;
}