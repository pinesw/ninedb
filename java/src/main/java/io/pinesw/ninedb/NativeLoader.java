package io.pinesw.ninedb;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

class NativeLoader {
    private static boolean copied = false;

    public static void load() {
        if (isJar()) {
            loadFromJar();
        } else {
            loadFromLibPath();
        }
    }

    private static void loadFromLibPath() {
        String javaLibPath = System.getProperty("java.library.path");
        File libFile = new File(javaLibPath, System.mapLibraryName("ninedb"));
        if (!libFile.exists()) {
            throw new RuntimeException("Native library not found in java.library.path");
        }
        System.load(libFile.getAbsolutePath());
    }

    private static void loadFromJar() {
        String libName = getLibName();
        String tempLibPath = getTempLibPath(libName);

        if (!copied) {
            try {
                copyLib(libName, tempLibPath);
                copied = true;
            } catch (IOException e) {
                throw new RuntimeException("Failed to copy native library", e);
            }
        }

        System.load(tempLibPath);
    }

    private static void copyLib(String libName, String tempLibPath) throws IOException {
        InputStream ins = NativeLoader.class.getResourceAsStream("/" + libName);
        OutputStream outs = new FileOutputStream(tempLibPath);

        ins.transferTo(outs);
        ins.close();
        outs.close();
    }

    private static boolean isJar() {
        String protocol = NativeLoader.class.getResource("NativeLoader.class").getProtocol();
        return protocol.equals("jar");
    }

    private static String getTempLibPath(String libName) {
        return new File(System.getProperty("java.io.tmpdir"), libName).getAbsolutePath();
    }

    private static String getLibName() {
        String os = System.getProperty("os.name");
        String arch = System.getProperty("os.arch");

        String osName;
        String archName;

        if (os.contains("Windows")) {
            osName = "windows";
        } else if (os.contains("Mac")) {
            osName = "macos";
        } else if (os.contains("Linux")) {
            osName = "linux";
        } else {
            throw new RuntimeException("Unsupported OS: " + os);
        }

        if (arch.contains("64")) {
            archName = "x64";
        } else if (arch.contains("86")) {
            archName = "x86";
        } else {
            throw new RuntimeException("Unsupported architecture: " + arch);
        }

        String libName = System.mapLibraryName("ninedb-" + osName + "-" + archName);
        return libName;
    }
}
