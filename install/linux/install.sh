#!/bin/bash
TARGET_DIR=/opt/igal_qt

echo "Installing igal_qt to $TARGET_DIR"
read -p "Proceed? (y/N):" -n 1 -r REPLY
echo ""

if [[ ! $REPLY =~ ^[Yy]$ ]] then
    exit 1
fi

read -p "Create application desktop entry? (Y/n):" -n 1 -r REPLY_DESKTOP_ENTRY
echo ""

read -p "Use system libs? (Y/n):" -n 1 -r REPLY_USE_SYSTEM_LIBS
echo ""

set -x

sudo rm -rf $TARGET_DIR
sudo mkdir $TARGET_DIR

sudo cp ./bin $TARGET_DIR/bin -r

if [[ ! $REPLY_USE_SYSTEM_LIBS ]] then
    sudo cp ./lib $TARGET_DIR/lib -r
    sudo cp ./plugins $TARGET_DIR/plugins -r
    sudo cp ./rsc $TARGET_DIR/rsc -r
else
    sudo rm $TARGET_DIR/bin/qt.conf
fi

if [[ $REPLY_DESKTOP_ENTRY =~ ^[Yy]$ ]] then
    sudo xdg-desktop-menu install igal_qt.desktop --mode system --novendor
    sudo xdg-desktop-menu forceupdate
fi

echo "Finished!"