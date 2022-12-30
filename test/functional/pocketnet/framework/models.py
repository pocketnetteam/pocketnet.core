#!/usr/bin/env python3
# Copyright (c) 2018-2023 The Pocketnet Core developers

class Account:
    TxType = '75736572496e666f'

    Name = ''
    Image = ''
    Language = ''
    About = ''
    S = ''
    B = ''
    PublicKey = ''

    def __init__(self, name, image, language, about, s, b, pubkey):
        self.Name = name
        self.Image = image
        self.Language = language
        self.About = about
        self.S = s
        self.B = b
        self.PublicKey = pubkey

    def Serialize(self):
        return {
            "n": self.Name,
            "i": self.Image,
            "l": self.Language,
            "a": self.About,
            "s": self.S,
            "b": self.B,
            "k": self.PublicKey
        }

# --------------------------------------------------------------------------------

class AccountDelete:
    TxType = '61636344656c'
    def Serialize(self):
        return { }
# --------------------------------------------------------------------------------

class ContentPost:
    TxType = '7368617265'

    Language = ''
    Message = ''
    Caption = ''
    Url = ''
    Tags = []
    Images = []

    def __init__(self, language, message, caption, url, tags, images):
        self.Language = language
        self.Message = message
        self.Caption = caption
        self.Url = url
        self.Tags = tags
        self.Images = images

    def Serialize(self):
        return {
            "l": self.Language,
            "m": self.Message,
            "c": self.Caption,
            "u": self.Url,
            "t": self.Tags,
            "i": self.Images
        }

# --------------------------------------------------------------------------------
# --------------------------------------------------------------------------------
# --------------------------------------------------------------------------------