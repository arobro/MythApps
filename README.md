# WHAT IS MYTHAPPS? #
MythApps is a wrapper that gives the illusion of Kodi plugins running natively from MythTV. Apps will comply with the Mythtv theming and remote commands. 
Kodi has a JSON API that allows remote control and to query the GUI. MythApps interprets this into its own GUI. Videos are played in the Kodi video player with some skinning tricks to make it feel like a native application. 

# SCREENSHOTS
![Screenshot](preview.png)
![Screenshot](preview_search_results.png)
![Screenshot](preview_settings_pause_finishedPlaying.png)

# FEATURES
Supports 4K \
Kodi has a very large library of Apps. \
Consistent interface and MythTV theming. \
Global search on home screen or specifc search when in a directory. \
Play local media not on the mythtv backend. \
MythTV remote support. \
Kodi supports L1 DRM on Android, L3 on Linux and Windows.
 
# HOW TO INSTALL
## Compile MythTV and MythPlugins (Linux)

Follow how to compile a plugin: https://www.mythtv.org/wiki/Building_Plugins:HelloMyth

Open terminal and navigate to the mythtv source folder.  \
cd mythtv/mythplugins\
git clone https://github.com/arobro/mythapps/ \
qmake\
sudo make install

You may need to copy the theme files from mythtv/mythplugins/mythapps/theme/default to /usr/local/share/mythtv/themes/default \
\
Add MythApps to menu by running 'addMythAppsToMenu.sh' or add in the below code: gedit /usr/local/share/mythtv/themes/defaultmenu/library.xml 

\<button\> \
<type\>MENU_MythApps\</type\> \
\<text>Myth Apps</text\> \
\<action\>PLUGIN mythapps\</action\> \
\</button\>

### Android
1. Use prebuilt mythtv apk with MythApps or compile mythtv with the MythApps plugins.
2. You will need to install an additional apk called MythApp Services.


Compile: https://www.mythtv.org/wiki/MythTV_on_Android and make sure to use https://github.com/MythTV/packaging.git

Apply android.patch under the MythApps directory.
Under the MythPlugins directory, add  SUBDIRS += mythapps to packaging/android/build/mythplugins/config.pro

#### MythApp Services
You can compile using Android Studio or a prebuild MythApp Services apk. <br />
MythApp Services apk - mythapps/MythAppsServices/app/build/intermediates/apk/debug/app-debug.apk

### Windows
Cross Compile MythTV for Windows via the w64-mingw32MythBuild_MXE.sh build script. <br />
Run ./"mythtv/platform/win32/w64-mingw32MythBuild_MXE.sh"

cd mythtv/mythplugins/mythapps \
sudo su \
export PATH=/home/ubuntu/Desktop/build/mxe/usr/bin:$PATH  \
export iPre="/home/ubuntu/Desktop/build/install"  \
export qt5="/home/ubuntu/Desktop/build/mxe/usr/i686-w64-mingw32.shared/qt5"

If compiling, you may need to open 'build/mythtv/mythplugins/mythapps/mythapps/makefile' and replace 'qmake' with 'i686-w64-mingw32.shared-qmake-qt5'

# SETUP 
Recommended to use the flathub Kodi as it's keep updated and has DRM support.
1.  sudo apt update \
	sudo apt install flatpak \
	sudo flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo \
	sudo flatpak install flathub tv.kodi.Kodi \
	sudo flatpak override tv.kodi.Kodi --filesystem=host \
	flatpak run tv.kodi.Kodi

3. Open Kodi. Enabled remote control via http in Kodi->Setting->Services->Control.     Allow Remote Control via HTTP - Yes. \
                   Require Aurthenitcation - Yes. Username / Password / Port (8080) \
                   Allow remote control from applications on this system - Yes 
				          
