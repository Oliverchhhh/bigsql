#include "catalog/catalog.h"

void CatalogMeta::SerializeTo(char *buf) const {
  ASSERT(GetSerializedSize() <= PAGE_SIZE, "Failed to serialize catalog metadata to disk.");
  MACH_WRITE_UINT32(buf, CATALOG_METADATA_MAGIC_NUM);
  buf += 4;
  MACH_WRITE_UINT32(buf, table_meta_pages_.size());
  buf += 4;
  MACH_WRITE_UINT32(buf, index_meta_pages_.size());
  buf += 4;
  for (auto iter : table_meta_pages_) {
    MACH_WRITE_TO(table_id_t, buf, iter.first);
    buf += 4;
    MACH_WRITE_TO(page_id_t, buf, iter.second);
    buf += 4;
  }
  for (auto iter : index_meta_pages_) {
    MACH_WRITE_TO(index_id_t, buf, iter.first);
    buf += 4;
    MACH_WRITE_TO(page_id_t, buf, iter.second);
    buf += 4;
  }
}

CatalogMeta *CatalogMeta::DeserializeFrom(char *buf) {
  // check valid
  uint32_t magic_num = MACH_READ_UINT32(buf);
  buf += 4;
  ASSERT(magic_num == CATALOG_METADATA_MAGIC_NUM, "Failed to deserialize catalog metadata from disk.");
  // get table and index nums
  uint32_t table_nums = MACH_READ_UINT32(buf);
  buf += 4;
  uint32_t index_nums = MACH_READ_UINT32(buf);
  buf += 4;
  // create metadata and read value
  CatalogMeta *meta = new CatalogMeta();
  for (uint32_t i = 0; i < table_nums; i++) {
    auto table_id = MACH_READ_FROM(table_id_t, buf);
    buf += 4;
    auto table_heap_page_id = MACH_READ_FROM(page_id_t, buf);
    buf += 4;
    meta->table_meta_pages_.emplace(table_id, table_heap_page_id);
  }
  for (uint32_t i = 0; i < index_nums; i++) {
    auto index_id = MACH_READ_FROM(index_id_t, buf);
    buf += 4;
    auto index_page_id = MACH_READ_FROM(page_id_t, buf);
    buf += 4;
    meta->index_meta_pages_.emplace(index_id, index_page_id);
  }
  return meta;
}

/**
 * TODO: Student Implement
 */
uint32_t CatalogMeta::GetSerializedSize() const {
  /*Consider MAGIC_NUM, size of table_meta_pages_ and size of index_meta_pages = 3*/
  return 3 * sizeof(uint32_t) + table_meta_pages_.size() * 2 * sizeof(uint32_t) +
         index_meta_pages_.size() * 2 * sizeof(uint32_t);
}

CatalogMeta::CatalogMeta() {}

/**
 * TODO: Student Implement
 */
CatalogManager::CatalogManager(BufferPoolManager *buffer_pool_manager, LockManager *lock_manager,
                               LogManager *log_manager, bool init)
    : buffer_pool_manager_(buffer_pool_manager), lock_manager_(lock_manager), log_manager_(log_manager) {
  // ASSERT(false, "Not Implemented yet")；

  table_names_.clear();
  tables_.clear();
  index_names_.clear();
  indexes_.clear();
  Page *page_ = buffer_pool_manager_->FetchPage(CATALOG_META_PAGE_ID);
  char *buf = page_->GetData();
  if (init == true) {
    catalog_meta_ = catalog_meta_->NewInstance();
    catalog_meta_->table_meta_pages_.clear();
    catalog_meta_->index_meta_pages_.clear();
    catalog_meta_->SerializeTo(buf);
  } else {
    catalog_meta_ = catalog_meta_->DeserializeFrom(buf);
    for (auto iter : catalog_meta_->table_meta_pages_) {
      LoadTable(iter.first, iter.second);
    }
    for (auto iter : catalog_meta_->index_meta_pages_) {
      LoadIndex(iter.first, iter.second);
    }
  }
  buffer_pool_manager->UnpinPage(CATALOG_META_PAGE_ID, true);
}

