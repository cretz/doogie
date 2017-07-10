
Small:

* Change status area (shows on link hover and what not) to change its width and position based on content, scrollbar
  presence, mouse position, and window size. Also handle text overflow w/ an ellipsis.
* Change favicon to a loading gif (have to use QMovie) so that we can be sure the favicon doesn't show for the previous
  page. Maybe also roll this into favicon GIF support in general.
* Configurable set of tab-open-styles to do certain things instead of assuming all non-normal clicks are of a certain
  type
* Better default no-favicon and "(New Window)" defaults
* Change focus proxy of main window to active browser window

Medium:

* Chrome dev tools inside dock window that changes based on what browser you are looking at
* Drag/drop pages between Doogie instances
* Create new Doogie window inside same process
* Session saving (and do by default on close/open)
* Crash dumps

Large:

* Tests. Need to use Qt Test and simulate clicks to do things. Need corpus of local HTML/JS files to test some things of
  course. Don't rely on public internet (at least at first), start local Go server if necessary
* Installer? Ug...need to have portable version too then
* Auto-updater for the CEF part to get Chrome updates. Essentially just replace CEF binary pieces after running unit
  tests
* CI tool (e.g. appveyor for Windows) that builds nightlies, does tests, etc. This means the build script has to be
  updated to download CEF for us and we have to store the desired CEF version somewhere
* Profile support inside of app...requires separating CEF instances which is hard
* Plugins
* Theming