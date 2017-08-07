# Contributing

## C++

Doogie follows the [Chromium Style Guide](https://chromium.googlesource.com/chromium/src/+/master/styleguide/c++/c++.md)
for C++ code except for anything listed below.

* Forward decls only when necessary. No need to worry about compile times (yet).
* We use a singleton here and there as necessary
* Avoid slots, use method references
* Class decls are usually public, protected, and private. But here they are public, signals, public slots, protected,
  protected slots, private, and private slots
* Prefer default by-value capture on local lambdas
* Obviously, some styles have to be broken to confirm to Qt, e.g. name format for overridden methods
* No file-level comments
* Passing around raw pointers is ok in Qt land, at least for now

## JavaScript

For JS code in tests, follow [Standard Style](https://www.npmjs.com/package/standard).