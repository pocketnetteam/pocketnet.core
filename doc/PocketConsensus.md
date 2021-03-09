# Pocketnet consensus rules

## Lottery consensus rules
- `OR_SCORE in (4,5) or (1)` - transaction OP_RETURN
- `select address from Scores where txid` - overwrite with 1st out
- `select address from Posts where txid` - overwrite with OP_RETURN
- `select value from Reputation where address and height`
- `select count from Scores where address_from and address_to and time_from and time_to and values in (4,5) or (1)`
- `select address from Users where refferer by post author referrer and regdate`

## Antibot Posts


# Database

```sql
create table Transactions
    TxType int not null,
    TxId string not null,
    Height int null,
    Address string not null,
    Value int null,

```

```sql
create table Reputations
    Key string not null,

```

```sql
create table Payload
    TxId string not null,
    Data blob null
```

