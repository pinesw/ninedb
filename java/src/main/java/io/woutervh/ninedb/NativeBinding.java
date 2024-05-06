package io.woutervh.ninedb;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

class NativeBinding {
    private static boolean loaded = false;

    static {
        try {
            load();
        } catch (IOException e) {
            e.printStackTrace();
            System.exit(1);
        }
    }

    public static void load() throws IOException {
        if (loaded) {
            return;
        }
        loaded = true;

        if (!isJar()) {
            System.loadLibrary("ninedb");
            return;
        }

        String libName = getLibName();
        String tempLibPath = getTempLibPath(libName);

        InputStream ins = NativeBinding.class.getResourceAsStream("/" + libName);
        OutputStream outs = new FileOutputStream(tempLibPath);

        ins.transferTo(outs);
        ins.close();
        outs.close();

        System.load(tempLibPath);
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
            extension = "jnilib";
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
