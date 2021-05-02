# Pocketcoin Daemon (Bitcoin base)
Описание главных модулей и структуры кода

# Source code structure
## Folders
- `contrib/` - repository tools
- `depends/` - ecosystem for cross platform building
- `doc/` - documentation, you here
- `share/` - additional features
- `src/` - source code

## Files
Точка входа сервиса - запуск потоков БД, стейкинга, WebSocket, Http & RPC.
```
src/init.cpp
```

Протокол обмена данными между узлами, передача и прием данных (хедеры, блоки, транзакции)
```
src/net_processing.cpp
```

Основной модуль валидации блока/транзакции, манипуляции с БД, построение цепочки из "кучи"
```
src/validation.cpp
```

Базовые модели блокчейн
```
src/primitives/block.h
src/pritives/transaction.h
```

Основная логика построения блоков с *методом подтверждения состоянием* - ProofOfStake
```
src/pos.cpp
```

Контекст базы данных sqlite3 для социальных данных
```
src/pocketdb/SQLiteDatabase.hpp
```

Базовые модели соцсети
```
src/pocketdb/models/
```

Репозитории для взаимодействия с sqlite БД
```
src/pocketdb/repositories/
```

Вспомогательная логика
```
src/pocketdb/services/
```

Консенсус логика
```
src/pocketdb/consensus/
```

<br>

# Blockhain storage structure
- `blocks/` - leveldb blocks db
- `chainstate/` - leveldb chain index db
- `indexes/txindex/` - leveldb transactions index db
- `pocketdb/main.sqlite3` - sqlite main database
- `mempool.dat` - structured database for non accepted transactions
- `pocketcoin.conf` - config file for pocketcoin daemon
- `wallet.dat` or `wallets/` - файлы кошелька
- `peers.dat` - structured database network connections
- `debug.log` - default logfile

# Network synchronization
TODO - описать методы обмена, пометить приоритеты и нагрузку

<br>

# Proof of stake
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
## Куча - LevelDb база данных для хранения блоков с индексом по Hash блока.


## 

















