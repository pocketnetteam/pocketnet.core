#!/usr/bin/env python3
# Copyright (c) 2018-2023 The Pocketnet Core developers

from enum import Enum

# -----------------------------------------------------------------------------------------------------------------

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

# -----------------------------------------------------------------------------------------------------------------

class Account:
    Address = ''
    PrivKey = ''
    Name = ''
    def __init__(self, address, privkey, name):
        self.Address = address
        self.PrivKey = privkey
        self.Name = name

        self.content = []
        self.comment = []
        self.subscribes = []
        self.blockings = []

# -----------------------------------------------------------------------------------------------------------------

class AccountPayload:
    TxType = '75736572496e666f'
    Referrer = ''
    Name = ''
    Image = ''
    Language = ''
    About = ''
    Url = ''
    Donations = ''
    PublicKey = ''

    def __init__(self, name, image='image', language='en', about='about', url='url', donations='donations', pubkey='pubkey', referrer=''):
        self.Referrer = referrer
        self.Name = name
        self.Image = image
        self.Language = language
        self.About = about
        self.Url = url
        self.Donations = donations
        self.PublicKey = pubkey

    def Serialize(self):
        return {
            "r": self.Referrer,
            "n": self.Name,
            "i": self.Image,
            "l": self.Language,
            "a": self.About,
            "s": self.Url,
            "b": self.Donations,
            "k": self.PublicKey
        }

# -----------------------------------------------------------------------------------------------------------------

class AccountDeletePayload:
    TxType = '61636344656c'
    def Serialize(self):
        return { }

# -----------------------------------------------------------------------------------------------------------------

class AccountSettingPayload:
    TxType = '616363536574'
    Data = ''
    def __init__(self, data={'settings1':'value1'}):
        self.Data = data
    def Serialize(self):
        return {
            "d": self.Data
        }

# -----------------------------------------------------------------------------------------------------------------

class ContentPostPayload:
    TxType = '7368617265'
    TxRepost = ''
    TxEdit = ''
    Language = ''
    Message = ''
    Caption = ''
    Url = ''
    Tags = []
    Images = []

    def __init__(self, language='en', message='massage', caption='captions', url='url', tags=['tag1','tag2'], images=['image1','image2'], txRepost='', txEdit=''):
        self.TxRepost = txRepost
        self.TxEdit = txEdit
        self.Language = language
        self.Message = message
        self.Caption = caption
        self.Url = url
        self.Tags = tags
        self.Images = images

    def Serialize(self):
        return {
            "txidRepost": self.TxRepost,
            "txidEdit": self.TxEdit,
            "l": self.Language,
            "m": self.Message,
            "c": self.Caption,
            "u": self.Url,
            "t": self.Tags,
            "i": self.Images
        }

# -----------------------------------------------------------------------------------------------------------------

class ContentArticlePayload(ContentPostPayload):
    TxType = '61727469636c65'

# -----------------------------------------------------------------------------------------------------------------

class ContentAudioPayload(ContentPostPayload):
    TxType = '617564696f'

# -----------------------------------------------------------------------------------------------------------------

class ContentVideoPayload(ContentPostPayload):
    TxType = '766964656f'

# -----------------------------------------------------------------------------------------------------------------

class ContentStreamPayload(ContentPostPayload):
    TxType = '73747265616d'

# -----------------------------------------------------------------------------------------------------------------

class ContentDeletePayload:
    TxType = '636f6e74656e7444656c657465'
    ContentTx = ''
    Settings = ''
    def __init__(self, contentTx, settings={'setting1':'value1'}):
        self.ContentTx = contentTx
        self.Settings = settings
    def Serialize(self):
        return {
            'txidEdit': self.ContentTx,
            's': self.Settings
        }

# -----------------------------------------------------------------------------------------------------------------

class CommentDeletePayload:
    TxType = '636f6d6d656e7444656c657465'
    PostId = ''
    ParentId = ''
    AnswerId = ''
    Id = ''
    def __init__(self, postId, editId, parentId='', answerId=''):
        self.PostId = postId
        self.ParentId = parentId
        self.AnswerId = answerId
        self.EditId = editId
    def Serialize(self):
        return {
            'postid': self.PostId,
            'parentid': self.ParentId,
            'answerid': self.AnswerId,
            'id': self.EditId
        }

# -----------------------------------------------------------------------------------------------------------------

