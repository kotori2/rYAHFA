package mk.a0x.yahfa;

import dalvik.system.DexClassLoader;
import lab.galaxy.yahfa.HookMain;

import android.app.Application;

public class Main extends Application {
    public void init(){
        ClassLoader classLoader = getClassLoader();
        DexClassLoader dexClassLoader = new DexClassLoader("/sdcard/demoPlugin-debug.apk",
                getCodeCacheDir().getAbsolutePath(), null, classLoader);
        HookMain.doHookDefault(dexClassLoader, classLoader);
    }
}
