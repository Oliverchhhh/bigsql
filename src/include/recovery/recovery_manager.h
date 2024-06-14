#ifndef MINISQL_RECOVERY_MANAGER_H
#define MINISQL_RECOVERY_MANAGER_H

#include <map>
#include <unordered_map>
#include <vector>

#include "recovery/log_rec.h"

using KvDatabase = std::unordered_map<KeyType, ValType>;
using ATT = std::unordered_map<txn_id_t, lsn_t>;

struct CheckPoint {
    lsn_t checkpoint_lsn_{INVALID_LSN};
    ATT active_txns_{};
    KvDatabase persist_data_{};

    inline void AddActiveTxn(txn_id_t txn_id, lsn_t last_lsn) { active_txns_[txn_id] = last_lsn; }

    inline void AddData(KeyType key, ValType val) { persist_data_.emplace(std::move(key), val); }
};

class RecoveryManager {
public:
    /**
    * TODO: Student Implement
    */
    void Init(CheckPoint &last_checkpoint) {
        data_ = last_checkpoint.persist_data_;
        active_txns_ = last_checkpoint.active_txns_;
        persist_lsn_ = last_checkpoint.checkpoint_lsn_;
    }

    /**
    * TODO: Student Implement
    */
    void RedoPhase() {//重做从时间点开始的日志记录
        //从最后一次检查点的日志序列号（的下一个开始，遍历所有日志记录到当前的日志序列号
        for(int i = persist_lsn_; i < LogRec::next_lsn_; i++) {
            auto log = log_recs_.at(i);
            active_txns_[log->txn_id] = log->lsn_;
            switch (log->type_) {
                case LogRecType::kInsert:
                    data_[log->new_key] = log->new_value;
                break;
                case LogRecType::kDelete:
                    data_.erase(log->new_key);
                break;
                case LogRecType::kUpdate:
                    data_.erase(log->old_key);
                    data_[log->new_key] = log->new_value;
                break;
                case LogRecType::kCommit:
                    active_txns_.erase(log->lsn_);
                break;
                case LogRecType::kAbort:
                    auto t_log = log;
                while(t_log->prev_lsn_ != INVALID_LSN){
                    auto t = t_log->prev_lsn_;
                    t_log = log_recs_.at(t);
                    if(t_log->type_ == LogRecType::kInsert){
                        data_.erase(t_log->new_key);
                    }else if(t_log->type_ == LogRecType::kDelete){
                        data_[t_log->new_key] = t_log->new_value;
                    }else if(t_log->type_ == LogRecType::kUpdate){
                        data_.erase(t_log->new_key);
                        data_[t_log->old_key] = t_log->old_value;
                    }
                }
                break;
            }
        }


    }

    /**
    * TODO: Student Implement
    */
    void UndoPhase() {
        for(const auto &txn : active_txns_) {
            auto final_res = log_recs_.at(txn.second);
            if(final_res->type_ != LogRecType::kCommit) {
                auto log = final_res;
                while(log->prev_lsn_ != INVALID_LSN) {
                    log = log_recs_.at(log->prev_lsn_);
                    switch (log->type_) {
                        case LogRecType::kInsert:
                            data_.erase(log->new_key);
                        break;
                        case LogRecType::kDelete:
                            data_[log->new_key] = log->new_value;
                        break;
                        case LogRecType::kUpdate:
                            data_.erase(log->new_key);
                        data_[log->old_key] = log->old_value;
                        break;
                        default:
                            break;
                    }
                }
            }
        }
    }



    // used for test only
    void AppendLogRec(LogRecPtr log_rec) { log_recs_.emplace(log_rec->lsn_, log_rec); }

    // used for test only
    inline KvDatabase &GetDatabase() { return data_; }

private:
    std::map<lsn_t, LogRecPtr> log_recs_{};
    lsn_t persist_lsn_{INVALID_LSN};
    ATT active_txns_{};
    KvDatabase data_{};  // all data in database
};

#endif  // MINISQL_RECOVERY_MANAGER_H

