# ==============================================================================
# SDL3 & MainActivity Complete ProGuard Rules
# ==============================================================================

-keep,includedescriptorclasses class org.libsdl.app.** { *; }
-keepclassmembers class org.libsdl.app.** { *; }
-dontwarn org.libsdl.app.**

-keep,includedescriptorclasses class org.libsdl.app.SDLActivity { *; }
-keep,includedescriptorclasses class org.libsdl.app.SDL { *; }

-keep,includedescriptorclasses class org.libsdl.app.MainActivity {
    public *** nativeMenu* (...);
    private native void nativeMenuItemClicked(int);
}
-keepclassmembers class org.libsdl.app.MainActivity { *; }

-keepclasseswithmembernames class * {
    native <methods>;
}

-keep class org.libsdl.app.HIDDeviceManager { *; }
-keep class org.libsdl.app.SDLAudioManager { *; }
-keep class org.libsdl.app.SDLControllerManager { *; }