class CommentEditPayload(CommentDeletePayload):
    TxType = '636f6d6d656e7445646974'
    Message = ''
    def __init__(self, postId, editId, parentId='', answerId='', message='comment test message'):
        CommentDeletePayload.__init__(self, postId, editId, parentId, answerId)
        self.Message = message
    def Serialize(self):
        ser = CommentDeletePayload.Serialize(self)
        ser['msg'] = self.Message
        return ser

# -----------------------------------------------------------------------------------------------------------------

class CommentPayload(CommentEditPayload):
    TxType = '636f6d6d656e74'
    def __init__(self, postId, parentId='', answerId='', message='comment test message'):
        CommentEditPayload.__init__(self, postId, '', parentId, answerId, message)

# -----------------------------------------------------------------------------------------------------------------

class BlockingPayload:
    TxType = '626c6f636b696e67'
    Address = ''
    Addresses = ''
    def __init__(self, address, addresses=''):
        self.Address = address
        self.Addresses = addresses
    def Serialize(self):
        return {
            'address': self.Address,
            'addresses': self.Addresses
        }

# -----------------------------------------------------------------------------------------------------------------

class UnblockingPayload(BlockingPayload):
    TxType = '756e626c6f636b696e67'

# -----------------------------------------------------------------------------------------------------------------

class BoostPayload:
    TxType = '636f6e74656e74426f6f7374'
    ContentTx = ''
    def __init__(self, contentTx):
        self.ContentTx = contentTx
    def Serialize(self):
        return {
            'content': self.ContentTx
        }

# -----------------------------------------------------------------------------------------------------------------

class ComplainPayload:
    TxType = '636f6d706c61696e5368617265'
    ContentTx = ''
    Reason = 1
    def __init__(self, contentTx, reason=1):
        self.ContentTx = contentTx
        self.Reason = reason
    def Serialize(self):
        return {
            'share': self.ContentTx,
            "reason": self.Reason
        }

# -----------------------------------------------------------------------------------------------------------------

class ScoreContentPayload:
    TxType = '7570766f74655368617265'
    ContentTx = ''
    Value = 0
    ContentAddress = ''
    def __init__(self, contentTx, value, contentAddress):
        self.ContentTx = contentTx
        self.Value = value
        self.ContentAddress = contentAddress
    def Serialize(self):
        return {
            'share': self.ContentTx,
            "value": self.Value
        }

# -----------------------------------------------------------------------------------------------------------------

class ScoreCommentPayload:
    TxType = '6353636f7265'
    CommentTx = ''
    Value = 0
    ContentAddress = ''
    def __init__(self, commentTx, value, contentAddress):
        self.CommentTx = commentTx
        self.Value = value
        self.ContentAddress = contentAddress
    def Serialize(self):
        return {
            'commentid': self.CommentTx,
            "value": self.Value
        }
# -----------------------------------------------------------------------------------------------------------------

class SubscribePayload:
    TxType = '737562736372696265'
    Address = ''
    def __init__(self, address):
        self.Address = address
    def Serialize(self):
        return {
            'address': self.Address
        }

# -----------------------------------------------------------------------------------------------------------------

class SubscribePrivatePayload(SubscribePayload):
    TxType = '73756273637269626550726976617465'

# -----------------------------------------------------------------------------------------------------------------

class UnsubscribePayload(SubscribePayload):
    TxType = '756e737562736372696265'

# -----------------------------------------------------------------------------------------------------------------

class ModFlagPayload:
    TxType = '6d6f64466c6167'
    ContentTx = ''
    ContentAddress = ''
    Reason = -1
    def __init__(self, contentTx, contentAddress, reason=1):
        self.ContentTx = contentTx
        self.ContentAddress = contentAddress
        self.Reason = reason
    def Serialize(self):
        return {
            's2': self.ContentTx,
            's3': self.ContentAddress,
            'i1': self.Reason
        }

# -----------------------------------------------------------------------------------------------------------------

class ModVotePayload:
    TxType = '6d6f64566f7465'
    JuryTx = ''
    Verdict = -1
    def __init__(self, juryTx, verdict):
        self.JuryTx = juryTx
        self.Verdict = verdict
    def Serialize(self):
        return {
            's2': self.JuryTx,
            'i1': self.Verdict
        }

# -----------------------------------------------------------------------------------------------------------------
# -----------------------------------------------------------------------------------------------------------------
# -----------------------------------------------------------------------------------------------------------------
# -----------------------------------------------------------------------------------------------------------------
# -----------------------------------------------------------------------------------------------------------------
# -----------------------------------------------------------------------------------------------------------------
# -----------------------------------------------------------------------------------------------------------------

class EmptyPayload:
    TxType = ''
    def __init__(self):
        pass
    def Serialize(self):
        return {
            
        }
