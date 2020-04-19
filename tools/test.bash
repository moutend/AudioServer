#!/bin/bash

curl -i -X POST --data '{"isForcePush":true,"commands": [{"type": 3, "value": "こんにちは、私の名前は強固です。日本語の音声をお届けします。"},{"type": 3, "value": "こんにちは、私の名前は強固です。日本語の音声をお届けします。"}]}' \
http://localhost:7902/v1/audio
