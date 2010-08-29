# This is an application
TEMPLATE=app

# The binary name
TARGET=melodiq

# This app uses Qtopia
CONFIG+=qtopia
DEFINES+=QTOPIA

QT+=webkit

# I18n info
STRING_LANGUAGE=en_US
LANGUAGES=en_US

HEADERS=\
    src/melodiq.h

SOURCES=\
    src/main.cpp \
    src/melodiq.cpp

# Package info
pkg [
    name=melodiq
    desc="Front-end for music recognition services"
    license=GPLv3
    version=1.0
    maintainer="Anton Olkhovik <ant007h@gmail.com>"
]

target [
    hint=sxe
    domain=untrusted
]

desktop [
    hint=desktop
    files=melodiq.desktop
    path=/apps/Applications
]

pics [
    hint=pics
    files=pics/*
    path=/pics/melodiq
]

help [
    hint=help
    source=help
    files=*.html
]
