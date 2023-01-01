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

    def __init__(self, referrer, name, image, language, about, url, donations, pubkey):
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

    def __init__(self, txRepost, txEdit, language, message, caption, url, tags, images):
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
    def __init__(self, txEdit, settings):
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
    def __init__(self, postId, parentId, answerId, editId):
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
    def __init__(self, postId, parentId, answerId, editId, message):
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
    def __init__(self, address, addresses):
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
# --------------------------------------------------------------------------------
# --------------------------------------------------------------------------------
# --------------------------------------------------------------------------------
# --------------------------------------------------------------------------------
# --------------------------------------------------------------------------------
# --------------------------------------------------------------------------------
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
