
* Change status area (shows on link hover and what not) to change its width and position based on content, scrollbar
  presence, mouse position, and window size. Also handle text overflow w/ an ellipsis.
* Change favicon to a loading gif (have to use QMovie) so that we can be sure the favicon doesn't show for the previous
  page. Maybe also roll this into favicon GIF support in general.
* Configurable set of tab-open-styles to do certain things instead of assuming all non-normal clicks are of a certain
  type
* Better default no-favicon and "(New Window)" defaults
* Change focus proxy of main window to active browser window