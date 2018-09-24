---
layout: guide
title: Development
---

# Development

Doogie is built in C++ using [CEF](https://bitbucket.org/chromiumembedded/cef) and [Qt](https://www.qt.io/). This
section has information on [issues](#issues), [building](#building), and [contributing](#contributing).

## Issues

Any bugs or enhancement requests can be filed in via [GitHub issues](https://github.com/cretz/doogie/issues/). When
filing a bug, please try to be as descriptive as possible on the issue. Ideally, provide a page or set of pages that
trigger the error.

If there is a crash, the crash file will be dumped on the user's computer. This can be found at
`C:\Users\USER_NAME\AppData\Local\Doogie\User Data\Crashpad\reports` on Windows (TODO: where on Linux). Please upload
this file to the issue tracker for bugs about crashes.

If you are a developer, one of the easiest ways to do this is to add the pages to `src/tests/integration/resources`.
Then by running `npm run resource-server` from `src/tests/integration` the resources can be browsed via
http://127.0.0.1:1993 (e.g. [http://127.0.0.1:1993/hello-world.html](http://127.0.0.1:1993/hello-world.html)).

## Building

Building Doogie is straightforward. A [Go](https://golang.org/) script at `src/build.go` handles all building regardless
of platform.

All instructions below assume the user is in the `src` folder.

### Windows Prerequisites

Before running the build script on Windows, you must have the prerequisites:

* Latest CMake installed w/ `cmake.exe` on the `PATH`
* Latest Python (2.x) installed w/ `python.exe` on the `PATH`
* Latest Go installed w/ `go.exe` on the `PATH`
* [MSVC 2015 or 2017 Build Tools](http://landinghub.visualstudio.com/visual-cpp-build-tools) installed w/ the following
  executed to put 64-bit VC compiler on the `PATH` for MSVC 2015:
  `"C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64`. Or the following executed for MSVC
  2017: `"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64`.
* Latest Qt (5.x) installed w/ `qmake.exe` on the `PATH` corresponding to the chosen MSVC version (e.g. with
  `"C:\Qt\<version>\msvc2017_64\bin"` on the `PATH`)
* Optionally, [jom.exe](https://wiki.qt.io/Jom) on the `PATH` to speed up builds (e.g. with
  `"C:\Qt\Tools\QtCreator\bin"` on the `PATH`)
* Latest [Windows 64-bit standard dist of CEF](http://opensource.spotify.com/cefbuilds/index.html#windows64_builds)
  extracted w/ `CEF_DIR` environment variable set to the base CEF extracted dir
* This repo cloned w/ the shell at the `src` folder

### Linux Prerequisites

Before running the build script on Linux, you must have the prerequisites:

* Latest Qt (5.x) installed w/ `qmake` on the `PATH`
* Latest CMake installed w/ `cmake` on the `PATH`
* Latest Python (2.x) installed w/ `python` on the `PATH`
* Latest Go installed w/ `go` on the `PATH`
* Latest GCC installed w/ `gcc` and `g++` on the `PATH`
* Latest Make installed w/ `make` on the `PATH`
* Latest [Linux 64-bit standard dist of CEF](http://opensource.spotify.com/cefbuilds/index.html#linux64_builds)
  extracted w/ `CEF_DIR` environment variable set to the base CEF extracted dir
* Latest GTK 2.x installed and on the library path (e.g. on Ubuntu `sudo apt-get install libgtk2.0-dev`)
* Latest Mesa 3D headers installed and on the include path (e.g. on Ubuntu `sudo apt-get install mesa-common-dev`)
* Latest `libGL` installed and on the library path (e.g. on Ubuntu `sudo apt-get install libgl1-mesa-dev`)
* The `chrpath` utility on the `PATH`
* This repo cloned w/ the shell at the `src` folder

### MacOS Prerequisites

(TODO)

### Running Build

All instructions below assume the user is in the `src` folder.

The application uses the CEF C++ wrapper, which can be built via:

    go run build.go build-cef

This only needs to be built one time. Now the application can easily be run via:

    go run build.go run

That will run the `debug` version. For the `release` version, run:

    go run build.go run release

Internally, that just builds and runs the exe. To just build, call `build` instead. To package a deployment artifact
from a previously run build, `package` is used. For example, to package a `release` deployment artifact from a
previously run `release` build, run:

    go run build.go package release

Once complete, the package(s) will in `release/package` (e.g. doogie.zip in Windows)

### Testing

There are two types of tests, unit and integration tests at `src/tests/unit` and `src/tests/integration` respectively.

#### Integration Tests

The integration tests are not yet integrated in `build.go`. The tests are written in node.js. Make sure you have the
latest LTS release of node installed and navigate to `src/tests/integration`. To setup the test infrastructure, run:

    npm install

To run the tests, make sure a debug instance of Doogie is running then run

    npm test

#### Unit Tests

Assuming CEF is built (see the first "Running Build" step), unit tests can be run with the following in `src/`:

    go run build.go unit-test

Like the `run` command, the `unit-test` command can be given a `release` target, otherwise it defaults to `debug`.

#### Benchmarks

There are a couple of benchmarks in the application. To run them, do the following in `src/`:

    go run build.go benchmark

Like the `run` command, the `benchmark` command can be given a `release` target, otherwise it defaults to `debug`.

## Contributing

### C++

Doogie follows the [Chromium Style Guide](https://chromium.googlesource.com/chromium/src/+/master/styleguide/c++/c++.md)
for C++ code except for anything listed below.

* Forward decls only when necessary. No need to worry about compile times (yet).
* We use a singleton here and there as necessary
* Avoid slots, use method references
* Class decl order is usually public, protected, and private. But here they are public, signals, public slots,
  protected, protected slots, private, and private slots
* Prefer default by-value capture on local lambdas
* Obviously, some styles have to be broken to confirm to Qt, e.g. name format for overridden methods
* No file-level comments
* Passing around raw pointers is ok in Qt land, at least for now

To do a lint check of the code, have the
[Chromium depot_tools](https://chromium.googlesource.com/chromium/tools/depot_tools.git) cloned somewhere and set the
`DEPOT_TOOLS_DIR` env var to the dir. Then run the following in `src/`:

    go run build.go lint

### JavaScript

For JS code in `src/tests/integration`, follow [Standard Style](https://www.npmjs.com/package/standard).

Lint checks automatically occur when `npm test` is run to do the integration tests.

