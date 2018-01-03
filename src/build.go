package main

import (
	"archive/tar"
	"archive/zip"
	"bufio"
	"bytes"
	"compress/gzip"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"
)

func main() {
	if err := start(); err != nil {
		fmt.Printf("Error: %v\n", err)
		os.Exit(2)
	}
}

var debugEnabled = false

func start() error {
	if runtime.GOOS != "windows" && runtime.GOOS != "linux" {
		return fmt.Errorf("Unsupported OS '%v'", runtime.GOOS)
	}
	if runtime.GOARCH != "amd64" {
		return fmt.Errorf("Unsupported OS '%v' or arch '%v'", runtime.GOOS, runtime.GOARCH)
	}
	if len(os.Args) < 2 {
		return fmt.Errorf("No command provided")
	}

	switch os.Args[1] {
	case "rerun":
		err := clean()
		if err == nil {
			err = run()
		}
		return err
	case "run":
		return run()
	case "clean":
		return clean()
	case "rebuild":
		err := clean()
		if err == nil {
			err = build()
		}
		return err
	case "build":
		return build()
	case "package":
		return pkg()
	case "build-cef":
		return buildCef()
	case "lint":
		return lint()
	case "unit-test":
		return unitTest()
	case "benchmark":
		return benchmark()
	default:
		return fmt.Errorf("Unrecognized command '%v'", os.Args[1])
	}
}

func run(extraQmakeArgs ...string) error {
	if err := build(extraQmakeArgs...); err != nil {
		return err
	}
	target, err := target()
	if err != nil {
		return err
	}
	return execCmd(filepath.Join(target, exeExt("doogie")), extraArgs()...)
}

func clean() error {
	err := os.RemoveAll("debug")
	if err == nil {
		err = os.RemoveAll("release")
	}
	return err
}

func build(extraQmakeArgs ...string) error {
	target, err := target()
	if err != nil {
		return err
	}
	// Get qmake path
	qmakePath, err := exec.LookPath(exeExt("qmake"))
	if err != nil {
		return err
	}

	// Make the dir for the target
	if err := os.MkdirAll(target, 0755); err != nil {
		return err
	}

	// Run qmake TODO: put behind flag
	qmakeArgs := extraQmakeArgs
	if target == "debug" {
		qmakeArgs = append(qmakeArgs, "CONFIG+=debug")
	} else {
		qmakeArgs = append(qmakeArgs, "CONFIG+=release", "CONFIG-=debug")
	}
	qmakeArgs = append(qmakeArgs, "doogie.pro")
	if err := execCmd(qmakePath, qmakeArgs...); err != nil {
		return fmt.Errorf("QMake failed: %v", err)
	}

	// Run nmake if windows, make if linux
	makeExe := "make"
	makeArgs := []string{}
	if runtime.GOOS == "windows" {
		makeExe = "nmake.exe"
		// This version takes the target name unlike the Linux one
		makeArgs = []string{target, "/NOLOGO"}
	}
	if err := execCmd(makeExe, makeArgs...); err != nil {
		return fmt.Errorf("NMake failed: %v", err)
	}

	// Chmod on linux
	if runtime.GOOS == "linux" {
		if err = os.Chmod(filepath.Join(target, "doogie"), 0755); err != nil {
			return err
		}
	}

	// Copy over resources
	if err := copyResources(qmakePath, target); err != nil {
		return err
	}

	return nil
}

func pkg() error {
	target, err := target()
	if err != nil {
		return err
	}
	// Just move over the files that matter to a new deploy dir and zip em up
	deployDir := filepath.Join(target, "package", "doogie")
	if err = os.MkdirAll(deployDir, 0755); err != nil {
		return err
	}

	// Get all base-dir items to copy, excluding only some
	filesToCopy := []string{}
	dirFiles, err := ioutil.ReadDir(target)
	if err != nil {
		return err
	}
	for _, file := range dirFiles {
		if !file.IsDir() {
			switch filepath.Ext(file.Name()) {
			case ".cpp", ".h", ".obj", ".res", ".manifest", ".log", ".o":
				// No-op
			default:
				filesToCopy = append(filesToCopy, file.Name())
			}
		}
	}
	if err = copyEachToDirIfNotPresent(target, deployDir, filesToCopy...); err != nil {
		return err
	}

	// And other dirs if present in folder
	subDirs := []string{"imageformats", "locales", "platforms", "sqldrivers", "styles"}
	for _, subDir := range subDirs {
		srcDir := filepath.Join(target, subDir)
		if _, err = os.Stat(srcDir); err == nil {
			if err = copyDirIfNotPresent(srcDir, filepath.Join(deployDir, subDir)); err != nil {
				return fmt.Errorf("Unable to copy %v: %v", subDir, err)
			}
		}
	}

	// Now create a zip or tar file with all the goods
	if runtime.GOOS == "windows" {
		err = createSingleDirZipFile(deployDir, filepath.Join(target, "package", "doogie.zip"))
	} else {
		err = createSingleDirTarGzFile(deployDir, filepath.Join(target, "package", "doogie.tar.gz"))
	}
	if err != nil {
		return err
	}

	return os.RemoveAll(deployDir)
}