CatalogManager::~CatalogManager() {
  FlushCatalogMetaPage();
  delete catalog_meta_;
  for (auto iter : tables_) {
    delete iter.second;
  }
  for (auto iter : indexes_) {
    delete iter.second;
  }
}

/**
 * TODO: Student Implement
 */
dberr_t CatalogManager::CreateTable(const string &table_name, TableSchema *schema, Txn *txn, TableInfo *&table_info) {
  // ASSERT(false, "Not Implemented yet");
  try {
    // Does the table exist?
    auto iter = table_names_.find(table_name);
    if (iter != table_names_.end()) {
      return DB_TABLE_ALREADY_EXIST;
    };

    // init
    page_id_t meta_page_id = 0;
    Page *meta_page = nullptr;
    page_id_t table_page_id = 0;
    Page *table_page = nullptr;
    table_id_t table_id = 0;
    TableMetadata *table_meta_ = nullptr;
    TableHeap *table_heap_ = nullptr;
    TableSchema *schema_ = nullptr;
    // init table info
    table_info = table_info->Create();
    // get table id
    table_id = catalog_meta_->GetNextTableId();
    // copy schema
    schema_ = schema_->DeepCopySchema(schema);
    // get new table meta page
    meta_page = buffer_pool_manager_->NewPage(meta_page_id);
    // get new table page
    table_page = buffer_pool_manager_->NewPage(table_page_id);
    // table meta init
    table_meta_ = table_meta_->Create(table_id, table_name, table_page_id, schema_);
    table_meta_->SerializeTo(meta_page->GetData());
    // table init
    table_heap_ = table_heap_->Create(buffer_pool_manager_, schema_, nullptr, nullptr, nullptr);
    // table info
    table_info->Init(table_meta_, table_heap_);

    // table meta
    table_names_[table_name] = table_id;
    tables_[table_id] = table_info;

    // catalog meta
    catalog_meta_->table_meta_pages_[table_id] = meta_page_id;
    Page *page_ = buffer_pool_manager_->FetchPage(CATALOG_META_PAGE_ID);
    char *buf = page_->GetData();
    catalog_meta_->SerializeTo(buf);
    buffer_pool_manager_->UnpinPage(CATALOG_META_PAGE_ID, true);
    buffer_pool_manager_->UnpinPage(meta_page_id, true);
    buffer_pool_manager_->UnpinPage(table_page_id, true);
    return DB_SUCCESS;
  } catch (exception e) {
    return DB_FAILED;
  }
}

/**
 * TODO: Student Implement
 */
// question
dberr_t CatalogManager::GetTable(const string &table_name, TableInfo *&table_info) {
  // ASSERT(false, "Not Implemented yet");
  if (table_names_.count(table_name) <= 0) return DB_TABLE_NOT_EXIST;
  table_id_t table_id = table_names_[table_name];
  table_info = tables_[table_id];
  return DB_SUCCESS;
}

/**
 * TODO: Student Implement
 */
// question
dberr_t CatalogManager::GetTables(vector<TableInfo *> &tables) const {
  // ASSERT(false, "Not Implemented yet");
  if (tables_.size() == 0) return DB_FAILED;
  tables.resize(tables_.size());  // allocate memory
  uint32_t i = 0;
  for (auto itera = tables_.begin(); itera != tables_.end(); ++itera, ++i) tables[i] = itera->second;

  return DB_SUCCESS;
}

/**
 * TODO: Student Implement
 */
