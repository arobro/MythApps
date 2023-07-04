include ( ../../mythconfig.mak )
include ( ../../settings.pro )

!exists( ../../settings.pro ) {
    PREFIX = /usr
}

TEMPLATE = aux

defaultfiles.path = $${PREFIX}/share/mythtv/themes/default
defaultfiles.files = default/*.xml default/*.png

INSTALLS += defaultfiles 