4. Kodi->Setting->System->Display.      Delay after change of refresh rate - 0.2 seconds \
  .  .  .  .Kodi->Setting->System->Audio->Play Gui Sounds - No \
  .  .  .  .Kodi->Setting->System->Addons->Set Unknown Sources (if required)
				   			   
5. Install some Kodi video addons such as the free Nasa TV in the Kodi gui and check the video plays without prompts in Kodi.

6. Optionally install and open the YouTube addon in Kodi, and sign up for an API key. Enter this in the MythApps setting menu instead of Kodi and click save. - https://www.linuxbabe.com/raspberry-pi/kodi-youtube  
Open the addon in Kodi and navigate to:		YouTube settings->MPEG-DASH->MPEG-DASH - Yes
											YouTube settings->MPEG-DASH->Video quality adaptive (WEBM/VP9) - (to enable 4k)
											YouTube settings->Folders->Quick Search (Incognito)
   
7. Open mythfrontend, and mythapps. press the m key to accessing the settings menu and check you have all dependencies installed.

8. Setup Searching. Each App has a search url and is auto-discovered in the settings menu after opening each App once from MythApps home screen. Some search url's may need to be disabled.

TIPS	<br />
Tested on mythbuntu skin. <br />
If you search in the home screen, this will search all apps. Recommended to open the App and then search to get search results just from one App.<br />
F2 will toggle fullscreen. <br />
F3 will temporarily stop Kodi from auto minimizing. 		

## Wayland
Wayland works with GNOME only and requires the activate-window-by-title extension. <br />
This should be installed automatically when entering the setting menu for the first time, <br />
but will require a logoff. You can check if this is setup correctly by running: (1 should be returned) <br />
"gnome-extensions show activate-window-by-title@lucaswerkmeister.de | grep -c ENABLED"<br />
<br />
https://github.com/lucaswerkmeister/activate-window-by-title<br />
Also make sure you have gnome extensions enabled: "gsettings set org.gnome.shell disable-user-extensions false"<br />

Wayland works beter than X11.

# DEVELOPER
While MythApps is stable for me, Please note this is a new plugin under developement.


## Code Formating
run ./format.sh or compile.sh<br />
See https://www.mythtv.org/wiki/Coding_Standards for more information.

## Generate developer documenation
doxygen <br />
<br />
Get lines of code summary. find . -name '*.cpp' | xargs wc -l <br />
mythpluginLoader - This allows you to run MythApps as a standalone application. <br />
MythAppsServices - Switches between Kodi and MythTV on Android via a Java web sockets interface.

## generateThemes.pl
Please only edit mythapps-ui.xml and then run "perl ./generateThemes.pl" after making any changes unless you want to fork the default theme.
This will generate the changes across all themes to help increase maintainability.

## Patches
If submitting a patch and need a fix for a specific app, please use feature detection or alternatively a settings that can be enabled/disabled to fix a bug in a specific app.
Please note, MythApps is a wrapper of the Kodi JSON user interface and with the exception of the Wikipedia search suggestions, does not communicate with any external internet services or APIs.
The media is opened and decoded in an external video player.

Please report any bugs on this Github page and pull requests are welcome.

# DEPENDENCIES #
 libqt5websockets5  <br />
 libqt5websockets5-dev  <br />
 kodi  <br />
 xdotool (X11 only) <br />
 git (Wayland only) <br />
 
 # TROUBLESHOOTING  #
1.	Can you log into http://127.0.0.1:8080 in a web browser? <br />
No? You need to install Kodi and enable the web interface. <br />
2.	Have you installed a video addon in Kodi and played a video in Kodi? <br />
3.	Are you getting unwanted prompts such as asking for the video quality? <br />
Most addons will have a setting not to prompt for video quality. <br />
4.	Confirm all dependencies are installed by opening the settings menu (m key).  <br />
5.	Run mythfrontend from a terminal, optionally with logging. mythfrontend --loglevel debug  <br />
6. 	Are you running Wayland without GNOME? Try X11. Check with echo $XDG_SESSION_TYPE.