dberr_t CatalogManager::CreateIndex(const std::string &table_name, const string &index_name,
                                    const std::vector<std::string> &index_keys, Txn *txn, IndexInfo *&index_info,
                                    const string &index_type) {
  try {
    // Does the table exist?
    auto iter_find_table = table_names_.find(table_name);
    if (iter_find_table == table_names_.end()) {
      return DB_TABLE_NOT_EXIST;
    }
    // Does the index exist?
    auto iter_find_index_table = index_names_.find(table_name);
    if (iter_find_index_table != index_names_.end()) {
      auto iter_find_index_name = iter_find_index_table->second.find(index_name);
      if (iter_find_index_name != iter_find_index_table->second.end()) {
        return DB_INDEX_ALREADY_EXIST;
      }
    }

    // init
    // table
    table_id_t table_id = 0;
    TableSchema *schema_ = nullptr;
    TableInfo *table_info_ = nullptr;
    // index
    page_id_t meta_page_id = 0;
    Page *meta_page = nullptr;
    index_id_t index_id = 0;
    IndexMetadata *index_meta_ = nullptr;
    // index key map
    std::vector<std::uint32_t> key_map{};
    // init index info
    index_info = index_info->Create();
    // get index id
    index_id = catalog_meta_->GetNextIndexId();
    // get table schema
    table_id = table_names_[table_name];
    table_info_ = tables_[table_id];
    schema_ = table_info_->GetSchema();
    // create key map
    uint32_t column_index = 0;
    for (auto column_name : index_keys) {
      if (schema_->GetColumnIndex(column_name, column_index) == DB_COLUMN_NAME_NOT_EXIST) {
        return DB_COLUMN_NAME_NOT_EXIST;
      }
      key_map.push_back(column_index);
    }
    // get new index meta page
    meta_page = buffer_pool_manager_->NewPage(meta_page_id);
    // create index meta
    index_meta_ = index_meta_->Create(index_id, index_name, table_id, key_map);
    index_meta_->SerializeTo(meta_page->GetData());
    // Init index info
    index_info->Init(index_meta_, table_info_, buffer_pool_manager_);
    // table meta
    index_names_[table_name][index_name] = index_id;
    indexes_[index_id] = index_info;

    // catalog meta
    catalog_meta_->index_meta_pages_[index_id] = meta_page_id;
    Page *page_ = buffer_pool_manager_->FetchPage(CATALOG_META_PAGE_ID);
    char *buf = page_->GetData();
    catalog_meta_->SerializeTo(buf);
    buffer_pool_manager_->UnpinPage(CATALOG_META_PAGE_ID, true);
    buffer_pool_manager_->UnpinPage(meta_page_id, true);
    return DB_SUCCESS;
  } catch (exception e) {
    return DB_FAILED;
  }
}

/**
 * TODO: Student Implement
 */
dberr_t CatalogManager::GetIndex(const std::string &table_name, const std::string &index_name,
                                 IndexInfo *&index_info) const {
  // find table
  if (index_names_.find(table_name) == index_names_.end()) return DB_TABLE_NOT_EXIST;

  // find index
  auto indname_id = index_names_.find(table_name)->second;
  if (indname_id.find(index_name) == indname_id.end()) return DB_INDEX_NOT_FOUND;

  // have found and return index_info
  index_id_t index_id = indname_id[index_name];
  index_info = indexes_.find(index_id)->second;

  return DB_SUCCESS;
}

/**
 * TODO: Student Implement
 */
dberr_t CatalogManager::GetTableIndexes(const std::string &table_name, std::vector<IndexInfo *> &indexes) const {
  // find table index
  auto table_indexes = index_names_.find(table_name);
  if (table_indexes == index_names_.end()) return DB_TABLE_NOT_EXIST;
  // update indexes
  auto indexes_map = table_indexes->second;
  for (auto it : indexes_map) {
    indexes.push_back(indexes_.find(it.second)->second);
  }
  return DB_SUCCESS;
}

/**
 * TODO: Student Implement
 */
dberr_t CatalogManager::DropTable(const string &table_name) {
  // not found
  if (table_names_.find(table_name) == table_names_.end()) return DB_TABLE_NOT_EXIST;
  // get table through hash map
  table_id_t table_id = table_names_[table_name];
  TableInfo *droptable = tables_[table_id];
  // drop
  if (droptable == nullptr) return DB_FAILED;
  tables_.erase(table_id);
  table_names_.erase(table_name);
  droptable->~TableInfo();
  return DB_SUCCESS;
}

/**
 * TODO: Student Implement
 */
