package io.woutervh.ninedb;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

class NativeBinding {
    private static boolean copied = false;

    public static void load() {
        if (!isJar()) {
            System.loadLibrary("ninedb");
            return;
        }

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
        InputStream ins = NativeBinding.class.getResourceAsStream("/" + libName);
        OutputStream outs = new FileOutputStream(tempLibPath);

        ins.transferTo(outs);
        ins.close();
        outs.close();
    }

    private static boolean isJar() {
        String protocol = NativeBinding.class.getResource("NativeBinding.class").getProtocol();
        return protocol.equals("jar");
    }

    private static String getTempLibPath(String libName) {
        return new File(System.getProperty("java.io.tmpdir"), libName).getAbsolutePath();
    }

    private static String getLibName() {
        String os = System.getProperty("os.name");
        String arch = System.getProperty("os.arch");

        String prefix;
        String osName;
        String archName;
        String extension;

        if (os.contains("Windows")) {
            prefix = "";
            osName = "windows";
            extension = "dll";
        } else if (os.contains("Mac")) {
            prefix = "lib";
            osName = "macos";
            extension = "dylib";
        } else if (os.contains("Linux")) {
            prefix = "lib";
            osName = "linux";
            extension = "so";
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

        return prefix + "ninedb-" + osName + "-" + archName + "." + extension;
    }
}