func buildCef() error {
	if runtime.GOOS == "windows" {
		return buildCefWindows()
	}
	return buildCefLinux()
}

func buildCefLinux() error {
	cefDir := os.Getenv("CEF_DIR")
	if cefDir == "" {
		return fmt.Errorf("Unable to find CEF_DIR env var")
	}
	// We have to run separate make runs for different target types
	makeLib := func(target string) error {
		if err := execCmdInDir(cefDir, "cmake", "-DCMAKE_BUILD_TYPE="+target, "."); err != nil {
			return fmt.Errorf("CMake failed: %v", err)
		}
		wrapperDir := filepath.Join(cefDir, "libcef_dll_wrapper")
		if err := execCmdInDir(wrapperDir, "make"); err != nil {
			return fmt.Errorf("Make failed: %v", err)
		}
		if err := os.Rename(filepath.Join(wrapperDir, "libcef_dll_wrapper.a"),
			filepath.Join(wrapperDir, "libcef_dll_wrapper_"+target+".a")); err != nil {
			return fmt.Errorf("Unable to rename .a file: %v", err)
		}
		// We also need to run strip on the Release libcef.so per:
		//	https://bitbucket.org/chromiumembedded/cef/issues/1979
		if target == "Release" {
			// Back it up first
			err := copyIfNotPresent(filepath.Join(cefDir, "Release/libcef.so"),
				filepath.Join(cefDir, "Release/libcef.fullsym.so"))
			if err != nil {
				return fmt.Errorf("Release libcef backup failed: %v", err)
			}
			if err = execCmdInDir(cefDir, "strip", "--strip-all", "Release/libcef.so"); err != nil {
				return fmt.Errorf("Failed stripping symbols: %v", err)
			}
		}
		return nil
	}

	if err := makeLib("Debug"); err != nil {
		return err
	}
	return makeLib("Release")
}

func buildCefWindows() error {
	cefDir := os.Getenv("CEF_DIR")
	if cefDir == "" {
		return fmt.Errorf("Unable to find CEF_DIR env var")
	}
	// Build the make files
	if err := execCmdInDir(cefDir, "cmake", "-G", "Visual Studio 14 Win64", "."); err != nil {
		return fmt.Errorf("CMake failed: %v", err)
	}

	// Replace a couple of strings in VC proj file on windows
	dllWrapperDir := filepath.Join(cefDir, "libcef_dll_wrapper")
	vcProjFile := filepath.Join(dllWrapperDir, "libcef_dll_wrapper.vcxproj")
	projXml, err := ioutil.ReadFile(vcProjFile)
	if err != nil {
		return fmt.Errorf("Unable to read VC proj file: %v", err)
	}
	// First one is debug, second is release
	projXml = bytes.Replace(projXml, []byte("<RuntimeLibrary>MultiThreadedDebug</RuntimeLibrary>"),
		[]byte("<RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary>"), 1)
	projXml = bytes.Replace(projXml, []byte("<RuntimeLibrary>MultiThreaded</RuntimeLibrary>"),
		[]byte("<RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>"), 1)
	if err = ioutil.WriteFile(vcProjFile, projXml, os.ModePerm); err != nil {
		return fmt.Errorf("Unable to write VC proj file: %v", err)
	}

	// Build debug and then build release
	if err = execCmdInDir(dllWrapperDir, "msbuild", "libcef_dll_wrapper.vcxproj", "/p:Configuration=Debug"); err != nil {
		return fmt.Errorf("Unable to build debug wrapper: %v", err)
	}
	if err = execCmdInDir(dllWrapperDir, "msbuild", "libcef_dll_wrapper.vcxproj", "/p:Configuration=Release"); err != nil {
		return fmt.Errorf("Unable to build release wrapper: %v", err)
	}
	return nil
}

