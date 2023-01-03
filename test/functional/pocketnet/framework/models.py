#!/usr/bin/env python3
# Copyright (c) 2018-2023 The Pocketnet Core developers

import json

# --------------------------------------------------------------------------------

class Account:
    Address = ''
    PrivKey = ''
    def __init__(self, address, privkey):
        self.Address = address
        self.PrivKey = privkey

# --------------------------------------------------------------------------------

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

# --------------------------------------------------------------------------------

class AccountDeletePayload:
    TxType = '61636344656c'
    def Serialize(self):
        return { }

# --------------------------------------------------------------------------------

class AccounSettingPayload:
    TxType = '616363536574'
    Data = ''
    def __init__(self, data):
        self.Data = data
    def Serialize(self):
        return {
            "d": self.Data
        }

# --------------------------------------------------------------------------------

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

    def __init__(self, language='', message='massage', caption='captions', url='url', tags=['tag1','tag2'], images=['image1','image2'], txRepost='', txEdit=''):
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

# --------------------------------------------------------------------------------

class ContentArticlePayload(ContentPostPayload):
    TxType = '61727469636c65'

# --------------------------------------------------------------------------------

class ContentAudioPayload(ContentPostPayload):
    TxType = '617564696f'

# --------------------------------------------------------------------------------

class ContentVideoPayload(ContentPostPayload):
    TxType = '766964656f'

# --------------------------------------------------------------------------------

class ContentStreamPayload(ContentPostPayload):
    TxType = '73747265616d'

# --------------------------------------------------------------------------------

class ContentDeletePayload:
    TxType = '636f6e74656e7444656c657465'
    TxEdit = ''
    Settings = ''
    def __init__(self, txEdit, settings={'setting1':'value1'}):
        self.TxEdit = txEdit
        self.Settings = settings
    def Serialize(self):
        return {
            'txidEdit': self.TxEdit,
            's': self.Settings
        }

# --------------------------------------------------------------------------------

class CommentDeletePayload:
    TxType = '636f6d6d656e7444656c657465'
    PostId = ''
    ParentId = ''
    AnswerId = ''
    Id = ''
    def __init__(self, editId, postId, parentId='', answerId=''):
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

# --------------------------------------------------------------------------------

class CommentPayload(CommentDeletePayload):
    TxType = '636f6d6d656e74'
    PostId = ''
    ParentId = ''
    AnswerId = ''
    Id = ''
    Message = ''
    def __init__(self, postId, message, editId='', parentId='', answerId=''):
        CommentDeletePayload.__init__(self, postId, parentId, answerId, editId)
        self.Message = message
    def Serialize(self):
        ser = CommentDeletePayload.Serialize(self)
        ser['msg'] = self.Message
        return ser

# --------------------------------------------------------------------------------

class CommentEditPayload:
    TxType = '636f6d6d656e7445646974'

# --------------------------------------------------------------------------------

class BlockingPayload:
    TxType = '626c6f636b696e67'
    Address = ''
    Addresses = []
    def __init__(self, address, addresses=[]):
        self.Address = address
        self.Addresses = addresses
    def Serialize(self):
        return {
            'address': self.Address,
            'addresses': json.dumps(self.Addresses)
        }

# --------------------------------------------------------------------------------

class UnblockingPayload(BlockingPayload):
    TxType = '756e626c6f636b696e67'

# --------------------------------------------------------------------------------

class BoostPayload:
    TxType = '636f6e74656e74426f6f7374'
    ContentTx = ''
    def __init__(self, contentTx):
        self.ContentTx = contentTx
    def Serialize(self):
        return {
            'content': self.ContentTx
        }

# --------------------------------------------------------------------------------

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

# --------------------------------------------------------------------------------

class ScorePostPayload:
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
            'commentid': self.ContentTx,
            "value": self.Value
        }

# --------------------------------------------------------------------------------

class ScoreCommentPayload(ScorePostPayload):
    TxType = '6353636f7265'

# --------------------------------------------------------------------------------

class SubscribePayload:
    TxType = '737562736372696265'
    Address = ''
    def __init__(self, address):
        self.Address = address
    def Serialize(self):
        return {
            'address': self.Address
        }

# --------------------------------------------------------------------------------

class SubscribePrivatePayload(SubscribePayload):
    TxType = '73756273637269626550726976617465'

# --------------------------------------------------------------------------------

class SubscribePrivatePayload(SubscribePayload):
    TxType = '756e737562736372696265'

# --------------------------------------------------------------------------------

class ModFlagPayload:
    TxType = ''
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

# --------------------------------------------------------------------------------
# --------------------------------------------------------------------------------
# --------------------------------------------------------------------------------
# --------------------------------------------------------------------------------
# --------------------------------------------------------------------------------
# --------------------------------------------------------------------------------
# --------------------------------------------------------------------------------
# --------------------------------------------------------------------------------

class EmptyPayload:
    TxType = ''
    def __init__(self):
        pass
    def Serialize(self):
        return {
            
        }
