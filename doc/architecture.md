# Pocketcoin Daemon (Bitcoin base)
Описание главных модулей и структуры кода

<br>

# Source code structure
## Folders
`contrib/` - repository tools\
`depends/` - ecosystem for cross platform building\
`doc/` - documentation, you here\
`share/` - additional features\
`src/` - source code

## Files
`src/init.cpp` - Точка входа сервиса - запуск потоков БД, стейкинга, WebSocket, Http & RPC.\
`src/net_processing.cpp` - Протокол обмена данными между узлами, передача и прием данных (хедеры, блоки, транзакции)\
`src/validation.cpp` - Основной модуль валидации блока/транзакции, манипуляции с БД, построение цепи\
`src/primitives/` - Базовые модели блокчейн\
`src/pos.cpp` - Основная логика построения блоков с *методом подтверждения состоянием* - ProofOfStake\
`src/pocketdb/SQLiteDatabase.hpp` - Контекст базы данных sqlite3 для социальных данных
`src/pocketdb/models/` - Базовые модели соцсети\
`src/pocketdb/repositories/` - Репозитории для взаимодействия с sqlite БД\
`src/pocketdb/services/` - Вспомогательная логика\
`src/pocketdb/consensus/` - Консенсус логика

<br>

# Blockchain storage folder structure
`blocks/` - leveldb blocks db\
`chainstate/` - leveldb chain index db\
`indexes/txindex/` - leveldb transactions index db\
`pocketdb/main.sqlite3` - sqlite main database\
`mempool.dat` - structured database for non accepted transactions\
`pocketcoin.conf` - config file for pocketcoin daemon\
`wallet.dat` or `wallets/` - файлы кошелька\
`peers.dat` - structured database network connections\
`debug.log` - default logfile

<br>

# Network synchronization
TODO - описать методы обмена, пометить приоритеты и нагрузку

<br>

# Proof of stake logic
TODO
- инит.спп стартует тред для пос воркера
- луп с попыткой расчета хеша
  - гет аловуед койнс фром валет
  - бац формирование блока через майнер.спп
  - формирование пос транзакции (1 в блоке)
  - определение победителей социал лотереи
  - сортировка и отбор
  - сборка и подпись блока
  - оповестить пиры о новом блоке

<br>

# Consensus validation
TODO

<br>

# Indexing
Каждая транзакция попавшая в узел проходит первичную валидацию перед записью в мемпул - проверки на даблСпенд, минимальный фи, локТайм.

Транзакции в мемпуле не имеют порядкого номера и высоты, как и связи с блоком.\
Pocket sqlite database сохраняет все транзакции в разряженую таблицу main.Transactions без указания `main.Transactions.TxOut` и `main.Transactions.Block` - [main.sql](https://github.com/pocketnetteam/pocketnet.core/blob/feature/sqlite/src/pocketdb/docs/main.sql)

> Здесь и далее `main.Transactions.TxOut` - `<Db>.<Table>.<Field>` означает SQLite базу данных Pocket Social Data - `pocketdb/main.sqlite3`

Все типы транзакций, хранящиеся в `main.Transaction` определены как Enum и имеют зафиксированногое целое значение в [src/pocketdb/models/base/Base.hpp]()
```c++
enum PocketTxType
{
    NOT_SUPPORTED = 0,

    USER_ACCOUNT = 100,
    VIDEO_SERVER_ACCOUNT = 101,
    MESSAGE_SERVER_ACCOUNT = 102,

    POST_CONTENT = 200,
    VIDEO_CONTENT = 201,
    TRANSLATE_CONTENT = 202,
    SERVERPING_CONTENT = 203,
    COMMENT_CONTENT = 204,

    SCORE_POST_ACTION = 300,
    SCORE_COMMENT_ACTION = 301,

    SUBSCRIBE_ACTION = 302,
    SUBSCRIBE_PRIVATE_ACTION = 303,
    SUBSCRIBE_CANCEL_ACTION = 304,

    BLOCKING_ACTION = 305,
    BLOCKING_CANCEL_ACTION = 306,

    COMPLAIN_ACTION = 307,
};
```

Формирование блока представляет из себя процесс фильтрации (1) и формирования (2) списка транзакций для нового блока в цепи, а также присваивается хэш (3) и автоматически присваивается высота (за счет связи с предыдущим блоком по хэшу).

Сформированный блок отправляется в тред формирования цепи (4), где дополнительно проверяется его "право" на размещение в цепи. После успешной проверки обновляются:

[src/validation.cpp]()
- `chainActive.Height` в `src/validation.cpp`

[src/pocketdb/services/BlockIndexer.hpp]()
- `main.Transactions.Block` & `main.Transaction.TxOut`
- `main.Utxo` добавляется запись с каждым значащим оутом из транзакции, а также фиксируются "потраченными" все оуты из секции `inputs` транзакции
- `main.Ratings` добавляются обновленные записи рейтингов по типам