func lint() error {
	toIgnore := []string{
		"No copyright message found.",
		"#ifndef header guard has wrong style, please use: SRC_",
		"#endif line should be \"#endif  // SRC_",
		"Include the directory when naming .h files",
		"Done processing",
		"Total errors found",
	}
	// Run lint on all cc and h files, and trim off any of the toIgnore stuff
	depotToolsDir := os.Getenv("DEPOT_TOOLS_DIR")
	if depotToolsDir == "" {
		return fmt.Errorf("Unable to find DEPOT_TOOLS_DIR env var")
	}
	args := []string{
		filepath.Join(depotToolsDir, "cpplint.py"),
		// Can't use, ref: https://github.com/google/styleguide/issues/22
		// "--root=doogie\\",
	}
	integrationTestDir := filepath.Join("tests", "integration")
	err := filepath.Walk(".", func(path string, info os.FileInfo, err error) error {
		if !info.IsDir() && !strings.HasPrefix(info.Name(), "moc_") &&
			!strings.HasPrefix(path, integrationTestDir) &&
			(strings.HasSuffix(path, ".cc") || strings.HasSuffix(path, ".h")) {
			args = append(args, path)
		}
		return nil
	})
	if err != nil {
		return err
	}
	pycmd := "python"
	if runtime.GOOS == "linux" {
		// python by itself may refer to python3 or python2 depending on the distro,
		// so invoke python2 explicitly.
		pycmd = "python2"
	}
	cmd := exec.Command(pycmd, args...)
	out, err := cmd.CombinedOutput()
	if err != nil && len(out) == 0 {
		return fmt.Errorf("Unable to run cpplint: %v", err)
	}
	scanner := bufio.NewScanner(bytes.NewReader(out))
	foundAny := false
	for scanner.Scan() {
		// If after the trimmed string after the second colon starts w/ any toIgnore, we ignore it
		ignore := false
		origLine := scanner.Text()
		checkLine := origLine
		if firstColon := strings.Index(origLine, ":"); firstColon != -1 {
			if secondColon := strings.Index(origLine[firstColon+1:], ":"); secondColon != -1 {
				checkLine = strings.TrimSpace(origLine[firstColon+secondColon+2:])
			}
		}
		for _, toCheck := range toIgnore {
			if strings.HasPrefix(checkLine, toCheck) {
				ignore = true
				break
			}
		}
		if !ignore {
			fmt.Println(origLine)
			foundAny = true
		}
	}
	if foundAny {
		return fmt.Errorf("Lint check returned one or more errors")
	}
	return nil
}

func unitTest() error {
	if err := build("CONFIG+=test"); err != nil {
		return err
	}
	target, err := target()
	if err != nil {
		return err
	}
	return execCmd(filepath.Join(target, exeExt("doogie-test")))
}

func benchmark() error {
	if err := build("CONFIG+=benchmark"); err != nil {
		return err
	}
	target, err := target()
	if err != nil {
		return err
	}
	return execCmd(filepath.Join(target, exeExt("doogie-benchmark")))
}

func target() (string, error) {
	target := "debug"
	if len(os.Args) >= 3 && !strings.HasPrefix(os.Args[2], "--") {
		if os.Args[2] != "release" && os.Args[2] != "debug" {
			return "", fmt.Errorf("Unknown target '%v'", os.Args[2])
		}
		target = os.Args[2]
	}
	return target, nil
}

func extraArgs() []string {
	argStartIndex := 1
	if len(os.Args) >= 2 {
		argStartIndex = 2
		if len(os.Args) > 2 && (os.Args[2] == "release" || os.Args[2] == "debug") {
			argStartIndex = 3
		}
	}
	return os.Args[argStartIndex:]
}

func exeExt(baseName string) string {
	if runtime.GOOS == "windows" {
		return baseName + ".exe"
	}
	return baseName
}

func execCmd(name string, args ...string) error {
	return execCmdInDir("", name, args...)
}

