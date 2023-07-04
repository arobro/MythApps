#!/bin/sh
#Adds MythApps to menu in mythtv

if [ "$(id -u)" != "0" ]; then
    echo "Sorry, you are not root."
    exit 1
fi

if [ -d "/usr/local/share/mythtv/themes/defaultmenu/" ]; then
    cd /usr/local/share/mythtv/themes/defaultmenu/
elif [ -d "/usr/share/mythtv/themes/defaultmenu/" ]; then
    cd /usr/share/mythtv/themes/defaultmenu/
else
    echo "Sorry, cant not find the correct path for MythTV."
    exit 1
fi

if grep -q mythapps "library.xml"; then
  exit 1
fi

sed -i '0,/\/button>/s//\/button> \
\
    <button> \
        <type>MENU_MythApps\<\/type> \
        <text>Myth Apps\<\/text> \
        <action>PLUGIN mythapps\<\/action> \
    <\/button> \
/' library.xml
