
## Accounts with activity in posts
```sql
select
  sum(
    (select count() from Transactions c indexed by Transactions_Type_Last_String3_Height where c.Type in (204) and c.Last in (0,1) and c.Height > (1853643 - (1440 * 30 * 3)) and c.String3 = t.String2)
    -
    (select count() from Transactions c indexed by Transactions_Type_String1_String3_Height where c.Type in (204) and c.Height > (1853643 - (1440 * 30 * 3)) and c.String3 = t.String2 and c.String1 = t.String1)
  )cmnt_cnt
from Transactions t indexed by Transactions_Type_Last_String1_Height_Id
where t.Type in (200,201,202) and t.Last = 1 and t.Height > 0
  and t.String1 = 'PTcArXMkhsKMUrzQKn2SXmaVZv4Q7sEpBt'
```