func execCmdInDir(dir string, name string, args ...string) error {
	cmd := exec.Command(name, args...)
	cmd.Dir = dir
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	return cmd.Run()
}

func copyResources(qmakePath string, target string) error {
	if runtime.GOOS == "windows" {
		return copyResourcesWindows(qmakePath, target)
	}
	return copyResourcesLinux(qmakePath, target)
}

func copyResourcesLinux(qmakePath string, target string) error {
	if _, err := exec.LookPath("chrpath"); err != nil {
		return fmt.Errorf("Unable to find chrpath on the PATH: %v", err)
	}
	cefDir := os.Getenv("CEF_DIR")
	if cefDir == "" {
		return fmt.Errorf("Unable to find CEF_DIR env var")
	}
	// Everything read only except by owner
	// Copy over crash reporter cfg
	err := copyAndChmodEachToDirIfNotPresent(0644, ".", target, "crash_reporter.cfg")
	if err != nil {
		return err
	}
	// Copy over some Qt DLLs
	err = copyAndChmodEachToDirIfNotPresent(0644, filepath.Join(filepath.Dir(qmakePath), "../lib"), target,
		"libQt5Core.so.5",
		"libQt5Gui.so.5",
		"libQt5Sql.so.5",
		"libQt5Widgets.so.5",
		// TODO: See https://bugreports.qt.io/browse/QTBUG-53865
		"libicui18n.so.56",
		"libicuuc.so.56",
		"libicudata.so.56",
		// Needed for libqxcb platform
		"libQt5XcbQpa.so.5",
		"libQt5DBus.so.5",
	)
	if err != nil {
		return err
	}
	// Some DLLs are needed in debug only
	if target == "debug" {
		err := copyAndChmodEachToDirIfNotPresent(0644, filepath.Join(filepath.Dir(qmakePath), "../lib"), target,
			"libQt5Network.so.5",
			"libQt5Test.so.5",
			"libQt5WebSockets.so.5",
		)
		if err != nil {
			return err
		}
	}

	// Need some plugins
	// Before that, record whether the xcb plugin is there yet
	hadXcbPlugin := true
	xcbPluginPath := filepath.Join(target, "platforms", "libqxcb.so")
	if _, err = os.Stat(xcbPluginPath); os.IsNotExist(err) {
		hadXcbPlugin = false
	}
	copyPlugins(qmakePath, target, "imageformats", "qgif")
	copyPlugins(qmakePath, target, "platforms", "qxcb")
	copyPlugins(qmakePath, target, "sqldrivers", "qsqlite")
	// If the xcb plugin wasn't there (but is now), change the rpath
	if !hadXcbPlugin {
		if err = execCmd("chrpath", "-r", "$ORIGIN/..", xcbPluginPath); err != nil {
			return fmt.Errorf("Unable to run chrpath: %v", err)
		}
	}

	// Copy over CEF libs
	err = copyAndChmodEachToDirIfNotPresent(0644, filepath.Join(cefDir, strings.Title(target)), target,
		"libcef.so",
		"natives_blob.bin",
		"snapshot_blob.bin",
	)
	if err != nil {
		return err
	}

	// Copy over CEF resources
	cefResDir := filepath.Join(cefDir, "Resources")
	err = copyAndChmodEachToDirIfNotPresent(0644, cefResDir, target,
		"icudtl.dat",
		"cef.pak",
		"cef_100_percent.pak",
		"cef_200_percent.pak",
		"cef_extensions.pak",
		"devtools_resources.pak",
	)
	if err != nil {
		return err
	}

	// And CEF locales
	targetLocaleDir := filepath.Join(target, "locales")
	if err = os.MkdirAll(targetLocaleDir, 0744); err != nil {
		return err
	}
	err = copyAndChmodEachToDirIfNotPresent(0644, filepath.Join(cefResDir, "locales"), targetLocaleDir, "en-US.pak")
	return err
}

