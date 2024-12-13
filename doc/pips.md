# PIPs that are implemented by Pocketcoin Core:


## `PIP 100`: Changes in the policy of emission payments

Fork start at:
- Main net: 2162400

Changes:
- All payments within the lottery for ratings on comments are canceled
- All payments within the lottery for ratings for referrals are canceled
- The total payment under the lottery for grades for posts is changing - a new value of 2.5% of the total issue amount
- The maximum number of lottery participants in one block changes - new value 5

[PR #563](https://github.com/pocketnetteam/pocketnet.core/pull/563)

## `PIP 101`: Correction of incorrect blocking logic

Fork start at:
- Main net: 2360000
- Test net: 1950500

Changes:

The error lies in the fact that for transactions before this correction, the registration of the account that created the transaction is not checked.

[PR #589](https://github.com/pocketnetteam/pocketnet.core/pull/589)


## `PIP 103`: Disable ratings (3) for trial users

Fork start at:
- Main net: 2552000
- Test net: 2267333

CHanges:

At the moment (version 0.21), it is forbidden to leave grades 1 and 2 with an account in the trial status. This change adds a score of 3 for to this list.

[PR #611](https://github.com/pocketnetteam/pocketnet.core/pull/611)


## `PIP 104`: Allow unsubscribe/unblocking for target deleted account

Fork start at:
- Main net: 2552000
- Test net: 2340000

Changes:
- Allow for a subscription cancellation/blocking transaction, even if the final account is deleted [PR #626](https://github.com/pocketnetteam/pocketnet.core/pull/626)


## `PIP 105`: Disable actions (comments, likes, etc.) for accounts that have been blocked by the authors of the "actions"

Fork start at:
- Main net: 2794500
- Test net: 2574300

Changes:

It is necessary to prohibit actions when blocking, regardless of the direction of blocking. If A has blocked B, then B cannot interact with A, and also A cannot interact with B.

This rule applies to the following transactions:
- Boost
- Comment
- CommentEdit
- Post (Repost)
- Score to comment
- Score to "content" (Post, Video, Article, Audio)
- Subscribe/Private (Unsubscribe allowed)

[PR #683](https://github.com/pocketnetteam/pocketnet.core/pull/683)


## `PIP 106`: Allow restore deleted account

Fork start at:
- Main net: 2930000
- Test net: 2850000

Changes:
- This change concerns the logic of processing the transaction of creating/editing an account profile. At the moment, after deleting the account (profile deletion transaction), there is no way to restore the account. PIP 106 cancels the verification condition that the account has been deleted [PR #700](https://github.com/pocketnetteam/pocketnet.core/pull/700)


## `PIP 107`: Increase the time limit for editing content

Fork start at:
- Main net: 2930000

Changes:
- Increasing the post editing time from 24 hours (1440 blocks) to 1 month (43200 blocks) [PR #702](https://github.com/pocketnetteam/pocketnet.core/pull/702)


## `PIP 108`: Extend moderation flag categories

Fork start at:
- Main net: 3123800
- Test net: 3100000

Changes:
- Increasing the number of categories for content moderation. Current max value: 5. New max value: 10. [PR #737](https://github.com/pocketnetteam/pocketnet.core/pull/737)


## `PIP 109`: Moderation on Barteron, Boost for Barteron offers and Multi‐Contetnt Collections

Fork start at:
- Main net: 3291900
- Test net: 3373650

Changes:
- Enabling the moderation subsystem for Barteron transactions (Flags for Offers and Accounts). [PR #767](https://github.com/pocketnetteam/pocketnet.core/pull/767)
- Enabling Boosts for Barteron Offers [PR #767](https://github.com/pocketnetteam/pocketnet.core/pull/767)
- Policy change restriction on the type of content in collections - now we cannot mix content (Posts and Videos, for example). This PIP changes this behavior by disabling this restriction. [PR #767](https://github.com/pocketnetteam/pocketnet.core/pull/767)
- Fix count limit for complain transaction. [PR #767](https://github.com/pocketnetteam/pocketnet.core/pull/767)
- Allow `.` symbol in App ID. [PR #773](https://github.com/pocketnetteam/pocketnet.core/pull/773)


## `PIP 110`: Lottery reward for moderation votes

Fork start at:
- Main net: 9999999
- Test net: 3500000

Changes:
- Redistribute lottery rewards: 87.5% for nodes, 2.5% for posts, 10% for moderation votes [PR #779](https://github.com/pocketnetteam/pocketnet.core/pull/779)


## `PIP 111`: Moderation votes check enable after jury creation

Fork start at:
- Main net: 3109300
- Test net: 3500000

Changes:
- Moderation votes check enable after jury creation [PR #786](https://github.com/pocketnetteam/pocketnet.core/pull/786)

