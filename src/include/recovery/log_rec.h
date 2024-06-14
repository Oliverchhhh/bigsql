#ifndef MINISQL_LOG_REC_H
#define MINISQL_LOG_REC_H

#include <unordered_map>
#include <utility>

#include "common/config.h"
#include "common/rowid.h"
#include "record/row.h"

enum class LogRecType {
    kInvalid,
    kInsert,
    kDelete,
    kUpdate,
    kBegin,
    kCommit,
    kAbort,
};

// used for testing only
using KeyType = std::string;
using ValType = int32_t;

/**
 * TODO: Student Implement
 */
struct LogRec {
    LogRec() = default;

    LogRecType type_{LogRecType::kInvalid};
    lsn_t lsn_{INVALID_LSN};
    lsn_t prev_lsn_{INVALID_LSN};

    [[maybe_unused]] KeyType old_key;
    [[maybe_unused]] ValType old_value;
    [[maybe_unused]] KeyType new_key;
    [[maybe_unused]] ValType new_value;
    txn_id_t txn_id;

    /* used for testing only */
    static std::unordered_map<txn_id_t, lsn_t> prev_lsn_map_;
    static lsn_t next_lsn_;
};

std::unordered_map<txn_id_t, lsn_t> LogRec::prev_lsn_map_ = {};
lsn_t LogRec::next_lsn_ = 0;

typedef std::shared_ptr<LogRec> LogRecPtr;

/**
 * TODO: Student Implement
 */
static LogRecPtr CreateInsertLog(txn_id_t txn_id, KeyType ins_key, ValType ins_val) {
  LogRec res ;
  res.type_ = LogRecType::kInsert;
  res.txn_id = txn_id;
  auto id = LogRec::prev_lsn_map_.at(txn_id);
  res.lsn_ = LogRec::next_lsn_;
  res.prev_lsn_ = id;
  LogRec::prev_lsn_map_[txn_id] = LogRec::next_lsn_;
  ++LogRec::next_lsn_;
  res.new_key = ins_key;
  res.new_value = ins_val;
  return std::make_shared<LogRec>(res);
}

/**
 * TODO: Student Implement
 */
static LogRecPtr CreateDeleteLog(txn_id_t txn_id, KeyType del_key, ValType del_val) {
  LogRec res ;
  res.type_ = LogRecType::kDelete;
  res.txn_id = txn_id;
  auto id = LogRec::prev_lsn_map_.at(txn_id);
  res.lsn_ = LogRec::next_lsn_;
  res.prev_lsn_ = id;
  LogRec::prev_lsn_map_[txn_id] = LogRec::next_lsn_;
  ++LogRec::next_lsn_;
  res.new_key = del_key;
  res.new_value = del_val;
  LogRec::prev_lsn_map_[txn_id] = res.lsn_;
  return std::make_shared<LogRec>(res);
}

/**
 * TODO: Student Implement
 */
static LogRecPtr CreateUpdateLog(txn_id_t txn_id, KeyType old_key, ValType old_val, KeyType new_key, ValType new_val) {
  LogRec res ;
  res.type_ = LogRecType::kUpdate;
  res.txn_id = txn_id;
  auto id = LogRec::prev_lsn_map_.at(txn_id);
  res.lsn_ = LogRec::next_lsn_;
  res.prev_lsn_ = id;
  LogRec::prev_lsn_map_[txn_id] = LogRec::next_lsn_;
  ++LogRec::next_lsn_;
  res.old_key = old_key;
  res.old_value = old_val;
  res.new_key = new_key;
  res.new_value = new_val;
  LogRec::prev_lsn_map_[txn_id] = res.lsn_;
  return std::make_shared<LogRec>(res);
}

/**
 * TODO: Student Implement
 */
static LogRecPtr CreateBeginLog(txn_id_t txn_id) {
 LogRec res;
 res.type_ = LogRecType::kBegin;
 res.txn_id = txn_id;
 res.lsn_ = LogRec::next_lsn_;
 res.prev_lsn_ = INVALID_LSN;
 LogRec::prev_lsn_map_[txn_id] = LogRec::next_lsn_;
 ++LogRec::next_lsn_;
 return std::make_shared<LogRec>(res);
}
/**
 * TODO: Student Implement
 */
static LogRecPtr CreateCommitLog(txn_id_t txn_id) {
 LogRec res ;
 res.type_ = LogRecType::kCommit;
 res.txn_id = txn_id;
 auto id = LogRec::prev_lsn_map_.at(txn_id);
 res.lsn_ = LogRec::next_lsn_;
 res.prev_lsn_ = id;
 LogRec::prev_lsn_map_[txn_id] = LogRec::next_lsn_;
 ++LogRec::next_lsn_;
 return std::make_shared<LogRec>(res);
}

/**
 * TODO: Student Implement
 */
static LogRecPtr CreateAbortLog(txn_id_t txn_id) {
 LogRec res ;
 res.type_ = LogRecType::kAbort;
 res.txn_id = txn_id;
 auto id = LogRec::prev_lsn_map_.at(txn_id);
 res.lsn_ = LogRec::next_lsn_;
 res.prev_lsn_ = id;
 LogRec::prev_lsn_map_[txn_id] = LogRec::next_lsn_;
 ++LogRec::next_lsn_;
 return std::make_shared<LogRec>(res);
}

#endif  // MINISQL_LOG_REC_H