func copyResourcesWindows(qmakePath string, target string) error {
	cefDir := os.Getenv("CEF_DIR")
	if cefDir == "" {
		return fmt.Errorf("Unable to find CEF_DIR env var")
	}
	// Copy over crash reporter cfg
	err := copyEachToDirIfNotPresent(".", target, "crash_reporter.cfg")
	if err != nil {
		return err
	}
	// Copy over some Qt DLLs
	qtDlls := []string{
		"Qt5Core.dll",
		"Qt5Gui.dll",
		"Qt5Sql.dll",
		"Qt5Widgets.dll",
	}
	// Debug libs are d.dll
	if target == "debug" {
		// Only need web sockets during debug
		qtDlls = append(qtDlls, "Qt5WebSockets.dll", "Qt5Network.dll", "Qt5Test.dll")
		for i := range qtDlls {
			qtDlls[i] = strings.Replace(qtDlls[i], ".dll", "d.dll", -1)
		}
		// Also want the PDB files if they are there
		for _, dll := range qtDlls {
			qtDlls = append(qtDlls, strings.Replace(dll, ".dll", ".pdb", -1))
		}
	}
	err = copyEachToDirIfNotPresent(filepath.Dir(qmakePath), target, qtDlls...)
	if err != nil {
		return err
	}

	// Need special ucrtbased.dll for debug builds
	if target == "debug" {
		err = copyEachToDirIfNotPresent("C:\\Program Files (x86)\\Windows Kits\\10\\bin\\x64\\ucrt",
			target, "ucrtbased.dll")
		if err != nil {
			return err
		}
	}

	// TODO: statically compile this, ref: https://github.com/cretz/doogie/issues/46
	// Need some plugins
	copyPlugins(qmakePath, target, "imageformats", "qgif")
	copyPlugins(qmakePath, target, "platforms", "qwindows")
	copyPlugins(qmakePath, target, "sqldrivers", "qsqlite")
	copyPlugins(qmakePath, target, "styles", "qwindowsvistastyle")

	// Copy over CEF libs
	err = copyEachToDirIfNotPresent(filepath.Join(cefDir, strings.Title(target)), target,
		"libcef.dll",
		"chrome_elf.dll",
		"natives_blob.bin",
		"snapshot_blob.bin",
		"d3dcompiler_43.dll",
		"d3dcompiler_47.dll",
		"libEGL.dll",
		"libGLESv2.dll",
	)
	if err != nil {
		return err
	}

	// Copy over CEF resources
	cefResDir := filepath.Join(cefDir, "Resources")
	err = copyEachToDirIfNotPresent(cefResDir, target,
		"icudtl.dat",
		"cef.pak",
		"cef_100_percent.pak",
		"cef_200_percent.pak",
		"cef_extensions.pak",
		"devtools_resources.pak",
	)
	if err != nil {
		return err
	}

	// And CEF locales
	targetLocaleDir := filepath.Join(target, "locales")
	if err = os.MkdirAll(targetLocaleDir, 0755); err != nil {
		return err
	}
	err = copyEachToDirIfNotPresent(filepath.Join(cefResDir, "locales"), targetLocaleDir, "en-US.pak")
	return err
}

func chmodEachInDir(mode os.FileMode, dir string, filenames ...string) error {
	for _, filename := range filenames {
		if err := os.Chmod(filepath.Join(dir, filename), mode); err != nil {
			return err
		}
	}
	return nil
}

func copyPlugins(qmakePath string, target string, dir string, plugins ...string) error {
	srcDir := filepath.Join(qmakePath, "../../plugins", dir)
	if _, err := os.Stat(srcDir); os.IsExist(err) {
		return fmt.Errorf("Unable to find Qt plugins dir %v: %v", dir, err)
	}
	destDir := filepath.Join(target, dir)
	if err := os.MkdirAll(destDir, 0755); err != nil {
		return fmt.Errorf("Unable to create dir: %v", err)
	}
	for _, plugin := range plugins {
		var fileName string
		if runtime.GOOS == "linux" {
			fileName = "lib" + plugin + ".so"
		} else if target == "debug" {
			fileName = plugin + "d.dll"
		} else {
			fileName = plugin + ".dll"
		}
		if err := copyAndChmodEachToDirIfNotPresent(0644, srcDir, destDir, fileName); err != nil {
			return err
		}
	}
	return nil
}