dberr_t CatalogManager::DropIndex(const string &table_name, const string &index_name) {
  try {
    index_id_t index_id = 0;
    // Does the index exist?
    auto iter_find_index_table = index_names_.find(table_name);
    if (iter_find_index_table != index_names_.end()) {
      auto iter_find_index_name = iter_find_index_table->second.find(index_name);
      if (iter_find_index_name == iter_find_index_table->second.end()) {
        return DB_INDEX_NOT_FOUND;
      } else {
        // delete index info
        delete indexes_[iter_find_index_name->second];
        // erase indexex map
        indexes_.erase(iter_find_index_name->second);
        // erase index_names map
        iter_find_index_table->second.erase(index_name);
        return DB_SUCCESS;
      }
    } else {
      return DB_INDEX_NOT_FOUND;
    }
  } catch (exception e) {
    return DB_FAILED;
  }
}

/**
 * TODO: Student Implement
 */
dberr_t CatalogManager::FlushCatalogMetaPage() const {
  if (buffer_pool_manager_->FlushPage(CATALOG_META_PAGE_ID)) return DB_SUCCESS;
  return DB_FAILED;
}

/**
 * TODO: Student Implement
 */
dberr_t CatalogManager::LoadTable(const table_id_t table_id, const page_id_t page_id) {
  try {
    // init
    page_id_t meta_page_id = 0;
    Page *meta_page = nullptr;
    page_id_t table_page_id = 0;
    Page *table_page = nullptr;
    string table_name_ = "";
    TableMetadata *table_meta_ = nullptr;
    TableHeap *table_heap_ = nullptr;
    TableSchema *schema_ = nullptr;
    TableInfo *table_info = nullptr;
    // init table info
    table_info = table_info->Create();
    // load table meta page
    meta_page_id = page_id;
    meta_page = buffer_pool_manager_->FetchPage(meta_page_id);
    // LOad table meta
    table_meta_->DeserializeFrom(meta_page->GetData(), table_meta_);
    // table init
    ASSERT(table_id == table_meta_->GetTableId(), "Load wrong table");
    table_name_ = table_meta_->GetTableName();
    table_page_id = table_meta_->GetFirstPageId();
    schema_ = table_meta_->GetSchema();
    table_heap_ = table_heap_->Create(buffer_pool_manager_, table_page_id, schema_, nullptr, nullptr);
    // table info
    table_info->Init(table_meta_, table_heap_);
    // table meta
    table_names_[table_name_] = table_id;
    tables_[table_id] = table_info;
    buffer_pool_manager_->UnpinPage(meta_page_id, true);
    return DB_SUCCESS;
  } catch (exception e) {
    return DB_FAILED;
  }
}

/**
 *
 *
 *
 * TODO: Student Implement
 */
dberr_t CatalogManager::LoadIndex(const index_id_t index_id, const page_id_t page_id) {
  try {
    // init
    // table
    string table_name = "";
    table_id_t table_id = 0;
    TableSchema *schema_ = nullptr;
    TableInfo *table_info_ = nullptr;
    // index
    string index_name = "";
    page_id_t meta_page_id = 0;
    Page *meta_page = nullptr;
    IndexMetadata *index_meta_ = nullptr;
    IndexInfo *index_info = nullptr;
    // index key map
    std::vector<std::uint32_t> key_map{};
    // init table info
    index_info = index_info->Create();
    // load index meta page
    meta_page_id = page_id;
    meta_page = buffer_pool_manager_->FetchPage(meta_page_id);
    // deserial index meta
    index_meta_->DeserializeFrom(meta_page->GetData(), index_meta_);
    // get index name from index meta
    index_name = index_meta_->GetIndexName();
    // get table id from index meta
    table_id = index_meta_->GetTableId();
    // get table info
    table_info_ = tables_[table_id];
    // get table name from table info
    table_name = table_info_->GetTableName();
    // Init index info
    index_info->Init(index_meta_, table_info_, buffer_pool_manager_);
    // table meta
    index_names_[table_name][index_name] = index_id;
    indexes_[index_id] = index_info;
    buffer_pool_manager_->UnpinPage(meta_page_id, true);
    return DB_SUCCESS;
  } catch (exception e) {
    return DB_FAILED;
  }
}

/**
 * TODO: Student Implement
 */
dberr_t CatalogManager::GetTable(const table_id_t table_id, TableInfo *&table_info) {
  // ASSERT(false, "Not Implemented yet");
  auto it = tables_.find(table_id);
  if (it == tables_.end()) return DB_TABLE_NOT_EXIST;
  table_info = it->second;
  return DB_SUCCESS;
}