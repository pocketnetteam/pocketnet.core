#!/usr/bin/env python3
# Copyright (c) 2023 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0

from dataclasses import asdict, dataclass, field
from enum import Enum

# -----------------------------------------------------------------------------------------------------------------

def AsDisct(obj):
    return asdict(obj, dict_factory=lambda x: {k: v for (k, v) in x if v is not None})

class ConsensusResult(Enum):
    Success = 0
    NotRegistered = 1
    ContentLimit = 2
    ScoreLimit = 3
    DoubleScore = 4
    SelfScore = 5
    ChangeInfoLimit = 6
    InvalideSubscribe = 7
    DoubleSubscribe = 8
    SelfSubscribe = 9
    Unknown = 10
    Failed = 11
    NotFound = 12
    DoubleComplain = 13
    SelfComplain = 14
    ComplainLimit = 15
    ComplainLowReputation = 16
    ContentSizeLimit = 17
    NicknameDouble = 18
    NicknameLong = 19
    ReferrerSelf = 20
    FailedOpReturn = 21
    InvalidBlocking = 22
    DoubleBlocking = 23
    SelfBlocking = 24
    DoubleContentEdit = 25
    ContentEditLimit = 26
    ContentEditUnauthorized = 27
    ManyTransactions = 28
    CommentLimit = 29
    CommentEditLimit = 30
    CommentScoreLimit = 31
    Blocking = 32
    Size = 33
    InvalidParentComment = 34
    InvalidAnswerComment = 35
    DoubleCommentEdit = 37
    SelfCommentScore = 38
    DoubleCommentDelete = 39
    DoubleCommentScore = 40
    CommentDeletedEdit = 42
    NotAllowed = 44
    ChangeTxType = 45
    ContentDeleteUnauthorized = 46
    ContentDeleteDouble = 47
    AccountSettingsDouble = 48
    AccountSettingsLimit = 49
    ChangeInfoDoubleInBlock = 50
    CommentDeletedContent = 51
    RepostDeletedContent = 52
    AlreadyExists = 53
    PocketDataNotFound = 54
    TxORNotFound = 55
    ComplainDeletedContent = 56
    ScoreDeletedContent = 57
    RelayContentNotFound = 58
    BadPayload = 59
    ScoreLowReputation = 60
    ChangeInfoDoubleInMempool = 61
    Duplicate = 62
    NotImplemeted = 63
    SelfFlag = 64
    ExceededLimit = 65
    LowReputation = 66
    AccountDeleted = 67
    AccountBanned = 68

# -----------------------------------------------------------------------------------------------------------------

@dataclass
class Account:
    Address: str = ""
    PrivKey: str = ""
    Name: str = ""
    content: list = field(default_factory=list)
    comment: list = field(default_factory=list)
    subscribes: list = field(default_factory=list)
    blockings: list = field(default_factory=list)

# -----------------------------------------------------------------------------------------------------------------

@dataclass
class AccountPayload:
    TxType = "75736572496e666f"
    name: str = ""
    image: str = "image"
    language: str = "en"
    about: str = "about"
    url: str = "url"
    donations: str = "donations"
    pubkey: str = "pubkey"
    referrer: str = ""
    def Serialize(self):
        return {
            "r": self.referrer,
            "n": self.name,
            "i": self.image,
            "l": self.language,
            "a": self.about,
            "s": self.url,
            "b": self.donations,
            "k": self.pubkey,
        }

# -----------------------------------------------------------------------------------------------------------------

@dataclass
class AccountDeletePayload:
    TxType = "61636344656c"
    def Serialize(self):
        return asdict(self)

# -----------------------------------------------------------------------------------------------------------------

@dataclass
class AccountSettingPayload:
    TxType = "616363536574"
    data: dict = field(default_factory=lambda: {"settings1": "value1"})
    def Serialize(self):
        return {
            "d": self.data,
        }

# -----------------------------------------------------------------------------------------------------------------

@dataclass
class ContentPostPayload:
    TxType = "7368617265"
    language: str = "en"
    message: str = "message"
    caption: str = "captions"
    url: str = "url"
    tags: list = field(default_factory=lambda: ["tag1", "tag2"])
    images: list = field(default_factory=lambda: ["image1", "image2"])
    txrepost: str = ""
    txedit: str = ""
    def Serialize(self):
        return {
            "txidRepost": self.txrepost,
            "txidEdit": self.txedit,
            "l": self.language,
            "m": self.message,
            "c": self.caption,
            "u": self.url,
            "t": self.tags,
            "i": self.images,
        }

# -----------------------------------------------------------------------------------------------------------------

@dataclass
class ContentArticlePayload(ContentPostPayload):
    TxType = "61727469636c65"

# -----------------------------------------------------------------------------------------------------------------

@dataclass
class ContentAudioPayload(ContentPostPayload):
    TxType = "617564696f"

# -----------------------------------------------------------------------------------------------------------------

@dataclass
class ContentVideoPayload(ContentPostPayload):
    TxType = "766964656f"

# -----------------------------------------------------------------------------------------------------------------

@dataclass
class ContentStreamPayload(ContentPostPayload):
    TxType = "73747265616d"

# -----------------------------------------------------------------------------------------------------------------