func copyDirIfNotPresent(srcDir string, destDir string) error {
	// Note, this is not recursive, but it does preserve permissions
	srcFi, err := os.Stat(srcDir)
	if err != nil {
		return fmt.Errorf("Unable to find src dir: %v", err)
	}
	if err = os.MkdirAll(destDir, srcFi.Mode()); err != nil {
		return fmt.Errorf("Unable to create dest dir: %v", err)
	}
	files, err := ioutil.ReadDir(srcDir)
	if err != nil {
		return fmt.Errorf("Unable to read src dir: %v", err)
	}
	for _, file := range files {
		srcFile := filepath.Join(srcDir, file.Name())
		if err = copyToDirIfNotPresent(srcFile, destDir); err != nil {
			return fmt.Errorf("Error copying file: %v", err)
		}
		if err = os.Chmod(srcFile, file.Mode()); err != nil {
			return fmt.Errorf("Unable to chmod file: %v", err)
		}
	}
	return nil
}

func copyAndChmodEachToDirIfNotPresent(mode os.FileMode, srcDir string, destDir string, srcFilenames ...string) error {
	if err := copyEachToDirIfNotPresent(srcDir, destDir, srcFilenames...); err != nil {
		return err
	}
	return chmodEachInDir(mode, destDir, srcFilenames...)
}

func copyEachToDirIfNotPresent(srcDir string, destDir string, srcFilenames ...string) error {
	for _, srcFilename := range srcFilenames {
		if err := copyToDirIfNotPresent(filepath.Join(srcDir, srcFilename), destDir); err != nil {
			return err
		}
	}
	return nil
}

func copyToDirIfNotPresent(src string, destDir string) error {
	return copyIfNotPresent(src, filepath.Join(destDir, filepath.Base(src)))
}

func copyIfNotPresent(src string, dest string) error {
	if _, err := os.Stat(dest); os.IsExist(err) {
		debugLogf("Skipping copying '%v' to '%v' because it already exists")
		return nil
	}
	debugLogf("Copying %v to %v\n", src, dest)
	in, err := os.Open(src)
	if err != nil {
		return err
	}
	defer in.Close()
	inStat, err := in.Stat()
	if err != nil {
		return err
	}
	out, err := os.OpenFile(dest, os.O_RDWR|os.O_CREATE|os.O_TRUNC, inStat.Mode())
	if err != nil {
		return err
	}
	defer out.Close()
	_, err = io.Copy(out, in)
	cerr := out.Close()
	if err != nil {
		return err
	}
	return cerr
}

func debugLogf(format string, v ...interface{}) {
	if debugEnabled {
		log.Printf(format, v...)
	}
}

func createSingleDirTarGzFile(dir string, tarFilename string) error {
	tarFile, err := os.Create(tarFilename)
	if err != nil {
		return err
	}
	defer tarFile.Close()

	gw := gzip.NewWriter(tarFile)
	defer gw.Close()
	w := tar.NewWriter(gw)
	defer w.Close()

	return filepath.Walk(dir, func(path string, info os.FileInfo, err error) error {
		if info.IsDir() {
			return nil
		}
		rel, err := filepath.Rel(dir, path)
		if err != nil {
			return err
		}
		tarPath := filepath.ToSlash(filepath.Join(filepath.Base(dir), rel))
		srcPath := filepath.Join(dir, rel)

		header, err := tar.FileInfoHeader(info, "")
		if err != nil {
			return err
		}
		header.Name = tarPath
		// Remove owner info
		header.Uname = ""
		header.Gname = ""
		header.Uid = 0
		header.Gid = 0
		if err := w.WriteHeader(header); err != nil {
			return err
		}
		src, err := os.Open(srcPath)
		if err != nil {
			return err
		}
		defer src.Close()
		_, err = io.Copy(w, src)
		return err
	})
}

func createSingleDirZipFile(dir string, zipFilename string) error {
	zipFile, err := os.Create(zipFilename)
	if err != nil {
		return err
	}
	defer zipFile.Close()

	w := zip.NewWriter(zipFile)
	defer w.Close()

	return filepath.Walk(dir, func(path string, info os.FileInfo, err error) error {
		if info.IsDir() {
			return nil
		}
		rel, err := filepath.Rel(dir, path)
		if err != nil {
			return err
		}
		zipPath := filepath.ToSlash(filepath.Join(filepath.Base(dir), rel))
		srcPath := filepath.Join(dir, rel)

		dest, err := w.Create(zipPath)
		if err != nil {
			return err
		}
		src, err := os.Open(srcPath)
		if err != nil {
			return err
		}
		defer src.Close()
		_, err = io.Copy(dest, src)
		return err
	})
}
