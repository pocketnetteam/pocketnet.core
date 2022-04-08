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

## General files
`src/init.cpp` - Инициализация БД, потока стейкинга, WebSocket, Http & RPC серверов.\
`src/net_processing.cpp` - Обечпечение протокола обмена данными между узлами, передача и прием (хедеры, блоки, транзакции)\
`src/validation.cpp` - Основной модуль валидации блока/транзакций, манипуляции с БД, построение цепи\
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

# Daemon workers and processes

## Nodes synchronization
TODO - описать методы обмена, пометить приоритеты и нагрузку


## Blockchain generating (Proof Of Stake)

TODO - описание алгоритма выбора узла-кандидата для сборки блока

<!-- - инит.спп стартует тред для пос воркера
- луп с попыткой расчета хеша
  - гет аловуед койнс фром валет
  - бац формирование блока через майнер.спп
  - формирование пос транзакции
  - определение победителей социал лотереи
  - сортировка и отбор
  - сборка и подпись блока
  - оповестить пиры о новом блоке -->

<br>

## Lottery
Лотерея - процесс отбора авторов постов, комментариев или другого материала, получивших положительные оценки от пользователей с правом влияния на репутацию и участие в лотерее.
```
src/pocketdb/consensus/Lottery.hpp
```

<br>

## HTTP server
TODO

<br>

## RPC server
TODO

<br>

# Consensus validation
TODO

В базовом виде проверка транзакции состоит из ряда обязательных условий, свойственных всем транзакциям:
```c++
src/pocketdb/consensus/Base.hpp
```

Механизм DTO расширяет базовый набор условий валидации, накладывая правила взаимодействия субъектов сети:
```
src/pocketdb/consensus/Post.hpp
src/pocketdb/consensus/User.hpp
...
```

> Базовые и ДТО правила могут содержать "чекпойнты", которые позволяют переопределять логику с привязкой к высоте или другим условиям. Класс наследуется, сохраняет основное наименование, добавляется префикс `_checkpoint_<condition>`, например:
>   ```c++
>   class ConsensusPost_checkpoint_1500000 : ConsensusPost
>   {
>   private:
>       const int condition = 1'500'000;
>   }
>   ```

<br>

# Indexing
Каждая транзакция попавшая в узел проходит первичную валидацию перед записью в мемпул - проверки на даблСпенд, минимальный фи, локТайм.

Транзакции в мемпуле не имеют порядкого номера и высоты, как и связи с блоком.\
Pocket sqlite database сохраняет все транзакции в разряженую таблицу `main.Transactions` без указания `main.Transactions.TxOut` и `main.Transactions.Block` - [main.sql](https://github.com/pocketnetteam/pocketnet.core/blob/feature/sqlite/src/pocketdb/docs/main.sql)

> Здесь и далее `main.Transactions.TxOut` - `<Db>.<Table>.<Field>` означает SQLite базу данных Pocket Social Data - `pocketdb/main.sqlite3`

Все типы транзакций, хранящиеся в `main.Transactions` определены как Enum и имеют зафиксированное целое значение - [src/pocketdb/models/base/PocketTypes.h]()

Сформированный блок (1) подключается к цепи (2).

После успешной проверки обновляются:
- `chainActive.Height()`
- `chainActive.Tip()`
- set block hash `main.Transactions.Block` for transactions in block
- set order number `main.Transaction.TxOut` for transactions in block
- `main.Utxo` добавляется запись с каждым значащим оутом из транзакции
- `main.Utxo` также фиксируются "потраченными" все оуты из секции `inputs` транзакции
- `main.Ratings` добавляются обновленные накопительные данные рейтингов по типам транзакций

<br>

> `(1)` - TODO miner.cpp\
> `(2)` - TODO validation.cpp