@dataclass
class ContentDeletePayload:
    TxType = "636f6e74656e7444656c657465"
    content_tx: str = ""
    settings: dict = field(default_factory=lambda: {"setting1": "value1"})
    def Serialize(self):
        return {"txidEdit": self.content_tx, "s": self.settings}

# -----------------------------------------------------------------------------------------------------------------

@dataclass
class ContentCollectionsPayload:
    TxType = "636f6c6c656374696f6e"
    language: str = "en"
    caption: str = "caption"
    image: str = "image"
    contenttypes: int = 200
    contentids: list = field(default_factory=lambda: ["123", "456"])
    txedit: str = ""
    def Serialize(self):
        return {
            "c": self.caption,
            "i": self.image,
            "l": self.language,
            "contentTypes": self.contenttypes,
            "contentIds": self.contentids,
            "txidEdit": self.txedit,
        }

# -----------------------------------------------------------------------------------------------------------------

@dataclass
class CommentDeletePayload:
    TxType = "636f6d6d656e7444656c657465"
    postid: str = ""
    id: str = ""
    parentid: str = ""
    answerid: str = ""
    def Serialize(self):
        return asdict(self)

# -----------------------------------------------------------------------------------------------------------------

@dataclass
class CommentEditPayload(CommentDeletePayload):
    TxType = "636f6d6d656e7445646974"
    msg: str = "comment test message"

# -----------------------------------------------------------------------------------------------------------------

@dataclass
class CommentPayload(CommentEditPayload):
    TxType = "636f6d6d656e74"
    def __init__(self, postId, parentId="", answerId="", message="comment test message"):
        CommentEditPayload.__init__(self, postId, "", parentId, answerId, message)

# -----------------------------------------------------------------------------------------------------------------

@dataclass
class BlockingPayload:
    TxType = "626c6f636b696e67"
    address: str = ""
    addresses: str = ""
    def Serialize(self):
        # return asdict(self)
        return {"address": self.address, "addresses": self.addresses}

# -----------------------------------------------------------------------------------------------------------------

@dataclass
class UnblockingPayload(BlockingPayload):
    TxType = "756e626c6f636b696e67"

# -----------------------------------------------------------------------------------------------------------------

@dataclass
class BoostPayload:
    TxType = "636f6e74656e74426f6f7374"
    content: str = ""
    def Serialize(self):
        return asdict(self)

# -----------------------------------------------------------------------------------------------------------------

@dataclass
class ComplainPayload:
    TxType = "636f6d706c61696e5368617265"
    share: str = ""
    reason: int = 1
    def Serialize(self):
        return asdict(self)

# -----------------------------------------------------------------------------------------------------------------

@dataclass
class ScoreContentPayload:
    TxType = "7570766f74655368617265"
    content_tx: str = ""
    value: int = 0
    content_address: str = ""
    @property
    def ContentAddress(self):
        return self.content_address
    def Serialize(self):
        return {
            "share": self.content_tx,
            "value": self.value,
        }

# -----------------------------------------------------------------------------------------------------------------

@dataclass
class ScoreCommentPayload:
    TxType = "6353636f7265"
    comment_tx: str = ""
    value: int = 0
    content_address: str = ""
    @property
    def ContentAddress(self):
        return self.content_address
    def Serialize(self):
        return {
            "commentid": self.comment_tx,
            "value": self.value,
        }

# -----------------------------------------------------------------------------------------------------------------

@dataclass
class SubscribePayload:
    TxType = "737562736372696265"
    address: str = ""
    def Serialize(self):
        return asdict(self)


# -----------------------------------------------------------------------------------------------------------------

@dataclass
class SubscribePrivatePayload(SubscribePayload):
    TxType = "73756273637269626550726976617465"

# -----------------------------------------------------------------------------------------------------------------

@dataclass
class UnsubscribePayload(SubscribePayload):
    TxType = "756e737562736372696265"

# -----------------------------------------------------------------------------------------------------------------

@dataclass
class ModFlagPayload:
    TxType = "6d6f64466c6167"
    content_tx: str = ""
    content_address: str = ""
    reason: int = 1
    def Serialize(self):
        return {
            "s2": self.content_tx,
            "s3": self.content_address,
            "i1": self.reason,
        }


# -----------------------------------------------------------------------------------------------------------------


@dataclass
class ModVotePayload:
    TxType = "6d6f64566f7465"
    jury_tx: str = ""
    verdict: int = -1
    def Serialize(self):
        return {
            "s2": self.jury_tx,
            "i1": self.verdict,
        }


# -----------------------------------------------------------------------------------------------------------------

@dataclass
class Payload:
    s1: str = None
    s2: str = None
    s3: str = None
    s4: str = None
    s5: str = None
    s6: str = None
    s7: str = None
    i1: int = None
    def Serialize(self):
        return AsDisct(self)

@dataclass
class Transaction:
    s1: str = None
    s2: str = None
    s3: str = None
    s4: str = None
    s5: str = None
    i1: int = None
    p: Payload = None
    def Serialize(self):
        return AsDisct(self)


# -----------------------------------------------------------------------------------------------------------------

@dataclass
class BartAccountPayload(Transaction):
    TxType = "6272746163636f756e74"

@dataclass
class BartOfferPayload(Transaction):
    TxType = "6272746f66666572"

# -----------------------------------------------------------------------------------------------------------------

@dataclass
class AppPayload(Transaction):
    TxType = "6d696e69617070"

# -----------------------------------------------------------------------------------------------------------------
